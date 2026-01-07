# 08. xoron_console.cpp - Console Output Management

**File Path**: `src/src/xoron_console.cpp`  
**Size**: 11,914 bytes  
**Lines**: 413

**Platform**: Cross-platform (with platform-specific output)

## Overview
Manages console output for the executor. Provides print, warn, error, and custom output functions with support for multiple output channels and formatting.

## Includes
- `xoron.h` (local)
- `cstdlib`, `cstring`, `cstdio` (system)
- `string`, `vector` (system)
- `sstream`, `iomanip` (system)
- `mutex`, `thread` (system)
- Platform-specific: `android/log.h` (Android), `objc/runtime.h` (iOS)
- `lua.h`, `lualib.h` (local - Luau)

## Core Architecture

### Output Callback System
```cpp
struct ConsoleState {
    xoron_output_fn print_fn = nullptr;
    xoron_output_fn warn_fn = nullptr;
    xoron_output_fn error_fn = nullptr;
    void* user_data = nullptr;
    std::mutex mutex;
    bool enabled = true;
    std::vector<std::string> history;
    size_t max_history = 1000;
};
```

### Output Levels
- **PRINT**: Standard output
- **WARN**: Warning messages
- **ERROR**: Error messages
- **DEBUG**: Debug information
- **INFO**: Informational messages

## C API Functions

### xoron_set_console_callbacks
```c
void xoron_set_console_callbacks(xoron_output_fn print_fn, 
                                  xoron_output_fn warn_fn, 
                                  xoron_output_fn error_fn, 
                                  void* user_data)
```
Sets custom output callbacks.

**Parameters**:
- `print_fn`: Callback for standard output
- `warn_fn`: Callback for warnings
- `error_fn`: Callback for errors
- `user_data`: User data passed to callbacks

**Example**:
```c
void my_print(const char* msg, void* ud) {
    printf("[CUSTOM] %s\n", msg);
}

xoron_set_console_callbacks(my_print, my_print, my_print, nullptr);
```

### xoron_console_print
```c
void xoron_console_print(const char* text)
```
Prints text to console.

**Parameters**:
- `text`: Text to print

### xoron_console_warn
```c
void xoron_console_warn(const char* text)
```
Prints warning to console.

**Parameters**:
- `text`: Warning text

### xoron_console_error
```c
void xoron_console_error(const char* text)
```
Prints error to console.

**Parameters**:
- `text`: Error text

### xoron_console_log
```c
void xoron_console_log(const char* level, const char* text)
```
Logs with level.

**Parameters**:
- `level`: Log level
- `text`: Text to log

### xoron_console_clear
```c
void xoron_console_clear()
```
Clears console history.

### xoron_console_get_history
```c
char* xoron_console_get_history()
```
Gets console history.

**Returns**: JSON string of history (must be freed)

### xoron_console_set_enabled
```c
void xoron_console_set_enabled(bool enabled)
```
Enables or disables console output.

**Parameters**:
- `enabled`: Enable flag

## Lua API

All functions are registered in the `console` global table and as global functions.

### Standard Output

#### print
```lua
print(...) -> void
```
Prints values to console (overridden).

**Parameters**:
- `...`: Values to print

**Example**:
```lua
print("Hello", "World", 123)
-- Output: Hello World 123
```

#### console.print
```lua
console.print(...) -> void
```
Same as global print.

#### console.log
```lua
console.log(...) -> void
```
Logs to console.

#### console.info
```lua
console.info(...) -> void
```
Logs info message.

#### console.warn
```lua
console.warn(...) -> void
```
Logs warning message.

#### console.error
```lua
console.error(...) -> void
```
Logs error message.

#### console.debug
```lua
console.debug(...) -> void
```
Logs debug message.

### Advanced Output

#### console.write
```lua
console.write(text: string) -> void
```
Writes text without newline.

**Parameters**:
- `text`: Text to write

#### console.writeLine
```lua
console.writeLine(text: string) -> void
```
Writes text with newline.

**Parameters**:
- `text`: Text to write

#### console.format
```lua
console.format(...) -> string
```
Formats values like print.

**Parameters**:
- `...`: Values to format

**Returns**: Formatted string

**Example**:
```lua
local msg = console.format("Value:", 42)
print(msg)  -- "Value: 42"
```

### Console Control

