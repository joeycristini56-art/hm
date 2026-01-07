# Xoron Source Code Documentation

## Overview

This directory contains the complete source code for the Xoron Executor Engine. The codebase is organized into modular components, each handling specific functionality while maintaining clean separation of concerns.

## Architecture Overview

```
src/
├── xoron.h              # Main header - API definitions and platform detection
├── xoron_luau.cpp       # Luau VM wrapper and script execution
├── xoron_http.cpp       # HTTP/HTTPS client implementation
├── xoron_crypto.cpp     # Cryptographic operations
├── xoron_websocket.cpp  # WebSocket client
├── xoron_drawing.cpp    # 2D rendering engine
├── xoron_env.cpp        # Lua environment management
├── xoron_filesystem.cpp # File operations and workspace
├── xoron_memory.cpp     # Memory management utilities
├── xoron_console.cpp    # Console output handling
├── xoron_input.cpp      # Input event handling
├── xoron_cache.cpp      # Data caching system
├── xoron_ui.cpp         # UI component management
├── xoron_android.cpp    # Android-specific implementations
├── xoron_ios.mm         # iOS-specific implementations
├── CMakeLists.txt       # Build configuration
├── lua/                 # Lua utility scripts
├── tests/               # Integration tests
└── workflows/           # CI/CD workflows
```

## Core Components

### 1. xoron.h - Main Header

**Purpose**: Central header file containing all API declarations, platform detection, and configuration.

**Key Features**:
- Platform detection (iOS/Android)
- Version information
- Error codes and types
- API function declarations
- Platform-specific macros

**Platform Detection**:
```c
// iOS Detection
#if defined(__APPLE__) && (TARGET_OS_IPHONE || TARGET_OS_IOS)
    #define XORON_PLATFORM_IOS 1
#endif

// Android Detection  
#if defined(__ANDROID__) || defined(ANDROID)
    #define XORON_PLATFORM_ANDROID 1
#endif
```

**Dependencies**: Standard C headers (stdint.h, stdbool.h, stddef.h)

### 2. xoron_luau.cpp - Luau VM Wrapper

**Purpose**: Wraps the Luau VM for script compilation and execution.

**Key Features**:
- VM lifecycle management (create, free, reset)
- Script compilation (source → bytecode)
- Script execution (bytecode → runtime)
- Error handling and reporting
- Lua library registration

**API Functions**:
```c
xoron_vm_t* xoron_vm_new(void);
void xoron_vm_free(xoron_vm_t* vm);
int xoron_dostring(xoron_vm_t* vm, const char* source, const char* name);
int xoron_dofile(xoron_vm_t* vm, const char* path);
```

**Lua Libraries Registered**:
- `print()` - Console output
- HTTP (GET/POST)
- Crypto (SHA256, Base64, etc.)
- Drawing API
- WebSocket
- Environment functions (getgenv, etc.)

**Dependencies**:
- Luau VM (`lua.h`, `lualib.h`)
- Luau Compiler (`luacode.h`)
- Standard C++ (string, mutex, thread)

**Integration**: This is the heart of the executor, connecting all other components through Lua bindings.

### 3. xoron_http.cpp - HTTP Client

**Purpose**: Provides HTTP/HTTPS client functionality using cpp-httplib.

**Key Features**:
- URL parsing (scheme, host, port, path)
- GET and POST methods
- HTTPS support via OpenSSL
- Configurable timeouts (30s default)
- Error handling

**Implementation Details**:
```cpp
// Uses cpp-httplib library
// HTTPS requires CPPHTTPLIB_OPENSSL_SUPPORT
// Timeout: 30 seconds connection + read
// Returns: malloc'd response buffer (must be freed)
```

**API Functions**:
```c
char* xoron_http_get(const char* url, int* status, size_t* len);
char* xoron_http_post(const char* url, const char* body, size_t body_len,
                      const char* content_type, int* status, size_t* len);
void xoron_http_free(char* response);
```

