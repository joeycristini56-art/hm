# Cross-Platform Development Guide

## Overview

This guide covers strategies for developing cross-platform applications with Xoron, maintaining code reuse while handling platform-specific requirements.

## Architecture Overview

### Layered Architecture

```
┌─────────────────────────────────────────┐
│      Application Layer                  │
│  (SwiftUI/UIKit, Jetpack Compose)       │
└─────────────────────────────────────────┘
                 ↓
┌─────────────────────────────────────────┐
│      Platform Abstraction Layer         │
│  (Common API + Platform Adapters)       │
└─────────────────────────────────────────┘
                 ↓
┌─────────────────────────────────────────┐
│      Xoron Core Engine                  │
│  (C++ + Lua VM, Platform Agnostic)      │
└─────────────────────────────────────────┘
                 ↓
┌─────────────────────────────────────────┐
│      Platform Native Layer              │
│  (iOS SDK, Android NDK)                 │
└─────────────────────────────────────────┘
```

## Code Organization Strategy

### 1. Platform-Agnostic Core (80%)

**Location**: `src/` directory

**Files**:
- `xoron.h` - Unified API
- `xoron_luau.cpp` - VM wrapper
- `xoron_http.cpp` - HTTP client
- `xoron_crypto.cpp` - Crypto functions
- `xoron_websocket.cpp` - WebSocket
- `xoron_drawing.cpp` - Drawing engine
- `xoron_env.cpp` - Environment
- `xoron_filesystem.cpp` - File operations
- `xoron_memory.cpp` - Memory utilities
- `xoron_console.cpp` - Console output
- `xoron_input.cpp` - Input handling
- `xoron_cache.cpp` - Caching
- `xoron_ui.cpp` - UI management

**Characteristics**:
- Pure C++ (or C with C++ wrappers)
- No platform-specific includes
- Uses standard libraries
- Cross-platform by design

### 2. Platform-Specific Adapters (15%)

**Location**: `src/` directory

**Files**:
- `xoron_ios.mm` - iOS implementation
- `xoron_android.cpp` - Android implementation

**Characteristics**:
- Platform-specific includes
- JNI (Android) or Objective-C++ (iOS)
- Implements platform-specific functions
- Bridges to core engine

### 3. Platform Bindings (5%)

**Location**: Platform-specific projects

**Files**:
- iOS: Swift/Objective-C wrappers
- Android: Java/Kotlin + JNI

**Characteristics**:
- High-level API for app developers
- Type conversions
- Error handling
- Memory management

## Conditional Compilation

### Preprocessor Macros

```cpp
// xoron.h
#if defined(__APPLE__) && TARGET_OS_IPHONE
    #define XORON_PLATFORM_IOS 1
    #define XORON_PLATFORM_NAME "iOS"
#elif defined(__ANDROID__)
    #define XORON_PLATFORM_ANDROID 1
    #define XORON_PLATFORM_NAME "Android"
#else
    #define XORON_PLATFORM_UNKNOWN 1
    #define XORON_PLATFORM_NAME "Unknown"
#endif

// Platform-specific includes
#if defined(XORON_PLATFORM_IOS)
    #include <Foundation/Foundation.h>
    #include <objc/objc.h>
#elif defined(XORON_PLATFORM_ANDROID)
    #include <android/log.h>
    #include <jni.h>
#endif
```

### Function Implementation

```cpp
// xoron_haptic.cpp (hypothetical unified file)
void xoron_haptic_feedback(int style) {
#if defined(XORON_PLATFORM_IOS)
    // iOS implementation
    UIImpactFeedbackStyle hapticStyle;
    switch (style) {
        case 0: hapticStyle = UIImpactFeedbackStyleLight; break;
        case 1: hapticStyle = UIImpactFeedbackStyleMedium; break;
        case 2: hapticStyle = UIImpactFeedbackStyleHeavy; break;
        default: hapticStyle = UIImpactFeedbackStyleLight;
    }
    UIImpactFeedbackGenerator* generator = 
        [[UIImpactFeedbackGenerator alloc] initWithStyle:hapticStyle];
    [generator impactOccurred];
    
#elif defined(XORON_PLATFORM_ANDROID)
    // Android implementation
    JNIEnv* env = get_jni_env();
    if (!env) return;
    
    // Get Vibrator service and vibrate
    // ... JNI calls ...
    
#else
    // Fallback: no haptic
    xoron_console_warn("Haptic feedback not supported on this platform");
#endif
}
```