#### console.clear
```lua
console.clear() -> void
```
Clears console output.

#### console.pause
```lua
console.pause() -> void
```
Pauses console (wait for input).

#### console.enable
```lua
console.enable() -> void
```
Enables console output.

#### console.disable
```lua
console.disable() -> void
```
Disables console output.

#### console.isEnabled
```lua
console.isEnabled() -> boolean
```
Checks if console is enabled.

**Returns**: true if enabled

### History Management

#### console.getHistory
```lua
console.getHistory() -> table
```
Gets console history.

**Returns**: Array of history entries

**Each entry**:
```lua
{
    text = "message",
    level = "PRINT",
    timestamp = 1234567890
}
```

#### console.clearHistory
```lua
console.clearHistory() -> void
```
Clears history.

#### console.setHistoryLimit
```lua
console.setHistoryLimit(limit: number) -> void
```
Sets max history entries.

**Parameters**:
- `limit`: Max entries (0 = unlimited)

### Formatting

#### console.table
```lua
console.table(data: table, columns: table?) -> void
```
Prints table as formatted table.

**Parameters**:
- `data`: Table to print
- `columns`: Column names (optional)

**Example**:
```lua
local data = {
    {name = "Alice", age = 30},
    {name = "Bob", age = 25}
}
console.table(data, {"name", "age"})
```

#### console.json
```lua
console.json(data: any, indent: number?) -> void
```
Prints data as formatted JSON.

**Parameters**:
- `data`: Data to print
- `indent`: Indentation (default: 2)

**Example**:
```lua
local data = {a = 1, b = {c = 2}}
console.json(data)
```

#### console.yaml
```lua
console.yaml(data: table) -> void
```
Prints table as YAML.

**Parameters**:
- `data`: Table to print

### Progress Indicators

#### console.progress
```lua
console.progress(current: number, total: number, label: string?) -> void
```
Shows progress bar.

**Parameters**:
- `current`: Current value
- `total`: Total value
- `label`: Optional label

**Example**:
```lua
for i = 1, 100 do
    console.progress(i, 100, "Processing")
    wait(0.01)
end
```

#### console.spinner
```lua
console.spinner(label: string?) -> spinner_object
```
Creates spinner.

**Parameters**:
- `label`: Optional label

**Returns**: Spinner object with `stop()` method

**Example**:
```lua
local spinner = console.spinner("Loading")
-- Do work
spinner.stop()
```

### Color Output

#### console.color
```lua
console.color(text: string, color: string) -> string
```
Colors text (if supported).

**Parameters**:
- `text`: Text to color
- `color`: Color name

**Returns**: Colored text

**Supported Colors**:
- red, green, blue, yellow
- cyan, magenta, white, black
- bright_red, bright_green, etc.

**Example**:
```lua
print(console.color("Error!", "red"))
```

#### console.style
```lua
console.style(text: string, style: string) -> string
```
Styles text.

**Parameters**:
- `text`: Text
- `style`: Style name

**Supported Styles**:
- bold, underline, reverse, dim

### Grouping

#### console.group
```lua
console.group(label: string?) -> void
```
Starts group.

**Parameters**:
- `label`: Group label

#### console.groupCollapsed
```lua
console.groupCollapsed(label: string?) -> void
```
Starts collapsed group.

#### console.groupEnd
```lua
console.groupEnd() -> void
```
Ends current group.

### Timing

#### console.time
```lua
console.time(label: string) -> void
```
Starts timer.

**Parameters**:
- `label`: Timer label

#### console.timeEnd
```lua
console.timeEnd(label: string) -> void
```
Ends timer and prints duration.

**Parameters**:
- `label`: Timer label

**Example**:
```lua
console.time("operation")
-- Do work
console.timeEnd("operation")  -- operation: 123.45ms
```

#### console.timeLog
```lua
console.timeLog(label: string, message: string?) -> void
```
Logs timer snapshot.

**Parameters**:
- `label`: Timer label
- `message`: Optional message

### Counters

#### console.count
```lua
console.count(label: string?) -> void
```
Increments counter.

**Parameters**:
- `label`: Counter label (default: "default")

**Example**:
```lua
for i = 1, 5 do
    console.count("loop")
end
-- Output: loop: 1, loop: 2, ...
```

#### console.countReset
```lua
console.countReset(label: string?) -> void
```
Resets counter.

