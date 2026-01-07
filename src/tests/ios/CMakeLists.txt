# ============================================================================
# Xoron iOS Integration Tests - CMake Configuration
# ============================================================================

cmake_minimum_required(VERSION 3.20)
project(XoronIOSIntegrationTests)

# Fix for older CMake versions with Luau
if(POLICY CMP0057)
    cmake_policy(SET CMP0057 NEW)
endif()
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# iOS-specific settings
if(NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
    message(FATAL_ERROR "This CMakeLists.txt is for iOS only")
endif()

# Find required frameworks
find_library(FOUNDATION Foundation REQUIRED)
find_library(UIKIT UIKit REQUIRED)
find_library(XCTest XCTest REQUIRED)
find_library(COREGRAPHICS CoreGraphics REQUIRED)

# Test executable
add_executable(XoronIOSIntegrationTests
    test_ios_integration.mm
)

# Link frameworks
target_link_libraries(XoronIOSIntegrationTests
    ${FOUNDATION}
    ${UIKIT}
    ${XCTest}
    ${COREGRAPHICS}
)

# Include directories
target_include_directories(XoronIOSIntegrationTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

# Set iOS deployment target
set_target_properties(XoronIOSIntegrationTests PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
    XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "15.0"
    XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH "YES"
    XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO"
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ""
)

# Compiler flags
target_compile_options(XoronIOSIntegrationTests PRIVATE
    -fobjc-arc
    -fno-exceptions
    -fno-rtti
)

# Test configuration
enable_testing()
add_test(NAME iOSIntegrationTests
    COMMAND XoronIOSIntegrationTests
)
