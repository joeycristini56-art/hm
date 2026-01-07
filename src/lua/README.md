# Lua Scripts

## Overview

This directory contains Lua utility scripts for Xoron Executor Engine. These scripts provide high-level functionality built on top of the core C API.

## Scripts

### saveinstance.lua

**Purpose**: Roblox instance serialization to RBXMX format

**Description**: 
A comprehensive script that saves Roblox game instances to RBXMX format, supporting various options for customization including script decompilation, filtering, and optimization modes.

**Features**:
- Full RBXMX serialization
- Script decompilation support
- Instance filtering
- Multiple save modes (optimized, full, scripts)
- Property serialization for 50+ property types
- Reference management
- XML escaping and encoding

**Usage**:
```lua
-- Load the script
local SaveInstance = loadstring(game:HttpGet("https://.../saveinstance.lua"))()

-- Basic usage
SaveInstance.Save()

-- With options
SaveInstance.Save({
    FileName = "my_game",
    DecompileScripts = false,
    RemovePlayerCharacters = true,
    Mode = "optimized",
    ShowStatus = true
})
```

**Configuration Options**:

```lua
local options = {
    -- File naming
    FileName = "game",                    -- Output filename (without extension)
    
    -- Script handling
    DecompileScripts = false,             -- Decompile scripts (requires decompile)
    IgnoreNotArchivable = true,           -- Skip non-archivable instances
    
    -- Instance filtering
    RemovePlayerCharacters = true,        -- Remove player characters
    SavePlayers = false,                  -- Include player instances
    IsolateStarterPlayer = false,         -- Isolate StarterPlayer
    IsolateLocalPlayer = false,           -- Isolate LocalPlayer
    IsolateLocalPlayerCharacter = false,  -- Isolate LocalPlayer character
    NilInstances = false,                 -- Save nil instances
    
    -- Property handling
    IgnoreDefaultProperties = true,       -- Skip default property values
    IgnorePropertiesOfNotScriptsOnScriptsMode = false,
    SaveNonCreatable = false,             -- Save non-creatable instances
    
    -- UI
    ShowStatus = true,                    -- Show progress in console
    
    -- Mode
    Mode = "optimized",                   -- "optimized", "full", or "scripts"
}
```

**Save Modes**:

1. **optimized**: Saves only essential services and properties
2. **full**: Saves everything including all services
3. **scripts**: Saves only scripts and their dependencies

**Property Types Supported**:
- string, bool, int, int64, float, double
- BinaryString, ProtectedString, Content
- Vector2, Vector3, CFrame
- Color3, BrickColor
- UDim, UDim2, Rect
- NumberSequence, ColorSequence, NumberRange
- Ref (Instance references)
- Enum, Font

**Example Output**:
```xml
<?xml version="1.0" encoding="utf-8"?>
<roblox xmlns:xmime="http://www.w3.org/2005/05/xmlmime" 
        xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" 
        xsi:noNamespaceSchemaLocation="http://www.roblox.com/roblox.xsd" 
        version="4">
    <Meta name="ExplicitAutoJoints">true</Meta>
    <External>null</External>
    <External>nil</External>
    <Item class="Workspace" referent="RBX001">
        <Properties>
            <string name="Name">Workspace</string>
        </Properties>
        <!-- Children -->
    </Item>
</roblox>
```

**Advanced Usage**:

```lua
local SaveInstance = loadstring(game:HttpGet("https://.../saveinstance.lua"))()

-- Save specific object
SaveInstance.Save({
    Object = workspace.Map,
    FileName = "map",
    Mode = "full"
})

-- Save with decompilation
SaveInstance.Save({
    DecompileScripts = true,
    Mode = "scripts"
})

-- Save nil instances
SaveInstance.Save({
    NilInstances = true,
    SavePlayers = true
})

-- Custom filtering
local function customFilter(instance)
    -- Skip certain instances
    if instance.Name:find("Test") then
        return false
    end
    return true
end

-- Note: You would need to modify the script to add custom filters
```

**Performance Considerations**:
- Large games may take several minutes
- Decompiling increases time significantly
- Use "optimized" mode for faster saves
- Consider saving specific objects instead of entire game

**Limitations**:
- Cannot save certain security-restricted properties
- Decompilation may not work for all scripts
- Very large games may hit memory limits
- Some custom instances may not serialize correctly

**Troubleshooting**:

1. **Script too large**: Use "optimized" mode or save specific objects
2. **Decompilation fails**: Some scripts cannot be decompiled
3. **Missing properties**: Check "IgnoreDefaultProperties" setting
4. **Memory errors**: Close other applications, try smaller scope
5. **Invalid XML**: Report as bug, includes invalid characters

**Integration with Xoron**:

```lua
-- Save and store in workspace
local SaveInstance = loadstring(game:HttpGet("https://.../saveinstance.lua"))()

SaveInstance.Save({
    FileName = "backup",
    Mode = "optimized"
})

-- The saved file can be accessed via:
local content = readfile("workspace/backup.rbxmx")

-- Or upload to server
local response = http.post("https://myserver.com/upload", content, "application/xml")
```

**Security Notes**:
- Only use on games you have permission to save
- Respect Roblox Terms of Service
- Be aware of copyright implications
- Do not distribute saved games without permission

## Other Potential Scripts

### autoexecute.lua (Template)

```lua
-- Auto-execute script template
-- Place in ~/Documents/Xoron/autoexecute/ (iOS)
-- Place in /data/data/<package>/files/Xoron/autoexecute/ (Android)

print("Auto-execute script loaded!")

-- Example: Load common libraries
local http = require("http")
local crypto = require("crypto")

-- Example: Set up environment
getgenv().MyLibrary = {
    version = "1.0.0",
    author = "You"
}

-- Example: Persistent cache
local cache = cache or {}
cache.lastRun = os.time()
print("Last run:", cache.lastRun)
```

### utility.lua (Template)

```lua
-- Common utility functions
local Utility = {}

-- String utilities
Utility.startsWith = function(str, prefix)
    return string.sub(str, 1, #prefix) == prefix
end

Utility.endsWith = function(str, suffix)
    return string.sub(str, -#suffix) == suffix
end

Utility.split = function(str, sep)
    local fields = {}
    local pattern = string.format("([^%s]+)", sep)
    str:gsub(pattern, function(c) fields[#fields+1] = c end)
    return fields
end

-- Table utilities
Utility.deepCopy = function(tbl)
    local result = {}
    for k, v in pairs(tbl) do
        if type(v) == "table" then
            result[k] = Utility.deepCopy(v)
        else
            result[k] = v
        end
    end
    return result
end

Utility.merge = function(t1, t2)
    for k, v in pairs(t2) do
        t1[k] = v
    end
    return t1
end

-- Network utilities
Utility.httpGetJSON = function(url)
    local response = http.get(url)
    if response then
        return game:GetService("HttpService"):JSONDecode(response.body)
    end
    return nil
end

Utility.httpPostJSON = function(url, data)
    local json = game:GetService("HttpService"):JSONEncode(data)
    return http.post(url, json, "application/json")
end

-- File utilities
Utility.saveJSON = function(filename, data)
    local json = game:GetService("HttpService"):JSONEncode(data)
    writefile(filename, json)
end

Utility.loadJSON = function(filename)
    local content = readfile(filename)
    return game:GetService("HttpService"):JSONDecode(content)
end

return Utility
```

### network.lua (Template)

```lua
-- Network utilities
local Network = {}

-- Retry wrapper
function Network.withRetry(func, maxRetries, delay)
    maxRetries = maxRetries or 3
    delay = delay or 1
    
    for i = 1, maxRetries do
        local success, result = pcall(func)
        if success then
            return result
        end
        
        if i < maxRetries then
            wait(delay)
        end
    end
    
    error("Max retries exceeded")
end

-- HTTP with caching
function Network.getCached(url, ttl)
    ttl = ttl or 3600  -- 1 hour
    
    local cacheKey = "net_cache_" .. url
    local cached = cache:get(cacheKey)
    
    if cached then
        return cached
    end
    
    local response = http.get(url)
    if response then
        cache:set(cacheKey, response.body, ttl)
        return response.body
    end
    
    return nil
end

-- WebSocket wrapper
function Network.createWebSocket(url, handlers)
    local ws = WebSocket.connect(url)
    
    if not ws then
        return nil
    end
    
    if handlers.onMessage then
        ws:on_message(handlers.onMessage)
    end
    
    if handlers.onClose then
        ws:on_close(handlers.onClose)
    end
    
    if handlers.onError then
        ws:on_error(handlers.onError)
    end
    
    return ws
end

return Network
```

## Loading Scripts

### From Local Storage

```lua
-- Load from workspace
local script = readfile("workspace/my_script.lua")
loadstring(script)()

-- Or with error handling
local success, result = pcall(function()
    local script = readfile("workspace/my_script.lua")
    return loadstring(script)
end)

if success then
    result()
else
    print("Error loading script:", result)
end
```

### From HTTP

```lua
-- Load from URL
local url = "https://raw.githubusercontent.com/user/repo/main/script.lua"
local response = http.get(url)

if response and response.status == 200 then
    loadstring(response.body)()
else
    print("Failed to load script")
end

-- With caching
local cached = cache:get(url)
if cached then
    loadstring(cached)()
else
    local response = http.get(url)
    if response then
        cache:set(url, response.body, 3600)  -- Cache for 1 hour
        loadstring(response.body)()
    end
end
```

### From Roblox Assets