## Path Management

### Unified Path API

```cpp
// xoron_filesystem.cpp
const char* xoron_get_workspace() {
#if defined(XORON_PLATFORM_IOS)
    // iOS: Documents directory
    NSArray* paths = NSSearchPathForDirectoriesInDomains(
        NSDocumentDirectory, NSUserDomainMask, YES);
    NSString* documents = [paths firstObject];
    static std::string path = std::string([documents UTF8String]) + "/Xoron";
    return path.c_str();
    
#elif defined(XORON_PLATFORM_ANDROID)
    // Android: Internal files
    static std::string path = "/data/data/com.xoron/files/Xoron";
    return path.c_str();
    
#else
    // Fallback
    static std::string path = "./xoron_workspace";
    return path.c_str();
#endif
}
```

### Path Helper Functions

```cpp
// Platform-agnostic path utilities
namespace PathUtils {
    std::string join(const std::string& base, const std::string& file) {
        return base + "/" + file;
    }
    
    std::string getAutoExecutePath() {
        return join(xoron_get_workspace(), "autoexecute");
    }
    
    std::string getScriptsPath() {
        return join(xoron_get_workspace(), "scripts");
    }
    
    std::string getLogsPath() {
        return join(xoron_get_workspace(), "logs");
    }
}
```

## Type Conversion

### String Conversion

```cpp
// iOS: NSString ↔ std::string
std::string nsstring_to_string(NSString* nsstr) {
    return std::string([nsstr UTF8String]);
}

NSString* string_to_nsstring(const std::string& str) {
    return [NSString stringWithUTF8String:str.c_str()];
}

// Android: jstring ↔ std::string
std::string jstring_to_string(JNIEnv* env, jstring jstr) {
    const char* cstr = env->GetStringUTFChars(jstr, nullptr);
    std::string str(cstr);
    env->ReleaseStringUTFChars(jstr, cstr);
    return str;
}

jstring string_to_jstring(JNIEnv* env, const std::string& str) {
    return env->NewStringUTF(str.c_str());
}
```

### Error Handling

```cpp
// Unified error reporting
void xoron_set_error(const char* fmt, ...) {
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    
    // Store error
    std::lock_guard<std::mutex> lock(g_state.mutex);
    g_state.last_error = buf;
    
    // Platform-specific logging
#if defined(XORON_PLATFORM_IOS)
    NSLog(@"Error: %s", buf);
#elif defined(XORON_PLATFORM_ANDROID)
    __android_log_print(ANDROID_LOG_ERROR, "Xoron", "%s", buf);
#else
    fprintf(stderr, "Error: %s\n", buf);
#endif
    
    // Callback if set
    if (g_state.error_fn) {
        g_state.error_fn(buf, g_state.output_ud);
    }
}
```

## Library Registration

### Unified Registration

```cpp
// xoron_luau.cpp
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

### Platform-Specific Lua Functions

```cpp
// iOS: Register iOS-specific functions
void xoron_register_ios(lua_State* L) {
    lua_newtable(L);
    
    // Haptic feedback
    lua_pushcfunction(L, lua_ios_haptic);
    lua_setfield(L, -2, "haptic");
    
    // Clipboard
    lua_pushcfunction(L, lua_ios_clipboard_get);
    lua_setfield(L, -2, "clipboard_get");
    lua_pushcfunction(L, lua_ios_clipboard_set);
    lua_setfield(L, -2, "clipboard_set");
    
    // UI
    lua_pushcfunction(L, lua_ios_ui_show);
    lua_setfield(L, -2, "ui_show");
    
    lua_setglobal(L, "ios");
}

// Android: Register Android-specific functions
void xoron_register_android(lua_State* L) {
    lua_newtable(L);
    
    // Haptic feedback
    lua_pushcfunction(L, lua_android_haptic);
    lua_setfield(L, -2, "haptic");
    
    // System properties
    lua_pushcfunction(L, lua_android_system_property);
    lua_setfield(L, -2, "system_property");
    
    // UI
    lua_pushcfunction(L, lua_android_ui_show);
    lua_setfield(L, -2, "ui_show");
    
    lua_setglobal(L, "android");
}
```

## Shared Lua Scripts

### Platform-Independent Scripts

```lua
-- common/utils.lua
local Utils = {}

