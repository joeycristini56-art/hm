# Component Model

## Component Relationships

This document details how Xoron's components interact, their dependencies, and data flow between them.

## Component Overview

### Central Hub: xoron_luau.cpp

All components are orchestrated through the Luau VM wrapper:

```cpp
xoron_luau.cpp
    ├── Registers all libraries
    ├── Manages VM lifecycle
    ├── Handles Lua ↔ C++ bridging
    └── Coordinates execution flow
```

### Dependency Matrix

| Component | Depends On | Used By | Key Dependencies |
|-----------|------------|---------|------------------|
| xoron_luau.cpp | Luau VM | All Lua scripts | luau, xoron.h |
| xoron_http.cpp | cpp-httplib, OpenSSL | xoron_luau.cpp | httplib.h |
| xoron_crypto.cpp | OpenSSL | xoron_luau.cpp | sha.h, evp.h |
| xoron_websocket.cpp | POSIX sockets, OpenSSL | xoron_luau.cpp | sys/socket.h |
| xoron_drawing.cpp | Platform graphics | xoron_luau.cpp | CoreGraphics/Android |
| xoron_env.cpp | LZ4, Platform APIs | xoron_luau.cpp | lz4.h |
| xoron_filesystem.cpp | Standard C I/O | xoron_luau.cpp | stdio.h |
| xoron_memory.cpp | LZ4 | xoron_luau.cpp | lz4.h |
| xoron_console.cpp | Platform logging | xoron_luau.cpp | Platform headers |
| xoron_input.cpp | Platform input | xoron_luau.cpp | Platform headers |
| xoron_cache.cpp | Standard containers | xoron_luau.cpp | unordered_map |
| xoron_ui.cpp | Platform UI | xoron_luau.cpp | UIKit/Android |
| xoron_android.cpp | Android NDK, JNI | xoron_luau.cpp | jni.h, log.h |
| xoron_ios.mm | iOS SDK, Obj-C | xoron_luau.cpp | Foundation, UIKit |

## Data Flow Patterns

### 1. Lua → C++ → Platform

**Example: HTTP GET Request**

```lua
-- Lua Layer
local response = http.get("https://api.example.com/data")
```

↓

```cpp
// xoron_luau.cpp
static int lua_http_get(lua_State* L) {
    const char* url = luaL_checkstring(L, 1);
    int status; size_t len;
    char* response = xoron_http_get(url, &status, &len);
    // ... return to Lua
}
```

↓

```cpp
// xoron_http.cpp
char* xoron_http_get(const char* url, int* status, size_t* len) {
    // Parse URL
    // Use cpp-httplib
    // Return response
}
```

### 2. Platform → C++ → Lua

**Example: Touch Event**

```cpp
// xoron_android.cpp (JNI callback)
extern "C" JNIEXPORT void JNICALL
Java_com_xoron_InputHandler_onTouch(JNIEnv* env, jobject thiz, 
                                    jfloat x, jfloat y, jint id) {
    // Get Lua state
    lua_State* L = xoron_android_get_lua_state();
    
    // Push event to Lua
    lua_getglobal(L, "Input");
    lua_getfield(L, -1, "TouchBegan");
    // ... call Lua callbacks
}
```

↓

```lua
-- Lua Layer
Input.TouchBegan:Connect(function(x, y, id)
    print("Touch at", x, y, "ID:", id)
end)
```

### 3. C++ → Platform → C++ → Lua

**Example: Drawing**

```lua
-- Lua Layer
Drawing:Line(100, 100, 200, 200, Color3.new(1, 0, 0), 2)
```

↓

```cpp
// xoron_luau.cpp
static int lua_drawing_line(lua_State* L) {
    float x1 = luaL_checknumber(L, 1);
    float y1 = luaL_checknumber(L, 2);
    // ... get parameters
    
    // Add to drawing queue
    DrawingObject obj;
    obj.type = DRAWING_LINE;
    // ... set properties
    
    // Thread-safe queue operation
    {
        std::lock_guard<std::mutex> lock(g_drawing_mutex);
        g_drawing_queue.push_back(obj);
    }
    
    return 0;
}
```

↓

