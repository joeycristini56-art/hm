# Xoron API Documentation

## Overview

This directory contains comprehensive API documentation for Xoron Executor Engine, covering both the C/C++ native API and the Lua scripting API.

## API Layers

### 1. Core C API (Native)
**Target**: C/C++ developers integrating Xoron

**Header**: `xoron.h`

**Key Functions**:
- Initialization and shutdown
- VM lifecycle management
- Script compilation and execution
- Direct service access

**Example**:
```c
#include "xoron.h"

int main() {
    xoron_init();
    xoron_vm_t* vm = xoron_vm_new();
    
    const char* script = "print('Hello from C!')";
    xoron_dostring(vm, script, "main");
    
    xoron_vm_free(vm);
    xoron_shutdown();
    return 0;
}
```

### 2. Lua API (Scripting)
**Target**: Script writers using Xoron

**Access**: Global functions and tables

**Key Libraries**:
- `http` - Network operations
- `crypto` - Cryptographic functions
- `Drawing` - 2D rendering
- `WebSocket` - Real-time communication
- `cache` - Data caching
- `Input` - Input handling
- `UI` - UI components
- Environment functions

**Example**:
```lua
-- HTTP request
local response = http.get("https://api.example.com/data")
if response then
    print("Status:", response.status)
    print("Body:", response.body)
end

-- Crypto
local hash = crypto.sha256("data")
local encoded = crypto.base64encode("data")

-- Drawing
local draw = Drawing.new()
draw:Line(100, 100, 200, 200, Color3.new(1, 0, 0), 2)
```

## Documentation Structure

```
api/
├── README.md              # This file - API overview
├── core_api.md           # C/C++ native API
├── lua_api.md            # Lua scripting API
├── crypto_api.md         # Cryptographic functions
├── http_api.md           # HTTP and WebSocket
├── drawing_api.md        # Drawing engine
└── env_api.md            # Environment functions
```

## Quick Reference

### Core C API

| Function | Purpose |
|----------|---------|
| `xoron_init()` | Initialize engine |
| `xoron_vm_new()` | Create VM instance |
| `xoron_dostring()` | Execute script |
| `xoron_compile()` | Compile to bytecode |
| `xoron_http_get()` | HTTP GET request |
| `xoron_sha256()` | SHA256 hash |
| `xoron_shutdown()` | Cleanup |

### Lua API

| Library | Functions |
|---------|-----------|
| `http` | `get()`, `post()` |
| `crypto` | `sha256()`, `base64encode()`, `aes_encrypt()` |
| `Drawing` | `Line()`, `Circle()`, `Text()`, `Clear()` |
| `WebSocket` | `connect()`, `send()`, `close()` |
| `cache` | `set()`, `get()`, `delete()` |
| `Input` | `TouchBegan`, `KeyDown` events |
| `UI` | `Button()`, `Show()`, `Hide()` |
| `getgenv()` | Global environment |
| `hookfunction()` | Function hooking |

## Common Use Cases

### 1. Network Operations
```lua
-- HTTP GET
local response = http.get("https://api.example.com")
if response then
    print(response.body)
end

-- HTTP POST
local response = http.post("https://api.example.com", 
    '{"data": "value"}', 
    "application/json")

-- WebSocket
local ws = WebSocket.connect("wss://echo.websocket.org")
ws:on_message(function(msg)
    print("Received:", msg)
end)
ws:send("Hello")
```

### 2. Cryptography
```lua
-- Hashing
local hash = crypto.sha256("data")
local hash = crypto.sha512("data")
local hash = crypto.md5("data")

-- Encoding
local b64 = crypto.base64encode("data")
local hex = crypto.hexencode("data")

-- Decoding
local data = crypto.base64decode(b64)
local data = crypto.hexdecode(hex)

-- AES Encryption
local encrypted = crypto.aes_encrypt("data", "key", "CBC")
local decrypted = crypto.aes_decrypt(encrypted, "key", "CBC")
```