function Utils.isIOS()
    return package.loaded.ios ~= nil
end

function Utils.isAndroid()
    return package.loaded.android ~= nil
end

function Utils.getPlatform()
    if Utils.isIOS() then return "iOS"
    elseif Utils.isAndroid() then return "Android"
    else return "Unknown" end
end

function Utils.vibrate(style)
    if Utils.isIOS() then
        ios.haptic(style)
    elseif Utils.isAndroid() then
        android.haptic(style)
    end
end

function Utils.copyToClipboard(text)
    if Utils.isIOS() then
        ios.clipboard_set(text)
    elseif Utils.isAndroid() then
        android.clipboard_set(text)
    end
end

return Utils
```

### Platform-Specific Script Loading

```lua
-- Load platform-specific modules
local platform = require("common.utils").getPlatform()

if platform == "iOS" then
    require("ios.specific")
elseif platform == "Android" then
    require("android.specific")
end
```

## Build System

### CMake Cross-Platform Configuration

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(xoron VERSION 2.0.0)

# Platform detection
if(IOS)
    set(XORON_IOS_BUILD ON)
    message(STATUS "Building for iOS")
elseif(ANDROID)
    set(XORON_ANDROID_BUILD ON)
    message(STATUS "Building for Android")
else()
    message(STATUS "Building for development")
endif()

# Common settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Platform-specific settings
if(XORON_IOS_BUILD)
    set(CMAKE_OSX_DEPLOYMENT_TARGET 15.0)
    enable_language(OBJCXX)
    # iOS frameworks
    set(PLATFORM_LIBS
        "-framework Foundation"
        "-framework UIKit"
        "-framework CoreHaptics"
        "-framework CoreGraphics"
        objc
    )
elseif(XORON_ANDROID_BUILD)
    set(CMAKE_ANDROID_NDK $ENV{ANDROID_NDK})
    set(CMAKE_ANDROID_API 29)
    # Android libraries
    set(PLATFORM_LIBS
        log
        android
    )
else()
    # Development
    set(PLATFORM_LIBS "")
endif()

# Source files
set(XORON_SOURCES
    xoron_luau.cpp
    xoron_http.cpp
    xoron_crypto.cpp
    xoron_websocket.cpp
    xoron_drawing.cpp
    xoron_env.cpp
    xoron_filesystem.cpp
    xoron_memory.cpp
    xoron_console.cpp
    xoron_input.cpp
    xoron_cache.cpp
    xoron_ui.cpp
)

# Platform sources
if(XORON_IOS_BUILD)
    list(APPEND XORON_SOURCES xoron_ios.mm)
elseif(XORON_ANDROID_BUILD)
    list(APPEND XORON_SOURCES xoron_android.cpp)
endif()

# Create library
add_library(xoron SHARED ${XORON_SOURCES})

# Include directories
target_include_directories(xoron PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${luau_SOURCE_DIR}/VM/include
    ${luau_SOURCE_DIR}/Compiler/include
    ${luau_SOURCE_DIR}/Common/include
    ${httplib_SOURCE_DIR}
    ${lz4_SOURCE_DIR}/lib
)

# Link libraries
target_link_libraries(xoron PRIVATE
    Luau.Compiler
    Luau.VM
    lz4_static
    ${PLATFORM_LIBS}
)

# OpenSSL
if(OPENSSL_FOUND)
    target_link_libraries(xoron PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    target_compile_definitions(xoron PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
endif()

# Platform-specific properties
if(XORON_IOS_BUILD)
    set_target_properties(xoron PROPERTIES
        SUFFIX ".dylib"
        PREFIX "lib"
        OUTPUT_NAME "xoron"
    )
elseif(XORON_ANDROID_BUILD)
    set_target_properties(xoron PROPERTIES
        SUFFIX ".so"
        PREFIX "lib"
        OUTPUT_NAME "xoron"
    )
endif()
```

### Multi-Platform Build Script

