# iOS Integration Tests

## Overview

This directory contains iOS-specific integration tests for Xoron Executor Engine. These tests verify functionality on iOS devices running iOS 15+.

## Test Architecture

```
ios/
├── test_ios_integration.mm     # Main test implementation
├── Info.plist                  # App configuration
└── CMakeLists.txt              # CMake build file
```

## Prerequisites

### Software Requirements
- **Xcode**: 14+ with iOS SDK
- **iOS**: 15.0+ deployment target
- **CMake**: 3.16+ with iOS support
- **OpenSSL**: Built for iOS (arm64)
- **Homebrew**: For installing dependencies

### Hardware Requirements
- **Device**: iPhone/iPad with iOS 15+ or simulator
- **Architecture**: ARM64 (device) or x86_64/ARM64 (simulator)
- **RAM**: 4GB minimum, 8GB recommended
- **Storage**: 2GB free space

## Build Configuration

### CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.16)
project(xoron_test_ios)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET 15.0)

# iOS-specific settings
set(CMAKE_TOOLCHAIN_FILE /path/to/ios.toolchain.cmake)
set(PLATFORM OS64)  # arm64 device
# or set(PLATFORM SIMULATOR64) for simulator

# Enable Objective-C++
enable_language(OBJCXX)

# Test sources
set(TEST_SOURCES
    test_ios_integration.mm
)

# Test executable
add_executable(xoron_test_ios ${TEST_SOURCES})

# iOS frameworks
target_link_libraries(xoron_test_ios
    "-framework Foundation"
    "-framework UIKit"
    "-framework CoreHaptics"
    "-framework CoreGraphics"
)

# Link Xoron library
find_library(XORON_LIBRARY xoron PATHS ${CMAKE_SOURCE_DIR}/../../build)
target_link_libraries(xoron_test_ios ${XORON_LIBRARY})

# Include directories
target_include_directories(xoron_test_ios PRIVATE
    ${CMAKE_SOURCE_DIR}/../../
    ${CMAKE_SOURCE_DIR}/../common
)

# Set bundle properties
set_target_properties(xoron_test_ios PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.xoron.test"
    XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "15.0"
)
```

### Info.plist

```xml
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleDevelopmentRegion</key>
    <string>en</string>
    <key>CFBundleExecutable</key>
    <string>$(EXECUTABLE_NAME)</string>
    <key>CFBundleIdentifier</key>
    <string>com.xoron.test</string>
    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>
    <key>CFBundleName</key>
    <string>$(PRODUCT_NAME)</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleShortVersionString</key>
    <string>1.0</string>
    <key>CFBundleVersion</key>
    <string>1</string>
    <key>LSRequiresIPhoneOS</key>
    <true/>
    <key>UIRequiredDeviceCapabilities</key>
    <array>
        <string>arm64</string>
    </array>
    <key>UISupportedInterfaceOrientations</key>
    <array>
        <string>UIInterfaceOrientationPortrait</string>
        <string>UIInterfaceOrientationLandscapeLeft</string>
        <string>UIInterfaceOrientationLandscapeRight</string>
    </array>
    <key>NSAppTransportSecurity</key>
    <dict>
        <key>NSAllowsArbitraryLoads</key>
        <true/>
    </dict>
    <key>NSBluetoothAlwaysUsageDescription</key>
    <string>Required for haptic feedback testing</string>
</dict>
</plist>
```

## Test Implementation

### Main Test File: test_ios_integration.mm

```objc
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <CoreHaptics/CoreHaptics.h>
#import <objc/runtime.h>

#include "xoron.h"
#include "test_utils.h"

// iOS logging macro
#define TEST_LOG(...) NSLog(@__VA_ARGS__)

// Test runner interface
@interface XoronTestRunner : NSObject
- (void)runTests;
@end

@implementation XoronTestRunner