```lua
-- Load from Roblox asset
local assetId = 123456789
local url = "https://www.roblox.com/asset/?id=" .. assetId

local response = http.get(url)
if response and response.status == 200 then
    loadstring(response.body)()
end
```

## Best Practices

### 1. Error Handling

```lua
-- Always use pcall for external scripts
local success, result = pcall(function()
    loadstring(script)()
end)

if not success then
    print("Script error:", result)
end
```

### 2. Sandboxing

```lua
-- Create isolated environment
local env = getgenv()
local sandbox = {}

-- Copy safe functions
sandbox.print = print
sandbox.http = http
sandbox.crypto = crypto

-- Execute in sandbox
local func = loadstring(script)
setfenv(func, sandbox)
func()
```

### 3. Version Checking

```lua
-- Check Xoron version
local name, version = identifyexecutor()
if version < "2.0.0" then
    error("Xoron 2.0.0+ required")
end
```

### 4. Resource Management

```lua
-- Clean up resources
local ws = WebSocket.connect("wss://...")

-- Set up cleanup
local function cleanup()
    if ws then
        ws:close()
    end
    cache:clear()
end

-- Register cleanup on exit
getgenv()._cleanup = cleanup
```

### 5. Performance Monitoring

```lua
-- Monitor execution time
local start = os.clock()

-- Your code here

local elapsed = os.clock() - start
print(string.format("Execution time: %.2f ms", elapsed * 1000))
```

## Script Organization

### Recommended Structure

```
workspace/
├── autoexecute/          # Auto-run scripts
│   ├── init.lua
│   └── config.lua
├── scripts/              # Utility scripts
│   ├── network.lua
│   ├── crypto.lua
│   └── ui.lua
├── saves/                # Saved data
│   ├── game.rbxmx
│   └── config.json
└── logs/                 # Log files
    └── execution.log
```

### Module Pattern

```lua
-- my_module.lua
local MyModule = {}

MyModule.config = {
    setting1 = true,
    setting2 = "default"
}

function MyModule.doSomething()
    -- Implementation
end

function MyModule.doSomethingElse()
    -- Implementation
end

return MyModule

-- Usage
local myModule = loadstring(readfile("workspace/scripts/my_module.lua"))()
myModule.doSomething()
```

## Integration Examples

### 1. Auto-Backup System

```lua
-- Save game every hour
while true do
    wait(3600)  -- 1 hour
    
    local timestamp = os.date("%Y%m%d_%H%M%S")
    local filename = string.format("workspace/backups/game_%s.rbxmx", timestamp)
    
    local SaveInstance = loadstring(game:HttpGet("https://.../saveinstance.lua"))()
    SaveInstance.Save({
        FileName = filename,
        Mode = "optimized"
    })
    
    print("Backup saved:", filename)
end
```

### 2. HTTP API Client

```lua
-- API client wrapper
local API = {
    baseURL = "https://api.example.com",
    token = nil
}

function API:login(username, password)
    local response = http.post(
        self.baseURL .. "/login",
        game:GetService("HttpService"):JSONEncode({
            username = username,
            password = password
        }),
        "application/json"
    )
    
    if response and response.status == 200 then
        local data = game:GetService("HttpService"):JSONDecode(response.body)
        self.token = data.token
        return true
    end
    
    return false
end

function API:getData(endpoint)
    if not self.token then
        error("Not logged in")
    end
    
    local response = http.get(self.baseURL .. endpoint)
    if response and response.status == 200 then
        return game:GetService("HttpService"):JSONDecode(response.body)
    end
    
    return nil
end

return API
```

### 3. WebSocket Chat

```lua
-- Simple chat client
local ws = WebSocket.connect("wss://chat.example.com")

ws:on_message(function(msg)
    local data = game:GetService("HttpService"):JSONDecode(msg)
    print(data.username .. ": " .. data.message)
end)

ws:on_close(function()
    print("Disconnected from chat")
end)

-- Send messages
local function sendMessage(text)
    local data = {
        username = "Player",
        message = text
    }
    ws:send(game:GetService("HttpService"):JSONEncode(data))
end

-- Usage
sendMessage("Hello from Xoron!")
```

### 4. Drawing Dashboard

```lua
-- Create a dashboard with drawing
local draw = Drawing.new()

local function drawDashboard()
    -- Background
    draw:Square(10, 10, 300, 200, Color3.new(0, 0, 0), true)
    
    -- Title
    draw:Text("Xoron Dashboard", 160, 25, 18, Color3.new(1, 1, 1), true)
    
    -- Stats
    local fps = getfpscap()
    draw:Text("FPS Cap: " .. fps, 20, 50, 14, Color3.new(0, 1, 0), false)
    
    -- Button
    draw:Square(20, 80, 100, 30, Color3.new(0.2, 0.2, 0.2), true)
    draw:Text("Click Me", 70, 95, 12, Color3.new(1, 1, 1), true)
end

drawDashboard()
```