```bash
#!/bin/bash
# build_all.sh

# Build iOS
echo "Building iOS..."
mkdir -p build-ios
cd build-ios
cmake .. \
  -DXORON_IOS_BUILD=ON \
  -DCMAKE_TOOLCHAIN_FILE=../ios-cmake/ios.toolchain.cmake \
  -DPLATFORM=OS64 \
  -DXORON_OPENSSL_ROOT=/path/to/openssl-ios
cmake --build . --config Release
cd ..

# Build Android (arm64-v8a)
echo "Building Android arm64-v8a..."
mkdir -p build-android-arm64
cd build-android-arm64
cmake .. \
  -DXORON_ANDROID_BUILD=ON \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-29 \
  -DXORON_OPENSSL_ROOT=/path/to/openssl-android-arm64
cmake --build . --config Release
cd ..

# Build Android (armeabi-v7a)
echo "Building Android armeabi-v7a..."
mkdir -p build-android-armv7
cd build-android-armv7
cmake .. \
  -DXORON_ANDROID_BUILD=ON \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=armeabi-v7a \
  -DANDROID_PLATFORM=android-29 \
  -DXORON_OPENSSL_ROOT=/path/to/openssl-android-armv7
cmake --build . --config Release
cd ..

echo "Build complete!"
```

## Testing Strategy

### Cross-Platform Test Suite

```cpp
// test_cross_platform.cpp
void run_cross_platform_tests() {
    TestSuite suite("Cross-Platform");
    Timer timer;
    
    // These tests run on both platforms
    timer.reset();
    bool vm_test = test_vm_operations();
    suite.recordResult("VM Operations", vm_test, "", timer.elapsed_ms());
    
    timer.reset();
    bool http_test = test_http_operations();
    suite.recordResult("HTTP Operations", http_test, "", timer.elapsed_ms());
    
    timer.reset();
    bool crypto_test = test_crypto_operations();
    suite.recordResult("Crypto Operations", crypto_test, "", timer.elapsed_ms());
    
    timer.reset();
    bool file_test = test_file_operations();
    suite.recordResult("File Operations", file_test, "", timer.elapsed_ms());
    
    // Platform-specific tests
#if defined(XORON_PLATFORM_IOS)
    timer.reset();
    bool ios_test = test_ios_specific();
    suite.recordResult("iOS Specific", ios_test, "", timer.elapsed_ms());
#elif defined(XORON_PLATFORM_ANDROID)
    timer.reset();
    bool android_test = test_android_specific();
    suite.recordResult("Android Specific", android_test, "", timer.elapsed_ms());
#endif
    
    suite.printSummary();
}
```

### Lua Test Script (Platform-Independent)

```lua
-- test_all.lua
local Utils = require("common.utils")

print("Running cross-platform tests...")
print("Platform:", Utils.getPlatform())

-- Test 1: Basic Lua
local sum = 0
for i = 1, 1000 do
    sum = sum + i
end
assert(sum == 500500, "Basic math failed")
print("✓ Basic math")

-- Test 2: HTTP
local response = http.get("https://httpbin.org/get")
assert(response and response.status == 200, "HTTP failed")
print("✓ HTTP GET")

-- Test 3: Crypto
local hash = crypto.sha256("test")
assert(#hash == 64, "SHA256 failed")
print("✓ Crypto")

-- Test 4: File operations
writefile("test.txt", "Hello")
local content = readfile("test.txt")
assert(content == "Hello", "File I/O failed")
deletefile("test.txt")
print("✓ File I/O")

-- Test 5: Platform-specific
if Utils.isIOS() then
    ios.haptic(0)
    print("✓ iOS haptic")
elseif Utils.isAndroid() then
    android.haptic(0)
    print("✓ Android haptic")
end

print("All tests passed!")
```

## Common Patterns

### 1. Factory Pattern

```cpp
// Platform-specific factory
class DrawingContext {
public:
    static DrawingContext* create();
    virtual void drawLine(float x1, float y1, float x2, float y2) = 0;
    virtual ~DrawingContext() {}
};

#if defined(XORON_PLATFORM_IOS)
class iOSDrawingContext : public DrawingContext {
    CGContextRef context;
public:
    void drawLine(float x1, float y1, float x2, float y2) override {
        // CoreGraphics implementation
    }
};
#elif defined(XORON_PLATFORM_ANDROID)
class AndroidDrawingContext : public DrawingContext {
    ACanvas* canvas;
public:
    void drawLine(float x1, float y1, float x2, float y2) override {
        // Canvas implementation
    }
};
#endif

DrawingContext* DrawingContext::create() {
#if defined(XORON_PLATFORM_IOS)
    return new iOSDrawingContext();
#elif defined(XORON_PLATFORM_ANDROID)
    return new AndroidDrawingContext();
#else
    return nullptr;
#endif
}
```

