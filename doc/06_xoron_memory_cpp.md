# 06. xoron_memory.cpp - Memory Management & Anti-Detection

**File Path**: `src/src/xoron_memory.cpp`  
**Size**: 19,581 bytes  
**Lines**: ~400

**Platform**: Cross-platform (with platform-specific memory operations)

## Overview
Provides memory utilities, pattern scanning, and anti-detection mechanisms for the executor. Includes memory allocation tracking, pattern matching, and security checks.

## Includes
- `xoron.h` (local)
- `cstdlib`, `cstring`, `cstdio` (system)
- `string`, `vector` (system)
- `mutex`, `thread`, `chrono` (system)
- `random` (system)
- Platform-specific:
  - iOS: `mach/mach.h`, `mach/vm_map.h`, `mach-o/dyld.h`, `dlfcn.h`, `sys/sysctl.h`
  - Android: `dlfcn.h`, `unistd.h`, `sys/mman.h`, `sys/ptrace.h`, `sys/types.h`, `sys/stat.h`, `fcntl.h`, `elf.h`, `link.h`, `android/log.h`, `sys/system_properties.h`
  - Linux: `dlfcn.h`, `unistd.h`
- `lua.h`, `lualib.h` (local - Luau)

## Memory Management

### Global State
```cpp
static std::mutex g_mem_mutex;
static bool g_anti_detection_enabled = true;
```

Thread-safe memory operations with anti-detection toggle.

## Anti-Detection Functions

### Debugger Detection

#### is_debugger_present
```cpp
static bool is_debugger_present()
```
Detects if a debugger is attached to the process.

**Platform Implementation**:
- **iOS/macOS**: Uses `sysctl` with `KERN_PROC_PID` and checks for `P_TRACED` flag
- **Android/Linux**: Uses `ptrace(PTRACE_TRACEME)` which fails if already being traced

**Returns**: `true` if debugger detected

#### is_vm_present
```cpp
static bool is_vm_present()
```
Detects if running in a virtual machine.

**Checks**:
- **iOS**: Checks for common VM indicators via `sysctl`
- **Android**: Checks CPU info, build properties
- **Generic**: Checks for VM-specific files and environment variables

**Returns**: `true` if VM detected

### Environment Checks

#### check_environment_safe
```cpp
static bool check_environment_safe()
```
Comprehensive environment safety check.

**Checks**:
1. Debugger presence
2. VM presence
3. Suspicious processes
4. Root/jailbreak detection
5. Hook detection

**Returns**: `true` if environment is safe

### Anti-Detection Control

#### xoron_enable_anti_detection
```cpp
void xoron_enable_anti_detection(bool enable)
```
Enables or disables anti-detection measures.

**Parameters**:
- `enable`: `true` to enable, `false` to disable

**Effects**:
- When enabled: Activates anti-debugging, anti-VM, and anti-hook measures
- When disabled: Allows debugging and analysis

#### xoron_check_environment
```cpp
bool xoron_check_environment()
```
Checks current environment safety.

**Returns**: `true` if safe, `false` if suspicious

## Memory Scanning

### Pattern Matching

#### find_pattern
```cpp
static std::vector<uintptr_t> find_pattern(const uint8_t* data, size_t data_len, 
                                           const uint8_t* pattern, size_t pattern_len,
                                           const char* mask)
```
Finds byte patterns in memory.

**Parameters**:
- `data`: Memory region to search
- `data_len`: Size of memory region
- `pattern`: Byte pattern to find
- `pattern_len`: Pattern length
- `mask`: Mask string ('x' = match byte, '?' = wildcard)

**Returns**: Vector of matching addresses

**Example**:
```cpp
uint8_t pattern[] = {0x48, 0x89, 0x5C};
char mask[] = "xxx";
auto matches = find_pattern(mem, size, pattern, 3, mask);
```

#### scan_memory
```cpp
static std::vector<uintptr_t> scan_memory(uintptr_t start, size_t length,
                                          const uint8_t* pattern, size_t pattern_len,
                                          const char* mask)
```
Scans a memory region for patterns.

