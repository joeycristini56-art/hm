# Xoron Executor Engine

## Overview

**Xoron** is a high-performance, cross-platform Luau executor engine designed for mobile platforms (iOS 15+ and Android 10+). It provides a comprehensive set of APIs for script execution, HTTP requests, cryptography, WebSocket communication, real-time drawing, and more.

### Platform Support

- **iOS**: iOS 15.0+ (ARM64) - `.dylib` shared library
- **Android**: Android 10+ (API 29+) - `.so` shared library (arm64-v8a, armeabi-v7a, x86, x86_64)

### Version

**Current Version**: 2.0.0

## Architecture

```
project/
├── src/                    # Core source code
│   ├── xoron.h            # Main header with API definitions
│   ├── xoron_luau.cpp     # Luau VM wrapper and execution
│   ├── xoron_http.cpp     # HTTP client (cpp-httplib + OpenSSL)
│   ├── xoron_crypto.cpp   # Cryptographic functions (OpenSSL)
│   ├── xoron_websocket.cpp # WebSocket client
│   ├── xoron_drawing.cpp  # Real-time rendering engine
│   ├── xoron_env.cpp      # Custom Lua environment (getgenv, hooks, etc.)
│   ├── xoron_filesystem.cpp # File system operations
│   ├── xoron_memory.cpp   # Memory management utilities
│   ├── xoron_console.cpp  # Console output handling
│   ├── xoron_input.cpp    # Input handling
│   ├── xoron_cache.cpp    # Data caching
│   ├── xoron_ui.cpp       # UI management
│   ├── xoron_android.cpp  # Android-specific implementations
│   ├── xoron_ios.mm       # iOS-specific implementations
│   ├── CMakeLists.txt     # Build configuration
│   ├── lua/               # Lua scripts (saveinstance, etc.)
│   ├── tests/             # Integration tests
│   └── workflows/         # CI/CD workflows
├── docs/                  # Comprehensive documentation
└── .github/               # GitHub workflows
```

## Key Features

### Core Execution
- **Luau VM Integration**: Full Luau 0.607 support with compiler and runtime
- **Script Compilation**: Compile and execute Luau scripts from source
- **Bytecode Support**: Load and run pre-compiled bytecode
- **Error Handling**: Comprehensive error reporting and stack traces

### Networking
- **HTTP Client**: Full HTTP/HTTPS support with GET, POST, and custom methods
- **WebSocket**: Real-time bidirectional communication with event callbacks
- **TLS/SSL**: OpenSSL-based secure connections
- **Timeout Control**: Configurable connection and read timeouts

### Cryptography
- **Hash Functions**: SHA256, SHA384, SHA512, MD5
- **Encoding**: Base64 encode/decode, Hex encode/decode
- **AES Encryption**: CBC and GCM modes
- **HMAC**: Keyed-hash message authentication
- **Random**: Secure random number generation

### Graphics & UI
- **Drawing API**: Real-time 2D rendering (lines, circles, squares, text, triangles, quads)
- **Overlay System**: Overlay rendering on iOS and Android
- **UI Management**: Native UI components and controls
- **Haptic Feedback**: Vibration/haptic feedback on mobile devices

### Environment & Security
- **Custom Lua Environment**: getgenv, getrenv, getsenv, getmenv
- **Function Hooking**: Hook and replace Lua functions
- **Signal Connections**: Manage event connections
- **Anti-Detection**: Environment detection and obfuscation
- **Clipboard**: Cross-platform clipboard operations

### File System
- **Workspace Management**: Configurable workspace paths
- **File Operations**: Read, write, append, delete
- **Auto-Execute**: Automatic script execution on startup
- **Script Management**: Organized script storage

## Quick Start

### Building for iOS

```bash
# Prerequisites: Xcode, CMake 3.16+
mkdir build && cd build
cmake ../src \
  -DXORON_IOS_BUILD=ON \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/ios.toolchain.cmake \
  -DPLATFORM=OS64 \
  -DXORON_OPENSSL_ROOT=/path/to/openssl-ios
cmake --build . --config Release
```

### Building for Android

```bash
# Prerequisites: Android NDK, CMake 3.16+
export ANDROID_NDK_HOME=/path/to/ndk
mkdir build && cd build
cmake ../src \
  -DXORON_ANDROID_BUILD=ON \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-29 \
  -DXORON_OPENSSL_ROOT=/path/to/openssl-android
cmake --build . --config Release
```

### Using the Library

