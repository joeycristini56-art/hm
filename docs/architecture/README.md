# Xoron Architecture Overview

## System Architecture

Xoron is designed as a **layered, modular architecture** optimized for mobile platforms (iOS and Android). The system is organized into distinct layers with clear responsibilities and minimal coupling.

### Architecture Layers

```
┌─────────────────────────────────────────────────────────────┐
│                    Application Layer                        │
│  (User Scripts, Lua Code, External Applications)            │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                    Lua API Layer                            │
│  (Lua Libraries: HTTP, Crypto, Drawing, WebSocket, etc.)    │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                    Core Engine Layer                        │
│  (Luau VM, Execution Engine, Environment Management)        │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                    Service Layer                            │
│  (HTTP, Crypto, Filesystem, Console, etc.)                  │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                    Platform Abstraction Layer               │
│  (iOS/Android Specific Implementations)                     │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                    Native Platform Layer                    │
│  (iOS SDK, Android NDK, System Libraries)                   │
└─────────────────────────────────────────────────────────────┘
```

## Component Architecture

### Core Components

#### 1. **Execution Engine** (`xoron_luau.cpp`)
- **Responsibility**: Script compilation and execution
- **Dependencies**: Luau VM, Compiler, Service Layer
- **Key Features**:
  - VM lifecycle management
  - Bytecode compilation
  - Runtime execution
  - Error handling and reporting

#### 2. **Environment Manager** (`xoron_env.cpp`)
- **Responsibility**: Lua environment customization
- **Dependencies**: Luau VM, Platform Abstraction
- **Key Features**:
  - Custom global environments
  - Function hooking
  - Signal management
  - Anti-detection mechanisms

#### 3. **HTTP Client** (`xoron_http.cpp`)
- **Responsibility**: Network communication
- **Dependencies**: cpp-httplib, OpenSSL
- **Key Features**:
  - HTTP/HTTPS support
  - GET/POST methods
  - Connection pooling
  - Timeout management

#### 4. **Crypto Engine** (`xoron_crypto.cpp`)
- **Responsibility**: Cryptographic operations
- **Dependencies**: OpenSSL
- **Key Features**:
  - Hash functions
  - Encoding/decoding
  - Encryption/decryption
  - Secure random generation

#### 5. **WebSocket Client** (`xoron_websocket.cpp`)
- **Responsibility**: Real-time communication
- **Dependencies**: POSIX sockets, OpenSSL
- **Key Features**:
  - WebSocket protocol
  - Async message handling
  - Event callbacks
  - TLS support

#### 6. **Drawing Engine** (`xoron_drawing.cpp`)
- **Responsibility**: 2D rendering
- **Dependencies**: Platform-specific graphics
- **Key Features**:
  - Shape primitives
  - Text rendering
  - Layer management
  - Platform backends

#### 7. **File System** (`xoron_filesystem.cpp`)
- **Responsibility**: File operations
- **Dependencies**: Standard C I/O
- **Key Features**:
  - Workspace management
  - File read/write
  - Path resolution
  - Auto-execute

#### 8. **Console Manager** (`xoron_console.cpp`)
- **Responsibility**: Output handling
- **Dependencies**: Platform logging
- **Key Features**:
  - Output callbacks
  - Platform logging
  - Message formatting
  - Thread safety

#### 9. **Memory Manager** (`xoron_memory.cpp`)
- **Responsibility**: Memory utilities
- **Dependencies**: LZ4, Standard C
- **Key Features**:
  - Allocation tracking
  - Compression
  - Pattern verification
  - Safety checks

#### 10. **Input Handler** (`xoron_input.cpp`)
- **Responsibility**: Input event management
- **Dependencies**: Platform input systems
- **Key Features**:
  - Touch events
  - Key events
  - Event callbacks
  - State tracking

#### 11. **Cache Manager** (`xoron_cache.cpp`)
- **Responsibility**: Data caching
- **Dependencies**: Standard containers
- **Key Features**:
  - Key-value storage
  - TTL support
  - Memory limits
  - Persistence

