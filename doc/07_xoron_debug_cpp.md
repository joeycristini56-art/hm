# 07. xoron_debug.cpp - Debugging & Diagnostics

**File Path**: `src/src/xoron_debug.cpp`  
**Size**: 17,311 bytes  
**Lines**: 634

**Platform**: Cross-platform

## Overview
Provides debugging utilities, error handling, stack traces, and diagnostic tools for the executor. Includes enhanced error reporting and debugging functions.

## Includes
- `xoron.h` (local)
- `cstdlib`, `cstring`, `cstdio` (system)
- `string`, `vector`, `map` (system)
- `sstream`, `iomanip` (system)
- `mutex`, `thread`, `chrono` (system)
- `execinfo.h` (backtrace - Linux/Android)
- Platform-specific: `unwind.h` (iOS), `android/log.h` (Android)
- `lua.h`, `lualib.h`, `lstate.h` (local - Luau internals)

## Core Functions

### Error Handling

#### xoron_set_error
```cpp
void xoron_set_error(const char* fmt, ...)
```
Sets the last error message (thread-safe).

**Parameters**:
- `fmt`: Format string
- `...`: Format arguments

**Thread Safety**: Protected by mutex

**Usage**:
```cpp
xoron_set_error("Failed to load module: %s", module_name);
```

#### xoron_get_last_error
```cpp
const char* xoron_get_last_error()
```
Returns the last error message.

**Returns**: Error string or NULL

### Stack Traces

#### get_stack_trace
```cpp
static std::string get_stack_trace(int max_frames = 64)
```
Captures current call stack.

**Parameters**:
- `max_frames`: Maximum frames to capture

**Returns**: Formatted stack trace string

**Platform Implementation**:
- **iOS/macOS**: Uses `backtrace()` and `backtrace_symbols()`
- **Android/Linux**: Uses `backtrace()` and `backtrace_symbols()`
- **iOS (unwind)**: Uses `_Unwind_Backtrace()` as fallback

#### xoron_get_stacktrace
```cpp
char* xoron_get_stacktrace()
```
C API for getting stack trace.

**Returns**: Allocated stack trace string (must be freed)

### Debug Logging

#### debug_print
```cpp
void debug_print(const char* level, const char* fmt, ...)
```
Logs debug message with level.

**Parameters**:
- `level`: Log level ("INFO", "WARN", "ERROR", "DEBUG")
- `fmt`: Format string
- `...`: Arguments

#### xoron_debug_log
```cpp
void xoron_debug_log(const char* fmt, ...)
```
Logs debug message.

**Parameters**:
- `fmt`: Format string
- `...`: Arguments

### Lua Debug Functions

#### debug.getinfo
```cpp
static int lua_debug_getinfo(lua_State* L)
```
Enhanced version of Lua's debug.getinfo.

**Parameters**:
- `L`: Lua state
- Function can be called with:
  - `debug.getinfo(func)` - Get info for function
  - `debug.getinfo(level)` - Get info for stack level

**Returns**: Table with debug information:
```lua
{
    source = "source",
    linedefined = 1,
    lastlinedefined = 10,
    what = "Lua",  -- "Lua", "C", "main"
    name = "func_name",
    namewhat = "global",
    currentline = 5,
    istailcall = false,
    ftransfer = 0,
    ntransfer = 0,
    short_src = "source",
    linedefined = 1,
    lastlinedefined = 10
}
```

#### debug.traceback
```cpp
static int lua_debug_traceback(lua_State* L)
```
Enhanced traceback with full details.

**Parameters**:
- `L`: Lua state
- Optional: message, level

**Returns**: Formatted traceback string

#### debug.getlocal
```cpp
static int lua_debug_getlocal(lua_State* L)
```
Gets local variables at stack level.

**Parameters**:
- `L`: Lua state
- `level`: Stack level
- `index`: Local variable index

**Returns**: Local variable name and value

#### debug.setlocal
```cpp
static int lua_debug_setlocal(lua_State* L)
```
Sets local variable value.

**Parameters**:
- `L`: Lua state
- `level`: Stack level
- `index`: Local variable index
- `value`: New value

**Returns**: Variable name or nil