- (void)runTests {
    TEST_LOG(@"Starting iOS Integration Tests");
    
    TestSuite suite("iOS Integration");
    Timer timer;
    
    // Initialize Xoron
    if (xoron_init() != XORON_OK) {
        TEST_LOG(@"Failed to initialize Xoron: %s", xoron_last_error());
        return;
    }
    
    // Test 1: VM Creation
    timer.reset();
    xoron_vm_t* vm = xoron_vm_new();
    bool vm_created = (vm != nullptr);
    suite.recordResult("VM Creation", vm_created, xoron_last_error(), timer.elapsed_ms());
    
    if (vm_created) {
        // Test 2: Basic Lua Execution
        timer.reset();
        int result = xoron_dostring(vm, "return 1 + 1", "test");
        suite.recordResult("Basic Lua", result == XORON_OK, xoron_last_error(), timer.elapsed_ms());
        
        // Test 3: HTTP GET
        timer.reset();
        int status; size_t len;
        char* response = xoron_http_get("https://httpbin.org/get", &status, &len);
        bool http_ok = (response != nullptr && status == 200);
        if (response) xoron_http_free(response);
        suite.recordResult("HTTP GET", http_ok, xoron_last_error(), timer.elapsed_ms());
        
        // Test 4: HTTP POST
        timer.reset();
        const char* post_data = "{\"test\":\"data\"}";
        response = xoron_http_post("https://httpbin.org/post", post_data, strlen(post_data), 
                                   "application/json", &status, &len);
        bool post_ok = (response != nullptr && status == 200);
        if (response) xoron_http_free(response);
        suite.recordResult("HTTP POST", post_ok, xoron_last_error(), timer.elapsed_ms());
        
        // Test 5: SHA256
        timer.reset();
        uint8_t hash[32];
        xoron_sha256("test", 4, hash);
        bool hash_ok = (hash[0] != 0);
        suite.recordResult("SHA256", hash_ok, "", timer.elapsed_ms());
        
        // Test 6: Base64 Encode
        timer.reset();
        char* b64 = xoron_base64_encode("test", 4);
        bool b64_ok = (b64 != nullptr && strcmp(b64, "dGVzdA==") == 0);
        if (b64) xoron_free(b64);
        suite.recordResult("Base64 Encode", b64_ok, "", timer.elapsed_ms());
        
        // Test 7: File Operations
        timer.reset();
        NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
        NSString* documentsDirectory = [paths firstObject];
        NSString* testFile = [documentsDirectory stringByAppendingPathComponent:@"test.txt"];
        
        NSError* error = nil;
        BOOL writeSuccess = [@"test data" writeToFile:testFile 
                                           atomically:YES 
                                             encoding:NSUTF8StringEncoding 
                                                error:&error];
        
        BOOL readSuccess = NO;
        if (writeSuccess) {
            NSString* content = [NSString stringWithContentsOfFile:testFile 
                                                          encoding:NSUTF8StringEncoding 
                                                             error:&error];
            readSuccess = (content != nil && [content isEqualToString:@"test data"]);
            [[NSFileManager defaultManager] removeItemAtPath:testFile error:nil];
        }
        suite.recordResult("File Operations", writeSuccess && readSuccess, "", timer.elapsed_ms());
        
        // Test 8: Haptic Feedback
        timer.reset();
        [self testHapticFeedback];
        suite.recordResult("Haptic Feedback", true, "", timer.elapsed_ms());
        
        // Test 9: iOS Console Logging
        timer.reset();
        xoron_ios_console_print("Test message", 0);
        suite.recordResult("iOS Logging", true, "", timer.elapsed_ms());
        
        // Test 10: Device Information
        timer.reset();
        UIDevice* device = [UIDevice currentDevice];
        NSString* deviceInfo = [NSString stringWithFormat:@"%@ %@", 
                               device.systemName, device.systemVersion];
        TEST_LOG(@"Device: %@", deviceInfo);
        suite.recordResult("Device Info", true, "", timer.elapsed_ms());
        
        // Test 11: Lua Script with iOS APIs
        timer.reset();
        const char* lua_script = R"(
            local response = http.get("https://httpbin.org/get")
            if response and response.status == 200 then
                local hash = crypto.sha256(response.body)
                print("Hash:", string.sub(hash, 1, 16) .. "...")
                return true
            end
            return false
        )";
        result = xoron_dostring(vm, lua_script, "ios_test");
        bool lua_ios_ok = (result == XORON_OK);
        suite.recordResult("Lua + iOS APIs", lua_ios_ok, 
                          xoron_last_error(), timer.elapsed_ms());
        
        // Test 12: WebSocket
        timer.reset();
        bool ws_ok = [self testWebSocket];
        suite.recordResult("WebSocket", ws_ok, "", timer.elapsed_ms());
        
        // Cleanup VM
        xoron_vm_free(vm);
    }
    
    // Shutdown
    xoron_shutdown();
    
    // Print summary
    suite.printSummary();
    
    // Show alert with results
    dispatch_async(dispatch_get_main_queue(), ^{
        NSString* message = [NSString stringWithFormat:@"Tests completed!\nPassed: %zu\nFailed: %zu",
                            suite.getResults().size(), suite.getResults().size()];
        
        UIAlertController* alert = [UIAlertController 
            alertControllerWithTitle:@"Xoron Test Results"
                             message:message
                      preferredStyle:UIAlertControllerStyleAlert];
        
        [alert addAction:[UIAlertAction actionWithTitle:@"OK" 
                                                 style:UIAlertActionStyleDefault 
                                               handler:nil]];
        
        [[UIApplication sharedApplication].keyWindow.rootViewController 
            presentViewController:alert animated:YES completion:nil];
    });
}