### Assertions

#### console.assert
```lua
console.assert(condition: any, message: string?) -> void
```
Asserts condition.

**Parameters**:
- `condition`: Condition to check
- `message`: Error message

**Example**:
```lua
console.assert(x > 0, "x must be positive")
```

### Trace

#### console.trace
```lua
console.trace(message: string?) -> void
```
Prints stack trace.

**Parameters**:
- `message`: Optional message

### Custom Formatters

#### console.setFormatter
```lua
console.setFormatter(level: string, formatter: function) -> void
```
Sets custom formatter for level.

**Parameters**:
- `level`: "PRINT", "WARN", "ERROR", etc.
- `formatter`: Function(text, level) -> string

**Example**:
```lua
console.setFormatter("PRINT", function(text, level)
    return string.format("[%s] %s", os.date("%X"), text)
end)
```

### Output Channels

#### console.addChannel
```lua
console.addChannel(name: string, callback: function) -> void
```
Adds output channel.

**Parameters**:
- `name`: Channel name
- `callback`: Function(text, level)

**Example**:
```lua
console.addChannel("file", function(text, level)
    fs.appendfile("console.log", text .. "\n")
end)
```

#### console.removeChannel
```lua
console.removeChannel(name: string) -> void
```
Removes output channel.

#### console.listChannels
```lua
console.listChannels() -> table
```
Gets all channels.

**Returns**: Array of channel names

### Redirect

#### console.redirect
```lua
console.redirect(func: function) -> void
```
Redirects output during function execution.

**Parameters**:
- `func`: Function to execute

**Example**:
```lua
console.redirect(function()
    print("This goes to console")
    warn("This too")
end)
```

### Capture

#### console.capture
```lua
console.capture(func: function) -> string
```
Captures output.

**Parameters**:
- `func`: Function to execute

**Returns**: Captured output

**Example**:
```lua
local output = console.capture(function()
    print("Hello")
    print("World")
end)
print(output)  -- "Hello\nWorld\n"
```

### Filter

#### console.setFilter
```lua
console.setFilter(pattern: string) -> void
```
Sets output filter.

**Parameters**:
- `pattern`: Lua pattern or string

**Example**:
```lua
console.setFilter("ERROR")  -- Only show errors
```

#### console.clearFilter
```lua
console.clearFilter() -> void
```
Clears filter.

### Prefix/Suffix

#### console.setPrefix
```lua
console.setPrefix(level: string, prefix: string) -> void
```
Sets prefix for level.

**Parameters**:
- `level`: Level name
- `prefix`: Prefix string

**Example**:
```lua
console.setPrefix("ERROR", "[!] ")
-- Error output: [!] Something went wrong
```

#### console.setSuffix
```lua
console.setSuffix(level: string, suffix: string) -> void
```
Sets suffix for level.

### Timestamp

#### console.setTimestamp
```lua
console.setTimestamp(enabled: boolean, format: string?) -> void
```
Enables timestamp.

**Parameters**:
- `enabled`: Enable flag
- `format`: Time format (default: "%H:%M:%S")

**Example**:
```lua
console.setTimestamp(true, "%Y-%m-%d %H:%M:%S")
-- Output: 2024-01-06 14:30:45 Hello
```

### Level Control

#### console.setLevel
```lua
console.setLevel(level: string) -> void
```
Sets minimum level to display.

**Parameters**:
- `level`: "DEBUG", "INFO", "WARN", "ERROR"

**Example**:
```lua
console.setLevel("WARN")  -- Only warnings and errors
```

### Silent Mode

#### console.silent
```lua
console.silent(func: function) -> void
```
Executes function silently.

**Parameters**:
- `func`: Function to execute

**Example**:
```lua
console.silent(function()
    print("This won't show")
end)
```

### Batch Output

#### console.batch
```lua
console.batch(func: function) -> void
```
Batches output until function completes.

**Parameters**:
- `func`: Function to execute

**Example**:
```lua
console.batch(function()
    for i = 1, 100 do
        print(i)
    end
end)  -- All output at once
```

### Flush

#### console.flush
```lua
console.flush() -> void
```
Flushes output buffer.

### Sync

#### console.sync
```lua
console.sync() -> void
```
Syncs output with system console.

## Platform-Specific Output

