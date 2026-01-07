# Platform Documentation

## Overview

Xoron is designed as a cross-platform executor engine supporting both iOS and Android. This documentation covers platform-specific implementations, integration guides, and cross-platform considerations.

## Platform Comparison

| Feature | iOS | Android | Notes |
|---------|-----|---------|-------|
| **Minimum Version** | iOS 15.0 | Android 10 (API 29) | Both require 64-bit |
| **Library Format** | `.dylib` | `.so` | Dynamic libraries |
| **Language** | C++ + Objective-C++ | C++ + JNI | Platform-specific code |
| **UI Framework** | UIKit | Android Views | Native UI integration |
| **Graphics** | CoreGraphics | Canvas | 2D rendering |
| **Haptics** | CoreHaptics | Vibrator | Haptic feedback |
| **Crypto** | OpenSSL | OpenSSL | Same implementation |
| **Network** | cpp-httplib | cpp-httplib | Same implementation |
| **Lua VM** | Luau | Luau | Same implementation |
| **File System** | Sandbox | Internal/External | Different paths |
| **Build System** | Xcode/CMake | Gradle/CMake | CMake common layer |

## Platform-Specific Guides

### iOS Integration

**Target**: iOS 15.0+ (iPhone/iPad)

**Key Characteristics**:
- **Sandboxed**: Apps run in isolated environment
- **ARC**: Automatic Reference Counting for Objective-C
- **Frameworks**: Rich system framework ecosystem
- **Security**: Strict app review process

**Integration Methods**:
1. **Dynamic Library**: Embed libxoron.dylib
2. **Static Linking**: Link as static library
3. **Framework**: Package as framework

**Requirements**:
- Xcode 14+
- iOS SDK 15+
- iOS deployment target 15.0+
- OpenSSL for iOS

**See Also**: [iOS Detailed Guide](ios.md)

---

### Android Integration

**Target**: Android 10+ (API 29+)

**Key Characteristics**:
- **JNI**: Java Native Interface for C++ integration
- **NDK**: Native Development Kit
- **Permissions**: Runtime permission model
- **Fragmentation**: Multiple devices/versions

**Integration Methods**:
1. **Native Library**: Load libxoron.so via System.loadLibrary
2. **JNI Bridge**: Java wrapper for native functions
3. **AAR Package**: Android Archive with native libs

**Requirements**:
- Android Studio 2022+
- Android NDK r26+
- Android SDK API 29+
- OpenSSL for Android

**See Also**: [Android Detailed Guide](android.md)

---

## Cross-Platform Considerations

### 1. Code Organization

**Platform-Agnostic Code** (src/):
- `xoron.h` - Common API
- `xoron_luau.cpp` - VM wrapper
- `xoron_http.cpp` - HTTP client
- `xoron_crypto.cpp` - Crypto functions
- `xoron_websocket.cpp` - WebSocket
- `xoron_drawing.cpp` - Drawing engine
- `xoron_env.cpp` - Environment management
- `xoron_filesystem.cpp` - File operations
- `xoron_memory.cpp` - Memory utilities
- `xoron_console.cpp` - Console output
- `xoron_input.cpp` - Input handling
- `xoron_cache.cpp` - Caching
- `xoron_ui.cpp` - UI management

**Platform-Specific Code**:
- `xoron_ios.mm` - iOS implementation
- `xoron_android.cpp` - Android implementation

**Conditional Compilation**:
```cpp
#if defined(XORON_PLATFORM_IOS)
    // iOS-specific code
    #include <Foundation/Foundation.h>
#elif defined(XORON_PLATFORM_ANDROID)
    // Android-specific code
    #include <android/log.h>
#endif
```

### 2. Path Management

**iOS Paths**:
```
~/Documents/Xoron/
├── workspace/      # User workspace
├── autoexecute/    # Auto-run scripts
├── scripts/        # Saved scripts
└── logs/           # Log files
```

**Android Paths**:
```
/data/data/<package>/files/Xoron/
├── workspace/      # User workspace
├── autoexecute/    # Auto-run scripts
├── scripts/        # Saved scripts
└── logs/           # Log files

Or external:
/storage/emulated/0/Xoron/
```

**Path Abstraction**:
```cpp
const char* xoron_get_workspace() {
#if defined(XORON_PLATFORM_IOS)
    // iOS: Documents directory
    NSArray* paths = NSSearchPathForDirectoriesInDomains(
        NSDocumentDirectory, NSUserDomainMask, YES);
    NSString* documents = [paths firstObject];
    return [documents UTF8String];
#elif defined(XORON_PLATFORM_ANDROID)
    // Android: Internal files
    return "/data/data/com.xoron/files/Xoron";
#else
    // Fallback
    return "./xoron_workspace";
#endif
}
```