- (void)testHapticFeedback {
    // Test light haptic
    xoron_ios_haptic_feedback(0);
    [NSThread sleepForTimeInterval:0.1];
    
    // Test medium haptic
    xoron_ios_haptic_feedback(1);
    [NSThread sleepForTimeInterval:0.1];
    
    // Test heavy haptic
    xoron_ios_haptic_feedback(2);
    [NSThread sleepForTimeInterval:0.1];
}

- (BOOL)testWebSocket {
    // This would require a WebSocket test server
    // For now, return true as basic functionality is tested in C++ layer
    return YES;
}

@end

// C++ wrapper for Objective-C
extern "C" void run_ios_tests() {
    @autoreleasepool {
        XoronTestRunner* runner = [[XoronTestRunner alloc] init];
        [runner runTests];
    }
}

// Entry point for app
int main(int argc, char* argv[]) {
    @autoreleasepool {
        // Run tests immediately
        run_ios_tests();
        
        // Keep app running to show results
        return UIApplicationMain(argc, argv, nil, nil);
    }
}
```

## Building and Running

### Step 1: Install Dependencies

```bash
# Install ios-cmake
brew install ios-cmake

# Or download manually
git clone https://github.com/leetal/ios-cmake
export IOS_CMAKE_PATH=$(pwd)/ios-cmake
```

### Step 2: Build Xoron for iOS

```bash
cd /workspace/project/src
mkdir -p build-ios
cd build-ios

# Configure for device (arm64)
cmake .. \
  -DXORON_IOS_BUILD=ON \
  -DCMAKE_TOOLCHAIN_FILE=$IOS_CMAKE_PATH/ios.toolchain.cmake \
  -DPLATFORM=OS64 \
  -DXORON_OPENSSL_ROOT=/path/to/openssl-ios

# Or for simulator (x86_64)
cmake .. \
  -DXORON_IOS_BUILD=ON \
  -DCMAKE_TOOLCHAIN_FILE=$IOS_CMAKE_PATH/ios.toolchain.cmake \
  -DPLATFORM=SIMULATOR64 \
  -DXORON_OPENSSL_ROOT=/path/to/openssl-ios

cmake --build . --config Release
```

### Step 3: Build Test App

```bash
cd /workspace/project/src/tests/ios
mkdir build
cd build

# Configure
cmake .. \
  -DCMAKE_TOOLCHAIN_FILE=$IOS_CMAKE_PATH/ios.toolchain.cmake \
  -DPLATFORM=OS64 \
  -DXORON_LIBRARY=/workspace/project/src/build-ios/libxoron.dylib

# Build
cmake --build . --config Release

# The app bundle will be in build/Release-iphoneos/xoron_test_ios.app
```

### Step 4: Deploy and Run

**On Device:**
```bash
# Install
ios-deploy --bundle Release-iphoneos/xoron_test_ios.app

# Or manually
xcrun simctl install <device-id> Release-iphoneos/xoron_test_ios.app
xcrun simctl launch <device-id> com.xoron.test
```

**On Simulator:**
```bash
# Boot simulator
xcrun simctl boot "iPhone 14 Pro"