### Android
- Uses `__android_log_print` for system log
- Levels map to Android log priorities:
  - PRINT → ANDROID_LOG_INFO
  - WARN → ANDROID_LOG_WARN
  - ERROR → ANDROID_LOG_ERROR
  - DEBUG → ANDROID_LOG_DEBUG

### iOS
- Uses `printf` for console
- Can integrate with NSLog if needed
- Supports ANSI colors in terminal

### Desktop
- Uses `printf` and `stderr`
- Full ANSI color support
- Terminal detection

## Implementation Details

### Output Flow
```
User Call (print)
    ↓
Format Arguments
    ↓
Check Filters
    ↓
Apply Prefix/Suffix
    ↓
Format with Timestamp
    ↓
Channel Callbacks
    ↓
System Output
```

### Thread Safety
- All output operations are mutex-locked
- Callbacks are called with lock held
- History is thread-safe

### Memory Management
- History has configurable limit
- Old entries are automatically removed
- No memory leaks in formatting

### Error Handling
- Invalid levels are treated as "PRINT"
- Callback errors are caught and logged
- Format errors don't crash console

## Usage Examples

### Basic Output
```lua
print("Hello, World!")
console.log("Log message")
console.warn("Warning!")
console.error("Error occurred!")
```

### Formatting
```lua
local name = "Alice"
local age = 30
print(string.format("%s is %d years old", name, age))

-- Or use console.format
print(console.format(name, "is", age, "years old"))
```

### Tables
```lua
local users = {
    {name = "Alice", age = 30, role = "admin"},
    {name = "Bob", age = 25, role = "user"},
    {name = "Charlie", age = 35, role = "moderator"}
}

console.table(users)
```

### JSON
```lua
local config = {
    server = "localhost",
    port = 8080,
    options = {
        ssl = true,
        timeout = 30
    }
}

console.json(config)
```

### Progress
```lua
local function process_files(files)
    for i, file in ipairs(files) do
        console.progress(i, #files, "Processing " .. file)
        -- Process file
        wait(0.1)
    end
end
```

### Timing
```lua
console.time("database_query")
local results = query_database()
console.timeEnd("database_query")
```

### Groups
```lua
console.group("Configuration")
print("Server:", config.server)
print("Port:", config.port)
console.groupEnd()
```

### Colors
```lua
print(console.color("SUCCESS", "green"))
print(console.color("FAILURE", "red"))
print(console.color("WARNING", "yellow"))
```

### Custom Channels
```lua
-- Log to file
console.addChannel("file", function(text, level)
    fs.appendfile("app.log", string.format("[%s] %s\n", level, text))
end)

-- Log to remote
console.addChannel("remote", function(text, level)
    -- Send to server
    http.post("https://logs.example.com", text)
end)
```

### Capture
```lua
-- Capture output for testing
local output = console.capture(function()
    print("Test 1")
    print("Test 2")
end)

assert(output == "Test 1\nTest 2\n")
```

### Filter
```lua
-- Only show errors
console.setLevel("ERROR")

-- Or use pattern
console.setFilter("database")  -- Only messages containing "database"
```

### Silent Operations
```lua
-- Suppress output during initialization
console.silent(function()
    -- Load config
    -- Setup modules
    -- etc.
end)

print("Initialization complete!")
```

### Redirect
```lua
-- Redirect to custom handler
local messages = {}
console.redirect(function(text, level)
    table.insert(messages, {text = text, level = level})
end)

-- Execute code
print("Hello")
warn("Warning")

-- Restore and process
console.redirect(nil)
for _, msg in ipairs(messages) do
    -- Process messages
end
```

### Timestamp
```lua
console.setTimestamp(true, "%H:%M:%S")
print("Event occurred")  -- 14:30:45 Event occurred
```

### Prefix/Suffix
```lua
console.setPrefix("ERROR", "[!] ")
console.setSuffix("ERROR", " !!!")

console.error("Problem")  -- [!] Problem !!!
```

### Batch Output
```lua
-- Collect all output and display at once
console.batch(function()
    for i = 1, 1000 do
        print("Line " .. i)
    end
end)
```

### History
```lua
-- Get recent output
local history = console.getHistory()
for _, entry in ipairs(history) do
    print(string.format("[%s] %s", entry.level, entry.text))
end

-- Clear old history
console.clearHistory()
```