### 3. Library Registration

**Common Pattern**:
```cpp
void xoron_register_all_libraries(lua_State* L) {
    // Platform-agnostic libraries
    xoron_register_http(L);
    xoron_register_crypto(L);
    xoron_register_drawing(L);
    xoron_register_websocket(L);
    xoron_register_env(L);
    xoron_register_filesystem(L);
    xoron_register_memory(L);
    xoron_register_console(L);
    xoron_register_input(L);
    xoron_register_cache(L);
    xoron_register_ui(L);
    
    // Platform-specific libraries
#if defined(XORON_PLATFORM_IOS)
    xoron_register_ios(L);
#elif defined(XORON_PLATFORM_ANDROID)
    xoron_register_android(L);
#endif
}
```

### 4. Error Handling

**Cross-Platform Error Reporting**:
```cpp
void xoron_set_error(const char* fmt, ...) {
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    
    std::lock_guard<std::mutex> lock(g_state.mutex);
    g_state.last_error = buf;
    
    // Platform-specific logging
#if defined(XORON_PLATFORM_ANDROID)
    __android_log_print(ANDROID_LOG_ERROR, "Xoron", "%s", buf);
#elif defined(XORON_PLATFORM_IOS)
    NSLog(@"Error: %s", buf);
#else
    fprintf(stderr, "Error: %s\n", buf);
#endif
    
    if (g_state.error_fn) {
        g_state.error_fn(buf, g_state.output_ud);
    }
}
```

### 5. Thread Safety

**Platform Thread Considerations**:
- **iOS**: Main thread for UI operations
- **Android**: UI operations must be on main thread
- **Both**: Background threads for network/IO

**Thread-Safe Patterns**:
```cpp
// Global state protection
static std::mutex g_state_mutex;

// Platform-specific thread attachment
#if defined(XORON_PLATFORM_ANDROID)
static JNIEnv* get_jni_env() {
    if (!g_jvm) return nullptr;
    JNIEnv* env;
    int status = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (status == JNI_EDETACHED) {
        g_jvm->AttachCurrentThread(&env, nullptr);
    }
    return env;
}
#endif
```

## Build Configuration

### CMake Cross-Platform Setup

```cmake
cmake_minimum_required(VERSION 3.16)
project(xoron VERSION 2.0.0)

# Platform detection
if(IOS)
    set(XORON_IOS_BUILD ON)
elseif(ANDROID)
    set(XORON_ANDROID_BUILD ON)
endif()

# Common compiler flags
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Platform-specific settings
if(XORON_IOS_BUILD)
    # iOS settings
    set(CMAKE_OSX_DEPLOYMENT_TARGET 15.0)
    enable_language(OBJCXX)
    # ... iOS frameworks
elseif(XORON_ANDROID_BUILD)
    # Android settings
    set(CMAKE_ANDROID_NDK $ENV{ANDROID_NDK})
    set(CMAKE_ANDROID_API 29)
    # ... Android libraries
endif()

# Source files
set(XORON_SOURCES
    xoron_luau.cpp
    xoron_http.cpp
    xoron_crypto.cpp
    # ... common sources
)

# Platform sources
if(XORON_IOS_BUILD)
    list(APPEND XORON_SOURCES xoron_ios.mm)
elseif(XORON_ANDROID_BUILD)
    list(APPEND XORON_SOURCES xoron_android.cpp)
endif()

# Create library
add_library(xoron SHARED ${XORON_SOURCES})

# Link libraries
target_link_libraries(xoron PRIVATE
    Luau.Compiler Luau.VM
    # ... common libs
)

if(XORON_IOS_BUILD)
    target_link_libraries(xoron PRIVATE
        "-framework Foundation"
        "-framework UIKit"
        # ... iOS frameworks
    )
elseif(XORON_ANDROID_BUILD)
    target_link_libraries(xoron PRIVATE
        log android
    )
endif()
```

## Testing Strategy

### Cross-Platform Tests

**Common Test Suite**:
```cpp
// test_cross_platform.cpp
void run_cross_platform_tests() {
    // These tests run on both platforms
    test_vm_operations();
    test_http_operations();
    test_crypto_operations();
    test_file_operations();
    
    // Platform-specific tests
#if defined(XORON_PLATFORM_IOS)
    test_ios_specific();
#elif defined(XORON_PLATFORM_ANDROID)
    test_android_specific();
#endif
}
```

**Platform-Specific Tests**:
- **iOS**: Haptic feedback, UIKit integration, Keychain
- **Android**: JNI bridge, Android logging, system properties

## Deployment

### iOS Deployment