## Security Considerations

### 1. Validate External Scripts

```lua
-- Check script source
local function isSafeScript(script)
    -- Block dangerous patterns
    local dangerous = {
        "os.execute", "io.popen", "require('ffi')",
        "debug.sethook", "collectgarbage"
    }
    
    for _, pattern in ipairs(dangerous) do
        if script:find(pattern) then
            return false
        end
    end
    
    return true
end

local script = readfile("workspace/external.lua")
if isSafeScript(script) then
    loadstring(script)()
else
    print("Unsafe script detected!")
end
```

### 2. Secure File Operations

```lua
-- Validate file paths
local function isValidPath(path)
    -- Must be within workspace
    local workspace = getworkspace()
    if not path:find(workspace, 1, true) then
        return false
    end
    
    -- Block directory traversal
    if path:find("%.%.") then
        return false
    end
    
    return true
end

if isValidPath("workspace/data.txt") then
    local data = readfile("workspace/data.txt")
end
```

### 3. Network Security

```lua
-- Validate URLs
local function isValidURL(url)
    -- Only allow HTTPS
    if not url:find("^https://") then
        return false
    end
    
    -- Block localhost and private IPs
    if url:find("localhost") or url:find("127%.0%.0%.1") then
        return false
    end
    
    return true
end

if isValidURL("https://api.example.com") then
    local response = http.get("https://api.example.com")
end
```

## Performance Tips

### 1. Cache Expensive Operations

```lua
-- Cache HTTP responses
local cache = {}
local function getCachedHTTP(url, ttl)
    ttl = ttl or 3600
    local now = os.time()
    
    if cache[url] and (now - cache[url].time) < ttl then
        return cache[url].data
    end
    
    local response = http.get(url)
    if response then
        cache[url] = {
            data = response.body,
            time = now
        }
        return response.body
    end
    
    return nil
end
```

### 2. Batch Operations

```lua
-- Batch drawing operations
local draw = Drawing.new()
local objects = {}

-- Create objects
for i = 1, 100 do
    table.insert(objects, {
        type = "circle",
        x = math.random(100, 500),
        y = math.random(100, 500),
        radius = math.random(5, 20)
    })
end

-- Render all at once
for _, obj in ipairs(objects) do
    draw:Circle(obj.x, obj.y, obj.radius, Color3.new(1, 0, 0), false)
end
```

### 3. Use Local Variables

```lua
-- Bad: Repeated global lookups
for i = 1, 1000 do
    print(math.sqrt(i))
end

-- Good: Local reference
local sqrt = math.sqrt
for i = 1, 1000 do
    print(sqrt(i))
end
```

## Debugging Scripts

### 1. Add Logging

```lua
-- Debug logging
local DEBUG = true

function debugLog(...)
    if DEBUG then
        print("[DEBUG]", ...)
    end
end

function debugTable(tbl, name)
    if DEBUG then
        print("[DEBUG] Table:", name)
        for k, v in pairs(tbl) do
            print("  ", k, "=", v)
        end
    end
end

-- Usage
debugLog("Starting operation")
debugTable({a=1, b=2}, "myTable")
```

### 2. Error Handling

```lua
-- Safe execution
function safeExecute(func, name)
    local success, result = pcall(func)
    if not success then
        print(string.format("Error in %s: %s", name, result))
        return nil
    end
    return result
end

-- Usage
safeExecute(function()
    -- Your code here
    error("Test error")
end, "Test Function")
```

### 3. Performance Profiling

```lua
-- Simple profiler
local Profiler = {}

function Profiler:time(name, func)
    local start = os.clock()
    local result = func()
    local elapsed = os.clock() - start
    print(string.format("%s: %.2f ms", name, elapsed * 1000))
    return result
end

function Profiler:countCalls(name, func)
    self.calls = self.calls or {}
    self.calls[name] = (self.calls[name] or 0) + 1
    
    print(string.format("%s called %d times", name, self.calls[name]))
    return func()
end

-- Usage
Profiler:time("HTTP Request", function()
    return http.get("https://api.example.com")
end)
```

## Summary

This directory contains utility scripts that extend Xoron's functionality:

- **saveinstance.lua**: Roblox game serialization
- **utility.lua**: Common utilities (template)
- **network.lua**: Network helpers (template)
- **autoexecute.lua**: Auto-run scripts

Best practices:
- Use error handling
- Validate inputs
- Cache expensive operations
- Organize code into modules
- Monitor performance

---

**Next:**
- [Core API Documentation](../../docs/api/core_api.md)
- [Lua API Documentation](../../docs/api/lua_api.md)
- [Integration Guide](../../docs/platforms/cross_platform.md)
