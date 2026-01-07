# 05. xoron_filesystem.cpp - File System Operations

**File Path**: `src/src/xoron_filesystem.cpp`  
**Size**: 31,241 bytes  
**Lines**: ~500

**Platform**: Cross-platform (with platform-specific paths)

## Overview
Provides comprehensive file I/O operations for the executor. Handles file reading, writing, listing, and directory management with platform-specific path resolution.

## Includes
- `xoron.h` (local)
- `cstdlib`, `cstring`, `cstdio` (system)
- `string`, `vector` (system)
- `fstream`, `sstream` (system)
- `filesystem` (system - C++17)
- `mutex` (system)
- `algorithm` (system)
- Platform-specific: `jni.h`, `android/log.h` (Android) or `unistd.h` (iOS)
- `lua.h`, `lualib.h`, `luacode.h` (local - Luau)
- `Luau/Compiler.h` (local - Luau compiler)

## Path Structure

### iOS (iOS 15+)
```
~/Documents/Xoron/
├── autoexecute/    - Auto-run scripts
├── scripts/        - User scripts
└── workspace/      - Working directory
```

### Android (Android 10+, API 29+)
```
Internal Storage:
/data/data/<package>/files/Xoron/
├── autoexecute/
├── scripts/
└── workspace/

External Storage (if available):
/storage/emulated/0/Xoron/
├── autoexecute/
├── scripts/
└── workspace/
```

### Development
```
Current directory/Xoron/
├── autoexecute/
├── scripts/
└── workspace/
```

## Core Functions

### Path Resolution

#### get_base_path
```cpp
std::string get_base_path()
```
Returns the base Xoron directory path.

**Platform Logic**:
- iOS: `~/Documents/Xoron`
- Android: `/data/data/<package>/files/Xoron` or `/storage/emulated/0/Xoron`
- Dev: `./Xoron`

**Returns**: Base path string

#### get_workspace_path
```cpp
std::string get_workspace_path()
```
Returns workspace directory path.

**Returns**: `<base>/workspace/`

#### get_autoexecute_path
```cpp
std::string get_autoexecute_path()
```
Returns autoexecute directory path.

**Returns**: `<base>/autoexecute/`

#### get_scripts_path
```cpp
std::string get_scripts_path()
```
Returns scripts directory path.

**Returns**: `<base>/scripts/`

### File Operations

#### xoron_readfile
```cpp
char* xoron_readfile(const char* path, size_t* len)
```
Reads file content.

**Parameters**:
- `path`: File path (relative to workspace or absolute)
- `len`: Output for file size

**Returns**:
- Allocated buffer with file content
- NULL on error

**Features**:
- Binary-safe
- Auto-creates directories if needed
- Handles large files

#### xoron_writefile
```cpp
bool xoron_writefile(const char* path, const void* data, size_t len)
```
Writes data to file.

**Parameters**:
- `path`: File path
- `data`: Data to write
- `len`: Data length

**Returns**: `true` on success

**Features**:
- Creates parent directories
- Binary-safe
- Overwrites existing files

#### xoron_appendfile
```cpp
bool xoron_appendfile(const char* path, const void* data, size_t len)
```
Appends data to file.

**Parameters**:
- `path`: File path
- `data`: Data to append
- `len`: Data length

**Returns**: `true` on success

#### xoron_listfiles
```cpp
std::vector<std::string> xoron_listfiles(const char* path)
```
Lists files in directory.

**Parameters**:
- `path`: Directory path

**Returns**: Vector of file/directory names

#### xoron_isfile
```cpp
bool xoron_isfile(const char* path)
```
Checks if path is a file.

**Parameters**:
- `path`: Path to check

**Returns**: `true` if file exists

#### xoron_isfolder
```cpp
bool xoron_isfolder(const char* path)
```
Checks if path is a directory.

**Parameters**:
- `path`: Path to check

**Returns**: `true` if directory exists

#### xoron_makefolder
```cpp
bool xoron_makefolder(const char* path)
```
Creates directory.