**App Store Distribution**:
1. Build with Release configuration
2. Sign with distribution certificate
3. Create archive
4. Upload to App Store Connect
5. Submit for review

**Enterprise Distribution**:
1. Build with enterprise certificate
2. Create IPA
3. Distribute via MDM or direct download

**Ad-Hoc Testing**:
1. Build with ad-hoc certificate
2. Add device UDIDs to provisioning profile
3. Distribute via TestFlight or direct install

### Android Deployment

**Google Play Store**:
1. Build with Release configuration
2. Sign with upload key
3. Create AAB (Android App Bundle)
4. Upload to Google Play Console
5. Submit for review

**Direct Distribution**:
1. Build APK with Release configuration
2. Sign with release key
3. Distribute via website or file sharing

**Enterprise Distribution**:
1. Build with enterprise certificate
2. Distribute via MDM or direct download

## Performance Considerations

### iOS Performance

**Optimizations**:
- Use Release builds for production
- Enable LTO (Link Time Optimization)
- Strip debug symbols
- Use bitcode for App Store

**Memory Limits**:
- Background: ~50MB
- Foreground: ~200MB
- Large games may need entitlements

### Android Performance

**Optimizations**:
- Use Release builds
- Enable ProGuard/R8
- Strip debug symbols
- Use ABI filters

**Memory Limits**:
- Varies by device
- Typical: 128MB - 512MB
- Can request more via manifest

## Security Considerations

### iOS Security

**Requirements**:
- App Transport Security (ATS)
- Privacy manifest
- Sandbox compliance
- Code signing

**Best Practices**:
- Use HTTPS only
- Validate all inputs
- Secure key storage (Keychain)
- No hardcoded secrets

### Android Security

**Requirements**:
- Permissions model
- Network security config
- Play Protect compliance
- Signing requirements

**Best Practices**:
- Request minimal permissions
- Use AndroidKeyStore
- Validate all inputs
- Secure JNI bridge

## Debugging

### iOS Debugging

**Tools**:
- Xcode Debugger
- Instruments (profiling)
- Console.app (logs)
- lldb (command line)

**Common Issues**:
- Library not loaded (rpath)
- Sandbox violations
- Memory leaks (ARC)
- Threading issues

### Android Debugging

**Tools**:
- Android Studio Debugger
- logcat (logs)
- ndk-stack (crash dumps)
- gdb (command line)

**Common Issues**:
- JNI signature mismatches
- Library not found
- Permission denials
- Threading issues

## Migration Guide

### From iOS to Android

**Key Differences**:
1. **Language**: Objective-C → Java/Kotlin + JNI
2. **UI**: UIKit → Android Views
3. **Paths**: Documents → Internal files
4. **Permissions**: Runtime → Manifest + Runtime
5. **Haptics**: CoreHaptics → Vibrator

**Code Mapping**:
```objective-c
// iOS
NSLog(@"%@", message);
NSString* path = [documents stringByAppendingPathComponent:@"file.txt"];
```

```cpp
// Android (JNI)
__android_log_print(ANDROID_LOG_INFO, "Tag", "%s", message);
const char* path = "/data/data/com.xoron/files/file.txt";
```

### From Android to iOS

**Key Differences**:
1. **Language**: Java/JNI → Objective-C++/Swift
2. **UI**: Android → UIKit
3. **Paths**: Internal → Documents
4. **Permissions**: Manifest → Info.plist
5. **Haptics**: Vibrator → CoreHaptics

**Code Mapping**:
```cpp
// Android
__android_log_print(ANDROID_LOG_INFO, "Tag", "%s", message);
```

```objective-c
// iOS
NSLog(@"%s", message);
```

## Platform-Specific Documentation

### Detailed Guides

- **[iOS Guide](ios.md)**: Complete iOS integration
- **[Android Guide](android.md)**: Complete Android integration
- **[Cross-Platform Guide](cross_platform.md)**: Portability strategies

### Quick Reference

**iOS**:
- Minimum: iOS 15.0
- Format: .dylib
- UI: UIKit
- Graphics: CoreGraphics
- Build: Xcode/CMake

**Android**:
- Minimum: Android 10 (API 29)
- Format: .so
- UI: Android Views
- Graphics: Canvas
- Build: Gradle/CMake

## Support

### Getting Help

1. **Documentation**: Check platform-specific guides
2. **Issues**: GitHub issues
3. **Community**: Discussions forum
4. **Examples**: See src/tests/ for working examples

### Contributing

When adding platform-specific features:
1. Use conditional compilation
2. Keep common API
3. Document platform differences
4. Test on both platforms
5. Update platform documentation

---

**Next:**
- [iOS Integration Guide](ios.md)
- [Android Integration Guide](android.md)
- [Cross-Platform Guide](cross_platform.md)
