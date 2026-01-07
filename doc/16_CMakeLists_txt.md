# 16. CMakeLists.txt - Build Configuration

**File Path**: `src/src/CMakeLists.txt`  
**Size**: 15,217 bytes  
**Lines**: 390

## Overview
CMake build configuration for Xoron executor. Handles cross-platform builds for iOS and Android, dependency management, and compiler settings.

## Structure

### CMake Version
```cmake
cmake_minimum_required(VERSION 3.16)
project(xoron VERSION 2.0.0 LANGUAGES C CXX)
```

**Requirements**:
- CMake 3.16+ (for modern features)
- C and CXX languages
- Project name: xoron
- Version: 2.0.0

### Compiler Settings
```cmake
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
```

**Settings**:
- C++17 standard required
- Position-independent code (PIC) for shared libraries
- Standard enforcement

## Build Options

### Platform Selection
```cmake
option(XORON_IOS_BUILD "Build for iOS ARM64 (.dylib)" OFF)
option(XORON_ANDROID_BUILD "Build for Android (.so)" OFF)
```

**Options**:
- `XORON_IOS_BUILD`: Enable iOS build
- `XORON_ANDROID_BUILD`: Enable Android build

### Android Configuration
```cmake
set(XORON_ANDROID_ABI "arm64-v8a" CACHE STRING "Android ABI to build for")
set(XORON_ANDROID_PLATFORM "android-29" CACHE STRING "Android platform version")
```

**ABI Options**:
- `arm64-v8a`: ARM64 (recommended)
- `armeabi-v7a`: ARM32
- `x86_64`: x86 64-bit
- `x86`: x86 32-bit

**Platform**:
- Minimum: `android-29` (Android 10)
- Can be higher for newer features

### iOS Configuration
```cmake
set(XORON_IOS_DEPLOYMENT_TARGET "15.0" CACHE STRING "iOS deployment target")
```

**Target**:
- Minimum: iOS 15.0
- Can be adjusted for compatibility

### OpenSSL Path
```cmake
set(XORON_OPENSSL_ROOT "" CACHE PATH "Path to pre-built OpenSSL for mobile")
```

**Usage**:
- Provide path to pre-built OpenSSL
- Required for mobile builds
- Can be left empty for dev builds

## Platform Validation

### No Platform Specified
```cmake
if(NOT XORON_IOS_BUILD AND NOT XORON_ANDROID_BUILD)
    message(STATUS "No mobile platform specified, defaulting to development build")
endif()
```

**Behavior**:
- Builds for host system
- Good for testing
- Not for production

### Android API Validation
```cmake
if(XORON_ANDROID_BUILD)
    string(REGEX MATCH "[0-9]+" ANDROID_API_LEVEL "${XORON_ANDROID_PLATFORM}")
    if(ANDROID_API_LEVEL LESS 29)
        message(WARNING "Android API level ${ANDROID_API_LEVEL} is below minimum (29). Setting to android-29.")
        set(XORON_ANDROID_PLATFORM "android-29" CACHE STRING "Android platform version" FORCE)
    endif()
endif()
```

**Validation**:
- Extracts API level from platform string
- Ensures minimum API 29
- Forces correct value if too low

## Platform Detection

### iOS Build
```cmake
if(XORON_IOS_BUILD)
    message(STATUS "=== Building for iOS ARM64 (.dylib) ===")
    message(STATUS "iOS Deployment Target: ${XORON_IOS_DEPLOYMENT_TARGET}")
    
    add_compile_definitions(TARGET_OS_IPHONE=1)
    add_compile_definitions(__APPLE__=1)
    add_compile_definitions(__arm64__=1)
    add_compile_definitions(XORON_PLATFORM_IOS=1)
    
    set(CMAKE_OSX_DEPLOYMENT_TARGET "${XORON_IOS_DEPLOYMENT_TARGET}")
    set(CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "${XORON_IOS_DEPLOYMENT_TARGET}")
    
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -fvisibility-inlines-hidden")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
endif()
```

**iOS Settings**:
- Defines iOS-specific macros
- Sets deployment target
- Hides symbols by default
- Uses Xcode attributes