### 3. Drawing
```lua
local draw = Drawing.new()

-- Lines
draw:Line(100, 100, 200, 200, Color3.new(1, 0, 0), 2)

-- Circles
draw:Circle(150, 150, 50, Color3.new(0, 1, 0), true)  -- filled

-- Text
draw:Text("Hello", 100, 200, 16, Color3.new(1, 1, 1))

-- Clear
draw:Clear()
```

### 4. Environment Manipulation
```lua
-- Get environments
local genv = getgenv()      -- Global
local renv = getrenv()      -- Roblox
local senv = getsenv(script) -- Script

-- Function hooking
local original = print
hookfunction(print, function(...)
    print("Hooked:", ...)
    return original(...)
end)

-- Signal connections
local connection = workspace.Changed:Connect(function(prop)
    print("Changed:", prop)
end)
connection:Disconnect()
```

### 5. File Operations
```lua
-- Reading
local content = readfile("workspace/script.lua")

-- Writing
writefile("workspace/output.txt", "data")

-- Appending
appendfile("workspace/log.txt", "line\n")

-- Deleting
deletefile("workspace/temp.txt")

-- Workspace management
setworkspace("/custom/path")
local ws = getworkspace()
```

### 6. Caching
```lua
-- Set with TTL (1 hour)
cache:set("key", "value", 3600)

-- Get
local value = cache:get("key")

-- Delete
cache:delete("key")

-- Clear all
cache:clear()
```

### 7. UI Components
```lua
-- Button
local btn = UI.Button({
    text = "Click me",
    position = {100, 100},
    size = {200, 50},
    callback = function()
        print("Clicked!")
    end
})

-- Show/Hide UI
UI.Show()
UI.Hide()
UI.Toggle()
```

## Error Handling

### C API
```c
xoron_vm_t* vm = xoron_vm_new();
if (!vm) {
    fprintf(stderr, "Error: %s\n", xoron_last_error());
    return 1;
}

int result = xoron_dostring(vm, script, "main");
if (result != XORON_OK) {
    fprintf(stderr, "Error: %s\n", xoron_last_error());
}
```

### Lua API
```lua
-- HTTP with error handling
local response, err = http.get("https://api.example.com")
if not response then
    print("Error:", err)
    return
end

-- Protected execution
local success, result = pcall(function()
    return risky_operation()
end)

if not success then
    print("Error:", result)
end
```

## Platform-Specific APIs

### iOS Only
```lua
-- Haptic feedback
hapticFeedback(0)  -- Light

-- iOS UI
ios_ui_show()
ios_ui_hide()
ios_ui_toggle()
```

### Android Only
```lua
-- Haptic feedback
hapticFeedback(1)  -- Medium

-- Android UI
android_ui_show()
android_ui_hide()
android_ui_toggle()
```

## Performance Tips

1. **Batch Operations**: Group drawing operations
2. **Connection Pooling**: Reuse HTTP clients
3. **Bytecode Caching**: Compile once, run many times
4. **Memory Limits**: Set appropriate limits
5. **Async Operations**: Use callbacks for network

## Security Considerations

1. **Input Validation**: Always validate external input
2. **URL Validation**: Use `validate_url()` before requests
3. **File Paths**: Keep within workspace
4. **Execution Limits**: Set time/memory limits
5. **Error Messages**: Don't expose sensitive info

## Getting Help

### For C API Questions
- See `core_api.md` for detailed function documentation
- Check `xoron.h` for function signatures
- Review `src/xoron_luau.cpp` for Lua bindings

### For Lua API Questions
- See `lua_api.md` for complete library reference
- Check `src/tests/` for working examples
- Review `src/lua/` for utility scripts

### For Platform-Specific Questions
- See `../platforms/ios.md` for iOS details
- See `../platforms/android.md` for Android details
- See `../platforms/cross_platform.md` for portability

## API Version

**Xoron Version**: 2.0.0  
**API Version**: 2.0.0  
**Last Updated**: 2026-01-07

---

**Next:**
- [Core C API](core_api.md) - Native interface
- [Lua API Reference](lua_api.md) - Scripting interface
- [Crypto API](crypto_api.md) - Cryptographic functions
- [HTTP API](http_api.md) - Network operations
- [Drawing API](drawing_api.md) - Graphics rendering
- [Environment API](env_api.md) - Lua environment
