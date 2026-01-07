# 10. xoron_websocket.cpp - WebSocket Client

**File Path**: `src/src/xoron_websocket.cpp`  
**Size**: 16,646 bytes  
**Lines**: 633

**Platform**: Cross-platform (POSIX sockets with OpenSSL)

## Overview
Provides WebSocket client functionality for real-time bidirectional communication. Supports both plain WebSocket (ws://) and secure WebSocket (wss://) connections.

## Includes
- `xoron.h` (local)
- `cstdlib`, `cstring`, `cstdio` (system)
- `string`, `vector`, `queue`, `unordered_map` (system)
- `mutex`, `thread`, `atomic`, `condition_variable` (system)
- `functional` (system)
- `random` (system)
- Network: `sys/socket.h`, `netinet/in.h`, `netdb.h`, `unistd.h`, `arpa/inet.h`, `fcntl.h`
- OpenSSL: `ssl.h`, `err.h`, `sha.h`
- `lua.h`, `lualib.h` (local - Luau)

## Core Architecture

### WebSocket State
```cpp
enum WSState {
    WS_DISCONNECTED = 0,
    WS_CONNECTING,
    WS_CONNECTED,
    WS_CLOSING,
    WS_ERROR
};
```

### WebSocket Object
```cpp
struct WebSocket {
    uint32_t id;
    std::string url;
    WSState state;
    int socket_fd;
    SSL* ssl_ctx;
    std::thread read_thread;
    std::thread write_thread;
    std::queue<std::string> send_queue;
    std::mutex send_mutex;
    std::condition_variable send_cv;
    std::vector<std::string> received_messages;
    std::mutex receive_mutex;
    std::function<void(const std::string&)> on_message;
    std::function<void()> on_open;
    std::function<void(int, const std::string&)> on_close;
    std::function<void(const std::string&)> on_error;
    bool closing;
    std::atomic<bool> alive{true};
};
```

### WebSocket Manager
```cpp
struct WebSocketManager {
    std::unordered_map<uint32_t, WebSocket*> connections;
    std::mutex mutex;
    std::atomic<uint32_t> next_id{1};
};
```

## C API Functions

### xoron_websocket_connect
```c
uint32_t xoron_websocket_connect(const char* url, 
                                  void (*on_message)(const char*, void*),
                                  void (*on_open)(void*),
                                  void (*on_close)(int, const char*, void*),
                                  void (*on_error)(const char*, void*),
                                  void* user_data)
```
Connects to WebSocket server.

**Parameters**:
- `url`: WebSocket URL (ws:// or wss://)
- `on_message`: Message callback
- `on_open`: Open callback
- `on_close`: Close callback with code and reason
- `on_error`: Error callback
- `user_data`: User data passed to callbacks

**Returns**: Connection ID or 0 on error

### xoron_websocket_send
```c
bool xoron_websocket_send(uint32_t id, const char* data)
```
Sends data over WebSocket.

**Parameters**:
- `id`: Connection ID
- `data`: Data to send

**Returns**: `true` on success

### xoron_websocket_close
```c
bool xoron_websocket_close(uint32_t id)
```
Closes WebSocket connection.

**Parameters**:
- `id`: Connection ID

**Returns**: `true` on success

### xoron_websocket_is_connected
```c
bool xoron_websocket_is_connected(uint32_t id)
```
Checks if WebSocket is connected.

**Parameters**:
- `id`: Connection ID

**Returns**: `true` if connected

### xoron_websocket_get_messages
```c
char* xoron_websocket_get_messages(uint32_t id)
```
Retrieves received messages.

**Parameters**:
- `id`: Connection ID

**Returns**: JSON array of messages (must be freed)

## Lua API

All functions are registered in the `websocket` global table.

### websocket.connect
```lua
websocket.connect(url: string, callbacks: table?) -> number
```
Connects to WebSocket server.

**Parameters**:
- `url`: WebSocket URL
- `callbacks`: Optional callback table

**Callbacks table**:
```lua
{
    on_message = function(data) end,
    on_open = function() end,
    on_close = function(code, reason) end,
    on_error = function(error) end
}
```

**Returns**: Connection ID

**Example**:
```lua
local ws_id = websocket.connect("wss://echo.websocket.org", {
    on_message = function(data)
        print("Received:", data)
    end,
    on_open = function()
        print("Connected!")
        websocket.send(ws_id, "Hello!")
    end,
    on_close = function(code, reason)
        print("Closed:", code, reason)
    end,
    on_error = function(err)
        print("Error:", err)
    end
})
```

### websocket.send
```lua
websocket.send(id: number, data: string) -> boolean
```
Sends data over WebSocket.

**Parameters**:
- `id`: Connection ID
- `data`: Data to send

**Returns**: true on success

**Example**:
```lua
websocket.send(ws_id, "Hello, Server!")
websocket.send(ws_id, json.encode({type = "message", content = "test"}))
```

### websocket.close
```lua
websocket.close(id: number) -> boolean
```
Closes WebSocket connection.

**Parameters**:
- `id`: Connection ID

**Returns**: true on success

**Example**:
```lua
websocket.close(ws_id)
```

### websocket.is_connected
```lua
websocket.is_connected(id: number) -> boolean
```
Checks if WebSocket is connected.

**Parameters**:
- `id`: Connection ID

**Returns**: true if connected

### websocket.get_messages
```lua
websocket.get_messages(id: number) -> table
```
Gets all received messages.

**Parameters**:
- `id`: Connection ID

**Returns**: Array of messages

**Example**:
```lua
local messages = websocket.get_messages(ws_id)
for _, msg in ipairs(messages) do
    print(msg)
end
```

### websocket.get_state
```lua
websocket.get_state(id: number) -> string
```
Gets connection state.

**Parameters**:
- `id`: Connection ID

**Returns**: "disconnected", "connecting", "connected", "closing", "error"

### websocket.get_url
```lua
websocket.get_url(id: number) -> string
```
Gets connection URL.

**Parameters**:
- `id`: Connection ID

**Returns**: URL string

### websocket.disconnect_all
```lua
websocket.disconnect_all() -> void
```
Closes all WebSocket connections.

### websocket.list
```lua
websocket.list() -> table
```
Lists all active connections.

**Returns**: Array of connection info

**Each entry**:
```lua
{
    id = 1,
    url = "wss://example.com",
    state = "connected"
}
```

## Protocol Details

### WebSocket Handshake
```
Client → Server:
GET /chat HTTP/1.1
Host: example.com
Upgrade: websocket
Connection: Upgrade
Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==
Sec-WebSocket-Version: 13

Server → Client:
HTTP/1.1 101 Switching Protocols
Upgrade: websocket
Connection: Upgrade
Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=
```

### Frame Format
```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-------+-+-------------+-------------------------------+
|F|R|R|R| opcode|M| Payload len |    Extended payload length    |
|I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
|N|V|V|V|       |S|             |   (if payload len==126/127)   |
| |1|2|3|       |K|             |                               |
+-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
|     Extended payload length continued, if payload len == 127  |
+ - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
|                     Masking-Key (0-3)                          |
+ - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
|                     Payload Data continued ...                |
+ - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
```

### Opcodes
- `0x0`: Continuation frame
- `0x1`: Text frame
- `0x2`: Binary frame
- `0x8`: Close frame
- `0x9`: Ping frame
- `0xA`: Pong frame

### Close Codes
- `1000`: Normal closure
- `1001`: Going away
- `1002`: Protocol error
- `1003`: Unsupported data
- `1006`: Abnormal closure
- `1007`: Invalid frame data
- `1008`: Policy violation
- `1009`: Message too big
- `1010`: Extension required
- `1011`: Internal server error

## Usage Examples

### Basic Echo Client
```lua
local ws = websocket.connect("wss://echo.websocket.org", {
    on_open = function()
        print("Connected!")
        websocket.send(ws, "Hello, Echo!")
    end,
    on_message = function(data)
        print("Echo:", data)
        websocket.close(ws)
    end
})
```

### Chat Application
```lua
local chat_ws = websocket.connect("wss://chat.example.com", {
    on_open = function()
        print("Connected to chat")
        -- Send join message
        websocket.send(chat_ws, json.encode({
            type = "join",
            username = "Player1"
        }))
    end,
    
    on_message = function(data)
        local msg = json.decode(data)
        if msg.type == "message" then
            print(msg.username .. ": " .. msg.text)
        elseif msg.type == "system" then
            print("[SYSTEM] " .. msg.text)
        end
    end,
    
    on_close = function(code, reason)
        print("Disconnected: " .. reason)
    end,
    
    on_error = function(err)
        print("Error: " .. err)
    end
})

-- Send message
local function send_chat(text)
    websocket.send(chat_ws, json.encode({
        type = "message",
        text = text
    }))
end
```

### Game State Sync
```lua
local sync_ws = websocket.connect("wss://sync.example.com", {
    on_open = function()
        print("Sync connected")
    end,
    on_message = function(data)
        local state = json.decode(data)
        -- Update game state
        update_game_state(state)
    end
})

-- Send updates
function send_position(x, y)
    websocket.send(sync_ws, json.encode({
        type = "position",
        x = x,
        y = y,
        timestamp = os.time()
    }))
end

-- Periodic updates
while true do
    local pos = get_player_position()
    send_position(pos.x, pos.y)
    wait(0.1)  -- 10 updates per second
end
```

### Binary Data Transfer
```lua
-- Send binary data
local ws = websocket.connect("wss://binary.example.com", {
    on_message = function(data)
        -- Binary data as string
        print("Received " .. #data .. " bytes")
    end
})

-- Send file
local file_data = fs.readfile("image.png")
websocket.send(ws, file_data)
```

### Reconnection Logic
```lua
local ws_id = nil
local reconnect_delay = 5

function connect()
    ws_id = websocket.connect("wss://server.example.com", {
        on_open = function()
            print("Connected")
            reconnect_delay = 5  -- Reset delay
        end,
        
        on_close = function(code, reason)
            print("Disconnected, reconnecting in " .. reconnect_delay .. "s")
            wait(reconnect_delay)
            reconnect_delay = math.min(reconnect_delay * 2, 60)  -- Exponential backoff
            connect()
        end,
        
        on_error = function(err)
            print("Error:", err)
        end,
        
        on_message = function(data)
            print("Message:", data)
        end
    })
end

connect()
```

### Heartbeat/Ping
```lua
local ws = websocket.connect("wss://server.example.com", {
    on_open = function()
        print("Connected")
        
        -- Start heartbeat
        spawn(function()
            while websocket.is_connected(ws) do
                websocket.send(ws, json.encode({type = "ping"}))
                wait(30)  -- Every 30 seconds
            end
        end)
    end,
    
    on_message = function(data)
        local msg = json.decode(data)
        if msg.type == "pong" then
            print("Heartbeat received")
        end
    end
})
```

### Multiple Connections
```lua
local connections = {}

-- Connect to multiple servers
local servers = {
    "wss://server1.example.com",
    "wss://server2.example.com",
    "wss://server3.example.com"
}

for i, url in ipairs(servers) do
    local id = websocket.connect(url, {
        on_open = function()
            print("Connected to " .. url)
            connections[url] = id
        end,
        on_message = function(data)
            print("From " .. url .. ": " .. data)
        end
    })
end

-- Broadcast
function broadcast(message)
    for _, id in pairs(connections) do
        websocket.send(id, message)
    end
end
```

### Message Queue
```lua
local ws = websocket.connect("wss://server.example.com")
local message_queue = {}

-- Queue messages while connecting
function send_or_queue(data)
    if websocket.is_connected(ws) then
        websocket.send(ws, data)
    else
        table.insert(message_queue, data)
    end
end

-- Send queued messages on connect
local callbacks = {
    on_open = function()
        print("Connected, sending queued messages")
        for _, msg in ipairs(message_queue) do
            websocket.send(ws, msg)
        end
        message_queue = {}
    end
}

-- Update callbacks
-- (Note: In real implementation, you'd need to re-register callbacks)
```

### Error Handling
```lua
local ws = websocket.connect("wss://server.example.com", {
    on_error = function(err)
        print("WebSocket Error:", err)
        
        if err:find("connection refused") then
            print("Server unavailable")
        elseif err:find("timeout") then
            print("Connection timeout")
        elseif err:find("TLS") then
            print("SSL/TLS error")
        end
    end,
    
    on_close = function(code, reason)
        local close_codes = {
            [1000] = "Normal closure",
            [1001] = "Going away",
            [1002] = "Protocol error",
            [1006] = "Abnormal closure"
        }
        
        print("Closed:", close_codes[code] or ("Code " .. code))
        if reason ~= "" then
            print("Reason:", reason)
        end
    end
})
```

### Protocol Upgrade
```lua
-- Connect with custom headers
-- (Note: This would require extended API)
local ws = websocket.connect("wss://server.example.com", {
    headers = {
        ["Authorization"] = "Bearer token123",
        ["X-Custom"] = "value"
    },
    on_open = function()
        print("Authenticated connection")
    end
})
```

### Large Message Handling
```lua
local ws = websocket.connect("wss://server.example.com", {
    on_message = function(data)
        if #data > 1024 * 1024 then  -- > 1MB
            print("Large message received:", #data, "bytes")
            -- Process in chunks or save to file
            fs.writefile("large_data.bin", data)
        else
            print("Message:", data)
        end
    end
})
```

### Connection Pool
```lua
local Pool = {
    connections = {},
    max_connections = 5
}

function Pool:connect(url, callbacks)
    if #self.connections >= self.max_connections then
        print("Max connections reached")
        return nil
    end
    
    local id = websocket.connect(url, callbacks)
    if id > 0 then
        table.insert(self.connections, {id = id, url = url})
        return id
    end
    return nil
end

function Pool:send_all(data)
    for _, conn in ipairs(self.connections) do
        websocket.send(conn.id, data)
    end
end

function Pool:close_all()
    for _, conn in ipairs(self.connections) do
        websocket.close(conn.id)
    end
    self.connections = {}
end
```

## Implementation Details

### Connection Handling
- Uses POSIX sockets for network I/O
- Non-blocking socket operations
- Separate threads for read and write
- Thread-safe message queues

### TLS/SSL Support
- OpenSSL for wss:// connections
- Certificate verification disabled for flexibility
- Can be enabled in production

### Frame Encoding/Decoding
- RFC 6455 compliant
- Supports text and binary frames
- Handles fragmentation
- Masking for client-to-server frames

### Error Handling
- Socket errors
- TLS handshake failures
- Protocol violations
- Network timeouts

### Performance
- Asynchronous I/O with threads
- Lock-free queues where possible
- Minimal copying of data
- Efficient frame parsing

## Limitations

1. **No Server Mode**: Only client functionality
2. **No Compression**: No permessage-deflate extension
3. **No Authentication**: Basic auth only via headers
4. **No Ping/Pong Auto**: Manual ping implementation needed
5. **No Subprotocols**: No WebSocket subprotocol negotiation

## Security Considerations

- **wss://**: Always use secure WebSocket in production
- **Certificate Validation**: Disabled by default (for development)
- **Input Validation**: Validate all received data
- **Rate Limiting**: Implement on server side
- **Origin Checking**: Server should validate Origin header

## Platform Notes

### iOS
- Uses BSD sockets (compatible with iOS networking)
- Background thread handling
- May require background mode entitlement

### Android
- Uses BSD sockets via NDK
- Requires INTERNET permission
- Works with both WiFi and mobile data

### Network Requirements
- Firewall must allow outbound connections on port 80 (ws) or 443 (wss)
- Proxy support not included
- IPv4 and IPv6 supported

## Related Files
- `xoron.h` (API declarations)
- `xoron_luau.cpp` (Lua bindings)
- `xoron_http.cpp` (HTTP client for comparison)