**Parameters**:
- `path`: Directory path

**Returns**: `true` on success

#### xoron_delfolder
```cpp
bool xoron_delfolder(const char* path)
```
Deletes directory and contents.

**Parameters**:
- `path`: Directory path

**Returns**: `true` on success

#### xoron_delfile
```cpp
bool xoron_delfile(const char* path)
```
Deletes file.

**Parameters**:
- `path`: File path

**Returns**: `true` on success

#### xoron_isdirempty
```cpp
bool xoron_isdirempty(const char* path)
```
Checks if directory is empty.

**Parameters**:
- `path`: Directory path

**Returns**: `true` if empty

#### xoron_movefile
```cpp
bool xoron_movefile(const char* from, const char* to)
```
Moves/renames file.

**Parameters**:
- `from`: Source path
- `to`: Destination path

**Returns**: `true` on success

#### xoron_copyfile
```cpp
bool xoron_copyfile(const char* from, const char* to)
```
Copies file.

**Parameters**:
- `from`: Source path
- `to`: Destination path

**Returns**: `true` on success

#### xoron_getfilesiz
```cpp
size_t xoron_getfilesize(const char* path)
```
Gets file size.

**Parameters**:
- `path`: File path

**Returns**: File size in bytes

#### xoron_getlastmodified
```cpp
time_t xoron_getlastmodified(const char* path)
```
Gets file modification time.

**Parameters**:
- `path`: File path

**Returns**: Unix timestamp

### Lua API

All functions are registered in the `fs` global table.

#### fs.readfile
```lua
fs.readfile(path: string) -> string
```
Reads file as string.

**Parameters**:
- `path`: File path

**Returns**: File content

**Example**:
```lua
local content = fs.readfile("workspace/data.txt")
```

#### fs.writefile
```lua
fs.writefile(path: string, content: string) -> boolean
```
Writes string to file.

**Parameters**:
- `path`: File path
- `content`: Content to write

**Returns**: true on success

**Example**:
```lua
fs.writefile("workspace/output.txt", "Hello, World!")
```

#### fs.appendfile
```lua
fs.appendfile(path: string, content: string) -> boolean
```
Appends string to file.

**Parameters**:
- `path`: File path
- `content`: Content to append

**Returns**: true on success

#### fs.listfiles
```lua
fs.listfiles(path: string?) -> table
```
Lists files in directory.

**Parameters**:
- `path`: Directory path (default: workspace)

**Returns**: Array of file names

**Example**:
```lua
local files = fs.listfiles("workspace")
for _, file in ipairs(files) do
    print(file)
end
```

#### fs.isfile
```lua
fs.isfile(path: string) -> boolean
```
Checks if path is a file.

**Parameters**:
- `path`: Path to check

**Returns**: true if file exists

#### fs.isfolder
```lua
fs.isfolder(path: string) -> boolean
```
Checks if path is a directory.

**Parameters**:
- `path`: Path to check

**Returns**: true if directory exists

#### fs.makefolder
```lua
fs.makefolder(path: string) -> boolean
```
Creates directory.

**Parameters**:
- `path`: Directory path

**Returns**: true on success

#### fs.delfolder
```lua
fs.delfolder(path: string) -> boolean
```
Deletes directory and contents.

**Parameters**:
- `path`: Directory path

**Returns**: true on success

#### fs.delfile
```lua
fs.delfile(path: string) -> boolean
```
Deletes file.

**Parameters**:
- `path`: File path

**Returns**: true on success

#### fs.isdirempty
```lua
fs.isdirempty(path: string) -> boolean
```
Checks if directory is empty.

**Parameters**:
- `path`: Directory path

**Returns**: true if empty

#### fs.movefile
```lua
fs.movefile(from: string, to: string) -> boolean
```
Moves/renames file.

**Parameters**:
- `from`: Source path
- `to`: Destination path

**Returns**: true on success

