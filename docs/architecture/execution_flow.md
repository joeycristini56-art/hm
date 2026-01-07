# Execution Flow

## Overview

This document details the complete execution flow of a script in Xoron, from source code to runtime execution, including compilation, library registration, and error handling.

## High-Level Flow

```
User Script (Lua Source)
    ↓
1. Initialization Phase
    - xoron_init()
    - xoron_vm_new()
    - Register libraries
    ↓
2. Compilation Phase
    - xoron_compile() / xoron_dostring()
    - Luau Compiler
    - Bytecode generation
    ↓
3. Execution Phase
    - xoron_run()
    - VM execution
    - Library function calls
    ↓
4. Runtime Phase
    - Service operations
    - Platform calls
    - Results to Lua
    ↓
5. Cleanup Phase
    - xoron_vm_free()
    - xoron_shutdown()
```

## Detailed Execution Flow

### Phase 1: Initialization

#### 1.1 Global State Initialization

```cpp
// Entry point
int main() {
    // Step 1: Initialize global state
    int result = xoron_init();
    // result = XORON_OK (0) on success
}
```

**xoron_init() implementation:**
```cpp
int xoron_init(void) {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    
    if (g_state.initialized) {
        return XORON_OK;  // Already initialized
    }
    
    // Initialize OpenSSL
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();
    
    // Initialize logging
    g_state.print_fn = nullptr;
    g_state.error_fn = nullptr;
    g_state.output_ud = nullptr;
    g_state.last_error = "";
    
    // Mark as initialized
    g_state.initialized = true;
    
    XORON_LOG("Xoron initialized (version %s)", XORON_VERSION);
    return XORON_OK;
}
```

#### 1.2 VM Creation

```cpp
// Step 2: Create VM instance
xoron_vm_t* vm = xoron_vm_new();
```

**xoron_vm_new() implementation:**
```cpp
xoron_vm_t* xoron_vm_new(void) {
    if (!g_state.initialized) {
        xoron_set_error("Xoron not initialized");
        return nullptr;
    }
    
    // Allocate VM structure
    xoron_vm_t* vm = (xoron_vm_t*)malloc(sizeof(xoron_vm_t));
    if (!vm) {
        xoron_set_error("Failed to allocate VM");
        return nullptr;
    }
    
    // Create Lua state with custom allocator
    vm->L = lua_newstate(luau_alloc, nullptr);
    if (!vm->L) {
        free(vm);
        xoron_set_error("Failed to create Lua state");
        return nullptr;
    }
    
    // Open standard libraries
    luaL_openlibs(vm->L);
    
    // Register all Xoron libraries
    xoron_register_all_libraries(vm->L);
    
    XORON_LOG("VM created: %p", vm);
    return vm;
}
```

#### 1.3 Library Registration

```cpp
void xoron_register_all_libraries(lua_State* L) {
    // Core environment
    xoron_register_env(L);
    
    // Networking
    xoron_register_http(L);
    xoron_register_websocket(L);
    
    // Cryptography
    xoron_register_crypto(L);
    
    // Graphics
    xoron_register_drawing(L);
    xoron_register_ui(L);
    
    // System
    xoron_register_filesystem(L);
    xoron_register_memory(L);
    xoron_register_console(L);
    xoron_register_input(L);
    xoron_register_cache(L);
    xoron_register_debug(L);
    
    // Platform-specific
#if defined(XORON_PLATFORM_IOS)
    xoron_register_ios(L);
#elif defined(XORON_PLATFORM_ANDROID)
    xoron_register_android(L);
#endif
}
```

**Example: HTTP Library Registration**
```cpp
void xoron_register_http(lua_State* L) {
    // Create HTTP table
    lua_newtable(L);
    
    // Register http.get
    lua_pushcfunction(L, lua_http_get);
    lua_setfield(L, -2, "get");
    
    // Register http.post
    lua_pushcfunction(L, lua_http_post);
    lua_setfield(L, -2, "post");
    
    // Set as global
    lua_setglobal(L, "http");
}
```

### Phase 2: Compilation

#### 2.1 Source Compilation

```cpp
// Step 3: Compile and execute
const char* source = "print('Hello, Xoron!')";
int result = xoron_dostring(vm, source, "main");
```

