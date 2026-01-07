# 01. xoron.h - Main Header File

**File Path**: `src/src/xoron.h`  
**Size**: 8,948 bytes  
**Lines**: 265

## Overview
Main header file providing the public C API for the Xoron executor engine. Contains platform detection, type definitions, and function declarations for all components.

## Platform Detection Macros

### Supported Platforms
- **XORON_PLATFORM_IOS**: iOS (iPhone/iPad) ARM64
  - Minimum: iOS 15.0
  - Extension: `.dylib`
  - Path: `~/Documents/Xoron/`

- **XORON_PLATFORM_ANDROID**: Android ARM64/ARM32/x86/x86_64
  - Minimum: Android 10 (API 29)
  - Extension: `.so`
  - Path: `/data/data/<package>/files/Xoron/`

- **XORON_PLATFORM_MACOS**: macOS (development only)
- **XORON_PLATFORM_UNKNOWN**: Unsupported platform

### Architecture Detection (Android)
- **XORON_ANDROID_ARM64**: ARM64-v8a
- **XORON_ANDROID_ARM32**: armeabi-v7a
- **XORON_ANDROID_X86_64**: x86_64
- **XORON_ANDROID_X86**: x86

### Utility Macros
- **XORON_PLATFORM_MOBILE**: Defined for iOS or Android
- **XORON_ARCH_NAME**: Architecture name string
- **XORON_LIBRARY_EXTENSION**: Platform-specific extension

## Version Information
```c
#define XORON_VERSION "2.0.0"
#define XORON_NAME "Xoron"
```

## Type Definitions

### Error Codes
```c
typedef enum {
    XORON_OK = 0,              // Success
    XORON_ERR_INIT = -1,       // Initialization error
    XORON_ERR_MEMORY = -2,     // Memory error
    XORON_ERR_COMPILE = -3,    // Compilation error
    XORON_ERR_RUNTIME = -4,    // Runtime error
    XORON_ERR_HTTP = -5,       // HTTP error
    XORON_ERR_INVALID = -6,    // Invalid argument
    XORON_ERR_IO = -7,         // I/O error
    XORON_ERR_WEBSOCKET = -8,  // WebSocket error
    XORON_ERR_SECURITY = -9    // Security violation
} xoron_error_t;
```

### Opaque Types
```c
typedef struct xoron_vm xoron_vm_t;           // Virtual machine instance
typedef struct xoron_bytecode xoron_bytecode_t; // Compiled bytecode
typedef void (*xoron_output_fn)(const char* msg, void* ud); // Output callback
```

## Export/Import Macros
```c
#define XORON_API __attribute__((visibility("default")))
#define XORON_LOCAL __attribute__((visibility("hidden")))
```
Used for symbol visibility control on mobile platforms.

## Core API

### xoron_init
```c
int xoron_init(void);
```
Initializes the Xoron engine. Must be called before any other functions.

**Returns**: `XORON_OK` on success, error code on failure

### xoron_shutdown
```c
void xoron_shutdown(void);
```
Shuts down the Xoron engine and releases all resources.

### xoron_version
```c
const char* xoron_version(void);
```
Returns version string ("2.0.0").

### xoron_last_error
```c
const char* xoron_last_error(void);
```
Returns last error message or NULL.

### xoron_set_output
```c
void xoron_set_output(xoron_output_fn print_fn, xoron_output_fn error_fn, void* ud);
```
Sets custom output callbacks.

**Parameters**:
- `print_fn`: Callback for standard output
- `error_fn`: Callback for error output
- `ud`: User data passed to callbacks

## VM API

### xoron_vm_new
```c
xoron_vm_t* xoron_vm_new(void);
```
Creates a new Luau VM instance.

**Returns**: Pointer to new VM or NULL on error

### xoron_vm_free
```c
void xoron_vm_free(xoron_vm_t* vm);
```
Frees a VM instance.

### xoron_vm_reset
```c
void xoron_vm_reset(xoron_vm_t* vm);
```
Resets a VM instance, clearing all state.

## Compilation API

### xoron_compile
```c
xoron_bytecode_t* xoron_compile(const char* source, size_t len, const char* name);
```
Compiles Luau source code into bytecode.

**Parameters**:
- `source`: Source code string
- `len`: Length of source code (0 = auto-detect)
- `name`: Chunk name for error messages

**Returns**: Bytecode object or NULL

### xoron_compile_file
```c
xoron_bytecode_t* xoron_compile_file(const char* path);
```
Compiles Luau source code from a file.

### xoron_bytecode_free
```c
void xoron_bytecode_free(xoron_bytecode_t* bc);
```
Frees bytecode object.

### xoron_bytecode_data
```c
const char* xoron_bytecode_data(xoron_bytecode_t* bc, size_t* len);
```
Gets raw bytecode data.

## Execution API

### xoron_run
```c
int xoron_run(xoron_vm_t* vm, xoron_bytecode_t* bc);
```
Executes compiled bytecode in a VM.

### xoron_dostring
```c
int xoron_dostring(xoron_vm_t* vm, const char* source, const char* name);
```
Compiles and executes Luau source code.

### xoron_dofile
```c
int xoron_dofile(xoron_vm_t* vm, const char* path);
```
Loads, compiles, and executes a Luau file.

## HTTP API

### xoron_http_get
```c
char* xoron_http_get(const char* url, int* status, size_t* len);
```
Performs HTTP GET request.

