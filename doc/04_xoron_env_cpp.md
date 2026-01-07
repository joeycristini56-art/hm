# 04. xoron_env.cpp - Environment & Global Functions

**File Path**: `src/src/xoron_env.cpp`  
**Size**: 53,091 bytes  
**Lines**: ~800

**Platform**: Cross-platform (with platform-specific sections)

## Overview
Provides custom Lua environment functions for the executor, including global environment access, executor identification, and environment manipulation utilities.

## Includes
- `xoron.h` (local)
- `cstdlib`, `cstring`, `cstdio` (system)
- `string`, `vector`, `unordered_map`, `unordered_set` (system)
- `mutex`, `thread`, `chrono` (system)
- `atomic`, `functional` (system)
- `lz4.h` (local - compression)
- Platform-specific: `objc/runtime.h`, `CoreFoundation.h` (iOS) or `android/log.h`, `jni.h` (Android)
- `lua.h`, `lualib.h` (local - Luau)

## Core Environment Functions

### Global Environment Access

#### getgenv
```lua
getgenv() -> table
```
Returns the global environment table.

**Returns**: The global Lua environment

**Usage**:
```lua
local _G = getgenv()
_G.myGlobal = "value"
```

#### getrenv
```lua
getrenv() -> table
```
Returns the Roblox environment (if injected into Roblox).

**Returns**: Roblox's global environment or nil

#### getmenv
```lua
getmenv() -> table
```
Returns the metatable environment.

**Returns**: Metatable environment

#### getsenv
```lua
getsenv(script: Instance) -> table
```
Returns the environment of a specific script.

**Parameters**:
- `script`: Roblox script instance

**Returns**: Script's environment

### Executor Identification

#### identifyexecutor
```lua
identifyexecutor() -> string
```
Returns executor name and version.

**Returns**: "Xoron 2.0.0"

#### getexecutorname
```lua
getexecutorname() -> string
```
Alias for `identifyexecutor()`.

### Environment Information

#### getloadedmodules
```lua
getloadedmodules() -> table
```
Returns list of loaded modules.

**Returns**: Array of loaded module instances

#### getconnections
```lua
getconnections(event: any) -> table
```
Returns connections for an event.

**Parameters**:
- `event`: Roblox event object

**Returns**: Array of connections

#### getgc
```lua
getgc(include_tables: boolean?) -> table
```
Returns garbage collector objects.

**Parameters**:
- `include_tables`: Include tables (default: false)

**Returns**: Array of GC objects

#### getreg
```lua
getreg() -> table
```
Returns registry table.

**Returns**: Lua registry

### Memory Management

#### getmemoryusage
```lua
getmemoryusage() -> number
```
Returns current memory usage in bytes.

**Returns**: Memory usage

#### getthreadcount
```lua
getthreadcount() -> number
```
Returns number of active threads.

**Returns**: Thread count

### Script Manipulation

#### getscript
```lua
getscript(name: string) -> Instance?
```
Finds a script by name.

**Parameters**:
- `name`: Script name

**Returns**: Script instance or nil

#### getscripts
```lua
getscripts() -> table
```
Returns all scripts.

**Returns**: Array of script instances

### Hooking & Metamethods

#### hookfunction
```lua
hookfunction(func: function, newfunc: function) -> function
```
Hooks a function.

**Parameters**:
- `func`: Function to hook
- `newfunc`: Replacement function

**Returns**: Original function

**Usage**:
```lua
local original = hookfunction(print, function(...)
    print("Hooked:", ...)
    return original(...)
end)
```

#### replaceclosure
```lua
replaceclosure(func: function, newfunc: function) -> function
```
Alias for `hookfunction()`.

#### hookmetamethod
```lua
hookmetamethod(object: any, method: string, newfunc: function) -> function
```
Hooks a metamethod.

**Parameters**:
- `object`: Object with metatable
- `method`: Metamethod name (e.g., "__index")
- `newfunc`: Replacement function

**Returns**: Original metamethod

### Debug & Utility

#### checkcaller
```lua
checkcaller() -> boolean
```
Checks if current execution is from executor.

**Returns**: true if from executor

#### isexecutor
```lua
isexecutor() -> boolean
```
Checks if running in executor.

**Returns**: true

