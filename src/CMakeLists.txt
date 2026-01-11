cmake_minimum_required(VERSION 3.16)
project(xoron VERSION 2.0.0 LANGUAGES C CXX)

# Fix for older CMake versions with Luau subproject
if(POLICY CMP0057)
    cmake_policy(SET CMP0057 NEW)
endif()
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ============================================================================
# Build Options - iOS 15+ and Android 10+ ONLY
# ============================================================================
option(XORON_IOS_BUILD "Build for iOS ARM64 (.dylib) - Requires iOS 15+" OFF)
option(XORON_ANDROID_BUILD "Build for Android (.so) - Requires Android 10+ (API 29+)" OFF)

# Android ABI selection (arm64-v8a, armeabi-v7a, x86, x86_64)
set(XORON_ANDROID_ABI "arm64-v8a" CACHE STRING "Android ABI to build for")
# IMPORTANT: Android 10 = API 29, we require API 29 minimum
set(XORON_ANDROID_PLATFORM "android-29" CACHE STRING "Android platform version (minimum: android-29 for Android 10)")

# iOS deployment target - minimum iOS 15.0
set(XORON_IOS_DEPLOYMENT_TARGET "15.0" CACHE STRING "iOS deployment target (minimum: 15.0)")

# OpenSSL pre-built path (required for mobile builds)
set(XORON_OPENSSL_ROOT "" CACHE PATH "Path to pre-built OpenSSL for mobile")

# ============================================================================
# Platform Validation
# ============================================================================
if(NOT XORON_IOS_BUILD AND NOT XORON_ANDROID_BUILD)
    message(STATUS "No mobile platform specified, defaulting to development build")
endif()

# Validate Android API level
if(XORON_ANDROID_BUILD)
    string(REGEX MATCH "[0-9]+" ANDROID_API_LEVEL "${XORON_ANDROID_PLATFORM}")
    if(ANDROID_API_LEVEL LESS 29)
        message(WARNING "Android API level ${ANDROID_API_LEVEL} is below minimum (29). Setting to android-29.")
        set(XORON_ANDROID_PLATFORM "android-29" CACHE STRING "Android platform version" FORCE)
    endif()
endif()

# ============================================================================
# Platform Detection and Configuration
# ============================================================================
if(XORON_IOS_BUILD)
    message(STATUS "=== Building for iOS ARM64 (.dylib) ===")
    message(STATUS "iOS Deployment Target: ${XORON_IOS_DEPLOYMENT_TARGET}")
    
    # Enable Objective-C++ language for iOS builds
    enable_language(OBJCXX)
    
    # iOS platform definitions
    add_compile_definitions(TARGET_OS_IPHONE=1)
    add_compile_definitions(__APPLE__=1)
    add_compile_definitions(__arm64__=1)
    add_compile_definitions(XORON_PLATFORM_IOS=1)
    
    # iOS deployment target
    set(CMAKE_OSX_DEPLOYMENT_TARGET "${XORON_IOS_DEPLOYMENT_TARGET}")
    set(CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "${XORON_IOS_DEPLOYMENT_TARGET}")
    
    # iOS-specific compiler flags
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -fvisibility-inlines-hidden")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
    
elseif(XORON_ANDROID_BUILD)
    message(STATUS "=== Building for Android (.so) ===")
    message(STATUS "Android ABI: ${XORON_ANDROID_ABI}")
    message(STATUS "Android Platform: ${XORON_ANDROID_PLATFORM}")
    
    # Android platform definitions
    add_compile_definitions(__ANDROID__=1)
    add_compile_definitions(XORON_PLATFORM_ANDROID=1)
    add_compile_definitions(XORON_MIN_ANDROID_API=29)
    
    # Android-specific compiler flags
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -fvisibility=hidden -fvisibility-inlines-hidden")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -fvisibility=hidden")
    
    # Android architecture-specific definitions
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

# Find required packages
find_package(Threads REQUIRED)

# ZLIB handling - use shared library for development builds to avoid -fPIC issues
if(NOT XORON_IOS_BUILD AND NOT XORON_ANDROID_BUILD)
    # Development build - use shared zlib
    set(ZLIB_USE_STATIC_LIBS OFF)
else()
    # Mobile builds - use static zlib
    set(ZLIB_USE_STATIC_LIBS ON)
endif()
find_package(ZLIB)

