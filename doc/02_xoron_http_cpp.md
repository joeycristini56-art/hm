# 02. xoron_http.cpp - HTTP Client

**File Path**: `src/src/xoron_http.cpp`  
**Size**: 12,234 bytes  
**Lines**: 380

**Platform**: Cross-platform (uses cpp-httplib)

## Overview
HTTP client implementation using cpp-httplib library. Supports GET, POST, PUT, DELETE, PATCH, HEAD, and OPTIONS methods with SSL/TLS support via OpenSSL.

## Includes
- `xoron.h` (local)
- `string` (system)
- `cstring` (system)
- `cstdlib` (system)
- `httplib.h` (local - cpp-httplib)
- `lua.h` (local - Luau)
- `lualib.h` (local - Luau)

## Core Functions

### parse_url
```cpp
static bool parse_url(const char* url, std::string& scheme, std::string& host, int& port, std::string& path)
```
Parses a URL into its components.

**Parameters**:
- `url`: Full URL string
- `scheme`: Output for protocol (http/https)
- `host`: Output for hostname
- `port`: Output for port number
- `path`: Output for path

**Returns**: `true` if valid, `false` otherwise

**Examples**:
- `https://example.com:443/path` → scheme="https", host="example.com", port=443, path="/path"
- `http://localhost:8080` → scheme="http", host="localhost", port=8080, path="/"
- `example.com/api` → scheme="http", host="example.com", port=80, path="/api"

## C API Functions

### xoron_http_get
```cpp
char* xoron_http_get(const char* url, int* status, size_t* len)
```
Performs HTTP GET request.

**Parameters**:
- `url`: Target URL
- `status`: Output for HTTP status code (can be NULL)
- `len`: Output for response body length (can be NULL)

**Returns**: 
- Response body as allocated C-string (must be freed with `xoron_http_free()`)
- NULL on error (check `xoron_last_error()`)

**Features**:
- 30-second connection timeout
- 30-second read timeout
- HTTPS support (if OpenSSL available)
- Server certificate verification disabled
- Automatic redirect handling (via cpp-httplib)

**Error Handling**:
- Invalid URL → returns NULL
- Network error → returns NULL
- HTTPS without OpenSSL → returns NULL

### xoron_http_post
```cpp
char* xoron_http_post(const char* url, const char* body, size_t body_len, 
                      const char* content_type, int* status, size_t* len)
```
Performs HTTP POST request.

**Parameters**:
- `url`: Target URL
- `body`: Request body (can be NULL)
- `body_len`: Body length
- `content_type`: Content-Type header (default: "application/json")
- `status`: Output for HTTP status code
- `len`: Output for response body length

**Returns**: Response body (must be freed with `xoron_http_free()`)

### xoron_http_free
```cpp
void xoron_http_free(char* response)
```
Frees memory allocated by HTTP functions.

**Parameters**:
- `response`: Response string to free

### http_request_method
```cpp
static char* http_request_method(const char* method, const char* url, const char* body, 
                                  size_t body_len, const char* content_type, 
                                  int* status, size_t* len)
```
Internal helper for additional HTTP methods.

**Supported Methods**:
- GET
- POST
- PUT
- DELETE
- PATCH
- HEAD
- OPTIONS

## Lua API

### lua_http_request_full
```cpp
static int lua_http_request_full(lua_State* L)
```
Full-featured HTTP request function for Lua.

**Lua Usage**:
```lua
-- Simple GET
local response = http.request("https://api.example.com/data")

-- Full request with options
local response = http.request({
    Url = "https://api.example.com/data",
    Method = "POST",
    Body = '{"key": "value"}',
    ContentType = "application/json",
    Headers = {
        ["Authorization"] = "Bearer token",
        ["User-Agent"] = "Xoron/2.0"
    }
})
```

**Returns Table**:
```lua
{
    Success = true,              -- Boolean success flag
    StatusCode = 200,            -- HTTP status code
    StatusMessage = "OK",        -- Status text
    Body = "...",                -- Response body
    Headers = {                  -- Response headers
        ["Content-Type"] = "application/json",
        ["Content-Length"] = "123"
    }
}
```

**Error Returns**:
```lua
nil, "error message"
```

### xoron_register_http
```cpp
void xoron_register_http(lua_State* L)
```
Registers HTTP library with Lua state.

**Creates**:
- `http` table with `request()` function
- Global `request()` function (alias)

## Configuration

### Timeouts
- Connection timeout: 30 seconds
- Read timeout: 30 seconds

### SSL/TLS
- OpenSSL required for HTTPS
- Server certificate verification: **DISABLED**
- This allows self-signed certificates but reduces security

### Content-Type Defaults
- POST/PUT/PATCH: "application/json"
- Can be overridden via parameter or Headers

## Error Handling

All functions use `xoron_set_error()` to report errors. Check `xoron_last_error()` for details.

**Common Errors**:
- "URL is null" → NULL URL parameter
- "Invalid URL" → Malformed URL
- "HTTPS not supported (OpenSSL not available)" → Build without OpenSSL
- "HTTP request failed: ..." → Network/connection error
- "HTTP exception: ..." → C++ exception

## Usage Examples

### C API
```c
int status;
size_t len;
char* response = xoron_http_get("https://api.example.com/data", &status, &len);
if (response) {
    printf("Status: %d\n", status);
    printf("Body: %s\n", response);
    xoron_http_free(response);
} else {
    printf("Error: %s\n", xoron_last_error());
}
```

### Lua API
```lua
-- Simple GET
local res = http.request("https://httpbin.org/get")
if res and res.Success then
    print(res.Body)
end

-- POST with JSON
local res = http.request({
    Url = "https://httpbin.org/post",
    Method = "POST",
    Body = '{"test": true}',
    ContentType = "application/json"
})

-- With headers
local res = http.request({
    Url = "https://api.example.com/data",
    Headers = {
        ["Authorization"] = "Bearer mytoken",
        ["X-Custom"] = "value"
    }
})
```

## Related Files
- `xoron.h` (API declarations)
- `xoron_luau.cpp` (Lua bindings)
- `xoron_crypto.cpp` (SSL/TLS support)