#### gettenv
```lua
gettenv(thread: thread) -> table
```
Returns thread environment.

**Parameters**:
- `thread`: Thread object

**Returns**: Thread's environment

#### getstack
```lua
getstack(level: number?) -> table
```
Returns call stack information.

**Parameters**:
- `level`: Stack level (default: 1)

**Returns**: Stack info

### File Operations

#### saveinstance
```lua
saveinstance(options: table?) -> string
```
Saves game instance to RBXMX format.

**Parameters**:
- `options`: Configuration table

**Returns**: RBXMX XML string

**Options**:
```lua
{
    FileName = "game",
    DecompileScripts = false,
    RemovePlayerCharacters = true,
    Mode = "optimized"  -- "optimized", "full", "scripts"
}
```

#### loadstring
```lua
loadstring(source: string, chunkname: string?) -> function
```
Compiles and loads Lua string.

**Parameters**:
- `source`: Lua source code
- `chunkname`: Chunk name for errors

**Returns**: Compiled function

#### loadfile
```lua
loadfile(path: string) -> function
```
Loads and compiles Lua file.

**Parameters**:
- `path`: File path

**Returns**: Compiled function

#### dofile
```lua
dofile(path: string) -> any
```
Loads, compiles, and executes Lua file.

**Parameters**:
- `path`: File path

**Returns**: Last value from file

### Notification & UI

#### notify
```lua
notify(title: string, message: string, duration: number?) -> void
```
Shows notification.

**Parameters**:
- `title`: Notification title
- `message`: Notification message
- `duration`: Duration in seconds (default: 3)

#### setclipboard
```lua
setclipboard(text: string) -> void
```
Sets system clipboard.

**Parameters**:
- `text`: Text to copy

#### getclipboard
```lua
getclipboard() -> string
```
Gets system clipboard.

**Returns**: Clipboard content

### Platform-Specific

#### isandroid
```lua
isandroid() -> boolean
```
Checks if running on Android.

**Returns**: true on Android

#### isios
```lua
isios() -> boolean
```
Checks if running on iOS.

**Returns**: true on iOS

#### ismobile
```lua
ismobile() -> boolean
```
Checks if running on mobile.

**Returns**: true on iOS or Android

### Security & Anti-Detection

#### isdebugger
```lua
isdebugger() -> boolean
```
Checks if debugger is attached.

**Returns**: true if debugger detected

#### isvm
```lua
isvm() -> boolean
```
Checks if running in virtual machine.

**Returns**: true if VM detected

#### securecall
```lua
securecall(func: function, ...) -> any
```
Calls function in secure environment.

**Parameters**:
- `func`: Function to call
- `...`: Arguments

**Returns**: Function result

### Advanced Features

#### getproperties
```lua
getproperties(instance: Instance) -> table
```
Returns all properties of instance.

**Parameters**:
- `instance`: Roblox instance

**Returns**: Property table

#### setproperty
```lua
setproperty(instance: Instance, property: string, value: any) -> void
```
Sets property on instance.

**Parameters**:
- `instance`: Roblox instance
- `property`: Property name
- `value`: Property value

#### firetouchinterest
```lua
firetouchinterest(part: Instance, target: Instance, state: number) -> void
```
Fires touch interest.

**Parameters**:
- `part`: Part to touch
- `target`: Target part
- `state`: 0 (begin) or 1 (end)

#### fireclickdetector
```lua
fireclickdetector(detector: Instance, distance: number?) -> void
```
Fires click detector.

**Parameters**:
- `detector`: ClickDetector instance
- `distance`: Distance (default: 0)

#### fireproximityprompt
```lua
fireproximityprompt(prompt: Instance) -> void
```
Fires proximity prompt.

**Parameters**:
- `prompt`: ProximityPrompt instance

### Compression (LZ4)

#### lz4_compress
```lua
lz4_compress(data: string) -> string
```
Compresses data using LZ4.

**Parameters**:
- `data`: Data to compress

**Returns**: Compressed data

#### lz4_decompress
```lua
lz4_decompress(data: string, original_size: number) -> string
```
Decompresses LZ4 data.

**Parameters**:
- `data`: Compressed data
- `original_size`: Original size

**Returns**: Decompressed data

### Thread Management

#### coroutine.wrap
```lua
coroutine.wrap(func: function) -> function
```
Wraps function in coroutine (overridden).