# OpenSSL handling - REQUIRED for iOS and Android
# Use static libs only for mobile builds
if(XORON_IOS_BUILD OR XORON_ANDROID_BUILD)
    set(OPENSSL_USE_STATIC_LIBS TRUE)
else()
    set(OPENSSL_USE_STATIC_LIBS FALSE)
endif()

if(XORON_OPENSSL_ROOT AND EXISTS "${XORON_OPENSSL_ROOT}/include/openssl/ssl.h")
    # Use pre-built OpenSSL provided by user
    message(STATUS "Using pre-built OpenSSL from: ${XORON_OPENSSL_ROOT}")
    set(OPENSSL_INCLUDE_DIR "${XORON_OPENSSL_ROOT}/include")
    set(OPENSSL_SSL_LIBRARY "${XORON_OPENSSL_ROOT}/lib/libssl.a")
    set(OPENSSL_CRYPTO_LIBRARY "${XORON_OPENSSL_ROOT}/lib/libcrypto.a")
    set(OPENSSL_FOUND TRUE)
else()
    # Try to find system OpenSSL (for dev builds)
    find_package(OpenSSL QUIET)
    if(NOT OPENSSL_FOUND AND (XORON_IOS_BUILD OR XORON_ANDROID_BUILD))
        message(STATUS "OpenSSL not found - will build from source")
        set(BUILD_OPENSSL_FROM_SOURCE TRUE)
    endif()
endif()

# Build OpenSSL from source if needed for mobile
if(BUILD_OPENSSL_FROM_SOURCE)
    include(ExternalProject)
    
    set(OPENSSL_VERSION "3.2.0")
    set(OPENSSL_INSTALL_DIR "${CMAKE_BINARY_DIR}/openssl-install")
    set(OPENSSL_INCLUDE_DIR "${OPENSSL_INSTALL_DIR}/include")
    set(OPENSSL_SSL_LIBRARY "${OPENSSL_INSTALL_DIR}/lib/libssl.a")
    set(OPENSSL_CRYPTO_LIBRARY "${OPENSSL_INSTALL_DIR}/lib/libcrypto.a")
    
    if(XORON_ANDROID_BUILD)
        # Determine Android target
        if(ANDROID_ABI STREQUAL "arm64-v8a")
            set(OPENSSL_TARGET "android-arm64")
        elseif(ANDROID_ABI STREQUAL "armeabi-v7a")
            set(OPENSSL_TARGET "android-arm")
        elseif(ANDROID_ABI STREQUAL "x86_64")
            set(OPENSSL_TARGET "android-x86_64")
        elseif(ANDROID_ABI STREQUAL "x86")
            set(OPENSSL_TARGET "android-x86")
        else()
            set(OPENSSL_TARGET "android-arm64")
        endif()
        
        # Get NDK path
        if(CMAKE_ANDROID_NDK)
            set(NDK_PATH ${CMAKE_ANDROID_NDK})
        elseif(ANDROID_NDK)
            set(NDK_PATH ${ANDROID_NDK})
        elseif(DEFINED ENV{ANDROID_NDK_HOME})
            set(NDK_PATH $ENV{ANDROID_NDK_HOME})
        else()
            message(FATAL_ERROR "Android NDK not found. Set ANDROID_NDK_HOME environment variable.")
        endif()
        
        message(STATUS "Building OpenSSL ${OPENSSL_VERSION} for ${OPENSSL_TARGET}")
        message(STATUS "Using NDK: ${NDK_PATH}")
        
        # Extract API level from platform string (android-29 -> 29)
        string(REGEX MATCH "[0-9]+" OPENSSL_ANDROID_API "${XORON_ANDROID_PLATFORM}")
        if(NOT OPENSSL_ANDROID_API)
            set(OPENSSL_ANDROID_API 29)
        endif()
        message(STATUS "OpenSSL Android API: ${OPENSSL_ANDROID_API}")
        
        ExternalProject_Add(openssl_build
            URL https://www.openssl.org/source/openssl-${OPENSSL_VERSION}.tar.gz
            URL_HASH SHA256=14c826f07c7e433706fb5c69fa9e25dab95684844b4c962a2cf1bf183eb4690e
            DOWNLOAD_NO_PROGRESS TRUE
            PREFIX ${CMAKE_BINARY_DIR}/openssl
            CONFIGURE_COMMAND ${CMAKE_COMMAND} -E env
                ANDROID_NDK_ROOT=${NDK_PATH}
                PATH=${NDK_PATH}/toolchains/llvm/prebuilt/linux-x86_64/bin:$ENV{PATH}
                <SOURCE_DIR>/Configure ${OPENSSL_TARGET}
                -D__ANDROID_API__=${OPENSSL_ANDROID_API}
                --prefix=${OPENSSL_INSTALL_DIR}
                no-shared no-tests no-ui-console no-engine
            BUILD_COMMAND make -j4
            INSTALL_COMMAND make install_sw
            BUILD_IN_SOURCE 1
            BUILD_BYPRODUCTS "${OPENSSL_SSL_LIBRARY}" "${OPENSSL_CRYPTO_LIBRARY}"
        )
        set(OPENSSL_DEPENDS openssl_build)
        
    elseif(XORON_IOS_BUILD)
        message(STATUS "Building OpenSSL ${OPENSSL_VERSION} for iOS arm64")
        
        ExternalProject_Add(openssl_build
            URL https://www.openssl.org/source/openssl-${OPENSSL_VERSION}.tar.gz
            DOWNLOAD_NO_PROGRESS TRUE
            PREFIX ${CMAKE_BINARY_DIR}/openssl
            CONFIGURE_COMMAND <SOURCE_DIR>/Configure ios64-xcrun
                --prefix=${OPENSSL_INSTALL_DIR}
                no-shared no-tests no-ui-console no-engine
            BUILD_COMMAND make -j4
            INSTALL_COMMAND make install_sw
            BUILD_IN_SOURCE 1
            BUILD_BYPRODUCTS "${OPENSSL_SSL_LIBRARY}" "${OPENSSL_CRYPTO_LIBRARY}"
        )
        set(OPENSSL_DEPENDS openssl_build)
    endif()
    
    set(OPENSSL_FOUND TRUE)
    
    # Create IMPORTED library targets for OpenSSL
    # Use a wrapper library that depends on the external project
    add_library(openssl_external INTERFACE)
    add_dependencies(openssl_external openssl_build)
    
    add_library(OpenSSL::SSL STATIC IMPORTED GLOBAL)
    add_library(OpenSSL::Crypto STATIC IMPORTED GLOBAL)
    
    # Set properties with generator expressions to defer evaluation
    set_target_properties(OpenSSL::SSL PROPERTIES
        IMPORTED_LOCATION "${OPENSSL_SSL_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${OPENSSL_INCLUDE_DIR}"
    )
    set_target_properties(OpenSSL::Crypto PROPERTIES
        IMPORTED_LOCATION "${OPENSSL_CRYPTO_LIBRARY}"
    )
    
    # Add the dependency
    add_dependencies(OpenSSL::SSL openssl_build)
    add_dependencies(OpenSSL::Crypto openssl_build)