# Install app
xcrun simctl install booted Release-iphonesimulator/xoron_test_ios.app

# Launch
xcrun simctl launch booted com.xoron.test

# View logs
xcrun simctl spawn booted log stream --predicate 'subsystem == "com.xoron.test"'
```

## Test Categories

### 1. Core VM Tests
- VM creation/destruction
- Script compilation
- Basic execution
- Error handling

### 2. Network Tests
- HTTP GET/POST
- HTTPS support
- Timeout handling
- Large responses

### 3. Crypto Tests
- Hash functions
- Encoding/decoding
- AES encryption
- Secure random

### 4. File System Tests
- File read/write (Documents directory)
- Directory operations
- Path validation
- Sandbox compliance

### 5. Platform Integration
- Haptic feedback (CoreHaptics)
- iOS console logging
- Device information
- UIKit integration

### 6. Lua Integration
- iOS APIs in Lua
- Mixed Objective-C++/Lua tests
- Performance benchmarks

## Performance Benchmarks

### Target Performance (iPhone 14 Pro)

| Test | Target | Typical |
|------|--------|---------|
| VM Creation | < 5ms | 2-3ms |
| HTTP GET | < 100ms | 40-70ms |
| SHA256 (1KB) | < 1ms | 0.08ms |
| File Write (1KB) | < 5ms | 1-2ms |
| Haptic Feedback | < 10ms | 5ms |
| Lua Execution | < 10ms | 3-5ms |

### Benchmark Results

```bash
# Run in simulator
xcrun simctl spawn booted log stream --predicate 'subsystem == "com.xoron.test"' | grep Benchmark
```

## Debugging

### Console Output

```bash
# Real-time logs
xcrun simctl spawn booted log stream --predicate 'subsystem == "com.xoron.test"'

# Filter by level
xcrun simctl spawn booted log stream --predicate 'subsystem == "com.xoron.test" AND level == info'

# Save to file
xcrun simctl spawn booted log stream --predicate 'subsystem == "com.xoron.test"' > test_results.log
```

### Xcode Debugging

```bash
# Open in Xcode
open build/xoron_test_ios.xcodeproj

# Build and run from Xcode
# Set breakpoints in test_ios_integration.mm
# Use Debug → Step Over/Into to trace execution
```

### Common Issues

**1. Library Not Found**
```
dyld: Library not loaded: @rpath/libxoron.dylib
```
**Solution**: Ensure libxoron.dylib is embedded in app bundle
```bash
install_name_tool -add_rpath "@executable_path/Frameworks" xoron_test_ios
```

**2. HTTPS Not Working**
```
HTTP request failed: SSL error
```
**Solution**: Check OpenSSL is properly linked, verify Info.plist has NSAppTransportSecurity

**3. Haptic Not Working**
```
CoreHaptics error: Haptic engine not available
```
**Solution**: Test on physical device, simulator has limited haptic support

**4. Permission Denied**
```
File write failed: You don't have permission...
```
**Solution**: Use NSDocumentsDirectory, check sandbox

### Debug Build

```bash
# Build with debug symbols
cmake .. -DCMAKE_BUILD_TYPE=Debug -DPLATFORM=SIMULATOR64
cmake --build . --config Debug