#### coroutine.create
```lua
coroutine.create(func: function) -> thread
```
Creates coroutine (overridden).

#### spawn
```lua
spawn(func: function) -> void
```
Spawns function in new thread.

**Parameters**:
- `func`: Function to execute

#### delay
```lua
delay(seconds: number, func: function) -> void
```
Delays function execution.

**Parameters**:
- `seconds`: Delay time
- `func`: Function to execute

### Math & Utility

#### random
```lua
random(min: number?, max: number?) -> number
```
Generates random number (overridden for better entropy).

**Parameters**:
- `min`: Minimum value (default: 0)
- `max`: Maximum value (default: 1)

**Returns**: Random number

#### clamp
```lua
clamp(value: number, min: number, max: number) -> number
```
Clamps value between min and max.

#### lerp
```lua
lerp(a: number, b: number, t: number) -> number
```
Linear interpolation.

### String Utilities

#### split
```lua
split(str: string, separator: string) -> table
```
Splits string by separator.

**Parameters**:
- `str`: String to split
- `separator`: Separator

**Returns**: Array of strings

#### trim
```lua
trim(str: string) -> string
```
Trims whitespace from string.

**Parameters**:
- `str`: String to trim

**Returns**: Trimmed string

#### starts_with
```lua
starts_with(str: string, prefix: string) -> boolean
```
Checks if string starts with prefix.

#### ends_with
```lua
ends_with(str: string, suffix: string) -> boolean
```
Checks if string ends with suffix.

### Table Utilities

#### table.find
```lua
table.find(tbl: table, value: any) -> number?
```
Finds value in table.

**Parameters**:
- `tbl`: Table to search
- `value`: Value to find

**Returns**: Index or nil

#### table.clonedeep
```lua
table.clonedeep(tbl: table) -> table
```
Deep clones a table.

**Parameters**:
- `tbl`: Table to clone

**Returns**: Cloned table

#### table.merge
```lua
table.merge(dest: table, src: table) -> table
```
Merges tables.

**Parameters**:
- `dest`: Destination table
- `src`: Source table

**Returns**: Merged table

### Instance Utilities

#### instance.new
```lua
instance.new(class: string, parent: Instance?) -> Instance
```
Creates new instance (overridden).

**Parameters**:
- `class`: Class name
- `parent`: Parent instance

**Returns**: New instance

#### findfirstchild
```lua
findfirstchild(instance: Instance, name: string) -> Instance?
```
Finds first child by name.

**Parameters**:
- `instance`: Parent instance
- `name`: Child name

**Returns**: Child or nil

#### findfirstclass
```lua
findfirstclass(instance: Instance, class: string) -> Instance?
```
Finds first child of class.

**Parameters**:
- `instance`: Parent instance
- `class`: Class name

**Returns**: Child or nil

### Event Utilities

#### connect
```lua
connect(event: any, func: function) -> RBXScriptConnection
```
Connects to event (overridden).

**Parameters**:
- `event`: Event object
- `func`: Callback function

**Returns**: Connection object

#### wait
```lua
wait(seconds: number?) -> number
```
Waits for specified time (overridden).

**Parameters**:
- `seconds`: Time to wait

**Returns**: Actual time waited

### Error Handling

#### error
```lua
error(message: string, level: number?) -> void
```
Throws error (overridden with better stack traces).

**Parameters**:
- `message`: Error message
- `level`: Stack level

#### pcall
```lua
protected_call(func: function, ...) -> boolean, any
```
Protected call (overridden for better debugging).

**Parameters**:
- `func`: Function to call
- `...`: Arguments

**Returns**: Success, result

#### xpcall
```lua
xpcall(func: function, error_handler: function, ...) -> boolean, any
```
Protected call with error handler.

### Debug Hooks

#### debug.sethook
```lua
debug.sethook(func: function?, mask: string?, count: number?) -> void
```
Sets debug hook (overridden).

**Parameters**:
- `func`: Hook function
- `mask`: Event mask ("c", "r", "l", "return")
- `count`: Line count

#### debug.getinfo
```lua
debug.getinfo(func: function, what: string?) -> table
```
Gets debug info (enhanced).

**Parameters**:
- `func`: Function
- `what`: Info fields