endif()

include(FetchContent)

# Fetch Luau
FetchContent_Declare(luau
    GIT_REPOSITORY https://github.com/Roblox/luau.git
    GIT_TAG 0.607)

# Fetch cpp-httplib
FetchContent_Declare(httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.14.3)

# Fetch LZ4
FetchContent_Declare(lz4
    GIT_REPOSITORY https://github.com/lz4/lz4.git
    GIT_TAG v1.9.4
    SOURCE_SUBDIR build/cmake)

# Luau build options
set(LUAU_BUILD_CLI OFF CACHE BOOL "" FORCE)
set(LUAU_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LUAU_BUILD_WEB OFF CACHE BOOL "" FORCE)

# LZ4 build options
set(LZ4_BUILD_CLI OFF CACHE BOOL "" FORCE)
set(LZ4_BUILD_LEGACY_LZ4C OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(luau httplib lz4)

# Main library sources
set(XORON_SOURCES
    xoron_luau.mm
    xoron_http.cpp
    xoron_crypto.cpp
    xoron_env.mm
    xoron_filesystem.mm
    xoron_memory.mm
    xoron_debug.cpp
    xoron_console.mm
    xoron_drawing.mm
    xoron_websocket.cpp
    xoron_input.cpp
    xoron_cache.cpp
    xoron_ui.mm)

# iOS-specific configuration
if(XORON_IOS_BUILD OR (APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin"))
    # Add iOS-specific source
    list(APPEND XORON_SOURCES xoron_ios.mm)
endif()

# Add Android-specific source for Android builds
if(XORON_ANDROID_BUILD)
    list(APPEND XORON_SOURCES xoron_android.cpp)
    # Android-specific libraries - will be linked after add_library
endif()

add_library(xoron SHARED ${XORON_SOURCES})

# Explicitly set .mm files to use Objective-C++ language for iOS builds
if(XORON_IOS_BUILD OR (APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin"))
    set_source_files_properties(
        xoron_env.mm
        xoron_console.mm
        xoron_filesystem.mm
        xoron_memory.mm
        xoron_luau.mm
        xoron_drawing.mm
        xoron_ui.mm
        PROPERTIES LANGUAGE OBJCXX
    )
endif()

# Link Android-specific libraries if building for Android
if(XORON_ANDROID_BUILD)
    target_link_libraries(xoron PRIVATE
        log      # Android logging
        android  # Android NDK
    )
endif()

target_include_directories(xoron PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${luau_SOURCE_DIR}/VM/include
    ${luau_SOURCE_DIR}/VM/src           # For internal headers (lstate.h, lobject.h, lfunc.h, etc.)
    ${luau_SOURCE_DIR}/Compiler/include
    ${luau_SOURCE_DIR}/Common/include
    ${httplib_SOURCE_DIR}
    ${lz4_SOURCE_DIR}/lib)

# Link libraries based on platform
target_link_libraries(xoron PRIVATE
    Luau.Compiler Luau.VM
    Threads::Threads
    lz4_static)

# OpenSSL linking - REQUIRED for iOS and Android
if(OPENSSL_FOUND)
    if(BUILD_OPENSSL_FROM_SOURCE)
        # Mobile builds with OpenSSL built from source - use IMPORTED targets
        target_link_libraries(xoron PRIVATE OpenSSL::SSL OpenSSL::Crypto)
        if(OPENSSL_DEPENDS)
            add_dependencies(xoron ${OPENSSL_DEPENDS})
        endif()
    elseif(XORON_IOS_BUILD OR XORON_ANDROID_BUILD)
        # Mobile builds with pre-built OpenSSL
        target_include_directories(xoron PRIVATE ${OPENSSL_INCLUDE_DIR})
        target_link_libraries(xoron PRIVATE ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY})
    else()
        # Dev builds - use CMake targets
        target_link_libraries(xoron PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    endif()
    target_compile_definitions(xoron PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
    message(STATUS "OpenSSL: ENABLED")
else()
    message(WARNING "OpenSSL not available - crypto features will be limited!")
    target_compile_definitions(xoron PRIVATE XORON_NO_OPENSSL=1)
endif()

# Platform-specific settings - iOS and Android ONLY
if(XORON_IOS_BUILD)
    set_target_properties(xoron PROPERTIES
        SUFFIX ".dylib"
        PREFIX "lib"
        OUTPUT_NAME "xoron")
    
    # Add iOS-specific compiler flags
    target_compile_options(xoron PRIVATE
        -fobjc-arc
        -fobjc-weak
    )
    
    # Ensure Foundation framework is properly linked
    target_link_libraries(xoron PRIVATE
        "-framework Foundation"
        "-framework CoreFoundation"
        "-framework CoreGraphics"
        "-framework CoreText"
        "-framework Security"
        "-framework UIKit"
        "-framework WebKit"
        "-framework QuartzCore"
        objc
    )
        
elseif(XORON_ANDROID_BUILD)
    set_target_properties(xoron PROPERTIES
        SUFFIX ".so"
        PREFIX "lib"
        OUTPUT_NAME "xoron")
    
    # Android-specific libraries
    target_link_libraries(xoron PRIVATE
        log      # Android logging
    )
    
    # Strip debug symbols for release builds
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        set_target_properties(xoron PROPERTIES
            LINK_FLAGS "-s")
    endif()
else()
    # Development build (for testing on host machine)
    message(STATUS "Development build - for testing only")
    if(APPLE)
        target_link_libraries(xoron PRIVATE
            "-framework Foundation"
            "-framework Security")
    elseif(UNIX)
        target_link_libraries(xoron PRIVATE dl)
    endif()
endif()

# Install rules
install(TARGETS xoron LIBRARY DESTINATION lib)
install(FILES xoron.h DESTINATION include)

# ============================================================================
# Build Configuration Summary
# ============================================================================
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