### 2. Strategy Pattern

```cpp
// Platform-specific strategies
class HapticStrategy {
public:
    virtual void trigger(int style) = 0;
    virtual ~HapticStrategy() {}
};

class iOSHapticStrategy : public HapticStrategy {
public:
    void trigger(int style) override {
        // iOS implementation
    }
};

class AndroidHapticStrategy : public HapticStrategy {
public:
    void trigger(int style) override {
        // Android implementation
    }
};

// Usage
class HapticManager {
    std::unique_ptr<HapticStrategy> strategy;
public:
    HapticManager() {
#if defined(XORON_PLATFORM_IOS)
        strategy = std::make_unique<iOSHapticStrategy>();
#elif defined(XORON_PLATFORM_ANDROID)
        strategy = std::make_unique<AndroidHapticStrategy>();
#endif
    }
    
    void vibrate(int style) {
        if (strategy) {
            strategy->trigger(style);
        }
    }
};
```

### 3. Adapter Pattern

```cpp
// Adapt platform APIs to common interface
class ClipboardAdapter {
public:
    virtual bool setText(const std::string& text) = 0;
    virtual std::string getText() = 0;
    virtual ~ClipboardAdapter() {}
};

#if defined(XORON_PLATFORM_IOS)
class iOSClipboardAdapter : public ClipboardAdapter {
public:
    bool setText(const std::string& text) override {
        // Use UIPasteboard
        return true;
    }
    std::string getText() override {
        // Get from UIPasteboard
        return "";
    }
};
#elif defined(XORON_PLATFORM_ANDROID)
class AndroidClipboardAdapter : public ClipboardAdapter {
public:
    bool setText(const std::string& text) override {
        // Use Android ClipboardManager via JNI
        return true;
    }
    std::string getText() override {
        // Get from ClipboardManager
        return "";
    }
};
#endif
```

## Data Synchronization

### Cross-Platform State Management

```cpp
// Global state with platform-specific storage
struct AppState {
    std::string workspace;
    std::map<std::string, std::string> settings;
    std::vector<std::string> recentScripts;
    
    void save() {
        std::string path = xoron_get_workspace() + "/state.json";
        // Serialize to JSON
        // Platform-specific file write
    }
    
    void load() {
        std::string path = xoron_get_workspace() + "/state.json";
        // Deserialize from JSON
        // Platform-specific file read
    }
};

// Singleton
AppState& getAppState() {
    static AppState state;
    return state;
}
```

### Lua State Bridge

```lua
-- Shared state in Lua
getgenv()._XORON_STATE = getgenv()._XORON_STATE or {
    settings = {},
    recent = {},
    cache = {}
}

-- Platform-agnostic state access
function getState()
    return getgenv()._XORON_STATE
end

function saveState()
    local state = getState()
    local json = game:GetService("HttpService"):JSONEncode(state)
    writefile("xoron_state.json", json)
end

function loadState()
    local content = readfile("xoron_state.json")
    if content then
        local state = game:GetService("HttpService"):JSONDecode(content)
        getgenv()._XORON_STATE = state
    end
end
```

## Error Handling Strategy

### Unified Error Types

```cpp
// xoron.h
typedef enum {
    XORON_OK = 0,
    XORON_ERR_INIT = -1,
    XORON_ERR_MEMORY = -2,
    XORON_ERR_COMPILE = -3,
    XORON_ERR_RUNTIME = -4,
    XORON_ERR_HTTP = -5,
    XORON_ERR_INVALID = -6,
    XORON_ERR_IO = -7,
    XORON_ERR_WEBSOCKET = -8,
    XORON_ERR_SECURITY = -9,
    XORON_ERR_PLATFORM = -10  // Platform-specific error
} xoron_error_t;

// Platform-specific error codes
#if defined(XORON_PLATFORM_IOS)
enum {
    XORON_ERR_IOS_KEYCHAIN = -100,
    XORON_ERR_IOS_HAPTIC = -101
};
#elif defined(XORON_PLATFORM_ANDROID)
enum {
    XORON_ERR_ANDROID_JNI = -100,
    XORON_ERR_ANDROID_PERMISSION = -101
};
#endif
```

### Error Propagation

