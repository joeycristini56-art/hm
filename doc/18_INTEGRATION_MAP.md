# 18. Integration Map - File Dependencies & Relationships

**File**: `src/src/doc/18_INTEGRATION_MAP.md`  
**Purpose**: Complete dependency graph and integration overview

## Overview

This document maps all dependencies between Xoron source files, showing how they interact and what each file provides to the system.

## Dependency Graph

```
xoron.h (Core API)
├── xoron_luau.cpp (Lua VM)
│   ├── xoron_http.cpp
│   ├── xoron_crypto.cpp
│   ├── xoron_env.cpp
│   ├── xoron_filesystem.cpp
│   ├── xoron_memory.cpp
│   ├── xoron_debug.cpp
│   ├── xoron_console.cpp
│   ├── xoron_drawing.cpp
│   ├── xoron_websocket.cpp
│   ├── xoron_input.cpp
│   ├── xoron_cache.cpp
│   └── xoron_ui.cpp
├── xoron_android.cpp (Android-specific)
└── xoron_ios.mm (iOS-specific)
```

## File-by-File Integration

### 1. xoron.h (Header File)
**Lines**: 265  
**Size**: 8,948 bytes

**Provides**:
- Core API declarations
- Type definitions
- Function prototypes
- Platform macros

**Dependencies**:
- Standard C headers
- Lua headers (lua.h, lualib.h)

**Used by**:
- All .cpp files (includes this header)
- All .mm files (includes this header)

**Key Exports**:
```c
// Error handling
void xoron_set_error(const char* fmt, ...);
const char* xoron_last_error(void);

// HTTP
char* xoron_http_get(const char* url, int* status, size_t* len);
char* xoron_http_post(const char* url, const char* body, size_t body_len, 
                      const char* content_type, int* status, size_t* len);
void xoron_http_free(char* response);

// Crypto
void xoron_sha256(const void* data, size_t len, uint8_t out[32]);
char* xoron_base64_encode(const void* data, size_t len);
uint8_t* xoron_base64_decode(const char* str, size_t* out_len);

// Filesystem
char* xoron_readfile(const char* path, size_t* len);
bool xoron_writefile(const char* path, const void* data, size_t len);
char* xoron_listfiles(const char* path);

// Memory
void xoron_memory_free(void* ptr, size_t size);
void* xoron_memory_allocate(size_t size, int prot);

// Debug
void xoron_debug_log(const char* fmt, ...);
char* xoron_get_stacktrace(void);

// Console
void xoron_console_print(const char* text);
void xoron_console_clear(void);

// Drawing
uint32_t xoron_drawing_new(const char* type);
bool xoron_drawing_set_property(uint32_t id, const char* prop, void* value);

// WebSocket
uint32_t xoron_websocket_connect(const char* url, ...);
bool xoron_websocket_send(uint32_t id, const char* data);

// Input
void xoron_mouse_move(float x, float y);
void xoron_key_press(int keycode);

// Cache
bool xoron_cache_set(const char* key, const char* value, int ttl);
char* xoron_cache_get(const char* key);

// UI
uint32_t xoron_ui_create_window(const char* title, float width, float height);
void xoron_ui_show(void);

// Platform-specific
void xoron_android_init(void);
void xoron_ios_init(void);
```

### 2. xoron_luau.cpp (Lua VM Wrapper)
**Lines**: 496  
**Size**: 14,589 bytes

**Purpose**: Main Lua VM integration, binds all C++ functions to Lua

**Dependencies**:
- `xoron.h` (API declarations)
- `lua.h`, `lualib.h` (Lua headers)
- All other .cpp files (calls their functions)