```cpp
// xoron_drawing.cpp (Render Thread)
void render_thread_func() {
    while (running) {
        // Get objects from queue
        std::vector<DrawingObject> objects;
        {
            std::lock_guard<std::mutex> lock(g_drawing_mutex);
            objects = g_drawing_queue;
            g_drawing_queue.clear();
        }
        
        // Sort by z-index
        std::sort(objects.begin(), objects.end(), 
                  [](auto& a, auto& b) { return a.zindex < b.zindex; });
        
        // Render each object
        for (auto& obj : objects) {
            if (obj.visible) {
                render_object(obj);
            }
        }
    }
}
```

↓

```cpp
// xoron_android.cpp / xoron_ios.mm
void render_object(DrawingObject& obj) {
#if defined(XORON_PLATFORM_IOS)
    // CoreGraphics rendering
    CGContextRef ctx = get_current_context();
    CGContextSetStrokeColor(ctx, obj.color.r, obj.color.g, obj.color.b);
    CGContextMoveToPoint(ctx, obj.from.x, obj.from.y);
    CGContextAddLineToPoint(ctx, obj.to.x, obj.to.y);
    CGContextStrokePath(ctx);
#elif defined(XORON_PLATFORM_ANDROID)
    // Android Canvas
    ACanvas* canvas = get_current_canvas();
    canvas->drawLine(obj.from.x, obj.from.y, 
                     obj.to.x, obj.to.y, 
                     obj.color.r, obj.color.g, obj.color.b, 
                     obj.thickness);
#endif
}
```

## Component Communication Patterns

### Pattern 1: Direct Function Calls

**Use Case**: Simple operations, no state sharing

```cpp
// xoron_luau.cpp calls xoron_crypto.cpp
void xoron_sha256(const void* data, size_t len, uint8_t out[32]);
```

**Characteristics**:
- Synchronous
- No shared state
- Simple parameter passing
- Fast execution

### Pattern 2: Callback Registration

**Use Case**: Event-driven operations (WebSocket, Input)

```cpp
// Lua registers callback
ws:on_message(function(msg) ... end)

// C++ stores reference
int callback_ref = luaL_ref(L, LUA_REGISTRYINDEX);
connection->on_message_ref = callback_ref;

// C++ calls callback when event occurs
lua_rawgeti(L, LUA_REGISTRYINDEX, callback_ref);
lua_pushstring(L, message.c_str());
lua_call(L, 1, 0);
```

**Characteristics**:
- Asynchronous
- Bidirectional communication
- Stateful
- Requires reference management

### Pattern 3: Shared State with Mutex

**Use Case**: Multi-threaded operations (Drawing, Cache)

```cpp
// Global state
std::vector<DrawingObject> g_drawing_queue;
std::mutex g_drawing_mutex;

// Writer (Lua thread)
{
    std::lock_guard<std::mutex> lock(g_drawing_mutex);
    g_drawing_queue.push_back(obj);
}

// Reader (Render thread)
{
    std::lock_guard<std::mutex> lock(g_drawing_mutex);
    auto queue = g_drawing_queue;
    g_drawing_queue.clear();
}
```

**Characteristics**:
- Thread-safe
- Shared memory
- Requires synchronization
- Potential for contention

### Pattern 4: Platform Bridge

**Use Case**: Platform-specific functionality

```cpp
// xoron_luau.cpp
void xoron_register_android(lua_State* L) {
    // Register Android-specific functions
    lua_pushcfunction(L, lua_android_haptic);
    lua_setglobal(L, "hapticFeedback");
}

// xoron_android.cpp
void xoron_android_haptic_feedback(int style) {
    // JNI call to Java
    JNIEnv* env = get_jni_env();
    env->CallVoidMethod(g_haptic_method, style);
}
```

**Characteristics**:
- Platform abstraction
- May involve JNI/Obj-C
- Conditional compilation
- Platform-specific dependencies

## Component Lifecycle

### VM Lifecycle

```
1. xoron_init()
   └─> Initialize global state
       └─> g_state.initialized = true

2. xoron_vm_new()
   └─> Create Lua state
       └─> lua_newstate(allocator, ud)
       └─> Open standard libraries
       └─> Register all XORON libraries
       └─> Return xoron_vm_t wrapper

3. xoron_dostring() / xoron_dofile()
   └─> Compile source
       └─> luau_compile()
       └─> xoron_run()
       └─> Execute bytecode

4. xoron_vm_free()
   └─> Close Lua state
       └─> lua_close()
       └─> Free wrapper

5. xoron_shutdown()
   └─> Cleanup global state
       └─> Clear caches
       └─> g_state.initialized = false
```

### WebSocket Lifecycle