```cpp
// Platform-specific error handling
int xoron_platform_operation() {
#if defined(XORON_PLATFORM_IOS)
    @try {
        // iOS code
        return XORON_OK;
    } @catch (NSException* exception) {
        xoron_set_error("iOS exception: %s", 
                       [[exception reason] UTF8String]);
        return XORON_ERR_PLATFORM;
    }
#elif defined(XORON_PLATFORM_ANDROID)
    JNIEnv* env = get_jni_env();
    if (!env) {
        xoron_set_error("Failed to get JNI environment");
        return XORON_ERR_ANDROID_JNI;
    }
    
    // Check for exceptions
    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        return XORON_ERR_ANDROID_JNI;
    }
    
    return XORON_OK;
#else
    return XORON_OK;
#endif
}
```

## Performance Considerations

### Platform-Specific Optimizations

```cpp
// Memory allocation
void* xoron_malloc(size_t size) {
#if defined(XORON_PLATFORM_IOS)
    // Use iOS optimized allocator
    return malloc(size);
#elif defined(XORON_PLATFORM_ANDROID)
    // Use Android allocator
    return malloc(size);
#else
    return malloc(size);
#endif
}

// Threading
void xoron_async_execute(std::function<void()> task) {
#if defined(XORON_PLATFORM_IOS)
    // Use Grand Central Dispatch
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        task();
    });
#elif defined(XORON_PLATFORM_ANDROID)
    // Use std::thread or Android thread pool
    std::thread([task]() {
        task();
    }).detach();
#else
    // Fallback
    std::thread([task]() {
        task();
    }).detach();
#endif
}
```

### Benchmarking

```cpp
// Cross-platform benchmark
void benchmark_cross_platform() {
    printf("\n=== Cross-Platform Benchmark ===\n");
    
    // VM creation
    {
        Timer t;
        for (int i = 0; i < 100; i++) {
            xoron_vm_t* vm = xoron_vm_new();
            xoron_vm_free(vm);
        }
        printf("VM Creation: %.2f ms avg\n", t.elapsed_ms() / 100);
    }
    
    // Script execution
    {
        xoron_vm_t* vm = xoron_vm_new();
        const char* script = "local x = 0; for i=1,10000 do x = x + i end";
        
        Timer t;
        for (int i = 0; i < 100; i++) {
            xoron_dostring(vm, script, "bench");
        }
        printf("Script Exec: %.2f ms avg\n", t.elapsed_ms() / 100);
        
        xoron_vm_free(vm);
    }
    
    // HTTP
    {
        Timer t;
        for (int i = 0; i < 10; i++) {
            int status; size_t len;
            char* resp = xoron_http_get("https://httpbin.org/get", &status, &len);
            if (resp) xoron_http_free(resp);
        }
        printf("HTTP GET: %.2f ms avg\n", t.elapsed_ms() / 10);
    }
    
    // Crypto
    {
        const char* data = "test data";
        Timer t;
        for (int i = 0; i < 1000; i++) {
            uint8_t hash[32];
            xoron_sha256(data, strlen(data), hash);
        }
        printf("SHA256: %.2f ms (1000 ops)\n", t.elapsed_ms());
    }
}
```

## Deployment Strategy

### Unified Release Process

```bash
#!/bin/bash
# release.sh

VERSION=$1

echo "Releasing Xoron $VERSION"

# Build all platforms
./build_all.sh

# Create release directory
mkdir -p release/$VERSION
cd release/$VERSION

# Copy iOS
cp ../../build-ios/libxoron.dylib ios/
cp ../../xoron.h ios/

# Copy Android
cp ../../build-android-arm64/libxoron.so android/arm64-v8a/
cp ../../build-android-armv7/libxoron.so android/armeabi-v7a/
cp ../../xoron.h android/

# Create package
zip -r xoron-$VERSION.zip ios/ android/

# Generate checksums
sha256sum xoron-$VERSION.zip > checksums.txt

echo "Release package created: xoron-$VERSION.zip"
```

### Documentation

```markdown
# Release Notes vX.Y.Z

## Changes
- Added feature X
- Fixed bug Y
- Improved performance Z

## Platforms
- iOS: Requires iOS 15.0+
- Android: Requires Android 10+ (API 29)

## Files
- `ios/libxoron.dylib` - iOS dynamic library
- `android/arm64-v8a/libxoron.so` - Android 64-bit ARM
- `android/armeabi-v7a/libxoron.so` - Android 32-bit ARM
- `xoron.h` - Header file

## Installation
See platform-specific guides:
- iOS: docs/platforms/ios.md
- Android: docs/platforms/android.md
```

