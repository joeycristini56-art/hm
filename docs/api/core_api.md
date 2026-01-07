# Core C API Reference

## Overview

The Core C API provides direct access to Xoron's functionality from C/C++ applications. This is the foundation layer that all Lua bindings are built upon.

## Header

```c
#include "xoron.h"
```

## Initialization

### xoron_init

```c
int xoron_init(void);
```

**Description**: Initializes the Xoron engine. Must be called before any other functions.

**Returns**:
- `XORON_OK` (0) on success
- `XORON_ERR_INIT` (-1) on failure

**Example**:
```c
if (xoron_init() != XORON_OK) {
    fprintf(stderr, "Failed to initialize: %s\n", xoron_last_error());
    return 1;
}
```

**Notes**:
- Thread-safe
- Idempotent (safe to call multiple times)
- Initializes OpenSSL (if available)
- Sets up global state

---

### xoron_shutdown

```c
void xoron_shutdown(void);
```

**Description**: Cleans up all resources and shuts down the engine.

**Example**:
```c
xoron_shutdown();
```

**Notes**:
- Cleans up all VMs
- Releases OpenSSL resources
- Stops background threads
- Should be called at program exit

---

### xoron_version

```c
const char* xoron_version(void);
```

**Description**: Returns the version string.

**Returns**: Version string (e.g., "2.0.0")

**Example**:
```c
printf("Xoron version: %s\n", xoron_version());
```

---

### xoron_last_error

```c
const char* xoron_last_error(void);
```

**Description**: Returns the last error message.

**Returns**: Error string or NULL if no error

**Example**:
```c
const char* error = xoron_last_error();
if (error) {
    fprintf(stderr, "Error: %s\n", error);
}
```

**Notes**:
- Thread-safe (uses mutex)
- Cleared on successful operations
- Persistent until next error

---

### xoron_set_output

```c
typedef void (*xoron_output_fn)(const char* msg, void* ud);

void xoron_set_output(xoron_output_fn print_fn, 
                      xoron_output_fn error_fn, 
                      void* ud);
```

**Description**: Sets custom output callbacks for console output.

**Parameters**:
- `print_fn`: Callback for print output (or NULL)
- `error_fn`: Callback for error output (or NULL)
- `ud`: User data passed to callbacks

**Example**:
```c
void my_print(const char* msg, void* ud) {
    FILE* fp = (FILE*)ud;
    fprintf(fp, "[PRINT] %s\n", msg);
}

void my_error(const char* msg, void* ud) {
    FILE* fp = (FILE*)ud;
    fprintf(fp, "[ERROR] %s\n", msg);
}

xoron_set_output(my_print, my_error, stderr);
```

**Notes**:
- If NULL, uses default logging
- Thread-safe
- Affects all VMs

---

## VM Management

### xoron_vm_new

```c
xoron_vm_t* xoron_vm_new(void);
```

**Description**: Creates a new Lua VM instance.

**Returns**: Pointer to VM or NULL on failure

**Example**:
```c
xoron_vm_t* vm = xoron_vm_new();
if (!vm) {
    fprintf(stderr, "Failed to create VM: %s\n", xoron_last_error());
    return 1;
}
```

**Notes**:
- Creates isolated Lua state
- Registers all libraries
- Each VM has its own environment
- Thread-safe (multiple VMs can coexist)

---

### xoron_vm_free

```c
void xoron_vm_free(xoron_vm_t* vm);
```

**Description**: Frees a VM instance.

**Parameters**:
- `vm`: VM to free (can be NULL)

**Example**:
```c
xoron_vm_free(vm);
vm = NULL;
```

**Notes**:
- Cleans up Lua state
- Releases all resources
- Safe to call with NULL

---

### xoron_vm_reset

```c
void xoron_vm_reset(xoron_vm_t* vm);
```

**Description**: Resets a VM to clean state.

**Parameters**:
- `vm`: VM to reset

**Example**:
```c
xoron_vm_reset(vm);
// VM is now clean, as if newly created
```

**Notes**:
- Clears all global variables
- Keeps libraries registered
- Frees memory used by previous execution

---

## Compilation

### xoron_compile

```c
xoron_bytecode_t* xoron_compile(const char* source, 
                                 size_t len, 
                                 const char* name);
```