**Provides to Lua**:
```lua
-- HTTP
http.get(url) -> response
http.post(url, body, content_type) -> response
http.request(options) -> response

-- Cryptography
crypt.hash(algorithm, data) -> hash
crypt.encrypt(algorithm, data, key, iv) -> encrypted
crypt.decrypt(algorithm, data, key, iv) -> decrypted
crypt.base64encode(data) -> base64
crypt.base64decode(base64) -> data
crypt.random(length) -> bytes

-- Filesystem
readfile(path) -> content
writefile(path, content) -> success
appendfile(path, content) -> success
listfiles(path) -> files
isfile(path) -> bool
isfolder(path) -> bool
makefolder(path) -> success
delfolder(path) -> success
delfile(path) -> success

-- Environment
getgenv() -> table
getrenv() -> table
getmenv() -> table
getsenv(script) -> table
identifyexecutor() -> string
getloadedmodules() -> table
getgc() -> table
getreg() -> table

-- Debug
debug.getinfo(func) -> info
debug.traceback(message) -> trace
debug.sethook(func, mask, count) -> nil

-- Console
print(...) -> void
warn(...) -> void
error(...) -> void
console.table(data) -> void
console.json(data) -> void

-- Drawing
Drawing.new(type) -> object
Drawing.clear() -> void

-- WebSocket
websocket.connect(url, callbacks) -> id
websocket.send(id, data) -> success

-- Input
input.mouse_move(x, y) -> void
input.key_press(keycode) -> void
input.is_key_down(keycode) -> bool

-- Cache
cache.set(key, value, ttl) -> success
cache.get(key) -> value
cache.has(key) -> bool

-- UI
ui.create_window(title, width, height) -> id
ui.create_button(parent, text, callback) -> id
ui.show() -> void

-- Platform-specific
android.get_api_level() -> number
ios.get_device_info() -> table

-- SaveInstance
saveinstance(options) -> path
```

**Integration Points**:
- Registers all C functions with Lua state
- Handles Lua-to-C++ type conversions
- Manages error propagation
- Provides convenience wrappers

### 3. xoron_http.cpp (HTTP Client)
**Lines**: 380  
**Size**: 12,234 bytes

**Purpose**: HTTP/HTTPS client using cpp-httplib

**Dependencies**:
- `xoron.h` (error handling)
- `httplib.h` (HTTP library)
- `openssl` (for HTTPS)
- `lua.h` (Lua bindings)

**Integration**:
```cpp
// Called by xoron_luau.cpp
xoron_register_http(lua_State* L) {
    // Registers http.* functions
}

// Uses xoron_set_error() for errors
xoron_http_get(...) {
    if (!url) {
        xoron_set_error("URL is null");
        return nullptr;
    }
}
```

**Provides**:
- HTTP GET/POST/PUT/DELETE/PATCH/HEAD/OPTIONS
- SSL/TLS support (via OpenSSL)
- 30-second timeouts
- Custom headers
- Response status codes

### 4. xoron_crypto.cpp (Cryptographic Functions)
**Lines**: 707  
**Size**: 20,877 bytes

**Purpose**: Cryptographic operations using OpenSSL

**Dependencies**:
- `xoron.h` (API)
- OpenSSL (SHA256, AES, HMAC, etc.)
- `lua.h` (Lua bindings)

**Integration**:
```cpp
// Called by xoron_luau.cpp
xoron_register_crypt(lua_State* L) {
    // Registers crypt.* functions
}

// Used by xoron_http.cpp for SSL
// Used by xoron_filesystem.cpp for encryption
```

**Provides**:
- Hash functions (SHA256/384/512, MD5)
- Encryption/Decryption (AES-CBC/GCM)
- HMAC
- Base64 encoding/decoding
- Hex encoding/decoding
- Random number generation

### 5. xoron_env.cpp (Environment Functions)
**Lines**: ~800  
**Size**: 53,091 bytes

**Purpose**: Global environment manipulation and executor functions

**Dependencies**:
- `xoron.h` (API)
- `lz4.h` (compression)
- Platform-specific headers
- `lua.h` (Lua bindings)

**Integration**:
```cpp
// Called by xoron_luau.cpp
xoron_register_env(lua_State* L) {
    // Registers getgenv, getrenv, etc.
}

// Uses xoron_filesystem.cpp for saveinstance
// Uses xoron_debug.cpp for stack traces
```