#### fs.copyfile
```lua
fs.copyfile(from: string, to: string) -> boolean
```
Copies file.

**Parameters**:
- `from`: Source path
- `to`: Destination path

**Returns**: true on success

#### fs.getfilesize
```lua
fs.getfilesize(path: string) -> number
```
Gets file size.

**Parameters**:
- `path`: File path

**Returns**: File size in bytes

#### fs.getlastmodified
```lua
fs.getlastmodified(path: string) -> number
```
Gets file modification time.

**Parameters**:
- `path`: File path

**Returns**: Unix timestamp

#### fs.isabsolute
```lua
fs.isabsolute(path: string) -> boolean
```
Checks if path is absolute.

**Parameters**:
- `path`: Path to check

**Returns**: true if absolute

#### fs.normalize
```lua
fs.normalize(path: string) -> string
```
Normalizes path (resolves .. and .).

**Parameters**:
- `path`: Path to normalize

**Returns**: Normalized path

#### fs.join
```lua
fs.join(...) -> string
```
Joins path components.

**Parameters**:
- `...`: Path components

**Returns**: Joined path

**Example**:
```lua
local path = fs.join("workspace", "subdir", "file.txt")
```

#### fs.getbasepath
```lua
fs.getbasepath() -> string
```
Returns base Xoron directory.

**Returns**: Base path

#### fs.getworkspace
```lua
fs.getworkspace() -> string
```
Returns workspace path.

**Returns**: Workspace path

#### fs.getautoexecute
```lua
fs.getautoexecute() -> string
```
Returns autoexecute path.

**Returns**: Autoexecute path

#### fs.getscripts
```lua
fs.getscripts() -> string
```
Returns scripts path.

**Returns**: Scripts path

### C API (xoron.h)

#### xoron_get_workspace
```c
const char* xoron_get_workspace(void)
```
Returns workspace path.

**Returns**: Workspace path string

#### xoron_set_workspace
```c
void xoron_set_workspace(const char* path)
```
Sets workspace path.

**Parameters**:
- `path`: New workspace path

#### xoron_get_autoexecute_path
```c
const char* xoron_get_autoexecute_path(void)
```
Returns autoexecute path.

**Returns**: Autoexecute path string

#### xoron_get_scripts_path
```c
const char* xoron_get_scripts_path(void)
```
Returns scripts path.

**Returns**: Scripts path string

### Lua Global Functions (Extended)

These are also registered as global functions for convenience:

#### readfile
```lua
readfile(path: string) -> string
```
Global alias for `fs.readfile()`.

#### writefile
```lua
writefile(path: string, content: string) -> boolean
```
Global alias for `fs.writefile()`.

#### appendfile
```lua
appendfile(path: string, content: string) -> boolean
```
Global alias for `fs.appendfile()`.

#### listfiles
```lua
listfiles(path: string?) -> table
```
Global alias for `fs.listfiles()`.

#### isfile
```lua
isfile(path: string) -> boolean
```
Global alias for `fs.isfile()`.

#### isfolder
```lua
isfolder(path: string) -> boolean
```
Global alias for `fs.isfolder()`.

#### makefolder
```lua
makefolder(path: string) -> boolean
```
Global alias for `fs.makefolder()`.

#### delfolder
```lua
delfolder(path: string) -> boolean
```
Global alias for `fs.delfolder()`.

#### delfile
```lua
delfile(path: string) -> boolean
```
Global alias for `fs.delfile()`.

## Implementation Details

### Directory Creation
All write operations automatically create parent directories if they don't exist.

### Path Resolution
Paths are resolved in this order:
1. Absolute path (starts with `/` or drive letter)
2. Relative to workspace
3. Relative to current directory

### Error Handling
- Functions return `false` or `nil` on error
- Last error is set via `xoron_set_error()`
- Check `xoron_last_error()` for details

### Thread Safety
All file operations are protected by mutex locks to ensure thread safety.

### Binary Safety
All read/write operations are binary-safe and handle null bytes correctly.