**Parameters**:
- `start`: Starting address
- `length`: Region size
- `pattern`: Byte pattern
- `pattern_len`: Pattern length
- `mask`: Mask string

**Returns**: Vector of matching addresses

### Memory Protection

#### get_memory_protection
```cpp
static int get_memory_protection(uintptr_t addr)
```
Gets memory page protection flags.

**Parameters**:
- `addr`: Memory address

**Returns**: Protection flags (read/write/execute)

#### set_memory_protection
```cpp
static bool set_memory_protection(uintptr_t addr, size_t size, int prot)
```
Sets memory page protection.

**Parameters**:
- `addr`: Memory address
- `size`: Region size
- `prot`: Protection flags

**Returns**: `true` on success

### Memory Operations

#### xoron_memory_read
```cpp
bool xoron_memory_read(uintptr_t addr, void* buffer, size_t size)
```
Reads from memory safely.

**Parameters**:
- `addr`: Source address
- `buffer`: Destination buffer
- `size`: Bytes to read

**Returns**: `true` on success

#### xoron_memory_write
```cpp
bool xoron_memory_write(uintptr_t addr, const void* buffer, size_t size)
```
Writes to memory safely.

**Parameters**:
- `addr`: Destination address
- `buffer`: Source buffer
- `size`: Bytes to write

**Returns**: `true` on success

#### xoron_memory_allocate
```cpp
void* xoron_memory_allocate(size_t size, int prot)
```
Allocates memory with protection.

**Parameters**:
- `size`: Bytes to allocate
- `prot`: Protection flags

**Returns**: Allocated memory address

#### xoron_memory_free
```cpp
void xoron_memory_free(void* ptr, size_t size)
```
Frees allocated memory.

**Parameters**:
- `ptr`: Memory to free
- `size`: Size of allocation

## Lua API

All functions are registered in the `memory` global table.

### Memory Scanning

#### memory.findpattern
```lua
memory.findpattern(pattern: string, mask: string?) -> table
```
Searches current process memory for pattern.

**Parameters**:
- `pattern`: Hex pattern string (e.g., "48 89 5C ?? 48")
- `mask`: Optional mask (e.g., "xx?x")

**Returns**: Array of matching addresses

**Example**:
```lua
local addresses = memory.findpattern("48 89 5C ?? 48")
for _, addr in ipairs(addresses) do
    print(string.format("0x%X", addr))
end
```

#### memory.scan
```lua
memory.scan(start: number, length: number, pattern: string, mask: string?) -> table
```
Scans specific memory region.

**Parameters**:
- `start`: Start address
- `length`: Region size
- `pattern`: Hex pattern
- `mask`: Optional mask

**Returns**: Array of matching addresses

#### memory.read
```lua
memory.read(address: number, size: number?) -> string
```
Reads bytes from memory.

**Parameters**:
- `address`: Memory address
- `size`: Bytes to read (default: 4)

**Returns**: Raw bytes as string

**Example**:
```lua
local data = memory.read(0x12345678, 16)
```

#### memory.write
```lua
memory.write(address: number, data: string) -> boolean
```
Writes bytes to memory.

**Parameters**:
- `address`: Memory address
- `data`: Bytes to write

**Returns**: true on success

#### memory.readbyte
```lua
memory.readbyte(address: number) -> number
```
Reads single byte.

**Parameters**:
- `address`: Memory address

**Returns**: Byte value (0-255)

#### memory.readword
```lua
memory.readword(address: number) -> number
```
Reads 2 bytes (word).

**Parameters**:
- `address`: Memory address

**Returns**: Word value

#### memory.readdword
```lua
memory.readdword(address: number) -> number
```
Reads 4 bytes (dword).

**Parameters**:
- `address`: Memory address

**Returns**: Dword value

#### memory.readqword
```lua
memory.readqword(address: number) -> number
```
Reads 8 bytes (qword).

**Parameters**:
- `address`: Memory address

**Returns**: Qword value

#### memory.readfloat
```lua
memory.readfloat(address: number) -> number
```
Reads 4-byte float.

**Parameters**:
- `address`: Memory address

**Returns**: Float value

#### memory.readdouble
```lua
memory.readdouble(address: number) -> number
```
Reads 8-byte double.