**Provides**:
- Environment access (getgenv, getrenv, etc.)
- Executor identification
- Script manipulation (getscripts, getloadedmodules)
- Memory inspection (getgc, getmemoryusage)
- Hooking (hookfunction, hookmetamethod)
- SaveInstance integration

### 6. xoron_filesystem.cpp (File Operations)
**Lines**: ~500  
**Size**: 31,241 bytes

**Purpose**: Cross-platform file I/O

**Dependencies**:
- `xoron.h` (API)
- C++17 filesystem
- Platform-specific paths
- `lua.h` (Lua bindings)

**Integration**:
```cpp
// Called by xoron_luau.cpp
xoron_register_filesystem(lua_State* L) {
    // Registers readfile, writefile, etc.
}

// Used by xoron_env.cpp for saveinstance
// Used by xoron_debug.cpp for logging
```

**Provides**:
- File read/write/append
- Directory listing
- File/folder existence checks
- Path manipulation
- Auto-execute folder support
- Workspace management

### 7. xoron_memory.cpp (Memory Management)
**Lines**: ~400  
**Size**: 19,581 bytes

**Purpose**: Memory scanning and anti-detection

**Dependencies**:
- `xoron.h` (API)
- Platform-specific memory APIs
- `lua.h` (Lua bindings)

**Integration**:
```cpp
// Called by xoron_luau.cpp
xoron_register_memory(lua_State* L) {
    // Registers memory.* functions
}

// Used by xoron_env.cpp for memory inspection
```

**Provides**:
- Memory scanning (find patterns)
- Memory read/write
- Memory allocation
- Anti-detection (debugger, VM checks)
- Protection management

### 8. xoron_debug.cpp (Debugging Utilities)
**Lines**: 634  
**Size**: 17,311 bytes

**Purpose**: Debugging and diagnostics

**Dependencies**:
- `xoron.h` (API)
- Platform backtrace APIs
- `lua.h` (Lua bindings)
- `lstate.h` (Lua internals)

**Integration**:
```cpp
// Called by xoron_luau.cpp
xoron_register_debug(lua_State* L) {
    // Registers debug.* functions
}

// Used by xoron_luau.cpp for error handling
// Used by xoron_env.cpp for stack traces
```

**Provides**:
- Stack traces
- Enhanced debug.getinfo
- Error handling with tracebacks
- Memory/system info
- Hook management
- Registry inspection

### 9. xoron_console.cpp (Console Output)
**Lines**: 413  
**Size**: 11,914 bytes

**Purpose**: Console output management

**Dependencies**:
- `xoron.h` (API)
- Platform-specific logging
- `lua.h` (Lua bindings)

**Integration**:
```cpp
// Called by xoron_luau.cpp
xoron_register_console(lua_State* L) {
    // Overrides print, registers console.*
}

// Used by all modules for logging
```

**Provides**:
- Print/warn/error functions
- Console formatting (tables, JSON)
- Output channels
- History management
- Filtering and redirection

### 10. xoron_drawing.cpp (Graphics)
**Lines**: 1,116  
**Size**: 42,846 bytes

**Purpose**: 2D graphics drawing

**Dependencies**:
- `xoron.h` (API)
- Platform graphics (CoreGraphics/Canvas)
- `lua.h` (Lua bindings)

**Integration**:
```cpp
// Called by xoron_luau.cpp
xoron_register_drawing(lua_State* L) {
    // Registers Drawing.* functions
}

// Uses xoron_ui.cpp for window management
```

**Provides**:
- Line, Circle, Square, Text, Triangle, Quad, Image
- Real-time rendering
- Z-indexing
- Transparency
- Color management

### 11. xoron_websocket.cpp (WebSocket Client)
**Lines**: 633  
**Size**: 16,646 bytes

**Purpose**: WebSocket communication