## Best Practices

### 1. Keep Core Pure
- 80% of code should be platform-agnostic
- Use standard C++ features
- Avoid platform-specific types in core

### 2. Abstract Platform Differences
- Use adapter pattern
- Implement platform-specific code once
- Document platform requirements

### 3. Test on All Platforms
- Run same test suite on both
- Verify behavior is consistent
- Check performance characteristics

### 4. Handle Errors Gracefully
- Unified error codes
- Platform-specific error messages
- Fallback behavior

### 5. Document Platform Differences
- Clearly mark platform-specific code
- Document requirements
- Provide migration guides

### 6. Use Conditional Compilation Sparingly
- Prefer runtime checks when possible
- Keep preprocessor blocks small
- Comment why platform-specific

### 7. Maintain API Compatibility
- Don't break cross-platform API
- Add new features consistently
- Deprecate carefully

## Common Pitfalls

### 1. Platform-Specific Types in Core
❌ **Bad**:
```cpp
// In core header
#include <Foundation/Foundation.h>  // iOS only!
```

✅ **Good**:
```cpp
// In core header
#ifdef __cplusplus
extern "C" {
#endif
// Use C types
#ifdef __cplusplus
}
#endif
```

### 2. Assuming File Paths
❌ **Bad**:
```cpp
std::string path = "/data/data/app/files";  // Android only!
```

✅ **Good**:
```cpp
std::string path = xoron_get_workspace();  // Platform-agnostic
```

### 3. Ignoring Memory Management
❌ **Bad**:
```cpp
// iOS: Manual memory in ARC
NSString* str = [[NSString alloc] initWithUTF8String:cstr];
// Forgot to release
```

✅ **Good**:
```cpp
// Use RAII
std::unique_ptr<char[]> buffer(new char[size]);
// Automatic cleanup
```

### 4. Not Testing on Both Platforms
❌ **Bad**: "It works on iOS, should work on Android"
✅ **Good**: Test on both, use CI/CD

### 5. Platform-Specific Lua Code
❌ **Bad**:
```lua
-- In shared script
local device = ios.getDevice()  -- Crashes on Android
```

✅ **Good**:
```lua
-- Check platform first
if package.loaded.ios then
    local device = ios.getDevice()
end
```

## Migration Guide

### Adding New Platform

**Step 1**: Identify requirements
- What platform APIs are needed?
- What are the limitations?
- What are the performance characteristics?

**Step 2**: Create platform file
```cpp
// src/xoron_newplatform.cpp
#include "xoron.h"

// Implement platform-specific functions
void xoron_newplatform_haptic(int style) {
    // Implementation
}

// Register in xoron_luau.cpp
#if defined(XORON_PLATFORM_NEWPLATFORM)
    xoron_register_newplatform(L);
#endif
```

**Step 3**: Update build system
```cmake
# Add to CMakeLists.txt
elseif(XORON_NEWPLATFORM_BUILD)
    list(APPEND XORON_SOURCES xoron_newplatform.cpp)
    # ... platform-specific settings
endif()
```

**Step 4**: Create platform guide
```markdown
# NewPlatform Integration Guide
- Requirements
- Build instructions
- API differences
- Testing
```

**Step 5**: Update documentation
- Add to platform comparison
- Update main README
- Add to CI/CD

### Migrating from Single Platform

**From iOS to Cross-Platform**:
1. Extract platform-agnostic code
2. Create Android adapter
3. Use conditional compilation
4. Test on both platforms

**From Android to Cross-Platform**:
1. Extract platform-agnostic code
2. Create iOS adapter
3. Use conditional compilation
4. Test on both platforms

## Summary

Cross-platform development with Xoron requires:

1. **Clear Architecture**: Separate core from platform code
2. **Unified API**: Consistent interface across platforms
3. **Conditional Compilation**: Handle platform differences
4. **Comprehensive Testing**: Verify on all platforms
5. **Good Documentation**: Explain platform requirements

**Key Principles**:
- 80% platform-agnostic code
- 15% platform adapters
- 5% platform bindings
- Test everything on all platforms
- Document platform differences

**Next Steps**:
- Review [iOS Guide](ios.md)
- Review [Android Guide](android.md)
- Check [Build System](../../src/CMakeLists.txt)
- See [Cross-Platform Tests](../../src/tests/README.md)