**Description**: Compiles Lua source to bytecode.

**Parameters**:
- `source`: Lua source code
- `len`: Length of source
- `name`: Name for error messages (can be NULL)

**Returns**: Bytecode object or NULL on error

**Example**:
```c
const char* source = "print('Hello')";
xoron_bytecode_t* bc = xoron_compile(source, strlen(source), "test");
if (!bc) {
    fprintf(stderr, "Compile error: %s\n", xoron_last_error());
    return 1;
}
```

**Notes**:
- Uses Luau compiler
- Optimized bytecode
- Thread-safe

---

### xoron_compile_file

```c
xoron_bytecode_t* xoron_compile_file(const char* path);
```

**Description**: Compiles a Lua file to bytecode.

**Parameters**:
- `path`: Path to Lua file

**Returns**: Bytecode object or NULL on error

**Example**:
```c
xoron_bytecode_t* bc = xoron_compile_file("script.lua");
if (!bc) return 1;
```

**Notes**:
- Reads file into memory
- Calls `xoron_compile()`
- File must be UTF-8 encoded

---

### xoron_bytecode_free

```c
void xoron_bytecode_free(xoron_bytecode_t* bc);
```

**Description**: Frees bytecode object.

**Parameters**:
- `bc`: Bytecode to free (can be NULL)

**Example**:
```c
xoron_bytecode_free(bc);
bc = NULL;
```

---

### xoron_bytecode_data

```c
const char* xoron_bytecode_data(xoron_bytecode_t* bc, size_t* len);
```

**Description**: Gets raw bytecode data.

**Parameters**:
- `bc`: Bytecode object
- `len`: Output for length (can be NULL)

**Returns**: Pointer to bytecode data

**Example**:
```c
size_t len;
const char* data = xoron_bytecode_data(bc, &len);
printf("Bytecode size: %zu bytes\n", len);
```

---

## Execution

### xoron_run

```c
int xoron_run(xoron_vm_t* vm, xoron_bytecode_t* bc);
```

**Description**: Executes compiled bytecode.

**Parameters**:
- `vm`: VM instance
- `bc`: Bytecode to execute

**Returns**:
- `XORON_OK` on success
- Error code on failure

**Example**:
```c
int result = xoron_run(vm, bc);
if (result != XORON_OK) {
    fprintf(stderr, "Runtime error: %s\n", xoron_last_error());
}
```

**Notes**:
- Protected execution (error handler)
- Execution limits apply
- Thread-safe per VM

---

### xoron_dostring

```c
int xoron_dostring(xoron_vm_t* vm, const char* source, const char* name);
```

**Description**: Compiles and executes a Lua string.

**Parameters**:
- `vm`: VM instance
- `source`: Lua source code
- `name`: Name for errors

**Returns**: XORON_OK or error code

**Example**:
```c
int result = xoron_dostring(vm, "print('Hello')", "test");
```

**Notes**:
- Combines compile and run
- Convenient for simple scripts
- Uses temporary bytecode

---

### xoron_dofile

```c
int xoron_dofile(xoron_vm_t* vm, const char* path);
```

**Description**: Compiles and executes a Lua file.

**Parameters**:
- `vm`: VM instance
- `path`: Path to Lua file

**Returns**: XORON_OK or error code

**Example**:
```c
int result = xoron_dofile(vm, "script.lua");
```

---

## HTTP API

### xoron_http_get

```c
char* xoron_http_get(const char* url, int* status, size_t* len);
```

**Description**: Performs HTTP GET request.

**Parameters**:
- `url`: Target URL
- `status`: Output for HTTP status code
- `len`: Output for response length

**Returns**: Allocated response buffer or NULL

**Example**:
```c
int status; size_t len;
char* response = xoron_http_get("https://api.example.com", &status, &len);
if (response) {
    printf("Status: %d\n", status);
    printf("Response: %.*s\n", (int)len, response);
    xoron_http_free(response);
}
```

**Notes**:
- Supports HTTP and HTTPS
- 30-second timeout
- Caller must free with `xoron_http_free()`
- Thread-safe

---

### xoron_http_post

```c
char* xoron_http_post(const char* url, 
                      const char* body, 
                      size_t body_len,
                      const char* content_type,
                      int* status, 
                      size_t* len);
```