## Usage Examples

### Basic File Operations
```lua
-- Write a file
fs.writefile("workspace/test.txt", "Hello, World!")

-- Read a file
local content = fs.readfile("workspace/test.txt")
print(content)  -- "Hello, World!"

-- Append to file
fs.appendfile("workspace/test.txt", "\nAppended text")

-- Check if exists
if fs.isfile("workspace/test.txt") then
    print("File exists!")
end
```

### Directory Operations
```lua
-- Create directory
fs.makefolder("workspace/data")

-- List files
local files = fs.listfiles("workspace")
for _, file in ipairs(files) do
    print(file)
end

-- Check if directory
if fs.isfolder("workspace/data") then
    print("It's a directory!")
end

-- Delete directory
fs.delfolder("workspace/data")
```

### File Management
```lua
-- Copy file
fs.copyfile("workspace/source.txt", "workspace/backup.txt")

-- Move file
fs.movefile("workspace/backup.txt", "workspace/moved.txt")

-- Get file info
local size = fs.getfilesize("workspace/moved.txt")
local modified = fs.getlastmodified("workspace/moved.txt")
print("Size:", size, "Modified:", modified)
```

### Path Utilities
```lua
-- Join paths
local path = fs.join("workspace", "subdir", "file.txt")
print(path)  -- "workspace/subdir/file.txt"

-- Normalize path
local normalized = fs.normalize("workspace/../workspace/file.txt")
print(normalized)  -- "workspace/file.txt"

-- Check if absolute
if fs.isabsolute("/system/file.txt") then
    print("Absolute path")
end
```

### Working with Scripts
```lua
-- Save a script
local script = [[
    print("Hello from saved script!")
    return "Done"
]]
fs.writefile("scripts/myscript.lua", script)

-- Load and execute
local content = fs.readfile("scripts/myscript.lua")
local func = loadstring(content)
if func then
    func()
end
```

### Auto-Execute Scripts
```lua
-- Scripts in autoexecute/ folder run automatically
-- when the executor injects

-- Create auto-execute script
fs.writefile("autoexecute/init.lua", [[
    print("Xoron initialized!")
    notify("Xoron", "Auto-execute complete")
]])
```

### Workspace Management
```lua
-- Get workspace path
local workspace = fs.getworkspace()
print("Workspace:", workspace)

-- Change workspace (C API only)
-- xoron_set_workspace("/custom/path")

-- List workspace contents
local items = fs.listfiles(workspace)
for _, item in ipairs(items) do
    print(item)
end
```

### Error Handling
```lua
-- Check for errors
local success = fs.writefile("workspace/test.txt", "data")
if not success then
    print("Error:", xoron_last_error())
end

-- Safe read
local ok, content = pcall(fs.readfile, "workspace/missing.txt")
if not ok then
    print("Failed to read:", content)
end
```

### Binary Data
```lua
-- Write binary data
local binary = string.char(0x00, 0x01, 0x02, 0xFF)
fs.writefile("workspace/binary.dat", binary)

-- Read binary data
local data = fs.readfile("workspace/binary.dat")
print(#data)  -- 4
```

## Platform-Specific Notes

### iOS
- Requires file access permissions
- Uses `~/Documents/Xoron/` which is backed up by iCloud
- Consider using `fs.getworkspace()` to get correct path

### Android
- Internal storage: `/data/data/<package>/files/Xoron/`
  - Private, no permissions needed
  - Not accessible without root
- External storage: `/storage/emulated/0/Xoron/`
  - Requires storage permissions
  - User-accessible
- Falls back to internal if external unavailable

### Development
- Uses current directory + `/Xoron/`
- Good for testing

## Performance

- File operations use C++17 filesystem (fast)
- Mutex locks for thread safety (minimal overhead)
- Efficient path resolution
- No unnecessary allocations

## Related Files
- `xoron.h` (API declarations)
- `xoron_luau.cpp` (Lua bindings)
- `xoron_env.cpp` (environment integration)
