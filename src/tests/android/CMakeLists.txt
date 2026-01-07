# ============================================================================
# Xoron Android Integration Tests - CMake Configuration
# ============================================================================

cmake_minimum_required(VERSION 3.20)
project(XoronAndroidIntegrationTests)

# Fix for older CMake versions with Luau
if(POLICY CMP0057)
    cmake_policy(SET CMP0057 NEW)
endif()
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Android-specific settings
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Android")
    message(FATAL_ERROR "This CMakeLists.txt is for Android only")
endif()

# Test library
add_library(xoron_test_integration SHARED
    test_android_integration.cpp
)

# Include directories
target_include_directories(xoron_test_integration PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

# Link libraries
target_link_libraries(xoron_test_integration
    log  # Android logging library
    android  # Android NDK
)

# Compiler flags
target_compile_options(xoron_test_integration PRIVATE
    -fno-exceptions
    -fno-rtti
    -Wall
    -Wextra
)

# Test configuration
enable_testing()

# Add test (requires Android device/emulator)
add_test(NAME AndroidIntegrationTests
    COMMAND adb shell am instrument -w com.xoron.tests/androidx.test.runner.AndroidJUnitRunner
)

# Build configuration summary
message(STATUS "========================================")
message(STATUS "Xoron Android Integration Tests")
message(STATUS "========================================")
message(STATUS "Target: libxoron_test_integration.so")
message(STATUS "Platform: Android API ${ANDROID_PLATFORM}")
message(STATUS "ABI: ${ANDROID_ABI}")
message(STATUS "C++ Standard: C++17")
message(STATUS "========================================")