**Parameters**:
- `address`: Memory address

**Returns**: Double value

#### memory.readstring
```lua
memory.readstring(address: number, length: number?) -> string
```
Reads null-terminated string.

**Parameters**:
- `address`: Memory address
- `length`: Max length (default: 256)

**Returns**: String

#### memory.writebyte
```lua
memory.writebyte(address: number, value: number) -> boolean
```
Writes single byte.

**Parameters**:
- `address`: Memory address
- `value`: Byte value

**Returns**: true on success

#### memory.writeword
```lua
memory.writeword(address: number, value: number) -> boolean
```
Writes 2 bytes.

#### memory.writedword
```lua
memory.writedword(address: number, value: number) -> boolean
```
Writes 4 bytes.

#### memory.writeqword
```lua
memory.writeqword(address: number, value: number) -> boolean
```
Writes 8 bytes.

#### memory.writefloat
```lua
memory.writefloat(address: number, value: number) -> boolean
```
Writes 4-byte float.

#### memory.writedouble
```lua
memory.writedouble(address: number, value: number) -> boolean
```
Writes 8-byte double.

#### memory.writestring
```lua
memory.writestring(address: number, str: string) -> boolean
```
Writes string with null terminator.

**Parameters**:
- `address`: Memory address
- `str`: String to write

**Returns**: true on success

### Memory Protection

#### memory.getprotection
```lua
memory.getprotection(address: number) -> string
```
Gets memory protection flags.

**Parameters**:
- `address`: Memory address

**Returns**: String like "RWX", "R--", etc.

#### memory.setprotection
```lua
memory.setprotection(address: number, size: number, flags: string) -> boolean
```
Sets memory protection.

**Parameters**:
- `address`: Memory address
- `size`: Region size
- `flags`: Protection string ("R", "W", "X", "RW", "RX", "RWX")

**Returns**: true on success

### Allocation

#### memory.allocate
```lua
memory.allocate(size: number, protection: string?) -> number
```
Allocates memory.

**Parameters**:
- `size`: Bytes to allocate
- `protection`: Protection flags (default: "RWX")

**Returns**: Allocated address

#### memory.free
```lua
memory.free(address: number, size: number) -> void
```
Frees allocated memory.

**Parameters**:
- `address`: Address to free
- `size`: Size of allocation

### Anti-Detection

#### memory.checkenvironment
```lua
memory.checkenvironment() -> boolean
```
Checks if environment is safe.

**Returns**: true if safe

#### memory.isdebugger
```lua
memory.isdebugger() -> boolean
```
Checks if debugger is present.

**Returns**: true if debugger detected

#### memory.isvm
```lua
memory.isvm() -> boolean
```
Checks if running in VM.

**Returns**: true if VM detected

#### memory.enableantidetection
```lua
memory.enableantidetection(enable: boolean) -> void
```
Enables or disables anti-detection.

**Parameters**:
- `enable`: true to enable

### Utility Functions

#### memory.hex2bin
```lua
memory.hex2bin(hex: string) -> string
```
Converts hex string to binary.

**Parameters**:
- `hex`: Hex string (e.g., "48 89 5C")

**Returns**: Binary string

#### memory.bin2hex
```lua
memory.bin2hex(bin: string) -> string
```
Converts binary to hex string.

**Parameters**:
- `bin`: Binary string

**Returns**: Hex string

#### memory.addressof
```lua
memory.addressof(func: function) -> number
```
Gets address of function.

**Parameters**:
- `func`: Function object

**Returns**: Function address

#### memory.pattern2mask
```lua
memory.pattern2mask(pattern: string) -> string, string
```
Converts pattern string to bytes and mask.

**Parameters**:
- `pattern`: Pattern like "48 89 5C ?? 48"

**Returns**: bytes, mask

## C API

### xoron_memory_read
```c
bool xoron_memory_read(uintptr_t addr, void* buffer, size_t size)
```
Reads memory safely.

### xoron_memory_write
```c
bool xoron_memory_write(uintptr_t addr, const void* buffer, size_t size)
```
Writes memory safely.

### xoron_memory_allocate
```c
void* xoron_memory_allocate(size_t size, int prot)
```
Allocates memory.