```
1. Lua: ws = WebSocket.connect("wss://...")
   └─> xoron_luau.cpp: lua_websocket_connect()
       └─> xoron_websocket.cpp: create_connection()
           └─> Parse URL
           └─> socket() / SSL_new()
           └─> connect() / SSL_connect()
           └─> Start recv_thread

2. Lua: ws:on_message(callback)
   └─> Store callback reference
       └─> luaL_ref(L, LUA_REGISTRYINDEX)
       └─> connection->on_message_ref = ref

3. recv_thread
   └─> while (running)
       └─> recv() / SSL_read()
       └─> Queue message
       └─> Signal condition_variable

4. Main thread
   └─> Check queue
       └─> Get callback reference
       └─> lua_rawgeti() + lua_call()

5. Lua: ws:close()
   └─> xoron_websocket.cpp: close_connection()
       └─> running = false
       └─> Close socket/SSL
       └─> Join thread
       └─> Clean up references
```

### Drawing Lifecycle

```
1. Lua: Drawing:Line(...)
   └─> xoron_luau.cpp: lua_drawing_line()
       └─> Create DrawingObject
       └─> Add to queue (mutex-protected)

2. Render Thread (started at init)
   └─> while (running)
       └─> Lock mutex
       └─> Copy queue
       └─> Clear queue
       └─> Unlock mutex
       └─> Sort by z-index
       └─> For each object:
           └─> Check visibility
           └─> Call platform renderer
       └─> Sleep (for FPS cap)

3. Platform Renderer
   └─> iOS: CoreGraphics
       └─> CGContext functions
   └─> Android: Canvas
       └─> ACanvas functions

4. Lua: Drawing:Clear()
   └─> Clear queue
       └─> Lock mutex
       └─> queue.clear()
       └─> Unlock mutex
```

## Data Structures

### Core Structures

```cpp
// VM Wrapper
struct xoron_vm {
    lua_State* L;           // Lua state
    bool owns_state;        // Ownership flag
};

// Bytecode Container
struct xoron_bytecode {
    std::string data;       // Compiled bytecode
    std::string name;       // Script name for errors
};

// Global State
struct {
    bool initialized;
    std::mutex mutex;
    xoron_output_fn print_fn;
    xoron_output_fn error_fn;
    void* output_ud;
    std::string last_error;
} g_state;
```

### Service Structures

```cpp
// HTTP Response
struct HttpResponse {
    int status;
    std::string body;
    std::map<std::string, std::string> headers;
};

// WebSocket Connection
struct WebSocketConnection {
    uint32_t id;
    std::string url;
    int socket_fd;
    SSL* ssl;
    WSState state;
    std::thread recv_thread;
    std::atomic<bool> running;
    std::queue<std::string> recv_queue;
    std::mutex send_mutex;
    lua_State* L;
    int on_message_ref;
    int on_close_ref;
    int on_error_ref;
};

// Drawing Object
struct DrawingObject {
    DrawingType type;
    bool visible;
    float transparency;
    Color3 color;
    int zindex;
    uint32_t id;
    // Type-specific properties...
};

// Hook Entry
struct HookEntry {
    lua_State* L;
    int original_ref;
    int hook_ref;
};

// Cache Entry
struct CacheEntry {
    std::string value;
    std::chrono::steady_clock::time_point expiry;
};
```

## Error Handling Flow

### Error Propagation

```
1. Platform Error (e.g., network failure)
   └─> xoron_http.cpp
       └─> xoron_set_error("Connection failed")
       └─> Return nullptr

2. Service Layer Error
   └─> xoron_luau.cpp
       └─> Check return value
       └─> lua_pushnil()
       └─> lua_pushstring(error_message)
       └─> Return 2 (nil + error)

3. Lua Error
   └─> lua_error() called
       └─> Protected call (lua_pcall)
       └─> Error handler captures stack
       └─> xoron_set_error() with details

4. Application Layer
   └─> User code
       └─> local success, err = pcall(function() ... end)
       └─> Handle error
```

### Error Recovery

```cpp
// Example: HTTP with error handling
static int lua_http_get(lua_State* L) {
    const char* url = luaL_checkstring(L, 1);
    int status; size_t len;
    
    char* response = xoron_http_get(url, &status, &len);
    
    if (response) {
        // Success: return table with status and body
        lua_newtable(L);
        lua_pushinteger(L, status);
        lua_setfield(L, -2, "status");
        lua_pushlstring(L, response, len);
        lua_setfield(L, -2, "body");
        xoron_http_free(response);
        return 1;
    } else {
        // Failure: return nil and error message
        lua_pushnil(L);
        lua_pushstring(L, xoron_last_error());
        return 2;
    }
}
```