### Assertions
```lua
-- Debug assertions
local value = get_value()
console.assert(value ~= nil, "Value should not be nil")
console.assert(type(value) == "number", "Value should be number")
```

### Trace
```lua
function deep_function()
    console.trace("Debug point")
end

deep_function()  -- Shows full stack trace
```

### Custom Formatters
```lua
-- Custom format for warnings
console.setFormatter("WARN", function(text, level)
    return string.format("⚠️  %s: %s", level, text)
end)

console.warn("Low disk space")  -- ⚠️  WARN: Low disk space
```

### Level Control
```lua
-- Show only important messages
console.setLevel("WARN")

print("This won't show")  -- Silent
console.warn("This shows")  -- Visible
console.error("This shows")  -- Visible
```

### Multiple Outputs
```lua
-- Console + File + Remote
console.addChannel("file", function(text)
    fs.appendfile("log.txt", text .. "\n")
end)

console.addChannel("remote", function(text)
    http.post("https://api.example.com/log", text)
end)

print("Logged everywhere")  -- Console, file, and remote
```

### Performance Logging
```lua
-- Profile sections
console.time("total")

console.time("load")
load_data()
console.timeEnd("load")

console.time("process")
process_data()
console.timeEnd("process")

console.timeEnd("total")
```

### Conditional Output
```lua
local DEBUG = true

function debug_print(...)
    if DEBUG then
        console.debug(...)
    end
end

debug_print("Debug info")  -- Shows if DEBUG = true
```

### Structured Logging
```lua
-- JSON structured logs
local log_entry = {
    timestamp = os.time(),
    level = "INFO",
    message = "User logged in",
    user = "alice",
    ip = "192.168.1.1"
}

console.json(log_entry)
```

### Progress with ETA
```lua
local start = os.time()
local total = 100

for i = 1, total do
    -- Process
    local elapsed = os.time() - start
    local rate = i / elapsed
    local eta = (total - i) / rate
    
    console.progress(i, total, string.format("ETA: %.1fs", eta))
end
```

### Spinner
```lua
local spinner = console.spinner("Loading")

-- Do async work
wait(2)

spinner.stop()
print("Done!")
```

### Color Themes
```lua
local theme = {
    success = "green",
    error = "red",
    warning = "yellow",
    info = "cyan"
}

function themed_print(msg, type)
    print(console.color(msg, theme[type] or "white"))
end

themed_print("Success!", "success")
themed_print("Error!", "error")
```

### Output Buffering
```lua
-- Buffer output
local buffer = {}
console.addChannel("buffer", function(text)
    table.insert(buffer, text)
end)

-- Do work
print("Line 1")
print("Line 2")

-- Process buffer
for _, line in ipairs(buffer) do
    -- Process each line
end
```

### Conditional Formatting
```lua
-- Format based on content
console.setFormatter("PRINT", function(text)
    if text:find("error") then
        return console.color(text, "red")
    elseif text:find("success") then
        return console.color(text, "green")
    end
    return text
end)
```

### Session Logging
```lua
-- Start session
local session_start = os.time()
console.setTimestamp(true)

print("Session started")

-- ... work ...

print("Session ended")
-- All output has timestamps
```

### Error Stack Traces
```lua
-- Enhanced error printing
function safe_call(func)
    local ok, err = pcall(func)
    if not ok then
        console.error("Error:", err)
        console.trace("Stack trace")
    end
end

safe_call(function()
    error("Something broke")
end)
```

### Multi-line Output
```lua
-- Print blocks
console.writeLine([[
This is a multi-line
string that spans
multiple lines
]])
```

### Summary Reports
```lua
-- End of run report
console.group("Summary")
console.count("operations")
console.count("errors")
console.timeEnd("total")
console.groupEnd()
```

## Performance

- **Lock overhead**: Minimal (mutex only for shared state)
- **Formatting**: Fast for simple types
- **History**: O(1) append, O(n) clear
- **Channels**: O(m) where m = number of channels
- **Filters**: O(1) pattern match

## Error Handling

- Callback errors are caught and logged to fallback
- Invalid formatters are ignored
- History overflow is handled gracefully
- Platform output failures don't crash

## Related Files
- `xoron.h` (API declarations)
- `xoron_luau.cpp` (print override)
- `xoron_env.cpp` (console integration)