#### 12. **UI Manager** (`xoron_ui.cpp`)
- **Responsibility**: UI component management
- **Dependencies**: Platform UI frameworks
- **Key Features**:
  - Component creation
  - Event handling
  - Layout management
  - Platform integration

### Platform-Specific Components

#### 13. **iOS Bridge** (`xoron_ios.mm`)
- **Responsibility**: iOS platform integration
- **Dependencies**: iOS SDK, Objective-C runtime
- **Key Features**:
  - UIKit integration
  - CoreGraphics rendering
  - Haptic feedback
  - Clipboard operations

#### 14. **Android Bridge** (`xoron_android.cpp`)
- **Responsibility**: Android platform integration
- **Dependencies**: Android NDK, JNI
- **Key Features**:
  - JNI bridge
  - Android logging
  - Native UI
  - System properties

## Data Flow

### Script Execution Flow

```
User Script (Lua)
    ↓
xoron_dostring() / xoron_dofile()
    ↓
Luau Compiler (Bytecode Generation)
    ↓
xoron_run() / VM Execution
    ↓
Lua Libraries (HTTP, Crypto, etc.)
    ↓
Service Layer (C++ Implementation)
    ↓
Platform Abstraction
    ↓
Native Platform (iOS/Android)
    ↓
Results → Lua VM → User
```

### HTTP Request Flow

```
Lua: http.get("https://example.com")
    ↓
xoron_luau.cpp: lua_http_get()
    ↓
xoron_http.cpp: xoron_http_get()
    ↓
cpp-httplib: Client::Get()
    ↓
OpenSSL: SSL_connect() (if HTTPS)
    ↓
Network: TCP/TLS
    ↓
Response → xoron_http_get() → lua_http_get() → Lua
```

### Drawing Flow

```
Lua: Drawing:Line(x1, y1, x2, y2, color, thickness)
    ↓
xoron_luau.cpp: lua_drawing_line()
    ↓
xoron_drawing.cpp: Add DrawingObject to queue
    ↓
Render Thread: Sort by z-index
    ↓
Platform Renderer:
    - iOS: CGContextDrawLine()
    - Android: Canvas.drawLine()
    ↓
Screen Display
```

## Dependency Graph

```
xoron_luau.cpp (Central Hub)
    ├── xoron.h (Definitions)
    ├── luau (VM + Compiler)
    │   ├── lua.h
    │   ├── lualib.h
    │   └── luacode.h
    ├── xoron_http.cpp
    │   └── httplib.h
    ├── xoron_crypto.cpp
    │   └── OpenSSL (sha.h, evp.h, etc.)
    ├── xoron_websocket.cpp
    │   ├── sys/socket.h
    │   └── OpenSSL (ssl.h)
    ├── xoron_drawing.cpp
    │   ├── CoreGraphics (iOS)
    │   └── Android Canvas (Android)
    ├── xoron_env.cpp
    │   ├── LZ4
    │   └── Platform APIs
    ├── xoron_filesystem.cpp
    ├── xoron_memory.cpp
    │   └── LZ4
    ├── xoron_console.cpp
    ├── xoron_input.cpp
    ├── xoron_cache.cpp
    ├── xoron_ui.cpp
    ├── xoron_android.cpp (JNI)
    └── xoron_ios.mm (Obj-C)
```

## Design Patterns

### 1. **RAII (Resource Acquisition Is Initialization)**
- VM lifecycle management
- File handle management
- Memory allocation

### 2. **Factory Pattern**
- Drawing object creation
- VM instance creation
- Connection management

### 3. **Observer Pattern**
- WebSocket event callbacks
- Signal connections
- Input event handling

### 4. **Singleton Pattern**
- Global state management
- Cache manager
- Environment registry

### 5. **Strategy Pattern**
- Platform-specific rendering
- Encryption algorithms
- Network protocols

## Concurrency Model

### Thread Safety

```
Main Thread (Lua Execution)
    ├── VM Operations (Mutex-protected)
    ├── HTTP Requests (Async)
    ├── Drawing Queue (Thread-safe)
    └── Console Output (Mutex-protected)

Background Threads
    ├── WebSocket Receiver
    ├── Drawing Renderer
    └── Async File I/O
```