## Thread Safety Model

### Thread Roles

1. **Main Thread** (Lua execution)
   - Executes Lua scripts
   - Calls C++ functions
   - Registers callbacks
   - Manages VM

2. **WebSocket Thread** (per connection)
   - Receives messages
   - Queues messages
   - Signals main thread

3. **Render Thread** (global)
   - Processes drawing queue
   - Renders to screen
   - Manages FPS

4. **I/O Threads** (optional)
   - Async file operations
   - Background downloads

### Synchronization Points

```cpp
// Mutex hierarchy (to prevent deadlock)
// Level 1: Global state mutex
std::lock_guard<std::mutex> lock(g_state.mutex);

// Level 2: Service-specific mutexes
std::lock_guard<std::mutex> lock(g_drawing_mutex);
std::lock_guard<std::mutex> lock(g_cache_mutex);
std::lock_guard<std::mutex> lock(g_connection_mutex);

// Level 3: Per-object mutexes (if needed)
std::lock_guard<std::mutex> lock(obj->mutex);
```

### Lock-Free Patterns

```cpp
// Atomic counters
std::atomic<int> g_ref_count;
std::atomic<bool> g_running;

// Lock-free queue (for simple cases)
std::queue<T> queue;
std::atomic<bool> processing{false};

// Try to acquire processing lock
bool expected = false;
if (processing.compare_exchange_strong(expected, true)) {
    // Process queue
    // ...
    processing = false;
}
```

## Memory Ownership

### Ownership Rules

1. **Lua-owned**: Objects created in Lua, garbage collected by Lua
   - Callback references
   - Userdata
   - Strings returned to Lua

2. **C++-owned**: Objects managed by C++ destructors
   - VM wrappers
   - Connection objects
   - Service instances

3. **Platform-owned**: Objects managed by platform APIs
   - iOS: ARC (Objective-C objects)
   - Android: JNI references
   - Graphics: Platform allocators

4. **Shared ownership**: Smart pointers for complex cases
   - `std::shared_ptr` for long-lived objects
   - `std::unique_ptr` for exclusive ownership
   - `std::weak_ptr` to break cycles

### Memory Management Examples

```cpp
// C++ owns, Lua borrows
char* xoron_http_get(...) {
    char* response = (char*)malloc(len + 1);
    // ... fill response
    return response;  // Caller must free
}

// xoron_http_free() - explicit cleanup
void xoron_http_free(char* response) {
    free(response);
}

// Lua wrapper manages lifetime
static int lua_http_get(lua_State* L) {
    char* response = xoron_http_get(...);
    if (response) {
        // Push as Lua string (copies data)
        lua_pushstring(L, response);
        xoron_http_free(response);  // Free immediately
        return 1;
    }
    return 0;
}
```

## Extension Points

### Adding New Component

**Step 1**: Create service file
```cpp
// xoron_newservice.cpp
#include "xoron.h"

extern "C" {
    char* xoron_newservice_operation(const char* input) {
        // Implementation
    }
}
```

**Step 2**: Register in xoron_luau.cpp
```cpp
static void register_newservice(lua_State* L) {
    lua_pushcfunction(L, lua_newservice_operation);
    lua_setglobal(L, "newservice");
}
```

**Step 3**: Call from xoron_luau.cpp init
```cpp
void xoron_register_all_libraries(lua_State* L) {
    // ... existing registrations
    register_newservice(L);
}
```

**Step 4**: Add to CMakeLists.txt
```cmake
set(XORON_SOURCES
    # ... existing sources
    xoron_newservice.cpp
)
```

### Adding Platform-Specific Feature

**Step 1**: Add to platform file
```cpp
// xoron_android.cpp
void xoron_android_new_feature(int param) {
    // JNI implementation
}
```

**Step 2**: Add to header
```cpp
// xoron.h
#if defined(XORON_PLATFORM_ANDROID)
void xoron_android_new_feature(int param);
#endif
```