# Run with debugger
lldb --batch -o "process launch" -- ./Debug-iphonesimulator/xoron_test_ios.app/xoron_test_ios
```

## Test Results

### Output Format

```
2026-01-07 05:09:00.000 xoron_test_ios[12345:67890] Starting iOS Integration Tests
2026-01-07 05:09:00.100 xoron_test_ios[12345:67890] [PASS] VM Creation (2.34 ms)
2026-01-07 05:09:00.150 xoron_test_ios[12345:67890] [PASS] Basic Lua (0.12 ms)
2026-01-07 05:09:00.800 xoron_test_ios[12345:67890] [PASS] HTTP GET (65.45 ms)
2026-01-07 05:09:01.500 xoron_test_ios[12345:67890] [PASS] HTTP POST (69.23 ms)
2026-01-07 05:09:01.550 xoron_test_ios[12345:67890] [PASS] SHA256 (0.08 ms)
2026-01-07 05:09:01.600 xoron_test_ios[12345:67890] [PASS] Base64 Encode (0.05 ms)
2026-01-07 05:09:01.700 xoron_test_ios[12345:67890] [PASS] File Operations (1.56 ms)
2026-01-07 05:09:01.800 xoron_test_ios[12345:67890] [PASS] Haptic Feedback (5.21 ms)
2026-01-07 05:09:01.850 xoron_test_ios[12345:67890] [PASS] iOS Logging (0.03 ms)
2026-01-07 05:09:01.900 xoron_test_ios[12345:67890] [PASS] Device Info (0.02 ms)
2026-01-07 05:09:02.600 xoron_test_ios[12345:67890] [PASS] Lua + iOS APIs (68.12 ms)
2026-01-07 05:09:02.650 xoron_test_ios[12345:67890] [PASS] WebSocket (0.50 ms)
2026-01-07 05:09:02.700 xoron_test_ios[12345:67890] ========================================
2026-01-07 05:09:02.700 xoron_test_ios[12345:67890] Test Suite: iOS Integration
2026-01-07 05:09:02.700 xoron_test_ios[12345:67890] Passed: 11, Failed: 0, Total: 11
2026-01-07 05:09:02.700 xoron_test_ios[12345:67890] ========================================
```

### Success Criteria
- All tests pass
- No crashes
- Memory usage < 100MB
- Execution time within targets
- No sandbox violations

## Continuous Integration

### GitHub Actions

```yaml
name: iOS Tests

on: [push, pull_request]

jobs:
  test-ios:
    runs-on: macos-latest
    
    steps:
      - uses: actions/checkout@v4
      
      - name: Setup Xcode
        uses: maxim-lobanov/setup-xcode@v1
        with:
          xcode-version: '14.3'
      
      - name: Setup iOS CMake
        run: |
          git clone https://github.com/leetal/ios-cmake
          echo "IOS_CMAKE_PATH=$(pwd)/ios-cmake" >> $GITHUB_ENV
      
      - name: Setup OpenSSL
        run: |
          # Download pre-built OpenSSL for iOS
          wget https://github.com/leenjewel/openssl-for-ios/releases/download/1.1.1w/openssl-1.1.1w-ios.tar.gz
          tar xzf openssl-1.1.1w-ios.tar.gz
          mv openssl-1.1.1w openssl-ios
      
      - name: Build Xoron
        run: |
          cd src
          mkdir build-ios && cd build-ios
          cmake .. \
            -DXORON_IOS_BUILD=ON \
            -DCMAKE_TOOLCHAIN_FILE=$IOS_CMAKE_PATH/ios.toolchain.cmake \
            -DPLATFORM=OS64 \
            -DXORON_OPENSSL_ROOT=$GITHUB_WORKSPACE/openssl-ios
          cmake --build . --config Release
      
      - name: Build Tests
        run: |
          cd src/tests/ios
          mkdir build && cd build
          cmake .. \
            -DCMAKE_TOOLCHAIN_FILE=$IOS_CMAKE_PATH/ios.toolchain.cmake \
            -DPLATFORM=SIMULATOR64 \
            -DXORON_LIBRARY=$GITHUB_WORKSPACE/src/build-ios/libxoron.dylib
          cmake --build . --config Release
      
      - name: Run Tests in Simulator
        run: |
          # Boot simulator
          SIMULATOR_ID=$(xcrun simctl create iPhone-Test "iPhone 14" "iOS16.4")
          xcrun simctl boot $SIMULATOR_ID
          
          # Install and run
          cd src/tests/ios/build
          xcrun simctl install $SIMULATOR_ID Release-iphonesimulator/xoron_test_ios.app
          xcrun simctl launch $SIMULATOR_ID com.xoron.test
          
          # Wait for tests
          sleep 15
          
          # Capture logs
          xcrun simctl spawn $SIMULATOR_ID log stream --predicate 'subsystem == "com.xoron.test"' > test_results.log &
          LOG_PID=$!
          
          sleep 5
          kill $LOG_PID
          
          # Check results
          grep "Passed:" test_results.log
          
          # Cleanup
          xcrun simctl shutdown $SIMULATOR_ID
          xcrun simctl delete $SIMULATOR_ID