**Dependencies**:
- cpp-httplib (header-only)
- OpenSSL (for HTTPS)
- Standard C++

**Integration**: Used by `xoron_luau.cpp` to expose HTTP functions to Lua scripts.

### 4. xoron_crypto.cpp - Cryptographic Operations

**Purpose**: Comprehensive cryptographic utilities using OpenSSL.

**Key Features**:
- Hash functions: SHA256, SHA384, SHA512, MD5
- Encoding: Base64 encode/decode, Hex encode/decode
- AES encryption: CBC and GCM modes
- HMAC: Keyed-hash authentication
- Random number generation

**API Functions**:
```c
void xoron_sha256(const void* data, size_t len, uint8_t out[32]);
char* xoron_base64_encode(const void* data, size_t len);
uint8_t* xoron_base64_decode(const char* str, size_t* out_len);
char* xoron_hex_encode(const void* data, size_t len);
```

**Dependencies**:
- OpenSSL: `sha.h`, `md5.h`, `evp.h`, `hmac.h`, `rand.h`, `aes.h`
- Standard C

**Integration**: Exposed to Lua via `xoron_luau.cpp`, used by other components for encoding/encryption.

### 5. xoron_websocket.cpp - WebSocket Client

**Purpose**: Real-time bidirectional WebSocket communication.

