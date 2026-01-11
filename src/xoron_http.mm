/*
 * xoron_http.cpp - HTTP client using cpp-httplib
 * Platforms: iOS (.dylib) and Android (.so)
 */

#include "xoron.h"
#include <string>
#include <cstring>
#include <cstdlib>

// OpenSSL support is defined via CMake (CPPHTTPLIB_OPENSSL_SUPPORT)
#include "httplib.h"

extern void xoron_set_error(const char* fmt, ...);

static bool parse_url(const char* url, std::string& scheme, std::string& host, int& port, std::string& path) {
    std::string u = url;
    
    size_t scheme_end = u.find("://");
    if (scheme_end != std::string::npos) {
        scheme = u.substr(0, scheme_end);
        u = u.substr(scheme_end + 3);
    } else {
        scheme = "http";
    }
    
    size_t path_start = u.find('/');
    std::string host_port = (path_start != std::string::npos) ? u.substr(0, path_start) : u;
    path = (path_start != std::string::npos) ? u.substr(path_start) : "/";
    
    size_t port_start = host_port.find(':');
    if (port_start != std::string::npos) {
        host = host_port.substr(0, port_start);
        port = std::stoi(host_port.substr(port_start + 1));
    } else {
        host = host_port;
        port = (scheme == "https") ? 443 : 80;
    }
    
    return !host.empty();
}

extern "C" {

char* xoron_http_get(const char* url, int* status, size_t* len) {
    if (!url) { xoron_set_error("URL is null"); return nullptr; }
    
    std::string scheme, host, path;
    int port;
    if (!parse_url(url, scheme, host, port, path)) {
        xoron_set_error("Invalid URL");
        return nullptr;
    }
    
    try {
        httplib::Result res;
        if (scheme == "https") {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
            httplib::SSLClient cli(host, port);
            cli.set_connection_timeout(30, 0);
            cli.set_read_timeout(30, 0);
            cli.enable_server_certificate_verification(false);
            res = cli.Get(path);
#else
            xoron_set_error("HTTPS not supported (OpenSSL not available)");
            return nullptr;
#endif
        } else {
            httplib::Client cli(host, port);
            cli.set_connection_timeout(30, 0);
            cli.set_read_timeout(30, 0);
            res = cli.Get(path);
        }
        
        if (!res) {
            xoron_set_error("HTTP request failed: %s", httplib::to_string(res.error()).c_str());
            return nullptr;
        }
        
        if (status) *status = res->status;
        if (len) *len = res->body.size();
        
        char* body = (char*)malloc(res->body.size() + 1);
        if (body) {
            memcpy(body, res->body.c_str(), res->body.size());
            body[res->body.size()] = '\0';
        }
        return body;
    } catch (const std::exception& e) {
        xoron_set_error("HTTP exception: %s", e.what());
        return nullptr;
    }
}

char* xoron_http_post(const char* url, const char* body, size_t body_len,
                      const char* content_type, int* status, size_t* len) {
    if (!url) { xoron_set_error("URL is null"); return nullptr; }
    
    std::string scheme, host, path;
    int port;
    if (!parse_url(url, scheme, host, port, path)) {
        xoron_set_error("Invalid URL");
        return nullptr;
    }
    
    std::string req_body = body ? std::string(body, body_len) : "";
    std::string ct = content_type ? content_type : "application/json";
    
    try {
        httplib::Result res;
        if (scheme == "https") {
            httplib::SSLClient cli(host, port);
            cli.set_connection_timeout(30, 0);
            cli.set_read_timeout(30, 0);
            cli.enable_server_certificate_verification(false);
            res = cli.Post(path, req_body, ct);
        } else {
            httplib::Client cli(host, port);
            cli.set_connection_timeout(30, 0);
            cli.set_read_timeout(30, 0);
            res = cli.Post(path, req_body, ct);
        }
        
        if (!res) {
            xoron_set_error("HTTP request failed: %s", httplib::to_string(res.error()).c_str());
            return nullptr;
        }
        
        if (status) *status = res->status;
        if (len) *len = res->body.size();
        
        char* resp_body = (char*)malloc(res->body.size() + 1);
        if (resp_body) {
            memcpy(resp_body, res->body.c_str(), res->body.size());
            resp_body[res->body.size()] = '\0';
        }
        return resp_body;
    } catch (const std::exception& e) {
        xoron_set_error("HTTP exception: %s", e.what());
        return nullptr;
    }
}

void xoron_http_free(char* response) {
    free(response);
}

}