#### debug.getupvalue
```cpp
static int lua_debug_getupvalue(lua_State* L)
```
Gets upvalue of function.

**Parameters**:
- `L`: Lua state
- `func`: Function
- `index`: Upvalue index

**Returns**: Upvalue name and value

#### debug.setupvalue
```cpp
static int lua_debug_setupvalue(lua_State* L)
```
Sets upvalue of function.

**Parameters**:
- `L`: Lua state
- `func`: Function
- `index`: Upvalue index
- `value`: New value

**Returns**: Upvalue name or nil

#### debug.getregistry
```cpp
static int lua_debug_getregistry(lua_State* L)
```
Returns the registry table.

**Returns**: Registry table

#### debug.getmetatable
```cpp
static int lua_debug_getmetatable(lua_State* L)
```
Gets metatable of value.

**Parameters**:
- `L`: Lua state
- `value`: Value

**Returns**: Metatable or nil

#### debug.setmetatable
```cpp
static int lua_debug_setmetatable(lua_State* L)
```
Sets metatable of value.

**Parameters**:
- `L`: Lua state
- `value`: Value
- `metatable`: Metatable

**Returns**: true on success

#### debug.gethook
```cpp
static int lua_debug_gethook(lua_State* L)
```
Gets current debug hook.

**Returns**: Hook function, mask, count

#### debug.sethook
```cpp
static int lua_debug_sethook(lua_State* L)
```
Sets debug hook.

**Parameters**:
- `L`: Lua state
- `func`: Hook function (or nil)
- `mask`: Event mask ("c", "r", "l", "return")
- `count`: Line count

**Returns**: nil

### Error Reporting

#### error_with_traceback
```cpp
static int error_with_traceback(lua_State* L)
```
Throws error with full traceback.

**Parameters**:
- `L`: Lua state
- `message`: Error message

**Returns**: Never returns (throws error)

### Diagnostic Functions

#### get_memory_info
```cpp
static std::string get_memory_info()
```
Gets memory usage information.

**Returns**: Formatted memory stats

#### get_system_info
```cpp
static std::string get_system_info()
```
Gets system information.

**Returns**: Formatted system info

#### get_executor_info
```cpp
static std::string get_executor_info()
```
Gets executor configuration.

**Returns**: Formatted executor info

## Lua API

All functions are registered in the `debug` global table.

### Error Handling

#### debug.getlasterror
```lua
debug.getlasterror() -> string
```
Returns last error message.

**Returns**: Error string

#### debug.seterror
```lua
debug.seterror(message: string) -> void
```
Sets error message.

**Parameters**:
- `message`: Error message

### Stack Traces

#### debug.getstacktrace
```lua
debug.getstacktrace() -> string
```
Gets current stack trace.

**Returns**: Formatted stack trace

**Example**:
```lua
local trace = debug.getstacktrace()
print(trace)
```

#### debug.getbacktrace
```lua
debug.getbacktrace() -> table
```
Gets stack trace as table.

**Returns**: Array of stack frames

**Example**:
```lua
local frames = debug.getbacktrace()
for i, frame in ipairs(frames) do
    print(string.format("%d: %s", i, frame))
end
```

### Enhanced Debug Functions

#### debug.getinfo
```lua
debug.getinfo(func_or_level: any, what: string?) -> table
```
Enhanced debug info.

**Parameters**:
- `func_or_level`: Function object or stack level
- `what`: Info fields (default: "Slnf")

**Returns**: Info table

**Example**:
```lua
local info = debug.getinfo(my_function)
print(info.source, info.linedefined, info.lastlinedefined)
```

#### debug.traceback
```lua
debug.traceback(message: string?, level: number?) -> string
```
Enhanced traceback.

**Parameters**:
- `message`: Optional message
- `level`: Starting level (default: 1)

**Returns**: Traceback string

**Example**:
```lua
local trace = debug.traceback("Error occurred", 2)
print(trace)
```

#### debug.getlocal
```lua
debug.getlocal(level: number, index: number) -> name: string, value: any
```
Gets local variable.

**Parameters**:
- `level`: Stack level
- `index`: Local index

**Returns**: Variable name and value