**xoron_dostring() implementation:**
```cpp
int xoron_dostring(xoron_vm_t* vm, const char* source, const char* name) {
    if (!vm || !source) {
        return XORON_ERR_INVALID;
    }
    
    // Step 2.1: Compile source to bytecode
    xoron_bytecode_t* bc = xoron_compile(source, strlen(source), name);
    if (!bc) {
        return XORON_ERR_COMPILE;
    }
    
    // Step 2.2: Run bytecode
    int result = xoron_run(vm, bc);
    
    // Cleanup
    xoron_bytecode_free(bc);
    
    return result;
}
```

#### 2.2 Bytecode Compilation

```cpp
xoron_bytecode_t* xoron_compile(const char* source, size_t len, const char* name) {
    // Allocate bytecode structure
    xoron_bytecode_t* bc = (xoron_bytecode_t*)malloc(sizeof(xoron_bytecode_t));
    if (!bc) return nullptr;
    
    bc->name = name ? name : "unknown";
    
    // Use Luau compiler
    Luau::CompileOptions options;
    options.optimizationLevel = 1;
    options.debugLevel = 1;
    
    // Compile
    std::string bytecode = Luau::compile(source, options);
    
    if (bytecode.empty()) {
        xoron_set_error("Compilation failed");
        free(bc);
        return nullptr;
    }
    
    bc->data = bytecode;
    
    XORON_LOG("Compiled '%s' (%zu bytes)", name, bytecode.size());
    return bc;
}
```

**Luau Compilation Process:**
```
Source Code
    ↓
Lexer (Tokenization)
    ↓
Parser (AST Construction)
    ↓
Type Checker (Optional)
    ↓
Optimizer (Dead code elimination, constant folding)
    ↓
Code Generator (Bytecode emission)
    ↓
Bytecode String
```

### Phase 3: Execution

#### 3.1 Bytecode Execution

```cpp
int xoron_run(xoron_vm_t* vm, xoron_bytecode_t* bc) {
    if (!vm || !bc) {
        return XORON_ERR_INVALID;
    }
    
    lua_State* L = vm->L;
    
    // Load bytecode
    int load_result = luaL_loadbuffer(L, bc->data.c_str(), bc->data.size(), bc->name.c_str());
    
    if (load_result != LUA_OK) {
        // Compilation error
        const char* error = lua_tostring(L, -1);
        xoron_set_error("Load error: %s", error);
        lua_pop(L, 1);  // Pop error
        return XORON_ERR_COMPILE;
    }
    
    // Execute with error handler
    lua_pushcfunction(L, xoron_error_handler);
    int errfunc = lua_gettop(L);
    
    // Move function to top
    lua_pushvalue(L, -2);
    lua_remove(L, -3);
    
    // Protected call
    int result = lua_pcall(L, 0, LUA_MULTRET, errfunc);
    
    if (result != LUA_OK) {
        // Runtime error
        const char* error = lua_tostring(L, -1);
        xoron_set_error("Runtime error: %s", error);
        lua_pop(L, 2);  // Pop error and handler
        return XORON_ERR_RUNTIME;
    }
    
    // Success - clean up handler
    lua_remove(L, errfunc);
    
    XORON_LOG("Execution successful");
    return XORON_OK;
}
```

#### 3.2 Error Handler

```cpp
static int xoron_error_handler(lua_State* L) {
    // Get stack trace
    luaL_traceback(L, L, lua_tostring(L, 1), 1);
    
    // Log error
    const char* traceback = lua_tostring(L, -1);
    xoron_set_error("%s", traceback);
    
    // Return traceback
    return 1;
}
```

### Phase 4: Runtime Operations

#### 4.1 Library Function Call Flow

**Example: HTTP GET**

```lua
-- Lua script
local response = http.get("https://api.example.com/data")
print(response.status)
print(response.body)
```

**Execution Flow:**