**Dependencies**:
- `xoron.h` (API)
- POSIX sockets
- OpenSSL (for wss://)
- `lua.h` (Lua bindings)

**Integration**:
```cpp
// Called by xoron_luau.cpp
xoron_register_websocket(lua_State* L) {
    // Registers websocket.* functions
}

// Uses xoron_crypto.cpp for SSL
```

**Provides**:
- WebSocket connection (ws://, wss://)
- Message sending/receiving
- Callbacks (on_message, on_open, on_close, on_error)
- Frame handling
- Thread-safe operations

### 12. xoron_input.cpp (Input Handling)
**Lines**: 561  
**Size**: 15,844 bytes

**Purpose**: Input simulation and detection

**Dependencies**:
- `xoron.h` (API)
- Platform input APIs
- `lua.h` (Lua bindings)

**Integration**:
```cpp
// Called by xoron_luau.cpp
xoron_register_input(lua_State* L) {
    // Registers input.* functions
}
```

**Provides**:
- Mouse movement/clicks
- Keyboard press/release
- Touch simulation (mobile)
- Gamepad support
- Input state tracking

### 13. xoron_cache.cpp (Caching System)
**Lines**: 649  
**Size**: 19,470 bytes

**Purpose**: In-memory caching with eviction policies

**Dependencies**:
- `xoron.h` (API)
- Standard containers
- `lua.h` (Lua bindings)

**Integration**:
```cpp
// Called by xoron_luau.cpp
xoron_register_cache(lua_State* L) {
    // Registers cache.* functions
}
```

**Provides**:
- Key-value storage
- TTL expiration
- LRU/LFU/FIFO eviction
- Size limits
- Statistics
- Pinned entries

### 14. xoron_ui.cpp (User Interface)
**Lines**: 868  
**Size**: 27,976 bytes

**Purpose**: Native UI components

**Dependencies**:
- `xoron.h` (API)
- Platform UI (UIKit/Android Views)
- `lua.h` (Lua bindings)

**Integration**:
```cpp
// Called by xoron_luau.cpp
xoron_register_ui(lua_State* L) {
    // Registers ui.* functions
}

// Uses xoron_android.cpp or xoron_ios.mm for rendering
```

**Provides**:
- Windows, Buttons, Inputs, Sliders
- Checkboxes, Dropdowns, Lists
- Progress bars, Separators
- Event callbacks
- Layout management

### 15. xoron_android.cpp (Android-Specific)
**Lines**: 594  
**Size**: 22,620 bytes

**Purpose**: Android NDK integration

**Dependencies**:
- `xoron.h` (API)
- Android NDK (jni.h, android/*.h)
- `lua.h` (Lua bindings)

**Integration**:
```cpp
// Called by xoron_luau.cpp
xoron_register_android(lua_State* L) {
    // Registers android.* functions
}

// Provides JNI bridge to Java
// Called from Java on init
```

**Provides**:
- JNI bridge
- Native UI
- Haptic feedback
- System info
- Permissions
- Sensors
- Battery info
- Network state
- Location
- Camera, Contacts, Calendar
- Notifications
- Share functions
- Clipboard
- App management

### 16. xoron_ios.mm (iOS-Specific)
**Lines**: 829  
**Size**: 35,271 bytes

**Purpose**: iOS Foundation integration

**Dependencies**:
- `xoron.h` (API)
- iOS frameworks (Foundation, UIKit, etc.)
- `lua.h` (Lua bindings)

**Integration**:
```cpp
// Called by xoron_luau.cpp
xoron_register_ios(lua_State* L) {
    // Registers ios.* functions
}

// Provides Objective-C++ bridge
// Called from app delegate
```

**Provides**:
- Objective-C++ bridge
- Native UI (UIKit)
- Haptic feedback (CoreHaptics)
- Biometric auth (LocalAuthentication)
- System info
- Location (CoreLocation)
- Motion sensors (CoreMotion)
- Camera/Photos
- Contacts/Calendar
- Music (AVFoundation)
- Notifications
- Share functions
- Clipboard
- App Store integration

### 17. CMakeLists.txt (Build System)
**Lines**: 390  
**Size**: 15,217 bytes

**Purpose**: Build configuration

**Dependencies**:
- All source files
- External libraries (Luau, httplib, LZ4, OpenSSL)

**Integration**:
- Compiles all .cpp/.mm files
- Links dependencies
- Handles platform-specific builds
- Sets compiler flags

**Build Targets**:
- iOS: libxoron.dylib
- Android: libxoron.so
- Dev: libxoron.so/.dylib

### 18. saveinstance.lua (Lua Script)
**Lines**: 496  
**Size**: 17,000 bytes

**Purpose**: RBXMX serializer

**Dependencies**:
- `xoron_env.cpp` (for getnilinstances, decompile)
- `xoron_filesystem.cpp` (for writefile)
- `xoron_crypto.cpp` (for base64)
- `xoron_luau.cpp` (for Lua execution)

**Integration**:
```lua
-- Registered by xoron_env.cpp
saveinstance = SaveInstance.Save
_G.saveinstance = saveinstance
```

**Provides**:
- Instance serialization
- RBXMX format generation
- Property serialization
- Script decompilation
- File output

## Cross-Module Dependencies

### Error Handling Chain
```
xoron_set_error() (xoron.h)
    ↓
Called by all modules
    ↓
xoron_last_error() (xoron.h)
    ↓
xoron_luau.cpp converts to Lua error
```

### Lua Registration Chain
```
xoron_luau.cpp (main registration)
    ↓
Calls xoron_register_*() for each module
    ↓
Each module registers its Lua functions
    ↓
Lua state has all functions available
```

### Platform Abstraction
```
xoron.h (platform macros)
    ↓
xoron_android.cpp (Android implementation)
xoron_ios.mm (iOS implementation)
    ↓
xoron_luau.cpp (unified Lua API)
```

### File I/O Chain
```
xoron_filesystem.cpp (file operations)
    ↓
Used by xoron_env.cpp (saveinstance)
    ↓
Used by xoron_debug.cpp (logging)
    ↓
Used by xoron_luau.cpp (readfile/writefile)
```

### Network Chain
```
xoron_http.cpp (HTTP client)
    ↓
Uses xoron_crypto.cpp (SSL/TLS)
    ↓
Used by xoron_luau.cpp (http.*)
    ↓
Used by saveinstance.lua (upload)
```

### Graphics Chain
```
xoron_drawing.cpp (drawing primitives)
    ↓
Uses platform-specific rendering
    ↓
Used by xoron_luau.cpp (Drawing.*)
    ↓
Can be used by xoron_ui.cpp (custom UI)
```

### UI Chain
```
xoron_ui.cpp (UI components)
    ↓
Uses xoron_android.cpp or xoron_ios.mm (native rendering)
    ↓
Used by xoron_luau.cpp (ui.*)
    ↓
Can be used by xoron_env.cpp (custom interfaces)
```

## Data Flow Examples

### HTTP Request Flow
```
Lua: http.get("https://api.example.com")
    ↓
xoron_luau.cpp: lua_http_get()
    ↓
xoron_http.cpp: xoron_http_get()
    ↓
httplib: HTTP request
    ↓
xoron_http.cpp: Parse response
    ↓
xoron_luau.cpp: Return to Lua
    ↓
Lua: Receive response
```

### File Save Flow
```
Lua: saveinstance({FileName = "game"})
    ↓
xoron_env.cpp: xoron_register_env() -> saveinstance()
    ↓
saveinstance.lua: Serialize instances
    ↓
xoron_filesystem.cpp: xoron_writefile()
    ↓
Filesystem: Write to disk
    ↓
Return path to Lua
```

### Memory Scan Flow
```
Lua: memory.findpattern("48 89 5C")
    ↓
xoron_luau.cpp: lua_memory_findpattern()
    ↓
xoron_memory.cpp: find_pattern()
    ↓
Platform memory APIs
    ↓
Return matches to Lua
```

### Drawing Flow
```
Lua: Drawing.new("Line")
    ↓
xoron_luau.cpp: lua_drawing_new()
    ↓
xoron_drawing.cpp: Create object
    ↓
Platform rendering (iOS/Android)
    ↓
Display on screen
```

## Build Dependencies

### Required Libraries
```
Luau VM (Roblox Lua)
├── xoron_luau.cpp
└── All Lua-exposed modules

cpp-httplib (HTTP)
└── xoron_http.cpp

OpenSSL (Crypto/TLS)
├── xoron_http.cpp
├── xoron_crypto.cpp
└── xoron_websocket.cpp

LZ4 (Compression)
└── xoron_env.cpp (for saveinstance)

C++17 Filesystem
└── xoron_filesystem.cpp

Platform Frameworks:
iOS: Foundation, UIKit, CoreGraphics, CoreText, Security, WebKit, CoreHaptics, CoreLocation, CoreMotion, AVFoundation, StoreKit, MessageUI, LocalAuthentication, Photos, Contacts, EventKit
Android: log, EGL, GLESv2, android, jni
```

### Build Order
```
1. xoron.h (header)
2. All .cpp files (compiled independently)
3. All .mm files (compiled independently)
4. Link with libraries
5. Create shared library
```

## Runtime Dependencies

### Initialization Order
```
1. xoron_luau.cpp creates Lua state
2. Calls xoron_register_*() for each module
3. Each module registers its functions
4. Lua state ready for execution
5. Platform init (xoron_android_init / xoron_ios_init)
6. Ready for user scripts
```

### Execution Flow
```
User script
    ↓
xoron_luau.cpp (Lua VM)
    ↓
Lua function calls C++ function
    ↓
Specific module (e.g., xoron_http.cpp)
    ↓
Platform APIs (if needed)
    ↓
Return result to Lua
    ↓
User receives result
```

## Integration Points Summary

### C++ to Lua
- All modules register functions via `xoron_luau.cpp`
- Type conversions handled by Lua C API
- Errors propagated via `lua_error()`

### Lua to C++
- Lua calls registered C functions
- Arguments converted to C types
- Results converted back to Lua types

### Platform to C++
- Android: JNI calls
- iOS: Objective-C++ bridge
- Both: Use xoron.h macros

### C++ to Platform
- Calls platform-specific APIs
- Handles platform differences
- Returns unified results

## File Size & Complexity

### Largest Files
1. `xoron_ui.cpp` (27,976 bytes, 868 lines) - UI components
2. `xoron_crypto.cpp` (20,877 bytes, 707 lines) - Crypto operations
3. `xoron_env.cpp` (53,091 bytes, ~800 lines) - Environment functions
4. `xoron_ios.mm` (35,271 bytes, 829 lines) - iOS integration
5. `xoron_android.cpp` (22,620 bytes, 594 lines) - Android integration

### Most Complex
1. `xoron_env.cpp` - Many features, platform-specific code
2. `xoron_ios.mm` - Extensive iOS framework integration
3. `xoron_android.cpp` - JNI complexity
4. `xoron_drawing.cpp` - Graphics rendering
5. `xoron_luau.cpp` - All Lua bindings

### Most Independent
1. `xoron.h` - Pure header
2. `xoron_memory.cpp` - Self-contained
3. `xoron_cache.cpp` - Self-contained
4. `xoron_console.cpp` - Simple output
5. `xoron_debug.cpp` - Debug utilities

## Testing Strategy

### Unit Tests
- Each module tested independently
- Mock platform APIs
- Test error conditions

### Integration Tests
- Test Lua bindings
- Test cross-module calls
- Test platform-specific features

### End-to-End Tests
- Full execution flow
- Real platform testing
- Performance benchmarks

## Maintenance Notes

### Adding New Module
1. Create header declarations in `xoron.h`
2. Implement in new .cpp file
3. Register in `xoron_luau.cpp`
4. Add to `CMakeLists.txt`
5. Update documentation

### Modifying Existing Module
1. Check all dependencies
2. Update header if API changes
3. Update Lua bindings if needed
4. Test all integration points
5. Update documentation

### Platform-Specific Changes
1. Update both iOS and Android implementations
2. Use platform macros in `xoron.h`
3. Test on both platforms
4. Maintain API consistency

## Related Documentation
- `00_PROJECT_OVERVIEW.md` - High-level overview
- `01_XORON_H.md` - Header file details
- Individual module docs (02-17)
- This file (18) - Integration map