### xoron_memory_free
```c
void xoron_memory_free(void* ptr, size_t size)
```
Frees memory.

## Platform-Specific Implementations

### iOS
- Uses Mach VM APIs (`vm_read`, `vm_write`, `vm_protect`)
- `sysctl` for debugger/VM detection
- Mach-O dyld for module info

### Android
- Uses `ptrace` for debugger detection
- `/proc/self/maps` for memory scanning
- `mprotect` for protection changes
- ELF parsing for module info

### Linux
- Similar to Android but without Android-specific APIs

## Pattern Syntax

### Pattern String
```
"48 89 5C ?? 48 8B"
```
- Bytes in hex separated by spaces
- `??` or `?` = wildcard

### Mask String
```
"xx?xx"
```
- `x` = match byte
- `?` = wildcard

### Combined
```lua
-- Pattern: 48 89 5C ?? 48
-- Mask:    xx?xx
local matches = memory.findpattern("48 89 5C ?? 48")
```

## Usage Examples

### Basic Memory Reading
```lua
-- Read from address
local addr = 0x12345678
local value = memory.readdword(addr)
print(string.format("Value: 0x%X", value))

-- Read string
local str = memory.readstring(addr, 100)
print("String:", str)
```

### Pattern Scanning
```lua
-- Find all occurrences
local pattern = "48 89 5C ?? 48 8B"
local matches = memory.findpattern(pattern)

print(string.format("Found %d matches", #matches))
for i, addr in ipairs(matches) do
    print(string.format("Match %d: 0x%X", i, addr))
end
```

### Memory Modification
```lua
-- Patch memory
local addr = 0x12345678
memory.writebyte(addr, 0x90)  -- NOP instruction

-- Write string
memory.writestring(addr, "Hello")

-- Restore
memory.writebyte(addr, 0x48)
```

### Anti-Detection
```lua
-- Check environment
if not memory.checkenvironment() then
    print("Suspicious environment detected!")
    return
end

-- Check specifically
if memory.isdebugger() then
    print("Debugger detected!")
end

if memory.isvm() then
    print("Running in VM!")
end
```

### Memory Protection
```lua
-- Make memory executable
local addr = memory.allocate(1024, "RWX")

-- Change protection
memory.setprotection(addr, 1024, "RX")

-- Free
memory.free(addr, 1024)
```

### Advanced Scanning
```lua
-- Scan specific region
local base = 0x10000000
local size = 0x100000
local pattern = "E8 ?? ?? ?? ?? 48"
local matches = memory.scan(base, size, pattern)

-- Convert pattern to bytes
local bytes, mask = memory.pattern2mask("48 89 5C ?? 48")
local matches = memory.scan(base, size, bytes, mask)
```

### Function Hooking
```lua
-- Get function address
local func_addr = memory.addressof(some_function)

-- Read original bytes
local original = memory.read(func_addr, 16)

-- Hook (write jump)
memory.writebyte(func_addr, 0xE9)  -- JMP
-- ... write offset

-- Restore
memory.write(func_addr, original)
```

## Security Considerations

### Anti-Detection Measures
1. **Debugger Detection**: Prevents analysis tools
2. **VM Detection**: Avoids sandboxed environments
3. **Hook Detection**: Detects function hooks
4. **Root Detection**: Checks for elevated privileges

### When to Disable
- Development and debugging
- Testing in controlled environments
- Educational purposes

### Memory Safety
- All operations validate addresses
- Protection changes require proper permissions
- Allocations are tracked and can be freed
- Pattern scanning is read-only

## Performance

- Pattern scanning: O(n*m) where n=memory size, m=pattern length
- Memory reads/writes: Direct with validation
- Anti-detection: One-time check with caching
- Mutex locks: Minimal overhead for thread safety

## Error Handling

All functions return `false` or `nil` on error. Check `xoron_last_error()` for details:

```lua
if not memory.write(addr, data) then
    print("Error:", xoron_last_error())
end
```

## Related Files
- `xoron.h` (API declarations)
- `xoron_luau.cpp` (Lua bindings)
- `xoron_debug.cpp` (debugging utilities)
