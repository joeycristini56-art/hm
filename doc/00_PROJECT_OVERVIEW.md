# Xoron Executor - Project Overview

## Project Description
Xoron is a full-featured Luau executor engine designed for mobile platforms (iOS 15+ and Android 10+). It provides a comprehensive set of APIs for script execution, HTTP requests, cryptography, WebSocket communication, drawing, and more.

## Platform Support
- **iOS**: iOS 15.0+ (ARM64) - Produces `.dylib` shared libraries
- **Android**: Android 10+ (API 29+) - Produces `.so` shared libraries
- **Development**: Can be built for testing on host machines

## Architecture
The project follows a modular architecture with separate components for different functionalities.

### Core Components
1. **xoron.h** - Main header file with API declarations and platform detection
2. **xoron_luau.cpp** - Luau VM integration and script execution
3. **xoron_http.cpp** - HTTP client functionality
4. **xoron_crypto.cpp** - Cryptographic operations
5. **xoron_filesystem.cpp** - File system operations
6. **xoron_memory.cpp** - Memory management utilities
7. **xoron_debug.cpp** - Debugging and diagnostics
8. **xoron_console.cpp** - Console output management
9. **xoron_drawing.cpp** - Graphics rendering and drawing
10. **xoron_websocket.cpp** - WebSocket client/server
11. **xoron_input.cpp** - Input handling
12. **xoron_cache.cpp** - Caching mechanisms
13. **xoron_ui.cpp** - User interface components
14. **xoron_env.cpp** - Environment and configuration management

### Platform-Specific Components
- **xoron_android.cpp** - Android-specific implementations (JNI, Android NDK)
- **xoron_ios.mm** - iOS-specific implementations (Objective-C++, native APIs)

### Script Components
- **lua/saveinstance.lua** - Roblox instance serialization to RBXMX format

## Build System
The project uses CMake (v3.16+) with the following features:
- Automatic dependency fetching (Luau, cpp-httplib, LZ4)
- Optional OpenSSL integration for crypto
- Platform-specific compiler flags and optimizations
- Cross-compilation support for mobile platforms

## Key Features
- **Luau Script Execution**: Full Luau VM integration with compilation and execution
- **HTTP Client**: GET/POST requests with SSL/TLS support
- **Cryptography**: SHA-256/384/512, MD5, Base64, Hex encoding
- **WebSocket**: Real-time bidirectional communication
- **Drawing**: Graphics rendering capabilities
- **File System**: Workspace management and file operations
- **Memory Management**: Safe memory operations and leak prevention
- **Debug Tools**: Diagnostics, error handling, and logging
- **Console**: Output management with multiple levels (print, warn, error)
- **Input Handling**: User input capture and processing
- **Cache System**: Data caching and retrieval
- **UI Components**: Native UI integration for both platforms
- **Environment Detection**: Security and anti-detection features

## File Structure
```
/workspace/project/src/src/
├── xoron.h                    # Main header (API declarations)
├── xoron_luau.cpp            # Luau VM integration
├── xoron_http.cpp            # HTTP client
├── xoron_crypto.cpp          # Cryptographic operations
├── xoron_filesystem.cpp      # File system operations
├── xoron_memory.cpp          # Memory management
├── xoron_debug.cpp           # Debugging utilities
├── xoron_console.cpp         # Console output
├── xoron_drawing.cpp         # Graphics rendering
├── xoron_websocket.cpp       # WebSocket client
├── xoron_input.cpp           # Input handling
├── xoron_cache.cpp           # Caching system
├── xoron_ui.cpp              # UI components
├── xoron_env.cpp             # Environment management
├── xoron_android.cpp         # Android-specific code
├── xoron_ios.mm              # iOS-specific code
├── CMakeLists.txt            # Build configuration
├── lua/
│   └── saveinstance.lua      # RBXMX serializer
└── doc/                      # Documentation (this folder)
```

## Dependencies
- **Luau** (v0.607) - Roblox's Lua VM
- **cpp-httplib** (v0.14.3) - HTTP client library
- **LZ4** (v1.9.4) - Compression library
- **OpenSSL** (3.2.0) - Cryptography (optional but recommended)
- **ZLIB** - Compression
- **Threads** - Multi-threading support

## Build Instructions

### iOS Build
```bash
cmake -S . -B build -DXORON_IOS_BUILD=ON -DXORON_OPENSSL_ROOT=/path/to/openssl
cmake --build build
```

### Android Build
```bash
cmake -S . -B build \
  -DXORON_ANDROID_BUILD=ON \
  -DXORON_ANDROID_ABI=arm64-v8a \
  -DXORON_ANDROID_PLATFORM=android-29 \
  -DANDROID_NDK_HOME=/path/to/ndk \
  -DXORON_OPENSSL_ROOT=/path/to/openssl
cmake --build build
```

### Development Build
```bash
cmake -S . -B build
cmake --build build
```

## API Usage Example
```c
// Initialize
xoron_init();

// Create VM
xoron_vm_t* vm = xoron_vm_new();

// Compile and run
xoron_bytecode_t* bc = xoron_compile("print('Hello, Xoron!')", 24, "test");
xoron_run(vm, bc);

// Cleanup
xoron_bytecode_free(bc);
xoron_vm_free(vm);
xoron_shutdown();
```

## Documentation Files
This documentation library includes:
1. **00_PROJECT_OVERVIEW.md** - This file
2. **01_XORON_H.md** - Header file and API documentation
3. **02_xoron_luau_cpp.md** - Luau VM integration
4. **03_xoron_http_cpp.md** - HTTP client
5. **04_xoron_crypto_cpp.md** - Cryptographic operations
6. **05_xoron_filesystem_cpp.md** - File system operations
7. **06_xoron_memory_cpp.md** - Memory management
8. **07_xoron_debug_cpp.md** - Debugging utilities
9. **08_xoron_console_cpp.md** - Console output
10. **09_xoron_drawing_cpp.md** - Graphics rendering
11. **10_xoron_websocket_cpp.md** - WebSocket client
12. **11_xoron_input_cpp.md** - Input handling
13. **12_xoron_cache_cpp.md** - Caching system
14. **13_xoron_ui_cpp.md** - UI components
15. **14_xoron_env_cpp.md** - Environment management
16. **15_xoron_android_cpp.md** - Android-specific code
17. **16_xoron_ios_mm.md** - iOS-specific code
18. **17_CMAKELISTS_txt.md** - Build configuration
19. **18_lua_saveinstance_lua.md** - Lua script documentation
20. **19_INTEGRATION_MAP.md** - File interdependencies and call graph