### Synchronization Primitives
- `std::mutex` - Global state protection
- `std::atomic` - Counter and flags
- `std::condition_variable` - Event signaling
- `std::lock_guard` - RAII locking

## Memory Management

### Allocation Strategy

```
Lua VM (Managed by Luau)
    ├── Bytecode (Arena allocation)
    ├── Runtime data (GC)
    └── Strings (Interned)

C++ Layer (Manual + RAII)
    ├── Service objects (Smart pointers)
    ├── Buffers (std::vector, std::string)
    └── Platform resources (RAII wrappers)

Platform Layer (Platform-managed)
    ├── iOS: ARC (Objective-C)
    ├── Android: JNI local/global refs
    └── Graphics: Platform allocators
```

### Memory Safety Features
1. **Bounds checking**: Standard containers
2. **Null checks**: All pointer operations
3. **Exception safety**: RAII and try-catch
4. **Resource cleanup**: Destructors
5. **Compression**: LZ4 for large data

## Security Architecture

### Layers of Security

```
┌─────────────────────────────────────┐
│  Application Layer Security         │
│  - Script validation                │
│  - Input sanitization               │
└─────────────────────────────────────┘
         ↓
┌─────────────────────────────────────┐
│  Environment Security               │
│  - Anti-detection                   │
│  - Hook protection                  │
│  - Environment isolation            │
└─────────────────────────────────────┘
         ↓
┌─────────────────────────────────────┐
│  Network Security                   │
│  - TLS/SSL                          │
│  - Certificate validation           │
│  - Timeout protection               │
└─────────────────────────────────────┘
         ↓
┌─────────────────────────────────────┐
│  Cryptographic Security             │
│  - OpenSSL implementation           │
│  - Secure random generation         │
│  - Key management                   │
└─────────────────────────────────────┘
         ↓
┌─────────────────────────────────────┐
│  Memory Security                    │
│  - Bounds checking                  │
│  - Use-after-free prevention        │
│  - Memory leak detection            │
└─────────────────────────────────────┘
```

### Security Features

1. **Environment Validation**
   - `xoron_check_environment()`
   - Detects debuggers, hooks
   - Validates execution context

2. **Anti-Detection**
   - `xoron_enable_anti_detection()`
   - Obfuscates signatures
   - Hides from detection tools

3. **Secure Crypto**
   - All operations via OpenSSL
   - No custom crypto implementations
   - Constant-time operations where needed

4. **Network Security**
   - TLS 1.2+ support
   - Certificate verification (optional)
   - Timeout protection

## Performance Characteristics

### Time Complexity

| Operation | Complexity | Notes |
|-----------|------------|-------|
| Script compilation | O(n) | Linear with source size |
| HTTP request | O(1) | Network dependent |
| Crypto hash | O(n) | Linear with data size |
| Drawing render | O(m) | m = number of objects |
| Cache lookup | O(1) | Hash table |
| File I/O | O(n) | Linear with file size |

### Space Complexity

| Component | Complexity | Notes |
|-----------|------------|-------|
| VM instance | O(1) | Fixed overhead |
| Script bytecode | O(n) | Linear with source |
| HTTP buffer | O(n) | Response size |
| Drawing queue | O(m) | Active objects |
| Cache | O(k) | Configurable limit |

### Optimization Strategies

1. **Lazy Loading**: Libraries loaded on first use
2. **Connection Pooling**: HTTP client reuse
3. **Bytecode Caching**: Compilation results cached
4. **Batch Rendering**: Drawing operations batched
5. **Compression**: LZ4 for cached data

## Platform Abstraction

### Conditional Compilation

```cpp
#if defined(XORON_PLATFORM_IOS)
    // iOS-specific code
    #include <Foundation/Foundation.h>
#elif defined(XORON_PLATFORM_ANDROID)
    // Android-specific code
    #include <android/log.h>
#else
    // Fallback/development
    #include <cstdio>
#endif
```

### Platform Detection

