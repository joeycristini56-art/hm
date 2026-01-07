# Xoron Project Documentation

**Version**: 2.0.0  
**Last Updated**: 2026-01-06  
**Location**: `src/src/doc/`

## Overview

This directory contains comprehensive manual documentation for all Xoron source files. Each document provides detailed analysis of its corresponding file's purpose, functions, and interdependencies.

## Documentation Structure

### Project Overview
- **[00_PROJECT_OVERVIEW.md](00_PROJECT_OVERVIEW.md)** - High-level project structure, architecture, and goals

### Core Files
- **[01_XORON_H.md](01_XORON_H.md)** - Header file with API declarations and type definitions
- **[02_xoron_http_cpp.md](02_xoron_http_cpp.md)** - HTTP/HTTPS client implementation
- **[03_xoron_crypto_cpp.md](03_xoron_crypto_cpp.md)** - Cryptographic operations (OpenSSL)
- **[04_xoron_env_cpp.md](04_xoron_env_cpp.md)** - Environment functions and executor API
- **[05_xoron_filesystem_cpp.md](05_xoron_filesystem_cpp.md)** - File I/O operations
- **[06_xoron_memory_cpp.md](06_xoron_memory_cpp.md)** - Memory management and scanning
- **[07_xoron_debug_cpp.md](07_xoron_debug_cpp.md)** - Debugging utilities
- **[08_xoron_console_cpp.md](08_xoron_console_cpp.md)** - Console output management
- **[09_xoron_drawing_cpp.md](09_xoron_drawing_cpp.md)** - 2D graphics rendering
- **[10_xoron_websocket_cpp.md](10_xoron_websocket_cpp.md)** - WebSocket client
- **[11_xoron_input_cpp.md](11_xoron_input_cpp.md)** - Input simulation
- **[12_xoron_cache_cpp.md](12_xoron_cache_cpp.md)** - Caching system
- **[13_xoron_ui_cpp.md](13_xoron_ui_cpp.md)** - UI components

### Platform-Specific Files
- **[14_xoron_android_cpp.md](14_xoron_android_cpp.md)** - Android NDK integration
- **[15_xoron_ios_mm.md](15_xoron_ios_mm.md)** - iOS Foundation integration

### Build & Scripts
- **[16_CMakeLists_txt.md](16_CMakeLists_txt.md)** - Build configuration
- **[17_saveinstance_lua.md](17_saveinstance_lua.md)** - RBXMX serializer script

### Integration
- **[18_INTEGRATION_MAP.md](18_INTEGRATION_MAP.md)** - Complete dependency graph

## Quick Reference

### File Statistics
| File | Lines | Size | Purpose |
|------|-------|------|---------|
| xoron.h | 265 | 8.9 KB | Core API |
| xoron_luau.cpp | 496 | 14.6 KB | Lua VM |
| xoron_http.cpp | 380 | 12.2 KB | HTTP client |
| xoron_crypto.cpp | 707 | 20.9 KB | Crypto |
| xoron_env.cpp | ~800 | 53.1 KB | Environment |
| xoron_filesystem.cpp | ~500 | 31.2 KB | File I/O |
| xoron_memory.cpp | ~400 | 19.6 KB | Memory |
| xoron_debug.cpp | 634 | 17.3 KB | Debug |
| xoron_console.cpp | 413 | 11.9 KB | Console |
| xoron_drawing.cpp | 1,116 | 42.8 KB | Graphics |
| xoron_websocket.cpp | 633 | 16.6 KB | WebSocket |
| xoron_input.cpp | 561 | 15.8 KB | Input |
| xoron_cache.cpp | 649 | 19.5 KB | Cache |
| xoron_ui.cpp | 868 | 28.0 KB | UI |
| xoron_android.cpp | 594 | 22.6 KB | Android |
| xoron_ios.mm | 829 | 35.3 KB | iOS |
| CMakeLists.txt | 390 | 15.2 KB | Build |
| saveinstance.lua | 496 | 17.0 KB | Serializer |

**Total**: 17 files, ~11,500 lines, ~400 KB

### Module Categories

#### Core (Required)
- xoron.h
- xoron_luau.cpp
- xoron_env.cpp

#### Network
- xoron_http.cpp
- xoron_websocket.cpp

#### Security
- xoron_crypto.cpp
- xoron_memory.cpp

#### File System
- xoron_filesystem.cpp
- saveinstance.lua

#### Debug & Console
- xoron_debug.cpp
- xoron_console.cpp

#### Graphics & UI
- xoron_drawing.cpp
- xoron_ui.cpp

#### Input
- xoron_input.cpp

#### Caching
- xoron_cache.cpp

#### Platform
- xoron_android.cpp
- xoron_ios.mm

#### Build
- CMakeLists.txt

## Key Features

### Cross-Platform Support
- **iOS**: ARM64 .dylib (iOS 15+)
- **Android**: ARM64 .so (API 29+, Android 10+)
- **Development**: Linux/macOS/Windows for testing