#### debug.setlocal
```lua
debug.setlocal(level: number, index: number, value: any) -> string?
```
Sets local variable.

**Parameters**:
- `level`: Stack level
- `index`: Local index
- `value`: New value

**Returns**: Variable name or nil

#### debug.getupvalue
```lua
debug.getupvalue(func: function, index: number) -> name: string, value: any
```
Gets upvalue.

**Parameters**:
- `func`: Function
- `index`: Upvalue index

**Returns**: Upvalue name and value

#### debug.setupvalue
```lua
debug.setupvalue(func: function, index: number, value: any) -> string?
```
Sets upvalue.

**Parameters**:
- `func`: Function
- `index`: Upvalue index
- `value`: New value

**Returns**: Upvalue name or nil

#### debug.getregistry
```lua
debug.getregistry() -> table
```
Returns registry table.

**Returns**: Registry

#### debug.getmetatable
```lua
debug.getmetatable(value: any) -> table?
```
Gets metatable.

**Parameters**:
- `value`: Value

**Returns**: Metatable or nil

#### debug.setmetatable
```lua
debug.setmetatable(value: any, metatable: table?) -> boolean
```
Sets metatable.

**Parameters**:
- `value`: Value
- `metatable`: Metatable

**Returns**: true on success

#### debug.gethook
```lua
debug.gethook() -> func: function?, mask: string?, count: number?
```
Gets debug hook.

**Returns**: Hook function, mask, count

#### debug.sethook
```lua
debug.sethook(func: function?, mask: string?, count: number?) -> void
```
Sets debug hook.

**Parameters**:
- `func`: Hook function
- `mask`: Event mask ("c", "r", "l", "return")
- `count`: Line count

**Hook Events**:
- `"c"`: Call events
- `"r"`: Return events
- `"l"`: Line events
- `"return"`: Return events

**Example**:
```lua
local function hook(event, line)
    print("Event:", event, "Line:", line)
end
debug.sethook(hook, "l", 1)  -- Hook every line
```

### Diagnostic Functions

#### debug.getmemoryinfo
```lua
debug.getmemoryinfo() -> string
```
Gets memory usage information.

**Returns**: Formatted memory stats

#### debug.getsysteminfo
```lua
debug.getsysteminfo() -> string
```
Gets system information.

**Returns**: Formatted system info

#### debug.getexecutorinfo
```lua
debug.getexecutorinfo() -> string
```
Gets executor configuration.

**Returns**: Formatted executor info

#### debug.print
```lua
debug.print(...) -> void
```
Prints debug message.

**Parameters**:
- `...`: Values to print

#### debug.warn
```lua
debug.warn(...) -> void
```
Prints warning message.

**Parameters**:
- `...`: Values to print

#### debug.error
```lua
debug.error(...) -> void
```
Prints error message.

**Parameters**:
- `...`: Values to print

### Advanced Debugging

#### debug.getthread
```lua
debug.getthread(index: number) -> thread?
```
Gets thread from registry.

**Parameters**:
- `index`: Thread index

**Returns**: Thread or nil

#### debug.getallthreads
```lua
debug.getallthreads() -> table
```
Gets all active threads.

**Returns**: Array of threads

#### debug.getallfunctions
```lua
debug.getallfunctions() -> table
```
Gets all functions in registry.

**Returns**: Array of functions

#### debug.getalltables
```lua
debug.getalltables() -> table
```
Gets all tables in registry.

**Returns**: Array of tables

### Error Handling Utilities

#### debug.pcall
```lua
debug.pcall(func: function, ...) -> success: boolean, result: any
```
Protected call with enhanced error handling.

**Parameters**:
- `func`: Function to call
- `...`: Arguments

**Returns**: Success flag and result/error

**Example**:
```lua
local ok, result = debug.pcall(function()
    error("test error")
end)
if not ok then
    print("Error:", result)
end
```

#### debug.xpcall
```lua
debug.xpcall(func: function, error_handler: function, ...) -> success: boolean, result: any
```
Protected call with custom error handler.

**Parameters**:
- `func`: Function to call
- `error_handler`: Error handler function
- `...`: Arguments

**Returns**: Success flag and result