**Returns**: Debug info table

### File System Integration

#### listfiles
```lua
listfiles(path: string?) -> table
```
Lists files in directory.

**Parameters**:
- `path`: Directory path

**Returns**: Array of file names

#### isfile
```lua
isfile(path: string) -> boolean
```
Checks if path is a file.

#### isfolder
```lua
isfolder(path: string) -> boolean
```
Checks if path is a folder.

#### makefolder
```lua
makefolder(path: string) -> void
```
Creates folder.

#### delfolder
```lua
delfolder(path: string) -> void
```
Deletes folder.

#### delfile
```lua
delfile(path: string) -> void
```
Deletes file.

#### readfile
```lua
readfile(path: string) -> string
```
Reads file content.

#### writefile
```lua
writefile(path: string, content: string) -> void
```
Writes file content.

#### appendfile
```lua
appendfile(path: string, content: string) -> void
```
Appends to file.

### HTTP Integration

#### http_request
```lua
http_request(options: table) -> table
```
Makes HTTP request (enhanced).

**Parameters**:
- `options`: Request options table

**Returns**: Response table

#### httpget
```lua
httpget(url: string) -> string
```
HTTP GET shortcut.

#### httppost
```lua
httppost(url: string, data: string) -> string
```
HTTP POST shortcut.

### WebSocket

#### websocket_connect
```lua
websocket_connect(url: string) -> WebSocket
```
Connects to WebSocket.

**Parameters**:
- `url`: WebSocket URL

**Returns**: WebSocket object

#### websocket_close
```lua
websocket_close(ws: WebSocket) -> void
```
Closes WebSocket.

### Drawing

#### drawing_new
```lua
drawing_new(type: string) -> Drawing
```
Creates drawing object.

**Parameters**:
- `type`: Drawing type ("Line", "Square", "Circle", "Text", "Quad", "Triangle")

**Returns**: Drawing object

### Input

#### iskeydown
```lua
iskeydown(key: number) -> boolean
```
Checks if key is down.

**Parameters**:
- `key`: Virtual key code

**Returns**: true if down

#### ismousebuttonpressed
```lua
ismousebuttonpressed(button: number) -> boolean
```
Checks if mouse button is pressed.

**Parameters**:
- `button`: 0=left, 1=right, 2=middle

**Returns**: true if pressed

#### mousemoverel
```lua
mousemoverel(x: number, y: number) -> void
```
Moves mouse relative to current position.

#### mousemoveabs
```lua
mousemoveabs(x: number, y: number) -> void
```
Moves mouse to absolute position.

#### keypress
```lua
keypress(key: number) -> void
```
Presses key.

#### keyrelease
```lua
keyrelease(key: number) -> void
```
Releases key.

### Cache

#### cache
```lua
cache = {
    get = function(key: string) -> any,
    set = function(key: string, value: any) -> void,
    clear = function() -> void
}
```
Global cache system.

### UI

#### ui
```lua
ui = {
    create = function(options: table) -> UIElement,
    notify = function(title: string, message: string) -> void,
    toggle = function() -> void
}
```
UI management system.

## Implementation Structure

The file is organized into:

1. **Global State Management**: Thread-safe state for environment data
2. **Environment Functions**: Core get* functions
3. **Hooking System**: Function and metamethod hooking
4. **Script Utilities**: Loading, saving, manipulation
5. **Platform Abstraction**: iOS/Android specific implementations
6. **Lua Library Registration**: Registering all functions with Lua
7. **Utility Functions**: Helper functions for various operations

## Error Handling

All functions use `xoron_set_error()` for error reporting and return appropriate values on failure.

## Performance

- Thread-safe operations using mutex locks
- Efficient hash maps for environment storage
- Minimal overhead for hot paths (getgenv, checkcaller)

## Related Files
- `xoron.h` (API declarations)
- `xoron_luau.cpp` (VM integration)
- `xoron_filesystem.cpp` (file operations)
- `xoron_http.cpp` (HTTP functions)
- `xoron_drawing.cpp` (graphics)
- `xoron_input.cpp` (input handling)
- `xoron_cache.cpp` (caching)
- `xoron_ui.cpp` (UI components)
- `xoron_android.cpp` (Android-specific)
- `xoron_ios.mm` (iOS-specific)
