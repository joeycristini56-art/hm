/*
 * xoron_websocket.cpp - WebSocket client for executor
 * Provides: WebSocket.connect, send, close, etc.
 * Platforms: iOS (.dylib) and Android (.so)
 * Uses POSIX sockets and OpenSSL for TLS
 */

#include "xoron.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
#include <queue>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <random>

// POSIX socket includes (iOS and Android)
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

// OpenSSL for TLS support
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/sha.h>

#include "lua.h"
#include "lualib.h"

extern void xoron_set_error(const char* fmt, ...);

// WebSocket opcodes
enum WSOpcode {
    WS_CONTINUATION = 0x0,
    WS_TEXT = 0x1,
    WS_BINARY = 0x2,
    WS_CLOSE = 0x8,
    WS_PING = 0x9,
    WS_PONG = 0xA
};

// WebSocket state
enum WSState {
    WS_CONNECTING,
    WS_OPEN,
    WS_CLOSING,
    WS_CLOSED
};

// WebSocket connection
struct WebSocketConnection {
    uint32_t id;
    std::string url;
    std::string host;
    std::string path;
    int port;
    bool secure;
    
    int socket_fd;
    SSL* ssl;
    SSL_CTX* ssl_ctx;
    
    WSState state;
    std::thread recv_thread;
    std::atomic<bool> running;
    
    std::mutex send_mutex;
    std::queue<std::string> recv_queue;
    std::mutex recv_mutex;
    std::condition_variable recv_cv;
    
    // Lua callbacks
    lua_State* L;
    int on_message_ref;
    int on_close_ref;
    int on_error_ref;
    
    WebSocketConnection() : id(0), port(80), secure(false), socket_fd(-1),
                            ssl(nullptr), ssl_ctx(nullptr), state(WS_CLOSED),
                            running(false), L(nullptr), on_message_ref(LUA_NOREF),
                            on_close_ref(LUA_NOREF), on_error_ref(LUA_NOREF) {}
    
    ~WebSocketConnection() {
        close_connection();
    }
    
    void close_connection() {
        running = false;
        state = WS_CLOSED;
        
        if (recv_thread.joinable()) {
            recv_thread.join();
        }
        
        if (ssl) {
            SSL_shutdown(ssl);
            SSL_free(ssl);
            ssl = nullptr;
        }
        if (ssl_ctx) {
            SSL_CTX_free(ssl_ctx);
            ssl_ctx = nullptr;
        }
        if (socket_fd >= 0) {
            ::close(socket_fd);
            socket_fd = -1;
        }
        
        if (L) {
            if (on_message_ref != LUA_NOREF) {
                lua_unref(L, on_message_ref);
                on_message_ref = LUA_NOREF;
            }
            if (on_close_ref != LUA_NOREF) {
                lua_unref(L, on_close_ref);
                on_close_ref = LUA_NOREF;
            }
            if (on_error_ref != LUA_NOREF) {
                lua_unref(L, on_error_ref);
                on_error_ref = LUA_NOREF;
            }
        }
    }
};

// Global state
static std::mutex g_ws_mutex;
static std::unordered_map<uint32_t, WebSocketConnection*> g_connections;
static std::atomic<uint32_t> g_next_ws_id{1};

static const char* WEBSOCKET_MT = "XoronWebSocket";

// Base64 encoding for WebSocket key
static std::string base64_encode(const unsigned char* data, size_t len) {
    static const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    result.reserve(((len + 2) / 3) * 4);
    
    for (size_t i = 0; i < len; i += 3) {
        unsigned int n = (data[i] << 16);
        if (i + 1 < len) n |= (data[i + 1] << 8);
        if (i + 2 < len) n |= data[i + 2];
        
        result += chars[(n >> 18) & 0x3F];
        result += chars[(n >> 12) & 0x3F];
        result += (i + 1 < len) ? chars[(n >> 6) & 0x3F] : '=';
        result += (i + 2 < len) ? chars[n & 0x3F] : '=';
    }
    
    return result;
}

// Generate WebSocket key
static std::string generate_ws_key() {
    unsigned char key[16];
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    for (int i = 0; i < 16; i++) {
        key[i] = dis(gen);
    }
    
    return base64_encode(key, 16);
}