**Description**: Performs HTTP POST request.

**Parameters**:
- `url`: Target URL
- `body`: Request body
- `body_len`: Body length
- `content_type`: Content-Type header
- `status`: Output for HTTP status
- `len`: Output for response length

**Returns**: Allocated response buffer or NULL

**Example**:
```c
const char* data = "{\"key\":\"value\"}";
int status; size_t len;
char* response = xoron_http_post("https://api.example.com", 
                                  data, strlen(data),
                                  "application/json",
                                  &status, &len);
if (response) {
    xoron_http_free(response);
}
```

---

### xoron_http_free

```c
void xoron_http_free(char* response);
```

**Description**: Frees HTTP response buffer.

**Parameters**:
- `response`: Response from HTTP functions

**Example**:
```c
char* response = xoron_http_get(url, &status, &len);
if (response) {
    // Use response
    xoron_http_free(response);
}
```

---

## Crypto API

### xoron_sha256

```c
void xoron_sha256(const void* data, size_t len, uint8_t out[32]);
```

**Description**: Computes SHA256 hash.

**Parameters**:
- `data`: Input data
- `len`: Data length
- `out`: Output buffer (32 bytes)

**Example**:
```c
uint8_t hash[32];
xoron_sha256("hello", 5, hash);
// hash now contains SHA256 of "hello"
```

---

### xoron_sha384

```c
void xoron_sha384(const void* data, size_t len, uint8_t out[48]);
```

**Description**: Computes SHA384 hash.

**Parameters**:
- `data`: Input data
- `len`: Data length
- `out`: Output buffer (48 bytes)

---

### xoron_sha512

```c
void xoron_sha512(const void* data, size_t len, uint8_t out[64]);
```

**Description**: Computes SHA512 hash.

**Parameters**:
- `data`: Input data
- `len`: Data length
- `out`: Output buffer (64 bytes)

---

### xoron_md5

```c
void xoron_md5(const void* data, size_t len, uint8_t out[16]);
```

**Description**: Computes MD5 hash.

**Parameters**:
- `data`: Input data
- `len`: Data length
- `out`: Output buffer (16 bytes)

---

### xoron_base64_encode

```c
char* xoron_base64_encode(const void* data, size_t len);
```

**Description**: Encodes data to Base64.

**Parameters**:
- `data`: Input data
- `len`: Data length

**Returns**: Allocated Base64 string (must be freed)

**Example**:
```c
char* b64 = xoron_base64_encode("hello", 5);
printf("Base64: %s\n", b64);
xoron_free(b64);
```

---

### xoron_base64_decode

```c
uint8_t* xoron_base64_decode(const char* str, size_t* out_len);
```

**Description**: Decodes Base64 string.

**Parameters**:
- `str`: Base64 string
- `out_len`: Output for decoded length

**Returns**: Allocated decoded data (must be freed)

---

### xoron_hex_encode

```c
char* xoron_hex_encode(const void* data, size_t len);
```

**Description**: Encodes data to hexadecimal.

**Parameters**:
- `data`: Input data
- `len`: Data length

**Returns**: Allocated hex string (must be freed)

---

### xoron_hex_decode

```c
uint8_t* xoron_hex_decode(const char* str, size_t* out_len);
```

**Description**: Decodes hexadecimal string.

**Parameters**:
- `str`: Hex string
- `out_len`: Output for decoded length

**Returns**: Allocated decoded data (must be freed)

---

### xoron_free

```c
void xoron_free(void* ptr);
```

**Description**: Frees memory allocated by Xoron functions.

**Parameters**:
- `ptr`: Pointer to free

**Example**:
```c
char* data = xoron_base64_encode(...);
// ... use data ...
xoron_free(data);
```

**Notes**:
- Use for all Xoron-allocated memory
- Safe for NULL

---

## Filesystem API

### xoron_get_workspace

```c
const char* xoron_get_workspace(void);
```

**Description**: Gets current workspace path.

**Returns**: Workspace path string

**Example**:
```c
const char* ws = xoron_get_workspace();
printf("Workspace: %s\n", ws);
```

---

### xoron_set_workspace

```c
void xoron_set_workspace(const char* path);
```