```c
#include "xoron.h"

// Initialize
xoron_init();

// Create VM
xoron_vm_t* vm = xoron_vm_new();

// Execute script
const char* script = "print('Hello from Xoron!')";
int result = xoron_dostring(vm, script, "test");

// HTTP request
int status; size_t len;
char* response = xoron_http_get("https://api.example.com/data", &status, &len);
if (response) {
    printf("Response: %s\n", response);
    xoron_http_free(response);
}

// Cleanup
xoron_vm_free(vm);
xoron_shutdown();
```

## Documentation Structure

This project includes comprehensive documentation:

- **[src/README.md](src/README.md)** - Detailed source code documentation
- **[docs/](docs/)** - Complete API reference and architecture documentation
- **[src/tests/](src/tests/)** - Integration tests and test utilities
- **[src/lua/](src/lua/)** - Lua utility scripts
- **[src/workflows/](src/workflows/)** - CI/CD pipeline documentation

## API Reference

### Core API
```c
int xoron_init(void);
void xoron_shutdown(void);
const char* xoron_version(void);
const char* xoron_last_error(void);
```

### VM API
```c
xoron_vm_t* xoron_vm_new(void);
void xoron_vm_free(xoron_vm_t* vm);
void xoron_vm_reset(xoron_vm_t* vm);
```

### Execution API
```c
int xoron_run(xoron_vm_t* vm, xoron_bytecode_t* bc);
int xoron_dostring(xoron_vm_t* vm, const char* source, const char* name);
int xoron_dofile(xoron_vm_t* vm, const char* path);
```

### HTTP API
```c
char* xoron_http_get(const char* url, int* status, size_t* len);
char* xoron_http_post(const char* url, const char* body, size_t body_len, 
                      const char* content_type, int* status, size_t* len);
void xoron_http_free(char* response);
```

### Crypto API
```c
void xoron_sha256(const void* data, size_t len, uint8_t out[32]);
void xoron_base64_encode(const void* data, size_t len);
uint8_t* xoron_base64_decode(const char* str, size_t* out_len);
```

## Dependencies

### Required Dependencies
- **Luau**: Roblox's Luau language (VM + Compiler)
- **OpenSSL**: Cryptography and TLS support (3.2.0+)
- **cpp-httplib**: HTTP/HTTPS client
- **LZ4**: Fast compression
- **Threads**: Multi-threading support

### Platform-Specific Dependencies
- **iOS**: Foundation, CoreFoundation, CoreGraphics, CoreText, Security, UIKit, WebKit, QuartzCore, libobjc
- **Android**: log, android (NDK libraries)

## Testing

The project includes comprehensive integration tests for both platforms:

```bash
# Android tests
cd src/tests/android
./run_tests.sh

# iOS tests
cd src/tests/ios
./run_tests.sh
```

See [src/tests/README.md](src/tests/README.md) for detailed test documentation.

## CI/CD

Automated build and test workflows are provided:

- **build-android.yml**: Builds Android .so library
- **build-ios.yml**: Builds iOS .dylib library
- **test-android.yml**: Runs Android integration tests
- **test-ios.yml**: Runs iOS integration tests

See [src/workflows/README.md](src/workflows/README.md) for workflow details.

## Lua Scripts

### SaveInstance
The `saveinstance.lua` script provides Roblox instance serialization to RBXMX format:

```lua
local SaveInstance = loadstring(game:HttpGet("https://.../saveinstance.lua"))()
SaveInstance.Save({
    FileName = "my_game",
    DecompileScripts = false,
    Mode = "optimized"
})
```

See [src/lua/README.md](src/lua/README.md) for complete documentation.

## Security Considerations

- **Anti-Detection**: Built-in anti-detection mechanisms
- **Environment Validation**: Check for safe execution environment
- **Secure Crypto**: All cryptographic operations use OpenSSL
- **Memory Safety**: RAII-based memory management
- **Thread Safety**: Mutex-protected global state

## Performance

- **Fast Compilation**: Luau compiler optimized for mobile
- **Low Memory**: Efficient memory management with LZ4 compression
- **Fast I/O**: Optimized file system operations
- **Real-time**: 60 FPS drawing capability
- **Network**: Non-blocking HTTP/WebSocket operations

## License

This project is provided for educational and research purposes.

## Support

For issues, questions, or contributions:
- Check the documentation in `docs/`
- Review test examples in `src/tests/`
- Examine workflow configurations in `src/workflows/`

## Changelog

### Version 2.0.0
- Complete rewrite with modular architecture
- Added WebSocket support
- Enhanced drawing engine
- Improved crypto API
- Better error handling
- Cross-platform CI/CD
- Comprehensive documentation

---

**Built with ❤️ for iOS and Android**