#### debug.assert
```lua
debug.assert(condition: any, message: string?) -> void
```
Assertion with enhanced error message.

**Parameters**:
- `condition`: Condition to check
- `message`: Error message

### Logging Levels

#### debug.log
```lua
debug.log(level: string, ...) -> void
```
Logs with level.

**Parameters**:
- `level`: "INFO", "WARN", "ERROR", "DEBUG"
- `...`: Values to log

#### debug.info
```lua
debug.info(...) -> void
```
Info level logging.

#### debug.warn
```lua
debug.warn(...) -> void
```
Warning level logging.

#### debug.error
```lua
debug.error(...) -> void
```
Error level logging.

#### debug.debug
```lua
debug.debug(...) -> void
```
Debug level logging.

### Stack Manipulation

#### debug.getstack
```lua
debug.getstack(level: number?) -> table
```
Gets stack information.

**Parameters**:
- `level`: Stack level (default: 1)

**Returns**: Stack frame info

#### debug.setstack
```lua
debug.setstack(level: number, var: string, value: any) -> void
```
Sets stack variable.

**Parameters**:
- `level`: Stack level
- `var`: Variable name
- `value`: New value

### Call Stack

#### debug.getcallstack
```lua
debug.getcallstack() -> table
```
Gets complete call stack.

**Returns**: Array of stack frames

**Each frame contains**:
```lua
{
    func = function,
    line = number,
    source = string,
    what = string
}
```

#### debug.getcurrentline
```lua
debug.getcurrentline() -> number
```
Gets current line number.

**Returns**: Line number

#### debug.getcurrentsource
```lua
debug.getcurrentsource() -> string
```
Gets current source file.

**Returns**: Source file path

### Variable Inspection

#### debug.inspect
```lua
debug.inspect(value: any, depth: number?) -> string
```
Inspects value with full details.

**Parameters**:
- `value`: Value to inspect
- `depth`: Max depth (default: 3)

**Returns**: Formatted inspection string

**Example**:
```lua
local tbl = {a = 1, b = {c = 2}}
print(debug.inspect(tbl))
```

#### debug.getvariables
```lua
debug.getvariables(level: number?) -> table
```
Gets all variables at stack level.

**Parameters**:
- `level`: Stack level (default: 1)

**Returns**: Table of variables

### Performance

#### debug.getprofiler
```lua
debug.getprofiler() -> table
```
Gets profiler functions.

**Returns**: Profiler API

**Returns table contains**:
```lua
{
    start = function(),
    stop = function(),
    getstats = function() -> table
}
```

#### debug.profile
```lua
debug.profile(func: function, iterations: number?) -> table
```
Profiles function execution.

**Parameters**:
- `func`: Function to profile
- `iterations`: Number of runs (default: 1000)

**Returns**: Statistics table

### Breakpoints

#### debug.setbreakpoint
```lua
debug.setbreakpoint(func: function, line: number) -> breakpoint_id
```
Sets breakpoint.

**Parameters**:
- `func`: Function
- `line`: Line number

**Returns**: Breakpoint ID

#### debug.removebreakpoint
```lua
debug.removebreakpoint(id: breakpoint_id) -> void
```
Removes breakpoint.

**Parameters**:
- `id`: Breakpoint ID

#### debug.getbreakpoints
```lua
debug.getbreakpoints() -> table
```
Gets all breakpoints.

**Returns**: Array of breakpoints

### Watchpoints

#### debug.setwatchpoint
```lua
debug.setwatchpoint(table: table, key: any) -> watchpoint_id
```
Sets watchpoint on table key.

**Parameters**:
- `table`: Table to watch
- `key`: Key to watch

**Returns**: Watchpoint ID

#### debug.removewatchpoint
```lua
debug.removewatchpoint(id: watchpoint_id) -> void
```
Removes watchpoint.

**Parameters**:
- `id`: Watchpoint ID

### Call Tracing

#### debug.tracecalls
```lua
debug.tracecalls(func: function, enable: boolean?) -> void
```
Enables call tracing for function.

**Parameters**:
- `func`: Function to trace
- `enable`: true to enable (default: true)

#### debug.getcalltrace
```lua
debug.getcalltrace() -> table
```
Gets call trace.