### Android Build
```cmake
elseif(XORON_ANDROID_BUILD)
    message(STATUS "=== Building for Android (.so) ===")
    message(STATUS "Android ABI: ${XORON_ANDROID_ABI}")
    message(STATUS "Android Platform: ${XORON_ANDROID_PLATFORM}")
    
    add_compile_definitions(__ANDROID__=1)
    add_compile_definitions(XORON_PLATFORM_ANDROID=1)
    add_compile_definitions(XORON_MIN_ANDROID_API=29)
    
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -fvisibility=hidden -fvisibility-inlines-hidden")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -fvisibility=hidden")
    
    if(ANDROID_ABI STREQUAL "arm64-v8a")
        add_compile_definitions(XORON_ANDROID_ARM64=1)
        add_compile_definitions(XORON_ARCH_NAME="arm64-v8a")
    elseif(ANDROID_ABI STREQUAL "armeabi-v7a")
        add_compile_definitions(XORON_ANDROID_ARM32=1)
        add_compile_definitions(XORON_ARCH_NAME="armeabi-v7a")
    elseif(ANDROID_ABI STREQUAL "x86_64")
        add_compile_definitions(XORON_ANDROID_X86_64=1)
        add_compile_definitions(XORON_ARCH_NAME="x86_64")
    elseif(ANDROID_ABI STREQUAL "x86")
        add_compile_definitions(XORON_ANDROID_X86=1)
        add_compile_definitions(XORON_ARCH_NAME="x86")
    endif()
endif()
```

**Android Settings**:
- Defines Android-specific macros
- Sets architecture definitions
- Position-independent code
- Symbol visibility control

## Dependencies

### Threads
```cmake
find_package(Threads REQUIRED)
```

**Usage**: pthread or std::thread support

### ZLIB
```cmake
if(NOT XORON_IOS_BUILD AND NOT XORON_ANDROID_BUILD)
    set(ZLIB_USE_STATIC_LIBS OFF)
else()
    set(ZLIB_USE_STATIC_LIBS ON)
endif()
find_package(ZLIB)
```

**Strategy**:
- Dev builds: Shared library
- Mobile builds: Static library

### OpenSSL
```cmake
if(XORON_IOS_BUILD OR XORON_ANDROID_BUILD)
    set(OPENSSL_USE_STATIC_LIBS TRUE)
else()
    set(OPENSSL_USE_STATIC_LIBS FALSE)
endif()

if(XORON_OPENSSL_ROOT AND EXISTS "${XORON_OPENSSL_ROOT}/include/openssl/ssl.h")
    message(STATUS "Using pre-built OpenSSL from: ${XORON_OPENSSL_ROOT}")
    set(OPENSSL_INCLUDE_DIR "${XORON_OPENSSL_ROOT}/include")
    set(OPENSSL_SSL_LIBRARY "${XORON_OPENSSL_ROOT}/lib/libssl.a")
    set(OPENSSL_CRYPTO_LIBRARY "${XORON_OPENSSL_ROOT}/lib/libcrypto.a")
    set(OPENSSL_FOUND TRUE)
else()
    find_package(OpenSSL QUIET)
    if(NOT OPENSSL_FOUND AND (XORON_IOS_BUILD OR XORON_ANDROID_BUILD))
        message(STATUS "OpenSSL not found - will build from source")
        set(BUILD_OPENSSL_FROM_SOURCE TRUE)
    endif()
endif()
```

**Strategy**:
1. Use pre-built if provided
2. Try system OpenSSL for dev
3. Build from source for mobile if needed

### OpenSSL from Source
```cmake
if(BUILD_OPENSSL_FROM_SOURCE)
    include(ExternalProject)
    
    set(OPENSSL_VERSION "3.2.0")
    set(OPENSSL_INSTALL_DIR "${CMAKE_BINARY_DIR}/openssl-install")
    set(OPENSSL_INCLUDE_DIR "${OPENSSL_INSTALL_DIR}/include")
    set(OPENSSL_SSL_LIBRARY "${OPENSSL_INSTALL_DIR}/lib/libssl.a")
    set(OPENSSL_CRYPTO_LIBRARY "${OPENSSL_INSTALL_DIR}/lib/libcrypto.a")
    
    if(XORON_ANDROID_BUILD)
        # Android-specific OpenSSL build
        # ... configuration ...
    elseif(XORON_IOS_BUILD)
        # iOS-specific OpenSSL build
        # ... configuration ...
    endif()
    
    set(OPENSSL_FOUND TRUE)
endif()
```