**Description**: Sets workspace path.

**Parameters**:
- `path`: New workspace path

**Example**:
```c
xoron_set_workspace("/custom/path");
```

---

### xoron_get_autoexecute_path

```c
const char* xoron_get_autoexecute_path(void);
```

**Description**: Gets auto-execute directory path.

**Returns**: Path string

---

### xoron_get_scripts_path

```c
const char* xoron_get_scripts_path(void);
```

**Description**: Gets scripts directory path.

**Returns**: Path string

---

## Security API

### xoron_check_environment

```c
bool xoron_check_environment(void);
```

**Description**: Checks if running in safe environment.

**Returns**: true if safe, false if debugger/detected

**Example**:
```c
if (!xoron_check_environment()) {
    fprintf(stderr, "Unsafe environment detected\n");
    return 1;
}
```

---

### xoron_enable_anti_detection

```c
void xoron_enable_anti_detection(bool enable);
```

**Description**: Enables/disables anti-detection features.

**Parameters**:
- `enable`: true to enable

**Example**:
```c
xoron_enable_anti_detection(true);
```

---

## Console API

### xoron_set_console_callbacks

```c
void xoron_set_console_callbacks(xoron_output_fn print_fn, 
                                  xoron_output_fn error_fn, 
                                  void* ud);
```

**Description**: Sets console output callbacks.

**Parameters**:
- `print_fn`: Print callback
- `error_fn`: Error callback
- `ud`: User data

---

### xoron_console_print

```c
void xoron_console_print(const char* text);
```

**Description**: Prints to console.

**Parameters**:
- `text`: Text to print

---

### xoron_console_warn

```c
void xoron_console_warn(const char* text);
```

**Description**: Prints warning to console.

**Parameters**:
- `text`: Warning text

---

### xoron_console_error

```c
void xoron_console_error(const char* text);
```

**Description**: Prints error to console.

**Parameters**:
- `text`: Error text

---

## Error Codes

```c
typedef enum {
    XORON_OK = 0,              // Success
    XORON_ERR_INIT = -1,       // Initialization failed
    XORON_ERR_MEMORY = -2,     // Memory allocation failed
    XORON_ERR_COMPILE = -3,    // Compilation error
    XORON_ERR_RUNTIME = -4,    // Runtime error
    XORON_ERR_HTTP = -5,       // HTTP request failed
    XORON_ERR_INVALID = -6,    // Invalid parameter
    XORON_ERR_IO = -7,         // I/O error
    XORON_ERR_WEBSOCKET = -8,  // WebSocket error
    XORON_ERR_SECURITY = -9    // Security violation
} xoron_error_t;
```

## Platform Detection

### XORON_PLATFORM_IOS

Defined when compiling for iOS.

### XORON_PLATFORM_ANDROID

Defined when compiling for Android.

### XORON_PLATFORM_NAME

String containing platform name ("iOS", "Android", or "Unknown").

### XORON_LIBRARY_EXTENSION

String containing library extension (".dylib", ".so", or "").

---

## Complete Example

```c
#include "xoron.h"
#include <stdio.h>

int main() {
    // Initialize
    if (xoron_init() != XORON_OK) {
        fprintf(stderr, "Init failed: %s\n", xoron_last_error());
        return 1;
    }
    
    // Create VM
    xoron_vm_t* vm = xoron_vm_new();
    if (!vm) {
        fprintf(stderr, "VM creation failed: %s\n", xoron_last_error());
        xoron_shutdown();
        return 1;
    }
    
    // Execute script
    const char* script = R"(
        local response = http.get("https://api.github.com")
        if response then
            print("Status:", response.status)
            print("Body length:", #response.body)
        end
        
        local hash = crypto.sha256("hello")
        print("SHA256:", hash)
    )";
    
    int result = xoron_dostring(vm, script, "example");
    if (result != XORON_OK) {
        fprintf(stderr, "Execution failed: %s\n", xoron_last_error());
    }
    
    // Cleanup
    xoron_vm_free(vm);
    xoron_shutdown();
    
    return 0;
}
```

---

**Next:**
- [Lua API Reference](lua_api.md)
- [Crypto API](crypto_api.md)
- [HTTP API](http_api.md)