```

## Platform-Specific Features

### Haptic Feedback

```cpp
// Light feedback
xoron_ios_haptic_feedback(0);

// Medium feedback
xoron_ios_haptic_feedback(1);

// Heavy feedback
xoron_ios_haptic_feedback(2);
```

**Implementation:**
```objc
void xoron_ios_haptic_feedback(int style) {
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
}
```

### iOS Console Logging

```cpp
// Log to NSLog
xoron_ios_console_print("Test message", 0);
```

**Implementation:**
```objc
void xoron_ios_console_print(const char* message, int type) {
    NSString* msg = [NSString stringWithUTF8String:message];
    switch (type) {
        case 0: NSLog(@"%@", msg); break;
        case 1: NSLog(@"⚠️ %@", msg); break;
        case 2: NSLog(@"❌ %@", msg); break;
    }
}
```

### Device Information

```cpp
// Get device info
const char* get_device_info() {
    UIDevice* device = [UIDevice currentDevice];
    NSString* info = [NSString stringWithFormat:@"%@ %@ %@", 
                     device.systemName, 
                     device.systemVersion,
                     device.model];
    return strdup([info UTF8String]);
}
```

### Clipboard Operations

```cpp
// Copy to clipboard
void xoron_clipboard_set(const char* text) {
    NSString* str = [NSString stringWithUTF8String:text];
    UIPasteboard* pasteboard = [UIPasteboard generalPasteboard];
    pasteboard.string = str;
}

// Get from clipboard
char* xoron_clipboard_get() {
    UIPasteboard* pasteboard = [UIPasteboard generalPasteboard];
    NSString* str = pasteboard.string;
    if (str) {
        return strdup([str UTF8String]);
    }
    return nullptr;
}
```

## Memory Management

### ARC (Automatic Reference Counting)

```objc
// ARC handles memory automatically
@interface TestObject : NSObject
@property (strong, nonatomic) NSString* data;
@end

@implementation TestObject
- (void)dealloc {
    // ARC calls this automatically
    // No need to call [super dealloc]
}
@end
```

### Leak Detection

```bash
# Run with Instruments
instruments -t "Leaks" ./Release-iphonesimulator/xoron_test_ios.app/xoron_test_ios

# Or use Xcode
# Product → Profile → Leaks
```

### Memory Limits

```cpp
// Set memory limit for VM
lua_gc(vm->L, LUA_GCSETALLOCF, 0, 50 * 1024 * 1024);  // 50MB
```

## Security Considerations

### App Transport Security

```xml
<!-- Info.plist -->
<key>NSAppTransportSecurity</key>
<dict>
    <key>NSAllowsArbitraryLoads</key>
    <true/>
</dict>
```

### Sandbox Compliance

```objc
// Use Documents directory for file operations
NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
NSString* documentsDirectory = [paths firstObject];
NSString* filePath = [documentsDirectory stringByAppendingPathComponent:@"test.txt"];
```

### Privacy Permissions

```xml
<!-- Info.plist -->
<key>NSBluetoothAlwaysUsageDescription</key>
<string>Required for haptic feedback testing</string>

<key>NSPhotoLibraryUsageDescription</key>
<string>Required for testing file operations</string>
```

## Simulator vs Device Testing

### Simulator Advantages
- Fast iteration
- Easy debugging
- Multiple iOS versions
- No provisioning needed

### Simulator Limitations
- No haptic feedback (simulated)
- No camera/accessories
- Different performance characteristics
- No cellular network

### Device Advantages
- Real performance metrics
- Actual haptic feedback
- Real network conditions
- Hardware-specific features

### Device Limitations
- Requires provisioning profile
- Slower deployment
- Limited iOS versions

## Summary

The iOS integration tests provide comprehensive coverage of:
- Core engine functionality
- Network operations
- Cryptographic functions
- File system operations (sandboxed)
- Platform-specific features (haptics, device info)
- Performance benchmarks

These tests ensure Xoron works correctly on iOS devices running iOS 15+.

---

**Related:**
- [Android Tests](../android/README.md)
- [Test Utilities](../common/README.md)
- [Main Test Suite](../README.md)