**Build Process**:
- Downloads OpenSSL 3.2.0
- Configures for target platform
- Builds static libraries
- Installs to build directory

### Luau
```cmake
FetchContent_Declare(luau
    GIT_REPOSITORY https://github.com/Roblox/luau.git
    GIT_TAG 0.607)
```

**Version**: 0.607 (Roblox's Lua VM)

### cpp-httplib
```cmake
FetchContent_Declare(httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.14.3)
```

**Version**: v0.14.3 (HTTP client/server)

### LZ4
```cmake
FetchContent_Declare(lz4
    GIT_REPOSITORY https://github.com/lz4/lz4.git
    GIT_TAG v1.9.4
    SOURCE_SUBDIR build/cmake)
```

**Version**: v1.9.4 (Compression)

### Luau Build Options
```cmake
set(LUAU_BUILD_CLI OFF CACHE BOOL "" FORCE)
set(LUAU_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LUAU_BUILD_WEB OFF CACHE BOOL "" FORCE)
```

**Disable**: CLI, tests, web build

### LZ4 Build Options
```cmake
set(LZ4_BUILD_CLI OFF CACHE BOOL "" FORCE)
set(LZ4_BUILD_LEGACY_LZ4C OFF CACHE BOOL "" FORCE)
```

**Disable**: CLI tools

### Fetch Dependencies
```cmake
FetchContent_MakeAvailable(luau httplib lz4)
```

**Action**: Downloads and makes available

## Source Files

### Core Sources
```cmake
set(XORON_SOURCES
    xoron_luau.cpp
    xoron_http.cpp
    xoron_crypto.cpp
    xoron_env.cpp
    xoron_filesystem.cpp
    xoron_memory.cpp
    xoron_debug.cpp
    xoron_console.cpp
    xoron_drawing.cpp
    xoron_websocket.cpp
    xoron_input.cpp
    xoron_cache.cpp
    xoron_ui.cpp)
```

**Order Matters**: Core files first

### Platform-Specific Sources

#### iOS
```cmake
if(XORON_IOS_BUILD OR (APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin"))
    list(APPEND XORON_SOURCES xoron_ios.mm)
    enable_language(OBJCXX)
endif()
```

**Requirements**:
- `.mm` extension (Objective-C++)
- Enable OBJCXX language
- Adds iOS-specific functionality

#### Android
```cmake
if(XORON_ANDROID_BUILD)
    list(APPEND XORON_SOURCES xoron_android.cpp)
    target_link_libraries(xoron PRIVATE
        log      # Android logging
        EGL      # OpenGL ES
        GLESv2   # OpenGL ES 2.0
        android  # Android NDK
    )
endif()
```

**Requirements**:
- Android NDK libraries
- Logging, graphics, Android API

## Target Creation

### Shared Library
```cmake
add_library(xoron SHARED ${XORON_SOURCES})
```

**Type**: SHARED (.dylib or .so)

### Include Directories
```cmake
target_include_directories(xoron PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${luau_SOURCE_DIR}/VM/include
    ${luau_SOURCE_DIR}/VM/src
    ${luau_SOURCE_DIR}/Compiler/include
    ${luau_SOURCE_DIR}/Common/include
    ${httplib_SOURCE_DIR}
    ${lz4_SOURCE_DIR}/lib)
```

**Paths**:
- Current directory (xoron.h)
- Luau VM headers
- Luau Compiler headers
- Luau Common headers
- cpp-httplib header
- LZ4 headers

### Link Libraries

#### Base Libraries
```cmake
target_link_libraries(xoron PRIVATE
    Luau.Compiler Luau.VM
    Threads::Threads
    lz4_static)
```

**Libraries**:
- Luau compiler and VM
- Thread support
- LZ4 static

#### OpenSSL
```cmake
if(OPENSSL_FOUND)
    if(XORON_IOS_BUILD OR XORON_ANDROID_BUILD OR BUILD_OPENSSL_FROM_SOURCE)
        target_include_directories(xoron PRIVATE ${OPENSSL_INCLUDE_DIR})
        target_link_libraries(xoron PRIVATE ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY})
        if(OPENSSL_DEPENDS)
            add_dependencies(xoron ${OPENSSL_DEPENDS})
        endif()
    else()
        target_link_libraries(xoron PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    endif()
    target_compile_definitions(xoron PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
    message(STATUS "OpenSSL: ENABLED")
else()
    message(WARNING "OpenSSL not available - crypto features will be limited!")
    target_compile_definitions(xoron PRIVATE XORON_NO_OPENSSL=1)
endif()
```

**Strategy**:
- Mobile: Direct library paths
- Dev: CMake targets
- Define CPPHTTPLIB_OPENSSL_SUPPORT
- Fallback: XORON_NO_OPENSSL

### Platform-Specific Linking

#### iOS
```cmake
if(XORON_IOS_BUILD)
    set_target_properties(xoron PROPERTIES
        SUFFIX ".dylib"
        PREFIX "lib"
        OUTPUT_NAME "xoron")
    
    target_link_libraries(xoron PRIVATE
        "-framework Foundation"
        "-framework CoreFoundation"
        "-framework CoreGraphics"
        "-framework CoreText"
        "-framework Security"
        "-framework UIKit"
        "-framework WebKit"
        "-framework QuartzCore"
        objc)
endif()
```

**Frameworks**:
- Foundation: Base classes
- CoreFoundation: C types
- CoreGraphics: Graphics
- CoreText: Text rendering
- Security: Crypto
- UIKit: UI components
- WebKit: Web view
- QuartzCore: Animation
- objc: Runtime

#### Android
```cmake
if(XORON_ANDROID_BUILD)
    set_target_properties(xoron PROPERTIES
        SUFFIX ".so"
        PREFIX "lib"
        OUTPUT_NAME "xoron")
    
    target_link_libraries(xoron PRIVATE
        log)
    
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        set_target_properties(xoron PROPERTIES
            LINK_FLAGS "-s")
    endif()
endif()
```

**Properties**:
- `.so` suffix
- `lib` prefix
- Strip debug symbols in Release

#### Development
```cmake
else()
    message(STATUS "Development build - for testing only")
    if(APPLE)
        target_link_libraries(xoron PRIVATE
            "-framework Foundation"
            "-framework Security")
    elseif(UNIX)
        target_link_libraries(xoron PRIVATE dl)
    endif()
endif()
```

**Dev Build**:
- Minimal frameworks
- Linux: dl library
- Not for production

## Installation

### Install Rules
```cmake
install(TARGETS xoron LIBRARY DESTINATION lib)
install(FILES xoron.h DESTINATION include)
```

**Destinations**:
- Library: `lib/`
- Header: `include/`

## Build Summary

### Configuration Display
```cmake
message(STATUS "")
message(STATUS "╔══════════════════════════════════════════════════════════════╗")
message(STATUS "║           Xoron Build Configuration Summary                  ║")
message(STATUS "╠══════════════════════════════════════════════════════════════╣")
message(STATUS "║ Version: ${PROJECT_VERSION}")
message(STATUS "║ Platform: ${CMAKE_SYSTEM_NAME}")
if(XORON_IOS_BUILD)
    message(STATUS "║ Target: iOS ARM64 (.dylib)")
    message(STATUS "║ iOS Deployment Target: ${XORON_IOS_DEPLOYMENT_TARGET}")
    message(STATUS "║ Minimum iOS Version: 15.0")
elseif(XORON_ANDROID_BUILD)
    message(STATUS "║ Target: Android (.so)")
    message(STATUS "║ Android ABI: ${ANDROID_ABI}")
    message(STATUS "║ Android Platform: ${XORON_ANDROID_PLATFORM}")
    message(STATUS "║ Minimum Android Version: 10 (API 29)")
else()
    message(STATUS "║ Target: Development build (not for production)")
endif()
message(STATUS "║ OpenSSL: ${OPENSSL_FOUND}")
message(STATUS "║ C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "╚══════════════════════════════════════════════════════════════╝")
```

**Output**: Clear summary of build configuration

## Build Commands

### iOS Build
```bash
cmake -S . -B build \
  -DXORON_IOS_BUILD=ON \
  -DXORON_OPENSSL_ROOT=/path/to/openssl \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/ios-toolchain.cmake

cmake --build build --config Release
```

**Requirements**:
- iOS toolchain file
- Pre-built OpenSSL
- Xcode installed

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

**Requirements**:
- Android NDK
- Android SDK
- Pre-built OpenSSL
- CMake 3.18+ (for NDK toolchain)

### Development Build
```bash
cmake -S . -B build
cmake --build build
```

**For**: Testing on host machine

## Advanced Configuration

### Custom Build Types
```cmake
# Add custom build types if needed
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
```

### Compiler Warnings
```cmake
# Enable warnings
if(MSVC)
    target_compile_options(xoron PRIVATE /W4)
else()
    target_compile_options(xoron PRIVATE -Wall -Wextra -Wpedantic)
endif()
```

### Optimization Flags
```cmake
# Release optimizations
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        target_compile_options(xoron PRIVATE /O2 /Ob2 /Oi /Ot)
    else()
        target_compile_options(xoron PRIVATE -O3 -ffast-math)
    endif()
endif()
```

### Debug Symbols
```cmake
# Debug information
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(xoron PRIVATE -g -O0)
endif()
```

### LTO (Link Time Optimization)
```cmake
# Enable LTO for Release
include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported)
if(ipo_supported AND CMAKE_BUILD_TYPE STREQUAL "Release")
    set_target_properties(xoron PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()
```

### Sanitizers
```cmake
# Address sanitizer for Debug
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(xoron PRIVATE -fsanitize=address)
    target_link_options(xoron PRIVATE -fsanitize=address)
endif()
```

### Coverage
```cmake
# Code coverage
option(XORON_COVERAGE "Enable code coverage" OFF)
if(XORON_COVERAGE)
    target_compile_options(xoron PRIVATE --coverage)
    target_link_options(xoron PRIVATE --coverage)
endif()
```

### Static Analysis
```cmake
# Static analysis (clang-tidy)
find_program(CLANG_TIDY clang-tidy)
if(CLANG_TIDY)
    set_target_properties(xoron PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY}")
endif()
```

### Sanitizers (Advanced)
```cmake
# Thread sanitizer
option(XORON_TSAN "Enable thread sanitizer" OFF)
if(XORON_TSAN)
    target_compile_options(xoron PRIVATE -fsanitize=thread)
    target_link_options(xoron PRIVATE -fsanitize=thread)
endif()

# Undefined behavior sanitizer
option(XORON_UBSAN "Enable UBSan" OFF)
if(XORON_UBSAN)
    target_compile_options(xoron PRIVATE -fsanitize=undefined)
    target_link_options(xoron PRIVATE -fsanitize=undefined)
endif()
```

## Dependency Management

### External Project Pattern
```cmake
# Pattern for building dependencies
ExternalProject_Add(
    dependency_name
    URL https://example.com/dependency.tar.gz
    CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR>
    BUILD_COMMAND make -j4
    INSTALL_COMMAND make install
)
```

### FetchContent vs ExternalProject
- **FetchContent**: Header-only or simple builds (httplib, lz4)
- **ExternalProject**: Complex builds (OpenSSL, Luau if needed)

### Version Pinning
```cmake
# Always pin versions
GIT_TAG 0.607  # Not main/master
GIT_TAG v1.9.4 # Specific release
```

## Cross-Platform Considerations

### Path Separators
```cmake
# CMake handles this automatically
file(TO_NATIVE_PATH "${path}" native_path)
```

### Line Endings
```cmake
# Set line endings for platform
set_property(TARGET xoron PROPERTY CXX_STANDARD 17)
```

### Architecture Detection
```cmake
# Detect architecture
if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|ARM64")
    # ARM64
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm|ARM")
    # ARM32
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    # x86_64
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "i686|i386")
    # x86
endif()
```

## Testing

### Unit Tests
```cmake
# Add test target
enable_testing()
add_executable(xoron_test test_main.cpp)
target_link_libraries(xoron_test xoron gtest)
add_test(NAME xoron_tests COMMAND xoron_test)
```

### Integration Tests
```cmake
# Integration test script
add_test(NAME xoron_integration
    COMMAND ${CMAKE_COMMAND} -E env
    LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/test_integration.sh)
```

## Packaging

### CPack Configuration
```cmake
# Package configuration
set(CPACK_PACKAGE_NAME "Xoron")
set(CPACK_PACKAGE_VERSION "2.0.0")
set(CPACK_PACKAGE_VENDOR "Xoron Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Xoron Executor Engine")

include(CPack)
```

### iOS Package
```cmake
# iOS framework bundle
if(XORON_IOS_BUILD)
    set_target_properties(xoron PROPERTIES
        FRAMEWORK TRUE
        FRAMEWORK_VERSION A
        MACOSX_FRAMEWORK_IDENTIFIER com.xoron.executor
        MACOSX_FRAMEWORK_INFO_PLIST ${CMAKE_SOURCE_DIR}/Info.plist)
endif()
```

### Android AAR
```cmake
# Android library package
if(XORON_ANDROID_BUILD)
    # Would create AAR structure
    # Includes lib, resources, manifest
endif()
```

## CI/CD Integration

### GitHub Actions
```yaml
# Example workflow
name: Build
on: [push, pull_request]
jobs:
  ios:
    runs-on: macos-latest
    steps:
      - uses: actions/checkout@v2
      - name: Build iOS
        run: |
          cmake -S . -B build -DXORON_IOS_BUILD=ON
          cmake --build build
```

### GitLab CI
```yaml
# Example .gitlab-ci.yml
build_android:
  image: android-ndk:latest
  script:
    - cmake -S . -B build -DXORON_ANDROID_BUILD=ON
    - cmake --build build
```

## Troubleshooting

### Common Issues

#### OpenSSL Not Found
```bash
# Solution 1: Provide pre-built
cmake -DXORON_OPENSSL_ROOT=/path/to/openssl

# Solution 2: Install system OpenSSL
# Ubuntu: sudo apt-get install libssl-dev
# macOS: brew install openssl
```

#### Android NDK Not Found
```bash
# Set NDK path
export ANDROID_NDK_HOME=/path/to/ndk
cmake -DANDROID_NDK_HOME=$ANDROID_NDK_HOME
```

#### iOS Toolchain Missing
```bash
# Use ios-cmake toolchain
cmake -DCMAKE_TOOLCHAIN_FILE=/path/to/ios.toolchain.cmake
```

#### Build Failures
```bash
# Clean and rebuild
rm -rf build
cmake -S . -B build [options]
cmake --build build --verbose
```

### Verbose Build
```bash
cmake --build build --verbose
```

### Clean Build
```bash
cmake --build build --target clean
cmake --build build
```

### Parallel Build
```bash
cmake --build build -- -j8
```

## Best Practices

### 1. Always Use Toolchains
```bash
# iOS
-DCMAKE_TOOLCHAIN_FILE=/path/to/ios.toolchain.cmake

# Android
-DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake
```

### 2. Pin Dependencies
```cmake
GIT_TAG 0.607  # Not main
```

### 3. Use Static Libraries for Mobile
```cmake
set(OPENSSL_USE_STATIC_LIBS TRUE)
set(ZLIB_USE_STATIC_LIBS ON)
```

### 4. Hide Symbols
```cmake
-fvisibility=hidden
```

### 5. Strip in Release
```cmake
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_target_properties(xoron PROPERTIES LINK_FLAGS "-s")
endif()
```

### 6. Validate API Levels
```cmake
if(ANDROID_API_LEVEL LESS 29)
    # Force minimum
endif()
```

### 7. Check Dependencies
```cmake
if(NOT OPENSSL_FOUND)
    message(FATAL_ERROR "OpenSSL required")
endif()
```

### 8. Provide Clear Messages
```cmake
message(STATUS "Building for iOS...")
message(WARNING "OpenSSL not found...")
message(FATAL_ERROR "Required dependency missing")
```

### 9. Use Cache Variables
```cmake
set(VAR "value" CACHE STRING "Description")
```

### 10. Document Options
```cmake
option(XORON_IOS_BUILD "Build for iOS ARM64 (.dylib) - Requires iOS 15+" OFF)
```

## Related Files
- `xoron.h` (API declarations)
- `xoron_luau.cpp` (Luau integration)
- `xoron_http.cpp` (HTTP client)
- `xoron_crypto.cpp` (Crypto functions)
- `xoron_filesystem.cpp` (File operations)
- `xoron_memory.cpp` (Memory management)
- `xoron_debug.cpp` (Debug utilities)
- `xoron_console.cpp` (Console output)
- `xoron_drawing.cpp` (Graphics)
- `xoron_websocket.cpp` (WebSocket)
- `xoron_input.cpp` (Input handling)
- `xoron_cache.cpp` (Caching)
- `xoron_ui.cpp` (UI components)
- `xoron_android.cpp` (Android-specific)
- `xoron_ios.mm` (iOS-specific)