1. **Lua → C++ Bridge**
```cpp
// xoron_luau.cpp
static int lua_http_get(lua_State* L) {
    // Step 4.1.1: Extract parameters from Lua stack
    const char* url = luaL_checkstring(L, 1);
    
    // Step 4.1.2: Call C++ implementation
    int status; size_t len;
    char* response = xoron_http_get(url, &status, &len);
    
    // Step 4.1.3: Handle result
    if (response) {
        // Create result table
        lua_newtable(L);
        
        // Add status field
        lua_pushinteger(L, status);
        lua_setfield(L, -2, "status");
        
        // Add body field
        lua_pushlstring(L, response, len);
        lua_setfield(L, -2, "body");
        
        // Free response
        xoron_http_free(response);
        
        return 1;  // Return table
    } else {
        // Error case
        lua_pushnil(L);
        lua_pushstring(L, xoron_last_error());
        return 2;  // Return nil, error
    }
}
```

2. **C++ Implementation**
```cpp
// xoron_http.cpp
char* xoron_http_get(const char* url, int* status, size_t* len) {
    // Parse URL
    std::string scheme, host, path;
    int port;
    if (!parse_url(url, scheme, host, port, path)) {
        xoron_set_error("Invalid URL");
        return nullptr;
    }
    
    // Create client
    if (scheme == "https") {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
        httplib::SSLClient cli(host, port);
        cli.set_connection_timeout(30, 0);
        cli.set_read_timeout(30, 0);
        cli.enable_server_certificate_verification(false);
        
        // Make request
        httplib::Result res = cli.Get(path);
        
        if (!res) {
            xoron_set_error("Request failed: %s", 
                           httplib::to_string(res.error()).c_str());
            return nullptr;
        }
        
        // Allocate and copy response
        *status = res->status;
        *len = res->body.size();
        
        char* body = (char*)malloc(res->body.size() + 1);
        memcpy(body, res->body.c_str(), res->body.size());
        body[res->body.size()] = '\0';
        
        return body;
#else
        xoron_set_error("HTTPS not supported");
        return nullptr;
#endif
    } else {
        httplib::Client cli(host, port);
        cli.set_connection_timeout(30, 0);
        cli.set_read_timeout(30, 0);
        
        httplib::Result res = cli.Get(path);
        
        if (!res) {
            xoron_set_error("Request failed: %s", 
                           httplib::to_string(res.error()).c_str());
            return nullptr;
        }
        
        *status = res->status;
        *len = res->body.size();
        
        char* body = (char*)malloc(res->body.size() + 1);
        memcpy(body, res->body.c_str(), res->body.size());
        body[res->body.size()] = '\0';
        
        return body;
    }
}
```

3. **Result to Lua**
```cpp
// Back in lua_http_get
lua_newtable(L);
lua_pushinteger(L, status);
lua_setfield(L, -2, "status");
lua_pushlstring(L, response, len);
lua_setfield(L, -2, "body");
xoron_http_free(response);
return 1;
```

#### 4.2 Asynchronous Operations

**WebSocket Message Flow:**

```lua
-- Lua script
ws = WebSocket.connect("wss://echo.websocket.org")

ws:on_message(function(msg)
    print("Received:", msg)
end)

ws:send("Hello")
```

**Execution Flow:**

1. **Connection**
```cpp
// xoron_luau.cpp
static int lua_websocket_connect(lua_State* L) {
    const char* url = luaL_checkstring(L, 1);
    
    // Create connection object
    WebSocketConnection* conn = create_connection(url);
    if (!conn) {
        lua_pushnil(L);
        lua_pushstring(L, xoron_last_error());
        return 2;
    }
    
    // Store in Lua userdata
    WebSocketConnection** ud = (WebSocketConnection**)lua_newuserdata(L, sizeof(WebSocketConnection*));
    *ud = conn;
    
    // Set metatable for methods
    luaL_getmetatable(L, "WebSocket");
    lua_setmetatable(L, -2);
    
    return 1;
}
```

2. **Callback Registration**
```cpp
static int lua_websocket_on_message(lua_State* L) {
    // Get connection
    WebSocketConnection** ud = (WebSocketConnection**)luaL_checkudata(L, 1, "WebSocket");
    WebSocketConnection* conn = *ud;
    
    // Get callback function
    luaL_checktype(L, 2, LUA_TFUNCTION);
    
    // Store reference
    lua_pushvalue(L, 2);  // Copy function
    int ref = luaL_ref(L, LUA_REGISTRYINDEX);
    conn->on_message_ref = ref;
    conn->L = L;  // Store Lua state
    
    return 0;
}
```