**Returns**: Response body (must be freed with `xoron_http_free()`)

### xoron_http_post
```c
char* xoron_http_post(const char* url, const char* body, size_t body_len, 
                      const char* content_type, int* status, size_t* len);
```
Performs HTTP POST request.

### xoron_http_free
```c
void xoron_http_free(char* response);
```
Frees HTTP response memory.

## Crypto API

### Hash Functions
```c
void xoron_sha256(const void* data, size_t len, uint8_t out[32]);
void xoron_sha384(const void* data, size_t len, uint8_t out[48]);
void xoron_sha512(const void* data, size_t len, uint8_t out[64]);
void xoron_md5(const void* data, size_t len, uint8_t out[16]);
```
Compute cryptographic hashes.

### Encoding Functions
```c
char* xoron_base64_encode(const void* data, size_t len);
uint8_t* xoron_base64_decode(const char* str, size_t* out_len);
char* xoron_hex_encode(const void* data, size_t len);
uint8_t* xoron_hex_decode(const char* str, size_t* out_len);
```
Encode/decode data. Returns allocated result (must be freed with `xoron_free()`).

### xoron_free
```c
void xoron_free(void* ptr);
```
Frees memory allocated by crypto functions.

## Filesystem API

### xoron_get_workspace
```c
const char* xoron_get_workspace(void);
```
Returns current workspace path.

### xoron_set_workspace
```c
void xoron_set_workspace(const char* path);
```
Sets the workspace directory.

### xoron_get_autoexecute_path
```c
const char* xoron_get_autoexecute_path(void);
```
Returns auto-execute scripts directory path.

### xoron_get_scripts_path
```c
const char* xoron_get_scripts_path(void);
```
Returns user scripts directory path.

## Security API

### xoron_check_environment
```c
bool xoron_check_environment(void);
```
Checks if running in a safe environment (detects emulators/debuggers).

**Returns**: `true` if safe, `false` if suspicious

### xoron_enable_anti_detection
```c
void xoron_enable_anti_detection(bool enable);
```
Enables/disables anti-detection measures.

## Console API

### xoron_set_console_callbacks
```c
void xoron_set_console_callbacks(xoron_output_fn print_fn, xoron_output_fn error_fn, void* ud);
```
Sets console output callbacks.

### xoron_console_print
```c
void xoron_console_print(const char* text);
```
Prints text to console.

### xoron_console_warn
```c
void xoron_console_warn(const char* text);
```
Prints warning to console.

### xoron_console_error
```c
void xoron_console_error(const char* text);
```
Prints error to console.

## C++ Registration Functions
These functions register Luau libraries with the VM (called from `xoron_luau.cpp`):

```c
void xoron_register_env(lua_State* L);
void xoron_register_filesystem(lua_State* L);
void xoron_register_memory(lua_State* L);
void xoron_register_debug(lua_State* L);
void xoron_register_console(lua_State* L);
void xoron_register_drawing(lua_State* L);
void xoron_register_websocket(lua_State* L);
void xoron_register_http(lua_State* L);
void xoron_register_crypt(lua_State* L);
void xoron_register_input(lua_State* L);
void xoron_register_cache(lua_State* L);
void xoron_register_ui(lua_State* L);
```

## Platform-Specific APIs

### iOS API (XORON_PLATFORM_IOS)
```c
void xoron_register_ios(lua_State* L);
void xoron_ios_ui_init(void);
void xoron_ios_ui_show(void);
void xoron_ios_ui_hide(void);
void xoron_ios_ui_toggle(void);
void xoron_ios_haptic_feedback(int style);
void xoron_ios_console_print(const char* message, int type);
void xoron_ios_set_lua_state(lua_State* L);
void xoron_drawing_render_ios(CGContextRef ctx);
void xoron_drawing_set_screen_size(float width, float height);
```

### Android API (XORON_PLATFORM_ANDROID)
```c
void xoron_register_android(lua_State* L);
void xoron_android_haptic_feedback(int style);
void xoron_android_ui_show(void);
void xoron_android_ui_hide(void);
void xoron_android_ui_toggle(void);
void xoron_android_console_print(const char* message, int type);
void xoron_android_set_lua_state(lua_State* L);
lua_State* xoron_android_get_lua_state(void);
void xoron_drawing_set_screen_size(float width, float height);
```

## Files That Include xoron.h
- xoron_luau.cpp
- xoron_http.cpp
- xoron_crypto.cpp
- xoron_env.cpp
- xoron_filesystem.cpp
- xoron_memory.cpp
- xoron_debug.cpp
- xoron_console.cpp
- xoron_drawing.cpp
- xoron_websocket.cpp
- xoron_input.cpp
- xoron_cache.cpp
- xoron_ui.cpp
- xoron_android.cpp
- xoron_ios.mm

## Usage Example
```c
#include "xoron.h"

int main() {
    // Initialize
    if (xoron_init() != XORON_OK) {
        return 1;
    }
    
    // Create VM
    xoron_vm_t* vm = xoron_vm_new();
    if (!vm) {
        xoron_shutdown();
        return 1;
    }
    
    // Execute code
    int result = xoron_dostring(vm, "print('Hello from Xoron!')", "main");
    
    // Cleanup
    xoron_vm_free(vm);
    xoron_shutdown();
    
    return result == XORON_OK ? 0 : 1;
}
```