### Core Capabilities
1. **HTTP/HTTPS**: Full client with SSL/TLS
2. **WebSocket**: Real-time bidirectional communication
3. **Cryptography**: Hash, encrypt, decrypt, encode/decode
4. **File I/O**: Read, write, list, manage files
5. **Memory**: Scanning, pattern matching, anti-detection
6. **Debugging**: Stack traces, error handling, diagnostics
7. **Console**: Formatted output, history, channels
8. **Graphics**: 2D drawing primitives
9. **UI**: Native components (buttons, inputs, etc.)
10. **Input**: Mouse, keyboard, touch simulation
11. **Caching**: TTL, eviction policies, size limits
12. **Environment**: Global access, hooking, utilities
13. **Serialization**: RBXMX format for Roblox

### Lua API
All features exposed to Lua with comprehensive bindings:
```lua
-- HTTP
http.get("https://api.example.com")

-- Crypto
crypt.hash("sha256", "data")

-- Filesystem
writefile("test.txt", "content")

-- Environment
getgenv().myVar = "value"

-- Debug
debug.traceback()

-- Drawing
local line = Drawing.new("Line")

-- WebSocket
websocket.connect("wss://echo.websocket.org")

-- UI
ui.create_window("App", 800, 600)

-- Platform-specific
android.get_api_level()  -- or ios.get_device_info()
```

## Build Instructions

### iOS Build
```bash
cmake -S . -B build \
  -DXORON_IOS_BUILD=ON \
  -DXORON_OPENSSL_ROOT=/path/to/openssl \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/ios-toolchain.cmake

cmake --build build --config Release
```

### Android Build
```bash
cmake -S . -B build \
  -DXORON_ANDROID_BUILD=ON \
  -DXORON_ANDROID_ABI=arm64-v8a \
  -DXORON_ANDROID_PLATFORM=android-29 \
  -DANDROID_NDK_HOME=/path/to/ndk \
  -DXORON_OPENSSL_ROOT=/path/to/openssl \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake

cmake --build build --config Release
```

### Development Build
```bash
cmake -S . -B build
cmake --build build
```

## Dependencies

### Required
- CMake 3.16+
- C++17 compiler
- OpenSSL (for mobile builds)
- Android NDK (for Android)
- Xcode (for iOS)

### Bundled (via FetchContent)
- Luau 0.607 (Roblox Lua VM)
- cpp-httplib 0.14.3 (HTTP client)
- LZ4 1.9.4 (Compression)

## Architecture Highlights

### Layered Design
```
User Scripts (Lua)
    ↓
Lua VM (Luau)
    ↓
C++ Bindings (xoron_luau.cpp)
    ↓
Module Functions (xoron_*.cpp)
    ↓
Platform APIs (iOS/Android)
```

### Error Handling
- C++: `xoron_set_error()` with format strings
- Lua: `pcall()` and `xpcall()`
- Platform: Platform-specific error reporting

### Thread Safety
- Mutex protection for shared state
- Platform main thread for UI
- Background threads for I/O

### Memory Management
- C++: RAII, smart pointers
- Lua: Garbage collection
- Manual: `xoron_http_free()`, `xoron_memory_free()`

## Documentation Style

Each file documentation includes:
1. **Header**: File path, size, lines, platform
2. **Overview**: Purpose and responsibilities
3. **Includes**: Dependencies
4. **Core Functions**: API details
5. **Lua API**: Exposed functions
6. **Usage Examples**: Practical code
7. **Implementation Details**: How it works
8. **Platform Notes**: Specific considerations
9. **Related Files**: Integration points

## Reading Order

For new developers, recommended reading order:

1. **00_PROJECT_OVERVIEW.md** - Understand the big picture
2. **01_XORON_H.md** - Learn the API structure
3. **18_INTEGRATION_MAP.md** - See how files connect
4. **02_xoron_http_cpp.md** - Study a complete module
5. **04_xoron_env_cpp.md** - Understand environment
6. **16_CMakeLists_txt.md** - Learn build system
7. **17_saveinstance_lua.md** - See Lua integration
8. **14_xoron_android_cpp.md** / **15_xoron_ios_mm.md** - Platform specifics

## Maintenance

### Adding Features
1. Design API in `xoron.h`
2. Implement in new or existing module
3. Register in `xoron_luau.cpp`
4. Update `CMakeLists.txt` if needed
5. Document in appropriate .md file
6. Update `18_INTEGRATION_MAP.md`

### Updating Documentation
1. Keep all .md files synchronized
2. Update line counts and sizes
3. Update integration map
4. Update README if structure changes

### Testing
- Unit tests for each module
- Integration tests for cross-module features
- Platform-specific tests
- Performance benchmarks

## Support

For questions about specific files, refer to:
- Individual module documentation (02-15)
- Integration map (18) for dependencies
- Project overview (00) for architecture

## License

This documentation is part of the Xoron project. See project license for details.

---

**Generated**: 2026-01-06  
**Total Files**: 19 documents  
**Total Pages**: ~500+ pages of documentation