**Step 3**: Register in Lua
```cpp
// xoron_luau.cpp
static int lua_new_feature(lua_State* L) {
    int param = luaL_checkinteger(L, 1);
#if defined(XORON_PLATFORM_ANDROID)
    xoron_android_new_feature(param);
#elif defined(XORON_PLATFORM_IOS)
    xoron_ios_new_feature(param);
#endif
    return 0;
}
```

## Performance Considerations

### Hot Paths

1. **Drawing Loop**
   - Called 60+ times per second
   - Optimize: Batch rendering, avoid allocations

2. **WebSocket Message Handler**
   - High-frequency messages
   - Optimize: Lock-free queue, minimal processing

3. **HTTP Requests**
   - Network latency dominates
   - Optimize: Connection pooling, keep-alive

4. **Lua → C++ Calls**
   - Parameter marshalling overhead
   - Optimize: Batch operations, minimize crossings

### Optimization Strategies

```cpp
// Bad: Frequent allocations in hot loop
for (auto& obj : objects) {
    std::string data = format_object(obj);  // Allocates
    process(data);
}

// Good: Reuse buffers
std::string buffer;
buffer.reserve(1024);
for (auto& obj : objects) {
    buffer.clear();
    format_object(obj, buffer);  // Reuses buffer
    process(buffer);
}
```

## Testing Component Interactions

### Unit Test Pattern

```cpp
TEST(HTTPComponent, GetRequest) {
    // Mock server
    MockServer server;
    server.on_get("/test").return(200, "OK");
    
    // Test component
    int status; size_t len;
    char* response = xoron_http_get(server.url(), &status, &len);
    
    ASSERT_EQ(status, 200);
    ASSERT_STREQ(response, "OK");
    xoron_http_free(response);
}
```

### Integration Test Pattern

```cpp
TEST(FullIntegration, ScriptWithHTTP) {
    xoron_vm_t* vm = xoron_vm_new();
    
    const char* script = R"(
        local response = http.get("https://api.example.com")
        assert(response.status == 200)
    )";
    
    int result = xoron_dostring(vm, script, "test");
    ASSERT_EQ(result, 0);  // No error
    
    xoron_vm_free(vm);
}
```

## Common Patterns and Anti-Patterns

### ✅ Good Patterns

1. **RAII for Resources**
```cpp
class ConnectionGuard {
    WebSocketConnection* conn;
public:
    ConnectionGuard(WebSocketConnection* c) : conn(c) {}
    ~ConnectionGuard() { if (conn) conn->close(); }
};
```

2. **Callback Reference Management**
```cpp
// Store reference
int ref = luaL_ref(L, LUA_REGISTRYINDEX);

// Use reference
lua_rawgeti(L, LUA_REGISTRYINDEX, ref);

// Clean up
luaL_unref(L, LUA_REGISTRYINDEX, ref);
```

3. **Thread-Safe Queues**
```cpp
std::queue<T> queue;
std::mutex mutex;
std::condition_variable cv;

// Producer
{
    std::lock_guard<std::mutex> lock(mutex);
    queue.push(item);
}
cv.notify_one();

// Consumer
std::unique_lock<std::mutex> lock(mutex);
cv.wait(lock, [&]{ return !queue.empty(); });
auto item = queue.front();
queue.pop();
```

### ❌ Anti-Patterns

1. **Raw Pointers Without Ownership**
```cpp
// BAD: Who owns this?
char* data = malloc(100);
return data;  // Leaks if caller forgets to free
```

2. **Global State Without Protection**
```cpp
// BAD: Race condition
std::vector<T> global_vector;

void add(T item) {
    global_vector.push_back(item);  // Not thread-safe!
}
```

3. **Lua References Without Cleanup**
```cpp
// BAD: Reference leak
int ref = luaL_ref(L, LUA_REGISTRYINDEX);
// ... never call luaL_unref
```

4. **Blocking in Main Thread**
```cpp
// BAD: Freezes Lua execution
static int lua_sleep(lua_State* L) {
    sleep(10);  // Blocks everything!
    return 0;
}
```

## Summary

The component model emphasizes:
- **Clear separation**: Each component has a single responsibility
- **Minimal coupling**: Components interact through well-defined interfaces
- **Thread safety**: Proper synchronization for concurrent operations
- **Resource management**: RAII and explicit ownership
- **Extensibility**: Easy to add new features
- **Testability**: Components can be tested independently

---

**Next:**
- [Execution Flow](execution_flow.md) - How scripts are executed
- [Memory Model](memory_model.md) - Memory management details
- [Security Model](security_model.md) - Security architecture