```cpp
// In xoron.h
#if defined(__APPLE__) && TARGET_OS_IPHONE
    #define XORON_PLATFORM_IOS 1
#elif defined(__ANDROID__)
    #define XORON_PLATFORM_ANDROID 1
#else
    #define XORON_PLATFORM_UNKNOWN 1
#endif
```

### Feature Availability

| Feature | iOS | Android | Notes |
|---------|-----|---------|-------|
| HTTP/HTTPS | ✓ | ✓ | cpp-httplib |
| WebSocket | ✓ | ✓ | POSIX sockets |
| Crypto | ✓ | ✓ | OpenSSL |
| Drawing | ✓ | ✓ | Platform-specific |
| UI | ✓ | ✓ | UIKit/Android |
| Haptic | ✓ | ✓ | CoreHaptics/Vibrator |
| Clipboard | ✓ | ✓ | Platform APIs |

## Testing Architecture

### Test Layers

```
Unit Tests (Component-level)
    ├── VM tests
    ├── Crypto tests
    ├── HTTP tests
    └── Drawing tests

Integration Tests (End-to-end)
    ├── Script execution
    ├── Network operations
    ├── File operations
    └── Platform features

Platform Tests
    ├── iOS integration
    ├── Android integration
    └── Cross-platform compatibility
```

### Test Utilities

See `src/tests/common/test_utils.h` for:
- Test suite framework
- Assertion utilities
- Timing measurements
- Memory verification

## Build Architecture

### CMake Structure

```
CMakeLists.txt (Root)
    ├── Platform Detection
    ├── Dependency Resolution
    │   ├── Luau (FetchContent)
    │   ├── cpp-httplib (FetchContent)
    │   ├── LZ4 (FetchContent)
    │   └── OpenSSL (System/External)
    ├── Library Definition
    │   ├── Source files
    │   ├── Include directories
    │   └── Link libraries
    └── Platform Configuration
        ├── iOS frameworks
        ├── Android libraries
        └── Compiler flags
```

### Build Variants

1. **iOS Build**
   - Toolchain: iOS CMake toolchain
   - Output: libxoron.dylib
   - Architectures: arm64
   - Deployment: iOS 15.0+

2. **Android Build**
   - Toolchain: Android NDK
   - Output: libxoron.so
   - Architectures: arm64-v8a, armeabi-v7a, x86, x86_64
   - API Level: 29+ (Android 10+)

3. **Development Build**
   - Native compiler
   - For testing on host
   - Limited platform features

## Extensibility

### Adding New Features

1. **Service Layer**: Add new `.cpp` file
2. **Lua Binding**: Register in `xoron_luau.cpp`
3. **Platform Support**: Add to `xoron_android.cpp` / `xoron_ios.mm`
4. **Documentation**: Update API docs
5. **Tests**: Add integration tests

### Extension Points

- **New Lua Libraries**: Register in `xoron_register_*()` functions
- **New Drawing Primitives**: Extend `DrawingObject` and renderer
- **New Crypto Functions**: Add to `xoron_crypto.cpp`
- **New Network Protocols**: Extend HTTP/WebSocket
- **New UI Components**: Add to `xoron_ui.cpp`

## Future Architecture

### Planned Enhancements

1. **Plugin System**
   - Dynamic library loading
   - Runtime plugin registration
   - Version compatibility

2. **Advanced Graphics**
   - Shader support
   - 3D rendering
   - GPU acceleration

3. **Performance Monitoring**
   - Profiling hooks
   - Memory tracking
   - Performance metrics

4. **Security Enhancements**
   - Script signing
   - Runtime verification
   - Sandboxing

## References

- **Luau**: https://github.com/Roblox/luau
- **cpp-httplib**: https://github.com/yhirose/cpp-httplib
- **OpenSSL**: https://www.openssl.org/
- **LZ4**: https://github.com/lz4/lz4
- **iOS SDK**: Apple Developer Documentation
- **Android NDK**: Android Developer Documentation

---

**Next Reading:**
- [Component Model](component_model.md) - Detailed component relationships
- [Execution Flow](execution_flow.md) - Script execution details
- [Memory Model](memory_model.md) - Memory management
- [Security Model](security_model.md) - Security architecture