// Parse URL
static bool parse_ws_url(const std::string& url, std::string& host, std::string& path, int& port, bool& secure) {
    size_t pos = 0;
    
    if (url.substr(0, 6) == "wss://") {
        secure = true;
        pos = 6;
        port = 443;
    } else if (url.substr(0, 5) == "ws://") {
        secure = false;
        pos = 5;
        port = 80;
    } else {
        return false;
    }
    
    size_t path_start = url.find('/', pos);
    std::string host_port;
    
    if (path_start != std::string::npos) {
        host_port = url.substr(pos, path_start - pos);
        path = url.substr(path_start);
    } else {
        host_port = url.substr(pos);
        path = "/";
    }
    
    size_t port_pos = host_port.find(':');
    if (port_pos != std::string::npos) {
        host = host_port.substr(0, port_pos);
        port = std::stoi(host_port.substr(port_pos + 1));
    } else {
        host = host_port;
    }
    
    return !host.empty();
}

// Connect to server
static bool ws_connect(WebSocketConnection* conn) {
    struct hostent* server = gethostbyname(conn->host.c_str());
    if (!server) {
        return false;
    }
    
    conn->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (conn->socket_fd < 0) {
        return false;
    }
    
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    serv_addr.sin_port = htons(conn->port);
    
    if (connect(conn->socket_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        ::close(conn->socket_fd);
        conn->socket_fd = -1;
        return false;
    }
    
    if (conn->secure) {
        SSL_library_init();
        SSL_load_error_strings();
        
        conn->ssl_ctx = SSL_CTX_new(TLS_client_method());
        if (!conn->ssl_ctx) {
            ::close(conn->socket_fd);
            conn->socket_fd = -1;
            return false;
        }
        
        conn->ssl = SSL_new(conn->ssl_ctx);
        SSL_set_fd(conn->ssl, conn->socket_fd);
        
        if (SSL_connect(conn->ssl) <= 0) {
            SSL_free(conn->ssl);
            SSL_CTX_free(conn->ssl_ctx);
            ::close(conn->socket_fd);
            conn->ssl = nullptr;
            conn->ssl_ctx = nullptr;
            conn->socket_fd = -1;
            return false;
        }
    }
    
    return true;
}

// Send data
static bool ws_send_raw(WebSocketConnection* conn, const void* data, size_t len) {
    if (conn->secure && conn->ssl) {
        return SSL_write(conn->ssl, data, (int)len) > 0;
    } else if (conn->socket_fd >= 0) {
        return send(conn->socket_fd, data, len, 0) > 0;
    }
    return false;
}

// Receive data
static int ws_recv_raw(WebSocketConnection* conn, void* data, size_t len) {
    if (conn->secure && conn->ssl) {
        return SSL_read(conn->ssl, data, (int)len);
    } else if (conn->socket_fd >= 0) {
        return (int)recv(conn->socket_fd, data, len, 0);
    }
    return -1;
}

// Perform WebSocket handshake
static bool ws_handshake(WebSocketConnection* conn) {
    std::string key = generate_ws_key();
    
    std::string request = "GET " + conn->path + " HTTP/1.1\r\n";
    request += "Host: " + conn->host + "\r\n";
    request += "Upgrade: websocket\r\n";
    request += "Connection: Upgrade\r\n";
    request += "Sec-WebSocket-Key: " + key + "\r\n";
    request += "Sec-WebSocket-Version: 13\r\n";
    request += "Origin: https://www.roblox.com\r\n";
    request += "\r\n";
    
    if (!ws_send_raw(conn, request.c_str(), request.size())) {
        return false;
    }
    
    // Read response
    char buffer[4096];
    int total = 0;
    while (total < (int)sizeof(buffer) - 1) {
        int n = ws_recv_raw(conn, buffer + total, 1);
        if (n <= 0) break;
        total += n;
        buffer[total] = '\0';
        if (strstr(buffer, "\r\n\r\n")) break;
    }
    
    // Check for 101 Switching Protocols
    return strstr(buffer, "101") != nullptr;
}

// Send WebSocket frame
static bool ws_send_frame(WebSocketConnection* conn, WSOpcode opcode, const void* data, size_t len) {
    std::lock_guard<std::mutex> lock(conn->send_mutex);
    
    std::vector<unsigned char> frame;
    
    // First byte: FIN + opcode
    frame.push_back(0x80 | opcode);
    
    // Second byte: MASK + length
    if (len < 126) {
        frame.push_back(0x80 | (unsigned char)len);
    } else if (len < 65536) {
        frame.push_back(0x80 | 126);
        frame.push_back((len >> 8) & 0xFF);
        frame.push_back(len & 0xFF);
    } else {
        frame.push_back(0x80 | 127);
        for (int i = 7; i >= 0; i--) {
            frame.push_back((len >> (i * 8)) & 0xFF);
        }
    }
    
    // Masking key
    unsigned char mask[4];
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    for (int i = 0; i < 4; i++) {
        mask[i] = dis(gen);
        frame.push_back(mask[i]);
    }
    
    // Masked data
    const unsigned char* bytes = (const unsigned char*)data;
    for (size_t i = 0; i < len; i++) {
        frame.push_back(bytes[i] ^ mask[i % 4]);
    }
    
    return ws_send_raw(conn, frame.data(), frame.size());
}

// Receive WebSocket frame
static bool ws_recv_frame(WebSocketConnection* conn, std::string& data, WSOpcode& opcode) {
    unsigned char header[2];
    if (ws_recv_raw(conn, header, 2) != 2) {
        return false;
    }
    
    bool fin = (header[0] & 0x80) != 0;
    opcode = (WSOpcode)(header[0] & 0x0F);
    bool masked = (header[1] & 0x80) != 0;
    size_t len = header[1] & 0x7F;
    
    if (len == 126) {
        unsigned char ext[2];
        if (ws_recv_raw(conn, ext, 2) != 2) return false;
        len = (ext[0] << 8) | ext[1];
    } else if (len == 127) {
        unsigned char ext[8];
        if (ws_recv_raw(conn, ext, 8) != 8) return false;
        len = 0;
        for (int i = 0; i < 8; i++) {
            len = (len << 8) | ext[i];
        }
    }
    
    unsigned char mask[4] = {0};
    if (masked) {
        if (ws_recv_raw(conn, mask, 4) != 4) return false;
    }
    
    data.resize(len);
    if (len > 0) {
        size_t received = 0;
        while (received < len) {
            int n = ws_recv_raw(conn, &data[received], len - received);
            if (n <= 0) return false;
            received += n;
        }
        
        if (masked) {
            for (size_t i = 0; i < len; i++) {
                data[i] ^= mask[i % 4];
            }
        }
    }
    
    (void)fin;
    return true;
}

// Receive thread
static void ws_recv_thread(WebSocketConnection* conn) {
    while (conn->running && conn->state == WS_OPEN) {
        std::string data;
        WSOpcode opcode;
        
        if (!ws_recv_frame(conn, data, opcode)) {
            break;
        }
        
        switch (opcode) {
            case WS_TEXT:
            case WS_BINARY: {
                std::lock_guard<std::mutex> lock(conn->recv_mutex);
                conn->recv_queue.push(data);
                conn->recv_cv.notify_one();
                break;
            }
            case WS_CLOSE:
                conn->state = WS_CLOSED;
                break;
            case WS_PING:
                ws_send_frame(conn, WS_PONG, data.c_str(), data.size());
                break;
            default:
                break;
        }
    }
    
    conn->state = WS_CLOSED;
}

// Get WebSocket from userdata
static WebSocketConnection* get_websocket(lua_State* L, int idx) {
    WebSocketConnection** ud = (WebSocketConnection**)luaL_checkudata(L, idx, WEBSOCKET_MT);
    if (!ud || !*ud) {
        luaL_error(L, "Invalid WebSocket object");
        return nullptr;
    }
    return *ud;
}

// WebSocket:Send(data)
static int ws_send(lua_State* L) {
    WebSocketConnection* conn = get_websocket(L, 1);
    size_t len;
    const char* data = luaL_checklstring(L, 2, &len);
    
    if (conn->state != WS_OPEN) {
        lua_pushboolean(L, false);
        return 1;
    }
    
    bool success = ws_send_frame(conn, WS_TEXT, data, len);
    lua_pushboolean(L, success);
    return 1;
}

// WebSocket:Close()
static int ws_close(lua_State* L) {
    WebSocketConnection* conn = get_websocket(L, 1);
    
    if (conn->state == WS_OPEN) {
        ws_send_frame(conn, WS_CLOSE, nullptr, 0);
        conn->state = WS_CLOSING;
    }
    
    conn->close_connection();
    return 0;
}

// WebSocket:OnMessage(callback)
static int ws_on_message(lua_State* L) {
    WebSocketConnection* conn = get_websocket(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    
    if (conn->on_message_ref != LUA_NOREF) {
        lua_unref(L, conn->on_message_ref);
    }
    
    lua_pushvalue(L, 2);
    conn->on_message_ref = lua_ref(L, -1);
    lua_pop(L, 1);
    conn->L = L;
    
    return 0;
}

// WebSocket:OnClose(callback)
static int ws_on_close(lua_State* L) {
    WebSocketConnection* conn = get_websocket(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    
    if (conn->on_close_ref != LUA_NOREF) {
        lua_unref(L, conn->on_close_ref);
    }
    
    lua_pushvalue(L, 2);
    conn->on_close_ref = lua_ref(L, -1);
    lua_pop(L, 1);
    conn->L = L;
    
    return 0;
}

// WebSocket __index
static int ws_index(lua_State* L) {
    WebSocketConnection* conn = get_websocket(L, 1);
    const char* key = luaL_checkstring(L, 2);
    
    if (strcmp(key, "Send") == 0) {
        lua_pushcfunction(L, ws_send, "Send");
    } else if (strcmp(key, "Close") == 0) {
        lua_pushcfunction(L, ws_close, "Close");
    } else if (strcmp(key, "OnMessage") == 0) {
        lua_pushvalue(L, 1);
        lua_pushcclosure(L, ws_on_message, "OnMessage", 1);
    } else if (strcmp(key, "OnClose") == 0) {
        lua_pushvalue(L, 1);
        lua_pushcclosure(L, ws_on_close, "OnClose", 1);
    } else {
        lua_pushnil(L);
    }
    
    (void)conn;
    return 1;
}

// WebSocket __gc
static int ws_gc(lua_State* L) {
    WebSocketConnection** ud = (WebSocketConnection**)lua_touserdata(L, 1);
    if (ud && *ud) {
        std::lock_guard<std::mutex> lock(g_ws_mutex);
        g_connections.erase((*ud)->id);
        delete *ud;
        *ud = nullptr;
    }
    return 0;
}

// WebSocket.connect(url)
static int lua_websocket_connect(lua_State* L) {
    const char* url = luaL_checkstring(L, 1);
    
    WebSocketConnection* conn = new WebSocketConnection();
    conn->id = g_next_ws_id++;
    conn->url = url;
    
    if (!parse_ws_url(url, conn->host, conn->path, conn->port, conn->secure)) {
        delete conn;
        lua_pushnil(L);
        lua_pushstring(L, "Invalid WebSocket URL");
        return 2;
    }
    
    if (!ws_connect(conn)) {
        delete conn;
        lua_pushnil(L);
        lua_pushstring(L, "Connection failed");
        return 2;
    }
    
    if (!ws_handshake(conn)) {
        conn->close_connection();
        delete conn;
        lua_pushnil(L);
        lua_pushstring(L, "Handshake failed");
        return 2;
    }
    
    conn->state = WS_OPEN;
    conn->running = true;
    conn->recv_thread = std::thread(ws_recv_thread, conn);
    
    {
        std::lock_guard<std::mutex> lock(g_ws_mutex);
        g_connections[conn->id] = conn;
    }
    
    // Create userdata
    WebSocketConnection** ud = (WebSocketConnection**)lua_newuserdata(L, sizeof(WebSocketConnection*));
    *ud = conn;
    
    luaL_getmetatable(L, WEBSOCKET_MT);
    lua_setmetatable(L, -2);
    
    return 1;
}

// Register WebSocket library
void xoron_register_websocket(lua_State* L) {
    // Create metatable
    luaL_newmetatable(L, WEBSOCKET_MT);
    
    lua_pushcfunction(L, ws_index, "__index");
    lua_setfield(L, -2, "__index");
    
    lua_pushcfunction(L, ws_gc, "__gc");
    lua_setfield(L, -2, "__gc");
    
    lua_pop(L, 1);
    
    // Create WebSocket table
    lua_newtable(L);
    
    lua_pushcfunction(L, lua_websocket_connect, "connect");
    lua_setfield(L, -2, "connect");
    
    lua_setglobal(L, "WebSocket");
    
    // Also register syn.websocket for compatibility
    lua_getglobal(L, "syn");
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        lua_newtable(L);
    }
    
    lua_newtable(L);
    lua_pushcfunction(L, lua_websocket_connect, "connect");
    lua_setfield(L, -2, "connect");
    lua_setfield(L, -2, "websocket");
    
    lua_setglobal(L, "syn");
}
