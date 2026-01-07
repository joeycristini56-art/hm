# Lua API Reference

## Overview

The Lua API provides high-level scripting capabilities for Xoron. All functions are available as global variables or in global tables.

## HTTP Library

### http.get

```lua
local response = http.get(url)
```

**Description**: Performs HTTP GET request.

**Parameters**:
- `url` (string): Target URL (must be http:// or https://)

**Returns**:
- `table` with `status` and `body` on success
- `nil` on error, followed by error message

**Example**:
```lua
local response = http.get("https://api.example.com/data")
if response then
    print("Status:", response.status)
    print("Body:", response.body)
else
    print("Error:", http.last_error())
end
```

**Notes**:
- Timeout: 30 seconds
- HTTPS requires OpenSSL support
- Returns immediately, synchronous

---

### http.post

```lua
local response = http.post(url, body, content_type)
```

**Description**: Performs HTTP POST request.

**Parameters**:
- `url` (string): Target URL
- `body` (string): Request body
- `content_type` (string, optional): Content-Type header (default: "application/json")

**Returns**:
- `table` with `status` and `body` on success
- `nil` on error, followed by error message

**Example**:
```lua
local data = '{"key": "value"}'
local response = http.post("https://api.example.com", data, "application/json")
if response then
    print("Response:", response.body)
end
```

---

### http.last_error

```lua
local error = http.last_error()
```

**Description**: Gets last HTTP error message.

**Returns**: Error string or nil

---

## Crypto Library

### crypto.sha256

```lua
local hash = crypto.sha256(data)
```

**Description**: Computes SHA256 hash.

**Parameters**:
- `data` (string): Input data

**Returns**: Hex string (64 characters)

**Example**:
```lua
local hash = crypto.sha256("hello")
print(hash)  -- 2cf24dba5fb0a30e...
```

---

### crypto.sha384

```lua
local hash = crypto.sha384(data)
```

**Description**: Computes SHA384 hash.

**Parameters**:
- `data` (string): Input data

**Returns**: Hex string (96 characters)

---

### crypto.sha512

```lua
local hash = crypto.sha512(data)
```

**Description**: Computes SHA512 hash.

**Parameters**:
- `data` (string): Input data

**Returns**: Hex string (128 characters)

---

### crypto.md5

```lua
local hash = crypto.md5(data)
```

**Description**: Computes MD5 hash.

**Parameters**:
- `data` (string): Input data

**Returns**: Hex string (32 characters)

**Note**: MD5 is cryptographically broken, use only for non-security purposes.

---

### crypto.base64encode

```lua
local encoded = crypto.base64encode(data)
```

**Description**: Encodes data to Base64.

**Parameters**:
- `data` (string): Input data

**Returns**: Base64 encoded string

**Example**:
```lua
local b64 = crypto.base64encode("hello")
print(b64)  -- aGVsbG8=
```

---

### crypto.base64decode

```lua
local data = crypto.base64decode(encoded)
```

**Description**: Decodes Base64 string.

**Parameters**:
- `encoded` (string): Base64 string

**Returns**: Decoded string

---

### crypto.hexencode

```lua
local hex = crypto.hexencode(data)
```

**Description**: Encodes data to hexadecimal.

**Parameters**:
- `data` (string): Input data

**Returns**: Hex string

**Example**:
```lua
local hex = crypto.hexencode("hello")
print(hex)  -- 68656c6c6f
```

---

### crypto.hexdecode

```lua
local data = crypto.hexdecode(hex)
```

**Description**: Decodes hexadecimal string.

**Parameters**:
- `hex` (string): Hex string

**Returns**: Decoded string

---

### crypto.aes_encrypt

```lua
local encrypted = crypto.aes_encrypt(data, key, mode, iv)
```

**Description**: Encrypts data using AES.

**Parameters**:
- `data` (string): Data to encrypt
- `key` (string): Encryption key (16, 24, or 32 bytes)
- `mode` (string, optional): "CBC" or "GCM" (default: "CBC")
- `iv` (string, optional): Initialization vector (16 bytes for CBC, 12 bytes for GCM)

**Returns**: Encrypted data (binary string)

**Example**:
```lua
local key = "1234567890123456"  -- 16 bytes
local iv = "1234567890123456"   -- 16 bytes
local encrypted = crypto.aes_encrypt("secret data", key, "CBC", iv)
```

---

### crypto.aes_decrypt

```lua
local decrypted = crypto.aes_decrypt(data, key, mode, iv)
```

**Description**: Decrypts AES-encrypted data.

**Parameters**:
- `data` (string): Encrypted data
- `key` (string): Decryption key
- `mode` (string, optional): "CBC" or "GCM"
- `iv` (string, optional): Initialization vector

**Returns**: Decrypted data

---

### crypto.hmac

```lua
local mac = crypto.hmac(data, key, algorithm)
```

**Description**: Computes HMAC.

**Parameters**:
- `data` (string): Input data
- `key` (string): HMAC key
- `algorithm` (string, optional): "sha256", "sha384", "sha512" (default: "sha256")

**Returns**: HMAC as hex string

---

### crypto.random

```lua
local bytes = crypto.random(length)
```

**Description**: Generates cryptographically secure random bytes.

**Parameters**:
- `length` (number): Number of bytes

**Returns**: Random bytes (binary string)

**Example**:
```lua
local random = crypto.random(32)
print(crypto.hexencode(random))
```

---

## Drawing Library

### Drawing.new

```lua
local draw = Drawing.new()
```

**Description**: Creates a new drawing context.

**Returns**: Drawing object

**Example**:
```lua
local draw = Drawing.new()
```

---

### Drawing:Line

```lua
draw:Line(x1, y1, x2, y2, color, thickness)
```

**Description**: Draws a line.

**Parameters**:
- `x1`, `y1` (number): Start point
- `x2`, `y2` (number): End point
- `color` (Color3): Color
- `thickness` (number, optional): Line thickness (default: 1)

**Example**:
```lua
draw:Line(100, 100, 200, 200, Color3.new(1, 0, 0), 2)
```

---

### Drawing:Circle

```lua
draw:Circle(x, y, radius, color, filled, thickness)
```

**Description**: Draws a circle.

**Parameters**:
- `x`, `y` (number): Center position
- `radius` (number): Circle radius
- `color` (Color3): Color
- `filled` (boolean, optional): Fill circle (default: false)
- `thickness` (number, optional): Outline thickness (default: 1)

**Example**:
```lua
draw:Circle(150, 150, 50, Color3.new(0, 1, 0), true)
```

---

### Drawing:Square

```lua
draw:Square(x, y, width, height, color, filled, thickness, rounding)
```

**Description**: Draws a square/rectangle.

**Parameters**:
- `x`, `y` (number): Top-left position
- `width`, `height` (number): Size
- `color` (Color3): Color
- `filled` (boolean, optional): Fill square (default: false)
- `thickness` (number, optional): Outline thickness (default: 1)
- `rounding` (number, optional): Corner rounding (default: 0)

**Example**:
```lua
draw:Square(100, 100, 200, 100, Color3.new(0, 0, 1), false, 2, 5)
```

---

### Drawing:Text

```lua
draw:Text(text, x, y, size, color, centered, outline, outline_color)
```

**Description**: Draws text.

**Parameters**:
- `text` (string): Text to draw
- `x`, `y` (number): Position
- `size` (number): Font size
- `color` (Color3): Text color
- `centered` (boolean, optional): Center text (default: false)
- `outline` (boolean, optional): Add outline (default: false)
- `outline_color` (Color3, optional): Outline color (default: white)

**Example**:
```lua
draw:Text("Hello", 150, 150, 16, Color3.new(1, 1, 1), true, true)
```

---

### Drawing:Triangle

```lua
draw:Triangle(x1, y1, x2, y2, x3, y3, color, filled, thickness)
```

**Description**: Draws a triangle.

**Parameters**:
- `x1`, `y1`, `x2`, `y2`, `x3`, `y3` (number): Three vertices
- `color` (Color3): Color
- `filled` (boolean, optional): Fill triangle (default: false)
- `thickness` (number, optional): Outline thickness (default: 1)

---

### Drawing:Quad

```lua
draw:Quad(x1, y1, x2, y2, x3, y3, x4, y4, color, filled, thickness)
```

**Description**: Draws a quadrilateral.

**Parameters**:
- `x1`, `y1`, ..., `x4`, `y4` (number): Four vertices
- `color` (Color3): Color
- `filled` (boolean, optional): Fill quad (default: false)
- `thickness` (number, optional): Outline thickness (default: 1)

---

### Drawing:Clear

```lua
draw:Clear()
```

**Description**: Clears all drawing objects.

---

### Drawing:Remove

```lua
draw:Remove()
```

**Description**: Removes the drawing context.

---

### Color3

```lua
Color3.new(r, g, b)
```

**Description**: Creates a color.

**Parameters**:
- `r`, `g`, `b` (number): RGB values (0-1)

**Example**:
```lua
local red = Color3.new(1, 0, 0)
local green = Color3.new(0, 1, 0)
local blue = Color3.new(0, 0, 1)
local white = Color3.new(1, 1, 1)
local black = Color3.new(0, 0, 0)
```

---

## WebSocket Library

### WebSocket.connect

```lua
local ws = WebSocket.connect(url)
```

**Description**: Connects to WebSocket server.

**Parameters**:
- `url` (string): WebSocket URL (ws:// or wss://)

**Returns**: WebSocket object or nil on error

**Example**:
```lua
local ws = WebSocket.connect("wss://echo.websocket.org")
if not ws then
    print("Connection failed")
    return
end
```

---

### WebSocket:on_message

```lua
ws:on_message(callback)
```

**Description**: Sets message callback.

**Parameters**:
- `callback` (function): Function called with message

**Example**:
```lua
ws:on_message(function(msg)
    print("Received:", msg)
end)
```

---

### WebSocket:on_close

```lua
ws:on_close(callback)
```

**Description**: Sets close callback.

**Parameters**:
- `callback` (function): Function called on close

**Example**:
```lua
ws:on_close(function()
    print("Connection closed")
end)
```

---

### WebSocket:on_error

```lua
ws:on_error(callback)
```

**Description**: Sets error callback.

**Parameters**:
- `callback` (function): Function called on error

**Example**:
```lua
ws:on_error(function(err)
    print("Error:", err)
end)
```

---

### WebSocket:send

```lua
ws:send(message)
```

**Description**: Sends a message.

**Parameters**:
- `message` (string): Message to send

**Example**:
```lua
ws:send("Hello from Xoron!")
```

---

### WebSocket:close

```lua
ws:close()
```

**Description**: Closes the connection.

---

## Filesystem Library

### readfile

```lua
local content = readfile(path)
```

**Description**: Reads file content.

**Parameters**:
- `path` (string): File path (relative to workspace)

**Returns**: File content or nil on error

**Example**:
```lua
local content = readfile("scripts/my_script.lua")
if content then
    print(content)
end
```

---

### writefile

```lua
writefile(path, content)
```

**Description**: Writes content to file.

**Parameters**:
- `path` (string): File path
- `content` (string): Content to write

**Example**:
```lua
writefile("output.txt", "Hello, World!")
```

---

### appendfile

```lua
appendfile(path, content)
```

**Description**: Appends content to file.

**Parameters**:
- `path` (string): File path
- `content` (string): Content to append

---

### deletefile

```lua
deletefile(path)
```

**Description**: Deletes a file.

**Parameters**:
- `path` (string): File path

---

### isfile

```lua
local exists = isfile(path)
```

**Description**: Checks if file exists.

**Parameters**:
- `path` (string): File path

**Returns**: boolean

---

### listfiles

```lua
local files = listfiles(path)
```

**Description**: Lists files in directory.

**Parameters**:
- `path` (string): Directory path

**Returns**: Table of file names

---

### getworkspace

```lua
local path = getworkspace()
```

**Description**: Gets current workspace path.

**Returns**: Path string

---

### setworkspace

```lua
setworkspace(path)
```

**Description**: Sets workspace path.

**Parameters**:
- `path` (string): New workspace path

---

## Cache Library

### cache.set

```lua
cache:set(key, value, ttl)
```

**Description**: Stores value in cache.

**Parameters**:
- `key` (string): Cache key
- `value` (any): Value to store
- `ttl` (number, optional): Time to live in seconds (default: unlimited)

**Example**:
```lua
cache:set("user_data", {name = "John"}, 3600)  -- 1 hour
```

---

### cache.get

```lua
local value = cache:get(key)
```

**Description**: Retrieves value from cache.

**Parameters**:
- `key` (string): Cache key

**Returns**: Stored value or nil

---

### cache.delete

```lua
cache:delete(key)
```

**Description**: Deletes cache entry.

**Parameters**:
- `key` (string): Cache key

---

### cache.clear

```lua
cache:clear()
```

**Description**: Clears all cache entries.

---

## Input Library

### Input.TouchBegan

```lua
Input.TouchBegan:Connect(callback)
```

**Description**: Connects to touch begin event.

**Parameters**:
- `callback` (function): Function called with (x, y, id)

**Example**:
```lua
Input.TouchBegan:Connect(function(x, y, id)
    print("Touch started at", x, y, "ID:", id)
end)
```

---

### Input.TouchMoved

```lua
Input.TouchMoved:Connect(callback)
```

**Description**: Connects to touch move event.

**Parameters**:
- `callback` (function): Function called with (x, y, id)

---

### Input.TouchEnded

```lua
Input.TouchEnded:Connect(callback)
```

**Description**: Connects to touch end event.

**Parameters**:
- `callback` (function): Function called with (x, y, id)

---

### Input.KeyDown

```lua
Input.KeyDown:Connect(callback)
```

**Description**: Connects to key down event.

**Parameters**:
- `callback` (function): Function called with (key)

**Example**:
```lua
Input.KeyDown:Connect(function(key)
    print("Key pressed:", key)
end)
```

---

### Input.KeyUp

```lua
Input.KeyUp:Connect(callback)
```

**Description**: Connects to key up event.

**Parameters**:
- `callback` (function): Function called with (key)

---

## UI Library

### UI.Button

```lua
local button = UI.Button(config)
```

**Description**: Creates a button.

**Parameters**:
- `config` (table): Configuration table
  - `text` (string): Button text
  - `position` (table): {x, y}
  - `size` (table): {width, height}
  - `callback` (function): Click callback

**Returns**: Button object

**Example**:
```lua
local btn = UI.Button({
    text = "Click me",
    position = {100, 100},
    size = {200, 50},
    callback = function()
        print("Button clicked!")
    end
})
```

---

### UI.Show

```lua
UI.Show()
```

**Description**: Shows the UI.

---

### UI.Hide

```lua
UI.Hide()
```

**Description**: Hides the UI.

---

### UI.Toggle

```lua
UI.Toggle()
```

**Description**: Toggles UI visibility.

---

## Environment Functions

### getgenv

```lua
local env = getgenv()
```

**Description**: Gets global environment.

**Returns**: Global environment table

**Example**:
```lua
local genv = getgenv()
genv.my_global = "value"
print(my_global)  -- "value"
```

---

### getrenv

```lua
local env = getrenv()
```

**Description**: Gets Roblox environment (if available).

**Returns**: Roblox environment table

---

### getsenv

```lua
local env = getsenv(script)
```

**Description**: Gets script environment.

**Parameters**:
- `script` (Script): Script object

**Returns**: Script environment table

---

### getmenv

```lua
local env = getmenv(module)
```

**Description**: Gets module environment.

**Parameters**:
- `module` (ModuleScript): Module object

**Returns**: Module environment table

---

### identifyexecutor

```lua
local name, version = identifyexecutor()
```

**Description**: Identifies the executor.

**Returns**: Name and version strings

**Example**:
```lua
local name, version = identifyexecutor()
print(name, version)  -- "Xoron" "2.0.0"
```

---

### hookfunction

```lua
local original = hookfunction(func, replacement)
```

**Description**: Hooks a function.

**Parameters**:
- `func` (function): Function to hook
- `replacement` (function): New function

**Returns**: Original function

**Example**:
```lua
local original_print = hookfunction(print, function(...)
    print("[HOOKED]", ...)
    return original_print(...)
end)
```

---

### hookmetamethod

```lua
local original = hookmetamethod(object, method, replacement)
```

**Description**: Hooks a metamethod.

**Parameters**:
- `object` (table): Object
- `method` (string): Metamethod name (e.g., "__index")
- `replacement` (function): New function

**Returns**: Original metamethod

---

### getgc

```lua
local objects = getgc()
```

**Description**: Gets garbage collector objects.

**Returns**: Table of objects

---

### getreg

```lua
local registry = getreg()
```

**Description**: Gets Lua registry.

**Returns**: Registry table

---

### getscripts

```lua
local scripts = getscripts()
```

**Description**: Gets all loaded scripts.

**Returns**: Table of scripts

---

### getmodules

```lua
local modules = getmodules()
```

**Description**: Gets all loaded modules.

**Returns**: Table of modules

---

### getfpscap

```lua
local fps = getfpscap()
```

**Description**: Gets FPS cap.

**Returns**: FPS limit number

---

### setfpscap

```lua
setfpscap(fps)
```

**Description**: Sets FPS cap.

**Parameters**:
- `fps` (number): FPS limit

**Example**:
```lua
setfpscap(60)  -- Limit to 60 FPS
```

---

### gethiddenproperty

```lua
local value = gethiddenproperty(object, property)
```

**Description**: Gets hidden property.

**Parameters**:
- `object` (Instance): Object
- `property` (string): Property name

**Returns**: Property value

---

### sethiddenproperty

```lua
sethiddenproperty(object, property, value)
```

**Description**: Sets hidden property.

**Parameters**:
- `object` (Instance): Object
- `property` (string): Property name
- `value` (any): Property value

---

### getnilinstances

```lua
local instances = getnilinstances()
```

**Description**: Gets nil instances.

**Returns**: Table of instances

---

### saveinstance

```lua
local data = saveinstance(options)
```

**Description**: Saves game instance to RBXMX format.

**Parameters**:
- `options` (table, optional): Configuration
  - `FileName` (string): Output filename
  - `DecompileScripts` (boolean): Decompile scripts
  - `Mode` (string): "optimized", "full", or "scripts"

**Returns**: RBXMX string

**Example**:
```lua
local data = saveinstance({
    FileName = "game",
    Mode = "optimized"
})
writefile("game.rbxmx", data)
```

---

## Platform-Specific Functions

### hapticFeedback

```lua
hapticFeedback(style)
```

**Description**: Triggers haptic feedback.

**Parameters**:
- `style` (number): 0=Light, 1=Medium, 2=Heavy

**Platform**: iOS, Android

---

### ios_ui_show / android_ui_show

```lua
ios_ui_show()
android_ui_show()
```

**Description**: Shows native UI.

**Platform**: iOS/Android specific

---

### ios_ui_hide / android_ui_hide

```lua
ios_ui_hide()
android_ui_hide()
```

**Description**: Hides native UI.

**Platform**: iOS/Android specific

---

### ios_ui_toggle / android_ui_toggle

```lua
ios_ui_toggle()
android_ui_toggle()
```

**Description**: Toggles native UI.

**Platform**: iOS/Android specific

---

## Utility Functions

### print

```lua
print(...)
```

**Description**: Prints to console.

**Example**:
```lua
print("Hello", "World", 123)
```

---

### warn

```lua
warn(...)
```

**Description**: Prints warning to console.

---

### error

```lua
error(message)
```

**Description**: Throws an error.

---

### pcall

```lua
local success, result = pcall(function)
```

**Description**: Protected call.

**Example**:
```lua
local success, result = pcall(function()
    return risky_operation()
end)

if not success then
    print("Error:", result)
end
```

---

### xpcall

```lua
local success = xpcall(function, error_handler)
```

**Description**: Protected call with error handler.

---

### type

```lua
local t = type(value)
```

**Description**: Gets type of value.

**Returns**: "nil", "number", "string", "boolean", "table", "function", "thread", "userdata"

---

### typeof

```lua
local t = typeof(value)
```

**Description**: Gets extended type.

**Returns**: Type string

---

### tostring

```lua
local str = tostring(value)
```

**Description**: Converts to string.

---

### tonumber

```lua
local num = tonumber(str)
```

**Description**: Converts to number.

---

### pairs

```lua
for key, value in pairs(table) do
    -- iterate
end
```

**Description**: Iterates table pairs.

---

### ipairs

```lua
for index, value in ipairs(table) do
    -- iterate
end
```

**Description**: Iterates array elements.

---

### table.insert

```lua
table.insert(table, value)
```

**Description**: Inserts value into table.

---

### table.remove

```lua
table.remove(table, index)
```

**Description**: Removes value from table.

---

### table.sort

```lua
table.sort(table, comparator)
```

**Description**: Sorts table.

---

### string.sub

```lua
local substr = string.sub(str, start, end)
```

**Description**: Extracts substring.

---

### string.find

```lua
local pos = string.find(str, pattern)
```

**Description**: Finds pattern in string.

---

### string.gsub

```lua
local result = string.gsub(str, pattern, repl)
```

**Description**: Replaces pattern in string.

---

### math.random

```lua
local num = math.random(min, max)
```

**Description**: Generates random number.

---

### math.floor / math.ceil / math.round

```lua
local n = math.floor(x)
local n = math.ceil(x)
local n = math.round(x)
```

**Description**: Rounding functions.

---

## Complete Example

```lua
-- HTTP and Crypto
local response = http.get("https://api.github.com")
if response then
    print("Status:", response.status)
    
    -- Hash the response
    local hash = crypto.sha256(response.body)
    print("SHA256:", hash)
    
    -- Save to file
    writefile("github_response.json", response.body)
end

-- Drawing
local draw = Drawing.new()
draw:Line(100, 100, 200, 200, Color3.new(1, 0, 0), 2)
draw:Circle(150, 150, 50, Color3.new(0, 1, 0), true)

-- WebSocket
local ws = WebSocket.connect("wss://echo.websocket.org")
if ws then
    ws:on_message(function(msg)
        print("Echo:", msg)
    end)
    
    ws:send("Hello Xoron!")
end

-- Caching
cache:set("api_data", response.body, 3600)
local cached = cache:get("api_data")

-- Environment
local genv = getgenv()
genv.my_global = "Xoron"

-- Hooking
local original_print = hookfunction(print, function(...)
    print("[HOOK]", ...)
    return original_print(...)
end)

-- UI
local btn = UI.Button({
    text = "Test",
    position = {100, 100},
    size = {100, 50},
    callback = function()
        print("Button clicked!")
    end
})

-- Input
Input.TouchBegan:Connect(function(x, y, id)
    print("Touch at", x, y)
end)

-- FPS control
setfpscap(60)
print("FPS Cap:", getfpscap())

-- Save instance
local game_data = saveinstance({
    FileName = "my_game",
    Mode = "optimized"
})
writefile("game.rbxmx", game_data)

print("All operations completed!")
```

---

**Next:**
- [Crypto API](crypto_api.md)
- [HTTP API](http_api.md)
- [Drawing API](drawing_api.md)
- [Environment API](env_api.md)