**Key Features**:
- WebSocket protocol implementation (RFC 6455)
- TLS/SSL support (wss://)
- Asynchronous message receiving
- Event callbacks (on_message, on_close, on_error)
- Connection state management

**Architecture**:
```cpp
struct WebSocketConnection {
    // Connection state
    int socket_fd;
    SSL* ssl;
    WSState state;
    
    // Threading
    std::thread recv_thread;
    std::atomic<bool> running;
    
    // Message queues
    std::queue<std::string> recv_queue;
    std::mutex send_mutex;
    
    // Lua callbacks
    lua_State* L;
    int on_message_ref;
    int on_close_ref;
    int on_error_ref;
};
```

**API Functions** (Lua):
```lua
-- Connect to WebSocket
local ws = WebSocket.connect("wss://example.com/ws")

-- Set callbacks
ws:on_message(function(msg)
    print("Received:", msg)
end)

ws:on_close(function()
    print("Connection closed")
end)

-- Send message
ws:send("Hello")
```

**Dependencies**:
- POSIX sockets (`sys/socket.h`, `netinet/in.h`)
- OpenSSL (`ssl.h`, `sha.h` for handshake)
- Standard C++ (thread, mutex, queue, atomic)

**Integration**: Registered as Lua library in `xoron_luau.cpp`.

### 6. xoron_drawing.cpp - 2D Rendering Engine

**Purpose**: Real-time 2D graphics rendering for overlays.

**Key Features**:
- Shape primitives: Line, Circle, Square, Triangle, Quad
- Text rendering with custom fonts
- Image rendering
- Color management (RGB)
- Transparency support
- Z-index layering
- Platform-specific rendering backends

**Drawing Object Structure**:
```cpp
struct DrawingObject {
    DrawingType type;
    Vector2 position, size, from, to;
    Color3 color;
    float transparency, thickness;
    int zindex;
    bool visible;
    // ... type-specific properties
};
```

**Platform-Specific Rendering**:
- **iOS**: CoreGraphics (CGContextRef)
- **Android**: Native Canvas

**API Functions** (Lua):
```lua
local draw = Drawing.new()

-- Line
draw:Line(100, 100, 200, 200, Color3.new(1, 0, 0), 2)

-- Circle
draw:Circle(150, 150, 50, Color3.new(0, 1, 0), true)

-- Text
draw:Text("Hello", 100, 200, 16, Color3.new(1, 1, 1))

-- Update
draw:Clear()
```

**Dependencies**:
- Platform-specific: CoreGraphics (iOS), Android NDK
- Standard C++ (vector, unordered_map, mutex)

**Integration**: Registered in `xoron_luau.cpp`, platform-specific code in `xoron_android.cpp` and `xoron_ios.mm`.

### 7. xoron_env.cpp - Environment Management

**Purpose**: Custom Lua environment with advanced features.

**Key Features**:
- Global environment management (getgenv, getrenv, getmenv, getsenv)
- Function hooking and replacement
- Signal connection management
- FPS control
- Teleport queue management
- Clipboard operations
- Anti-detection mechanisms
- Identity management

**Environment Functions** (Lua):
```lua
-- Environment access
getgenv()      -- Global environment
getrenv()      -- Roblox environment
getsenv(script) -- Script environment
getmenv(module) -- Module environment

-- Function hooking
hookfunction(func, replacement)
hookmetamethod(object, method, replacement)

-- Signals
connect(signal, callback)

-- Utility
identifyexecutor()  -- Returns "Xoron 2.0.0"
getfpscap()         -- Get FPS limit
setfpscap(60)       -- Set FPS limit
```

**Hooking Implementation**:
```cpp
struct HookEntry {
    lua_State* L;
    int original_ref;
    int hook_ref;
};
```

**Platform-Specific**:
- **Android**: JNI for clipboard, system properties
- **iOS**: Objective-C for clipboard, device info

**Dependencies**:
- Luau VM internals
- Platform-specific: JNI (Android), Foundation (iOS)
- LZ4 compression

**Integration**: Core component registered in `xoron_luau.cpp`, used by all scripts.

### 8. xoron_filesystem.cpp - File System Operations

**Purpose**: Cross-platform file operations and workspace management.

**Key Features**:
- Workspace path management
- Auto-execute directory
- Script storage
- File read/write operations
- Directory creation
- Path resolution

**Platform Paths**:
```c
// iOS
~/Documents/Xoron/workspace/
~/Documents/Xoron/autoexecute/
~/Documents/Xoron/scripts/

// Android
/data/data/<package>/files/Xoron/workspace/
/storage/emulated/0/Xoron/workspace/ (external)
```

**API Functions**:
```c
const char* xoron_get_workspace(void);
void xoron_set_workspace(const char* path);
const char* xoron_get_autoexecute_path(void);
const char* xoron_get_scripts_path(void);
```

**Lua Integration**:
```lua
-- File operations
local content = readfile("workspace/script.lua")
writefile("workspace/output.txt", "data")
appendfile("workspace/log.txt", "line\n")
deletefile("workspace/temp.txt")

-- Path management
setworkspace("/custom/path")
local ws = getworkspace()
```

**Dependencies**: Standard C file I/O, platform-specific path APIs

**Integration**: Exposed to Lua via `xoron_luau.cpp`.

### 9. xoron_memory.cpp - Memory Management

**Purpose**: Memory utilities and optimization.

**Key Features**:
- Memory allocation tracking
- Pattern verification
- Memory filling
- Compression utilities (LZ4)
- Memory safety checks

**API Functions**:
```c
void* xoron_malloc(size_t size);
void xoron_free(void* ptr);
void xoron_memset_pattern(void* ptr, size_t size, unsigned char pattern);
bool xoron_verify_pattern(void* ptr, size_t size, unsigned char pattern);
```

**Dependencies**:
- LZ4 compression
- Standard C memory functions

**Integration**: Used internally by other components for memory safety.

### 10. xoron_console.cpp - Console Output

**Purpose**: Centralized console output handling.

**Key Features**:
- Output callbacks (print, error, warn)
- Platform-specific logging
- Message formatting
- Thread-safe output

**API Functions**:
```c
void xoron_set_console_callbacks(xoron_output_fn print_fn, 
                                  xoron_output_fn error_fn, void* ud);
void xoron_console_print(const char* text);
void xoron_console_warn(const char* text);
void xoron_console_error(const char* text);
```

**Platform Logging**:
- **Android**: `__android_log_print`
- **iOS**: `NSLog`
- **Other**: `printf`

**Integration**: Used by `xoron_luau.cpp` for Lua print() function.

### 11. xoron_input.cpp - Input Handling

**Purpose**: Input event management and touch handling.

**Key Features**:
- Touch event tracking
- Key input simulation
- Input state management
- Event callbacks

**API Functions** (Lua):
```lua
-- Touch events
Input.TouchBegan:Connect(function(x, y, id)
    print("Touch at", x, y)
end)

-- Key events
Input.KeyDown:Connect(function(key)
    print("Key pressed:", key)
end)
```

**Integration**: Registered in `xoron_luau.cpp`, platform-specific in `xoron_android.cpp` and `xoron_ios.mm`.

### 12. xoron_cache.cpp - Data Caching

**Purpose**: In-memory data caching system.

**Key Features**:
- Key-value storage
- TTL (Time To Live) support
- Memory limits
- Cache persistence

**API Functions** (Lua):
```lua
cache:set("key", "value", 3600)  -- 1 hour TTL
local value = cache:get("key")
cache:delete("key")
cache:clear()
```

**Integration**: Registered in `xoron_luau.cpp`.

### 13. xoron_ui.cpp - UI Management

**Purpose**: UI component management and native UI integration.

**Key Features**:
- UI component creation
- Event handling
- Layout management
- Platform-specific UI integration

**API Functions** (Lua):
```lua
-- UI creation
local button = UI.Button({
    text = "Click me",
    position = {100, 100},
    size = {200, 50},
    callback = function()
        print("Clicked!")
    end
})

-- Show/hide UI
UI.Show()
UI.Hide()
UI.Toggle()
```

**Platform-Specific**:
- **iOS**: UIKit components via Objective-C
- **Android**: Android UI via JNI

**Integration**: Registered in `xoron_luau.cpp`, platform code in `xoron_android.cpp` and `xoron_ios.mm`.

### 14. xoron_android.cpp - Android-Specific Code

**Purpose**: Android platform integration.

**Key Features**:
- JNI bridge to Java
- Android logging
- UI integration
- Haptic feedback
- Clipboard operations
- System information

**Key Functions**:
```cpp
void xoron_android_haptic_feedback(int style);
void xoron_android_ui_show(void);
void xoron_android_console_print(const char* message, int type);
void xoron_register_android(lua_State* L);
```

**JNI Integration**:
- Stores JVM reference
- Attaches/detaches threads
- Calls Java methods for clipboard, UI, etc.

**Dependencies**: Android NDK (log, android, jni.h)

### 15. xoron_ios.mm - iOS-Specific Code

**Purpose**: iOS platform integration (Objective-C++).

**Key Features**:
- Objective-C runtime integration
- iOS UI management (UIKit)
- Haptic feedback (UIFeedbackGenerator)
- Clipboard operations (UIPasteboard)
- Device information
- Drawing context (CoreGraphics)

**Key Functions**:
```cpp
void xoron_ios_haptic_feedback(int style);
void xoron_ios_ui_show(void);
void xoron_drawing_render_ios(CGContextRef ctx);
void xoron_register_ios(lua_State* L);
```

**Frameworks Used**:
- Foundation (NSLog, NSString)
- UIKit (UI components, pasteboard)
- CoreGraphics (drawing)
- CoreHaptics (feedback)

**Dependencies**: iOS SDK, Objective-C runtime

## Build System

### CMakeLists.txt

**Purpose**: Cross-platform build configuration.

**Key Features**:
- Platform detection and validation
- Dependency management (Luau, OpenSSL, cpp-httplib, LZ4)
- Compiler flags for mobile platforms
- Library linking configuration
- Install rules

**Build Options**:
```cmake
-XORON_IOS_BUILD=ON          # Build for iOS
-XORON_ANDROID_BUILD=ON      # Build for Android
-XORON_OPENSSL_ROOT=PATH     # Pre-built OpenSSL path
-XORON_ANDROID_ABI=arm64-v8a # Android architecture
```

**Dependencies Management**:
- **Luau**: Fetched from GitHub (Roblox/luau)
- **cpp-httplib**: Fetched from GitHub (yhirose/cpp-httplib)
- **LZ4**: Fetched from GitHub (lz4/lz4)
- **OpenSSL**: Pre-built for mobile, system for dev

## Integration Flow

### How Components Work Together

```
1. Application starts
   ↓
2. xoron_init() - Initialize global state
   ↓
3. xoron_vm_new() - Create Luau VM
   ↓
4. Register all libraries (HTTP, Crypto, Drawing, etc.)
   ↓
5. xoron_dostring() - Execute user script
   ↓
6. Script calls library functions
   ↓
7. Libraries perform operations
   ↓
8. Results returned to Lua
   ↓
9. xoron_vm_free() - Cleanup VM
   ↓
10. xoron_shutdown() - Cleanup global state
```

### Library Registration Chain

```
xoron_luau.cpp
├── xoron_register_env()      → xoron_env.cpp
├── xoron_register_http()     → xoron_http.cpp
├── xoron_register_crypto()   → xoron_crypto.cpp
├── xoron_register_drawing()  → xoron_drawing.cpp
├── xoron_register_websocket()→ xoron_websocket.cpp
├── xoron_register_filesystem() → xoron_filesystem.cpp
├── xoron_register_memory()   → xoron_memory.cpp
├── xoron_register_console()  → xoron_console.cpp
├── xoron_register_input()    → xoron_input.cpp
├── xoron_register_cache()    → xoron_cache.cpp
├── xoron_register_ui()       → xoron_ui.cpp
└── Platform-specific:
    ├── xoron_register_android() → xoron_android.cpp
    └── xoron_register_ios()     → xoron_ios.mm
```

## Testing

See `tests/` directory for integration tests covering:
- VM execution
- HTTP requests
- Crypto operations
- Drawing engine
- File system
- Environment functions

## Workflows

See `workflows/` directory for CI/CD:
- Android build workflow
- iOS build workflow
- Android test workflow
- iOS test workflow

## Platform-Specific Notes

### iOS Requirements
- Minimum iOS 15.0
- ARM64 architecture
- Xcode with iOS SDK
- OpenSSL built for iOS
- Frameworks: Foundation, CoreFoundation, CoreGraphics, CoreText, Security, UIKit, WebKit, QuartzCore

### Android Requirements
- Minimum Android API 29 (Android 10)
- ARM64-v8a, ARMv7a, x86, x86_64
- Android NDK r26+
- OpenSSL built for Android
- Libraries: log, android

## Performance Considerations

1. **Memory**: Use LZ4 compression for cached data
2. **Threading**: Mutex protection for shared state
3. **I/O**: Buffered file operations
4. **Network**: Connection pooling in cpp-httplib
5. **Graphics**: Z-index sorting for efficient rendering
6. **Lua**: Bytecode caching for repeated execution

## Security Considerations

1. **Memory Safety**: RAII, smart pointers, bounds checking
2. **Crypto**: OpenSSL for all cryptographic operations
3. **Network**: TLS verification options, timeout protection
4. **Environment**: Anti-detection, validation checks
5. **Input**: Sanitization and validation

## Future Enhancements

- [ ] WebSocket ping/pong support
- [ ] Advanced drawing (gradients, shadows)
- [ ] SQLite integration
- [ ] Network proxy support
- [ ] Script obfuscation
- [ ] Performance profiling
- [ ] Memory leak detection
- [ ] Cross-platform UI framework

---

For detailed documentation on specific components, see:
- `docs/api/` - API reference
- `docs/architecture/` - Architecture diagrams
- `tests/` - Integration examples