**Returns**: Array of calls

### Garbage Collection

#### debug.getgc
```lua
debug.getgc() -> table
```
Gets garbage collector objects.

**Returns**: Array of GC objects

#### debug.collectgarbage
```lua
debug.collectgarbage(opt: string?, arg: any?) -> any
```
Controls garbage collector.

**Parameters**:
- `opt`: "collect", "stop", "restart", "count", "step", "setpause", "setstepmul"
- `arg`: Additional argument

**Returns**: Depends on opt

### Registry Inspection

#### debug.getregistrykeys
```lua
debug.getregistrykeys() -> table
```
Gets all registry keys.

**Returns**: Array of keys

#### debug.getregistryvalues
```lua
debug.getregistryvalues() -> table
```
Gets all registry values.

**Returns**: Array of values

### Thread Debugging

#### debug.getthreadinfo
```lua
debug.getthreadinfo(thread: thread) -> table
```
Gets thread information.

**Parameters**:
- `thread`: Thread object

**Returns**: Thread info table

#### debug.resumethread
```lua
debug.resumethread(thread: thread, ...) -> any
```
Resumes thread with debugging.

**Parameters**:
- `thread`: Thread to resume
- `...`: Values to pass

**Returns**: Thread result

### Error Traceback

#### debug.errorhandler
```lua
debug.errorhandler(err: any) -> any
```
Default error handler.

**Parameters**:
- `err`: Error value

**Returns**: Formatted error

#### debug.seterrorhandler
```lua
debug.seterrorhandler(handler: function) -> void
```
Sets custom error handler.

**Parameters**:
- `handler`: Error handler function

### Diagnostic Utilities

#### debug.isdebug
```lua
debug.isdebug() -> boolean
```
Checks if in debug mode.

**Returns**: true if debug mode

#### debug.setdebug
```lua
debug.setdebug(enable: boolean) -> void
```
Enables/disables debug mode.

**Parameters**:
- `enable`: Debug mode

#### debug.getversion
```lua
debug.getversion() -> string
```
Gets debug library version.

**Returns**: Version string

## C API

### xoron_debug_init
```c
void xoron_debug_init(lua_State* L)
```
Initializes debug library.

**Parameters**:
- `L`: Lua state

### xoron_set_error
```c
void xoron_set_error(const char* fmt, ...)
```
Sets error message.

### xoron_get_last_error
```c
const char* xoron_get_last_error()
```
Gets last error.

### xoron_get_stacktrace
```c
char* xoron_get_stacktrace()
```
Gets stack trace.

**Returns**: Allocated string (must be freed)

### xoron_debug_log
```c
void xoron_debug_log(const char* fmt, ...)
```
Logs debug message.

## Implementation Details

### Error Storage
- Thread-safe error storage using mutex
- Last error per thread or global
- Formatted with printf-style

### Stack Trace Capture
- Uses platform-specific backtrace APIs
- Symbol resolution for function names
- Formatted for readability

### Debug Hooks
- Intercepts Lua execution events
- Allows breakpoints and watchpoints
- Minimal performance impact when disabled

### Registry Access
- Safe access to Lua registry
- Filters internal keys
- Provides type information

## Usage Examples

### Basic Error Handling
```lua
-- Set error
debug.seterror("Something went wrong")

-- Get error
local err = debug.getlasterror()
print(err)

-- Enhanced pcall
local ok, result = debug.pcall(function()
    error("test error")
end)
if not ok then
    print(debug.traceback(result))
end
```

### Stack Inspection
```lua
-- Get stack trace
local trace = debug.getstacktrace()
print(trace)

-- Get current line
local line = debug.getcurrentline()
print("Current line:", line)

-- Get all variables at level 1
local vars = debug.getvariables(1)
for k, v in pairs(vars) do
    print(k, "=", v)
end
```

### Function Debugging
```lua
-- Get function info
local info = debug.getinfo(my_function)
print("Source:", info.source)
print("Lines:", info.linedefined, "-", info.lastlinedefined)

-- Get upvalues
for i = 1, math.huge do
    local name, value = debug.getupvalue(my_function, i)
    if not name then break end
    print("Upvalue", i, ":", name, "=", value)
end
```