3. **Background Thread**
```cpp
// xoron_websocket.cpp
void WebSocketConnection::start_recv_thread() {
    recv_thread = std::thread([this]() {
        while (running) {
            // Receive data
            char buffer[4096];
            ssize_t n = recv(socket_fd, buffer, sizeof(buffer), 0);
            
            if (n > 0) {
                // Parse WebSocket frame
                std::string message = parse_websocket_frame(buffer, n);
                
                // Queue message
                {
                    std::lock_guard<std::mutex> lock(recv_mutex);
                    recv_queue.push(message);
                }
                
                // Signal main thread
                recv_cv.notify_one();
            }
        }
    });
}
```

4. **Message Processing**
```cpp
// In main thread (called periodically or via event)
void process_websocket_messages() {
    for (auto& conn : active_connections) {
        std::string msg;
        
        // Get message from queue
        {
            std::lock_guard<std::mutex> lock(conn->recv_mutex);
            if (!conn->recv_queue.empty()) {
                msg = conn->recv_queue.front();
                conn->recv_queue.pop();
            }
        }
        
        if (!msg.empty() && conn->on_message_ref != LUA_NOREF) {
            // Call Lua callback
            lua_rawgeti(conn->L, LUA_REGISTRYINDEX, conn->on_message_ref);
            lua_pushstring(conn->L, msg.c_str());
            lua_call(conn->L, 1, 0);
        }
    }
}
```

#### 4.3 Drawing Operations

```lua
-- Lua script
local draw = Drawing.new()

-- Create objects
draw:Line(100, 100, 200, 200, Color3.new(1, 0, 0), 2)
draw:Circle(150, 150, 50, Color3.new(0, 1, 0), true)

-- Update
draw:Clear()
```

**Execution Flow:**

1. **Lua Call**
```cpp
static int lua_drawing_line(lua_State* L) {
    // Extract parameters
    float x1 = luaL_checknumber(L, 1);
    float y1 = luaL_checknumber(L, 2);
    float x2 = luaL_checknumber(L, 3);
    float y2 = luaL_checknumber(L, 4);
    
    // Get color
    luaL_checktype(L, 5, LUA_TTABLE);
    lua_getfield(L, 5, "r");
    float r = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    
    float thickness = luaL_optnumber(L, 6, 1.0f);
    
    // Create object
    DrawingObject obj;
    obj.type = DRAWING_LINE;
    obj.from = Vector2(x1, y1);
    obj.to = Vector2(x2, y2);
    obj.color = Color3(r, g, b);
    obj.thickness = thickness;
    obj.zindex = 0;
    obj.visible = true;
    
    // Add to queue (thread-safe)
    {
        std::lock_guard<std::mutex> lock(g_drawing_mutex);
        g_drawing_queue.push_back(obj);
    }
    
    return 0;
}
```

2. **Render Thread**
```cpp
void drawing_render_thread() {
    while (g_drawing_running) {
        // Get objects from queue
        std::vector<DrawingObject> objects;
        {
            std::lock_guard<std::mutex> lock(g_drawing_mutex);
            objects = g_drawing_queue;
            g_drawing_queue.clear();
        }
        
        // Sort by z-index
        std::sort(objects.begin(), objects.end(), 
                  [](const DrawingObject& a, const DrawingObject& b) {
                      return a.zindex < b.zindex;
                  });
        
        // Render each object
        for (const auto& obj : objects) {
            if (!obj.visible) continue;
            
            render_object(obj);
        }
        
        // FPS limiting
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / g_fps_cap));
    }
}
```