// Additional HTTP methods for Lua bindings
static char* http_request_method(const char* method, const char* url, const char* body, 
                                  size_t body_len, const char* content_type, 
                                  int* status, size_t* len) {
    if (!url) { xoron_set_error("URL is null"); return nullptr; }
    
    std::string scheme, host, path;
    int port;
    if (!parse_url(url, scheme, host, port, path)) {
        xoron_set_error("Invalid URL");
        return nullptr;
    }
    
    std::string req_body = body ? std::string(body, body_len) : "";
    std::string ct = content_type ? content_type : "application/json";
    
    try {
        httplib::Result res;
        
        auto make_request = [&](auto& cli) -> httplib::Result {
            if (strcmp(method, "PUT") == 0) {
                return cli.Put(path, req_body, ct);
            } else if (strcmp(method, "DELETE") == 0) {
                if (req_body.empty()) {
                    return cli.Delete(path);
                } else {
                    return cli.Delete(path, req_body, ct);
                }
            } else if (strcmp(method, "PATCH") == 0) {
                return cli.Patch(path, req_body, ct);
            } else if (strcmp(method, "HEAD") == 0) {
                return cli.Head(path);
            } else if (strcmp(method, "OPTIONS") == 0) {
                return cli.Options(path);
            }
            return httplib::Result{nullptr, httplib::Error::Unknown};
        };
        
        if (scheme == "https") {
            httplib::SSLClient cli(host, port);
            cli.set_connection_timeout(30, 0);
            cli.set_read_timeout(30, 0);
            cli.enable_server_certificate_verification(false);
            res = make_request(cli);
        } else {
            httplib::Client cli(host, port);
            cli.set_connection_timeout(30, 0);
            cli.set_read_timeout(30, 0);
            res = make_request(cli);
        }
        
        if (!res) {
            xoron_set_error("HTTP request failed: %s", httplib::to_string(res.error()).c_str());
            return nullptr;
        }
        
        if (status) *status = res->status;
        if (len) *len = res->body.size();
        
        char* resp_body = (char*)malloc(res->body.size() + 1);
        if (resp_body) {
            memcpy(resp_body, res->body.c_str(), res->body.size());
            resp_body[res->body.size()] = '\0';
        }
        return resp_body;
    } catch (const std::exception& e) {
        xoron_set_error("HTTP exception: %s", e.what());
        return nullptr;
    }
}

// Lua HTTP request function with full options
#include "lua.h"
#include "lualib.h"