### Variable Manipulation
```lua
function test()
    local x = 10
    local y = 20
    
    -- Get locals
    local name, value = debug.getlocal(1, 1)
    print(name, "=", value)  -- "x = 10"
    
    -- Modify locals
    debug.setlocal(1, 1, 100)
    print(x)  -- 100
end

test()
```

### Debug Hooks
```lua
-- Line-by-line tracing
local function trace(event, line)
    local info = debug.getinfo(2, "S")
    print(string.format("[%s:%d] %s", info.short_src, line, event))
end

debug.sethook(trace, "l")

-- Your code here...

debug.sethook(nil)  -- Disable
```

### Breakpoints
```lua
-- Set breakpoint
local bp_id = debug.setbreakpoint(my_function, 5)

-- When execution hits line 5 in my_function, it will pause
-- (In a real debugger, you'd have a breakpoint handler)

-- Remove breakpoint
debug.removebreakpoint(bp_id)
```

### Call Tracing
```lua
-- Enable tracing for specific function
debug.tracecalls(my_function)

-- Call the function
my_function()

-- Get trace
local trace = debug.getcalltrace()
for _, call in ipairs(trace) do
    print(call.func, "called at", call.time)
end
```

### Memory Inspection
```lua
-- Get memory info
local mem = debug.getmemoryinfo()
print(mem)

-- Get system info
local sys = debug.getsysteminfo()
print(sys)

-- Get executor info
local exe = debug.getexecutorinfo()
print(exe)
```

### Garbage Collection
```lua
-- Get all GC objects
local objects = debug.getgc()

-- Count by type
local counts = {}
for _, obj in ipairs(objects) do
    local t = type(obj)
    counts[t] = (counts[t] or 0) + 1
end

for t, count in pairs(counts) do
    print(t, ":", count)
end
```

### Advanced Inspection
```lua
-- Deep inspect value
local complex_table = {
    nested = {
        data = {1, 2, 3},
        func = function() end
    }
}

print(debug.inspect(complex_table, 5))

-- Get all registry keys
local keys = debug.getregistrykeys()
for _, key in ipairs(keys) do
    print(key)
end
```

### Thread Debugging
```lua
-- Get all threads
local threads = debug.getallthreads()

for i, thread in ipairs(threads) do
    local info = debug.getthreadinfo(thread)
    print(string.format("Thread %d: %s", i, info.status))
end
```

### Performance Profiling
```lua
-- Profile function
local stats = debug.profile(function()
    -- Code to profile
    for i = 1, 1000 do
        math.sqrt(i)
    end
end, 100)

print("Average time:", stats.avg_time, "ms")
print("Total time:", stats.total_time, "ms")
print("Calls:", stats.calls)
```

### Error Handling with Traceback
```lua
-- Custom error handler
function my_error_handler(err)
    local trace = debug.traceback(err, 2)
    print("ERROR:", trace)
    return err
end

debug.seterrorhandler(my_error_handler)

-- Now errors will use custom handler
error("Something went wrong")
```

### Watchpoints
```lua
local my_table = {value = 42}

-- Set watchpoint
local wp_id = debug.setwatchpoint(my_table, "value")

-- When my_table.value is accessed or modified, debugger is notified

-- Remove watchpoint
debug.removewatchpoint(wp_id)
```

## Platform-Specific Features

### iOS
- Uses `unwind` for stack traces
- Mach-O symbol resolution
- Integration with Xcode debugger

### Android
- Uses `backtrace` from bionic
- Logcat integration
- Android Studio compatibility

### Linux
- Full backtrace support
- DWARF symbol resolution
- GDB compatibility

## Performance Considerations

- Stack traces: Fast (microseconds)
- Debug hooks: Minimal overhead when disabled
- Registry inspection: O(n) where n = registry size
- Memory inspection: Fast
- GC traversal: O(n) where n = objects

## Error Handling

All debug functions are designed to be safe:
- Invalid stack levels return nil
- Invalid functions return nil
- Protected calls catch internal errors
- Error messages include context

## Related Files
- `xoron.h` (API declarations)
- `xoron_luau.cpp` (VM integration)
- `xoron_env.cpp` (environment functions)