3. **Platform Rendering**
```cpp
void render_object(const DrawingObject& obj) {
#if defined(XORON_PLATFORM_IOS)
    CGContextRef ctx = get_current_context();
    
    // Set color
    CGContextSetRGBStrokeColor(ctx, obj.color.r, obj.color.g, obj.color.b, 1.0);
    CGContextSetLineWidth(ctx, obj.thickness);
    
    // Draw based on type
    switch (obj.type) {
        case DRAWING_LINE:
            CGContextMoveToPoint(ctx, obj.from.x, obj.from.y);
            CGContextAddLineToPoint(ctx, obj.to.x, obj.to.y);
            CGContextStrokePath(ctx);
            break;
            
        case DRAWING_CIRCLE:
            CGContextAddArc(ctx, obj.position.x, obj.position.y, 
                           obj.radius, 0, 2 * M_PI, 0);
            if (obj.filled) {
                CGContextFillPath(ctx);
            } else {
                CGContextStrokePath(ctx);
            }
            break;
            
        // ... other types
    }
    
#elif defined(XORON_PLATFORM_ANDROID)
    ACanvas* canvas = get_current_canvas();
    
    // Draw based on type
    switch (obj.type) {
        case DRAWING_LINE:
            canvas->drawLine(obj.from.x, obj.from.y, 
                            obj.to.x, obj.to.y,
                            obj.color.r, obj.color.g, obj.color.b,
                            obj.thickness);
            break;
            
        // ... other types
    }
#endif
}
```

### Phase 5: Cleanup

#### 5.1 VM Cleanup

```cpp
// Step 5: Cleanup
xoron_vm_free(vm);
```

**xoron_vm_free() implementation:**
```cpp
void xoron_vm_free(xoron_vm_t* vm) {
    if (!vm) return;
    
    if (vm->L) {
        // Close Lua state
        // This triggers garbage collection
        lua_close(vm->L);
    }
    
    free(vm);
    XORON_LOG("VM freed: %p", vm);
}
```

#### 5.2 Global Cleanup

```cpp
xoron_shutdown();
```

**xoron_shutdown() implementation:**
```cpp
void xoron_shutdown(void) {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    
    if (!g_state.initialized) return;
    
    // Cleanup OpenSSL
    EVP_cleanup();
    ERR_free_strings();
    
    // Stop background threads
    g_drawing_running = false;
    if (g_drawing_thread.joinable()) {
        g_drawing_thread.join();
    }
    
    // Clear caches
    g_cache.clear();
    
    // Clear connections
    for (auto& conn : g_websocket_connections) {
        conn->close_connection();
    }
    g_websocket_connections.clear();
    
    g_state.initialized = false;
    XORON_LOG("Xoron shutdown");
}
```

## Error Handling Flow

### Compilation Error

```lua
-- Script with syntax error
print("Hello"
-- Missing closing parenthesis
```

**Flow:**
```
1. luaL_loadbuffer() fails
   ↓
2. Returns LUA_ERRSYNTAX
   ↓
3. lua_tostring(L, -1) gets error message
   ↓
4. xoron_set_error() stores error
   ↓
5. xoron_dostring() returns XORON_ERR_COMPILE
   ↓
6. Error message available via xoron_last_error()
```

### Runtime Error

```lua
-- Script with runtime error
local x = nil
print(x + 1)  -- Error: attempt to perform arithmetic on nil
```

**Flow:**
```
1. lua_pcall() executes
   ↓
2. Error occurs during execution
   ↓
3. xoron_error_handler() called
   ↓
4. luaL_traceback() generates stack trace
   ↓
5. xoron_set_error() stores full traceback
   ↓
6. lua_pcall() returns LUA_ERRRUN
   ↓
7. xoron_run() returns XORON_ERR_RUNTIME
   ↓
8. Full traceback available via xoron_last_error()
```

### Library Error

```lua
-- HTTP request to invalid URL
local response = http.get("not-a-valid-url")
if not response then
    print("Error:", http.last_error())
end
```

**Flow:**
```
1. lua_http_get() called
   ↓
2. xoron_http_get() fails to parse URL
   ↓
3. xoron_set_error("Invalid URL") called
   ↓
4. Returns nullptr
   ↓
5. lua_http_get() pushes nil and error message
   ↓
6. Lua code handles error
```

## Performance Characteristics

### Execution Time Breakdown

For a typical script:

```
Total: 100ms
├─ Initialization: 2ms (2%)
├─ Compilation: 8ms (8%)
├─ Execution: 15ms (15%)
│   ├─ Lua code: 5ms
│   ├─ Library calls: 10ms
│   │   ├─ HTTP: 50ms (network)
│   │   ├─ Crypto: 2ms
│   │   └─ Drawing: 3ms
│   └─ Result handling: 2ms
└─ Cleanup: 1ms (1%)
```

### Optimization Opportunities