static int lua_http_request_full(lua_State* L) {
    // Accept either a table or URL string
    std::string url;
    std::string method = "GET";
    std::string body;
    std::string content_type = "application/json";
    httplib::Headers headers;
    
    if (lua_istable(L, 1)) {
        // Table format: {Url = "...", Method = "...", Body = "...", Headers = {...}}
        lua_getfield(L, 1, "Url");
        if (lua_isstring(L, -1)) url = lua_tostring(L, -1);
        lua_pop(L, 1);
        
        lua_getfield(L, 1, "Method");
        if (lua_isstring(L, -1)) method = lua_tostring(L, -1);
        lua_pop(L, 1);
        
        lua_getfield(L, 1, "Body");
        if (lua_isstring(L, -1)) {
            size_t len;
            const char* b = lua_tolstring(L, -1, &len);
            body = std::string(b, len);
        }
        lua_pop(L, 1);
        
        lua_getfield(L, 1, "Headers");
        if (lua_istable(L, -1)) {
            lua_pushnil(L);
            while (lua_next(L, -2) != 0) {
                if (lua_isstring(L, -2) && lua_isstring(L, -1)) {
                    headers.emplace(lua_tostring(L, -2), lua_tostring(L, -1));
                }
                lua_pop(L, 1);
            }
        }
        lua_pop(L, 1);
        
        // Check for ContentType in Headers or as separate field
        lua_getfield(L, 1, "ContentType");
        if (lua_isstring(L, -1)) content_type = lua_tostring(L, -1);
        lua_pop(L, 1);
    } else {
        url = luaL_checkstring(L, 1);
    }
    
    if (url.empty()) {
        lua_pushnil(L);
        lua_pushstring(L, "URL is required");
        return 2;
    }
    
    // Parse URL
    std::string scheme, host, path;
    int port;
    if (!parse_url(url.c_str(), scheme, host, port, path)) {
        lua_pushnil(L);
        lua_pushstring(L, "Invalid URL");
        return 2;
    }
    
    try {
        httplib::Result res;
        
        auto make_request = [&](auto& cli) -> httplib::Result {
            // Set headers
            for (const auto& h : headers) {
                cli.set_default_headers({{h.first, h.second}});
            }
            
            if (method == "GET") {
                return cli.Get(path);
            } else if (method == "POST") {
                return cli.Post(path, body, content_type);
            } else if (method == "PUT") {
                return cli.Put(path, body, content_type);
            } else if (method == "DELETE") {
                if (body.empty()) {
                    return cli.Delete(path);
                } else {
                    return cli.Delete(path, body, content_type);
                }
            } else if (method == "PATCH") {
                return cli.Patch(path, body, content_type);
            } else if (method == "HEAD") {
                return cli.Head(path);
            } else if (method == "OPTIONS") {
                return cli.Options(path);
            }
            return httplib::Result{nullptr, httplib::Error::Unknown};
        };
        
        if (scheme == "https") {
            httplib::SSLClient cli(host, port);
            cli.set_connection_timeout(30, 0);
            cli.set_read_timeout(30, 0);
            cli.enable_server_certificate_verification(false);
            res = make_request(cli);
        } else {
            httplib::Client cli(host, port);
            cli.set_connection_timeout(30, 0);
            cli.set_read_timeout(30, 0);
            res = make_request(cli);
        }
        
        if (!res) {
            lua_pushnil(L);
            lua_pushstring(L, httplib::to_string(res.error()).c_str());
            return 2;
        }
        
        // Return response table
        lua_newtable(L);
        
        lua_pushboolean(L, res->status >= 200 && res->status < 300);
        lua_setfield(L, -2, "Success");
        
        lua_pushinteger(L, res->status);
        lua_setfield(L, -2, "StatusCode");
        
        lua_pushstring(L, res->reason.c_str());
        lua_setfield(L, -2, "StatusMessage");
        
        lua_pushlstring(L, res->body.c_str(), res->body.size());
        lua_setfield(L, -2, "Body");
        
        // Headers table
        lua_newtable(L);
        for (const auto& h : res->headers) {
            lua_pushstring(L, h.second.c_str());
            lua_setfield(L, -2, h.first.c_str());
        }
        lua_setfield(L, -2, "Headers");
        
        return 1;
    } catch (const std::exception& e) {
        lua_pushnil(L);
        lua_pushstring(L, e.what());
        return 2;
    }
}

// Register HTTP functions in Lua
void xoron_register_http(lua_State* L) {
    // Create http table
    lua_newtable(L);
    
    lua_pushcfunction(L, lua_http_request_full, "request");
    lua_setfield(L, -2, "request");
    
    lua_setglobal(L, "http");
    
    // Also register as global request function
    lua_pushcfunction(L, lua_http_request_full, "request");
    lua_setglobal(L, "request");
}