1. **Bytecode Caching**
```cpp
// Cache compiled bytecode
std::unordered_map<std::string, xoron_bytecode_t*> g_bytecode_cache;

xoron_bytecode_t* get_cached_bytecode(const std::string& source) {
    auto it = g_bytecode_cache.find(source);
    if (it != g_bytecode_cache.end()) {
        return it->second;
    }
    
    // Compile and cache
    xoron_bytecode_t* bc = xoron_compile(source.c_str(), source.size(), "cached");
    g_bytecode_cache[source] = bc;
    return bc;
}
```

2. **Connection Pooling**
```cpp
// Reuse HTTP clients
class HTTPClientPool {
    std::map<std::string, httplib::Client> pool;
    
    httplib::Client& get_client(const std::string& host, int port) {
        std::string key = host + ":" + std::to_string(port);
        if (pool.find(key) == pool.end()) {
            pool[key] = httplib::Client(host, port);
        }
        return pool[key];
    }
};
```

3. **Batch Operations**
```cpp
// Instead of:
for (int i = 0; i < 1000; i++) {
    draw:Line(...);  // 1000 mutex locks
}

// Use:
local batch = Drawing.Batch()
for i = 1, 1000 do
    batch:Line(...)  // Single lock at end
end
batch:Commit()
```

## Common Execution Patterns

### Pattern 1: Synchronous Operation

```lua
-- Simple synchronous call
local result = crypto.sha256("data")
print(result)
```

**Flow:**
```
Lua → C++ → Crypto → Result → Lua
(Blocking, immediate return)
```

### Pattern 2: Asynchronous with Callback

```lua
-- Async HTTP with callback
http.async_get("https://api.com", function(response)
    print(response.body)
end)
print("Request sent...")
```

**Flow:**
```
Lua → C++ → Start HTTP (background)
    ↓
Lua continues execution
    ↓
HTTP completes → Callback queue
    ↓
Main thread processes callback
    ↓
Lua callback executes
```

### Pattern 3: Event-Driven

```lua
-- WebSocket events
ws:on_message(function(msg)
    print("Got:", msg)
end)

ws:on_close(function()
    print("Disconnected")
end)
```

**Flow:**
```
Background thread receives message
    ↓
Queues message
    ↓
Main thread checks queue
    ↓
Calls registered Lua callback
    ↓
Lua handles event
```

### Pattern 4: Stateful Operations

```lua
-- Drawing with state
local draw = Drawing.new()
draw:SetColor(Color3.new(1, 0, 0))
draw:Line(0, 0, 100, 100)
draw:Circle(50, 50, 20)
```

**Flow:**
```
Lua creates state object
    ↓
State stored in Lua userdata
    ↓
Methods called on object
    ↓
C++ extracts state from userdata
    ↓
Operations performed
    ↓
State updated/returned
```

## Debugging Execution

### Logging Points

```cpp
// Add at key points
XORON_LOG("VM created: %p", vm);
XORON_LOG("Compiled '%s' (%zu bytes)", name, bytecode.size());
XORON_LOG("Execution started");
XORON_LOG("HTTP request to: %s", url);
XORON_LOG("Drawing: %zu objects", objects.size());
XORON_LOG("Execution complete");
```

### Stack Inspection

```cpp
// Debug Lua stack
void debug_stack(lua_State* L, const char* label) {
    printf("\n=== Stack: %s ===\n", label);
    int top = lua_gettop(L);
    for (int i = 1; i <= top; i++) {
        int type = lua_type(L, i);
        printf("[%d] %s: %s\n", i, 
               lua_typename(L, type), 
               lua_tostring(L, i));
    }
    printf("=== End Stack ===\n\n");
}
```

### Execution Tracing

```lua
-- Enable tracing
xoron.enable_tracing(true)

-- All operations will be logged
local x = 10
print(x)
http.get("https://api.com")
```

## Summary

The execution flow ensures:
- **Safety**: Protected calls, proper error handling
- **Performance**: Efficient compilation, optimized runtime
- **Flexibility**: Supports sync, async, and event-driven patterns
- **Debugging**: Comprehensive logging and error reporting
- **Resource Management**: Proper cleanup at all stages

---

**Next:**
- [Memory Model](memory_model.md) - Memory management details
- [Security Model](security_model.md) - Security architecture
- [Component Model](component_model.md) - Component relationships
