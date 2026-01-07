# Xoron Integration Test Guide

## Overview

This guide provides comprehensive instructions for running integration tests for Xoron on iOS and Android platforms. These tests verify platform-specific functionality, logging, memory management, and performance.

## Test Structure

```
tests/
├── README.md                          # Main test documentation
├── INTEGRATION_TEST_GUIDE.md          # This file
├── common/
│   └── test_utils.h                   # Cross-platform test utilities
├── ios/
│   ├── test_ios_integration.mm        # iOS integration tests
│   ├── CMakeLists.txt                 # iOS test build configuration
│   ├── Info.plist                     # iOS app configuration
│   └── README.md                      # iOS-specific guide
└── android/
    ├── test_android_integration.cpp   # Android integration tests
    ├── XoronTestRunner.java           # Android test activity
    ├── AndroidManifest.xml            # Android manifest
    ├── build.gradle                   # Android build config
    ├── CMakeLists.txt                 # Android test build configuration
    └── README.md                      # Android-specific guide
```

## Test Categories

### 1. Platform Detection Tests
**Purpose**: Verify correct platform identification and framework availability

**iOS Tests**:
- ✓ XORON_PLATFORM_IOS defined
- ✓ XORON_PLATFORM_MACOS NOT defined
- ✓ UIKit available
- ✓ Foundation available
- ✓ CoreGraphics available

**Android Tests**:
- ✓ XORON_PLATFORM_ANDROID defined
- ✓ XORON_PLATFORM_IOS NOT defined
- ✓ Android NDK available
- ✓ JNI support available
- ✓ liblog available

### 2. Logging Tests
**Purpose**: Verify platform-specific logging works correctly

**iOS**: Uses `NSLog(@__VA_ARGS__)`
- XORON_LOG
- CONSOLE_LOG, CONSOLE_LOG_WARN, CONSOLE_LOG_ERROR
- ENV_LOG
- FS_LOG
- MEM_LOG

**Android**: Uses `__android_log_print()`
- XORON_LOG
- CONSOLE_LOG, CONSOLE_LOG_WARN, CONSOLE_LOG_ERROR
- ENV_LOG
- FS_LOG
- MEM_LOG

### 3. Console Tests
**Purpose**: Test console output functions

**Tests**:
- Console output formatting
- Platform-specific console handling
- Error and warning messages

### 4. Environment Tests
**Purpose**: Test environment variable and system information access

**iOS Tests**:
- Device information
- OS version
- System properties

**Android Tests**:
- Android version
- API level
- Device properties

### 5. Filesystem Tests
**Purpose**: Test file I/O operations

**Tests**:
- File creation
- File reading
- File writing
- Directory operations
- Path resolution

### 6. Memory Tests
**Purpose**: Test memory management and scanning

**Tests**:
- Memory allocation
- Pattern matching
- Memory scanning
- Anti-detection mechanisms

### 7. Luau Integration Tests
**Purpose**: Test Luau script integration

**Tests**:
- Script execution
- API binding
- Error handling
- Logging integration

### 8. Drawing Tests
**Purpose**: Test graphics and drawing operations

**iOS Tests**:
- CoreGraphics operations
- UIKit drawing
- View rendering

**Android Tests**:
- Canvas operations
- View drawing
- Color operations

### 9. UI Tests
**Purpose**: Test UI components and interactions

**Tests**:
- View creation
- Event handling
- Interface components

### 10. Network Tests
**Purpose**: Test network operations

**Tests**:
- HTTP client
- WebSocket client
- Network error handling

### 11. Thread Safety Tests
**Purpose**: Test concurrent operations

**Tests**:
- Multi-threaded logging
- Synchronization
- Race condition prevention

### 12. Error Handling Tests
**Purpose**: Test error and warning logging

**Tests**:
- Error messages
- Warning messages
- Format specifiers

### 13. Performance Tests
**Purpose**: Benchmark logging and operations

**Tests**:
- Logging performance
- Memory operation speed
- File I/O performance

## Running Tests

### iOS Integration Tests

#### Prerequisites
- macOS 14+ (Sonoma)
- Xcode 15+
- iOS Simulator or physical device
- CMake 3.20+
- Ninja

#### Method 1: Using CMake (Command Line)

```bash
cd /workspace/project/src/src

# Build the main library first
cmake -B build -S . -G Ninja \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 \
  -DXORON_IOS_BUILD=ON

cmake --build build

# Build tests
cd tests/ios
cmake -B build-tests -S . -G Ninja \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0

cmake --build build-tests

# Run on simulator
xcrun simctl boot <device-id>
xcrun simctl install <device-id> build-tests/XoronIOSIntegrationTests.app
xcrun simctl launch <device-id> com.xoron.ios.tests
```

#### Method 2: Using Xcode

```bash
cd /workspace/project/src/src/tests/ios
open XoronIOSIntegrationTests.xcodeproj
```

Then in Xcode:
1. Select target device (Simulator or Physical)
2. Build (⌘B)
3. Run (⌘R)
4. View results in Test Navigator

#### Method 3: Using GitHub Actions

The workflow `.github/workflows/test-ios.yml` automatically runs tests on every push/PR.

### Android Integration Tests

#### Prerequisites
- Android SDK (API 29+)
- Android NDK (r26+)
- CMake 3.20+
- Ninja
- Android device or emulator

#### Method 1: Using Gradle (Command Line)

```bash
cd /workspace/project/src/src/tests/android

# Build the main library first
cd ../..
cmake -B build -S . -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-29 \
  -DXORON_ANDROID_BUILD=ON
cmake --build build

# Build and run tests
cd src/tests/android
./gradlew assembleDebug
./gradlew connectedAndroidTest
```

#### Method 2: Using Android Studio

1. Open `tests/android` directory in Android Studio
2. Sync Gradle
3. Connect device or start emulator
4. Run `XoronTestRunner` activity
5. View results in Logcat

Filter Logcat with: `tag:XoronTest`

#### Method 3: Using ADB Directly

```bash
# Build test library
cd src/src/tests/android
cmake -B build-tests -S . -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-29
cmake --build build-tests

# Push library to device
adb push build-tests/libxoron_test_integration.so /data/local/tmp/

# Install APK
adb install -r build/outputs/apk/debug/android-tests-debug.apk

# Run tests
adb shell am start -n com.xoron.tests/.XoronTestRunner

# View results
adb logcat -d | grep XoronTest
```

#### Method 4: Using GitHub Actions

The workflow `.github/workflows/test-android.yml` automatically runs tests on every push/PR.

## Test Results Interpretation

### iOS Test Output

Successful test output:
```
[TEST] Platform Detection: PASSED
[TEST] NSLog Integration: PASSED
[TEST] Console Functions: PASSED
...
Test Summary
Passed: 13, Failed: 0, Total: 13
ALL TESTS PASSED ✓
```

### Android Test Output

Successful test output:
```
XoronTest: [PASS] Platform Detection: XORON_PLATFORM_ANDROID defined
XoronTest: [PASS] Platform Detection: XORON_PLATFORM_IOS not defined
XoronTest: [PASS] Logging: XORON_LOG macro
...
XoronTest: Passed: 13, Failed: 0, Total: 13
XoronTest: ALL TESTS PASSED ✓
```

### Common Issues

#### iOS Issues

1. **Simulator not found**
   ```bash
   # Create simulator
   xcrun simctl create "iPhone 15" "com.apple.CoreSimulator.SimDeviceType.iPhone-15"
   ```

2. **Code signing error**
   - Disable code signing in test target settings
   - Or use a development team

3. **Framework not found**
   - Verify Xcode is installed
   - Check SDK path: `xcrun --sdk iphoneos --show-sdk-path`

#### Android Issues

1. **Emulator not starting**
   ```bash
   # Check available AVDs
   avdmanager list avd
   
   # Create new AVD
   avdmanager create avd -n test -k "system-images;android-34;google_apis;arm64-v8a"
   ```

2. **Native library not loaded**
   - Verify library is in `jniLibs` directory
   - Check ABI matches device architecture

3. **Permission denied**
   - Add permissions to AndroidManifest.xml
   - Request runtime permissions for storage

## CI/CD Integration

### GitHub Actions Workflows

#### iOS Workflow (test-ios.yml)
- Triggers: push to main/develop, pull requests
- Jobs:
  1. Build iOS library
  2. Run integration tests (matrix: platform, logging, filesystem, memory, performance)
  3. Generate summary

#### Android Workflow (test-android.yml)
- Triggers: push to main/develop, pull requests
- Jobs:
  1. Build Android library
  2. Build test application
  3. Run tests on emulator
  4. Generate summary

### Workflow Triggers

```yaml
on:
  push:
    branches: [ main, develop ]
    paths:
      - 'src/**'
      - '.github/workflows/test-*.yml'
  pull_request:
    branches: [ main ]
  workflow_dispatch:  # Manual trigger
```

## Performance Benchmarks

### iOS Performance Targets
- Logging: < 1ms per message
- Memory allocation: < 10ms for 1MB
- File I/O: < 50ms for 1KB file

### Android Performance Targets
- Logging: < 2ms per message
- Memory allocation: < 15ms for 1MB
- File I/O: < 100ms for 1KB file

## Continuous Testing Strategy

### Pre-commit
- Run platform detection tests
- Verify logging macros compile
- Check for debug statements

### Pre-push
- Run full integration test suite
- Verify no regressions
- Check performance benchmarks

### CI/CD
- Automated testing on every PR
- Cross-platform validation
- Performance regression detection

## Troubleshooting

### General
1. Check test logs carefully
2. Verify platform-specific includes
3. Confirm library paths
4. Check compiler warnings

### iOS Specific
1. Verify Xcode version
2. Check simulator availability
3. Confirm deployment target
4. Review signing settings

### Android Specific
1. Verify NDK version
2. Check emulator state
3. Confirm API level compatibility
4. Review ABI settings

## Additional Resources

- [Xoron Documentation](../doc/README.md)
- [Build Configuration](../workflows/README.md)
- [Platform-Specific Guides](../doc/platforms/)
- [API Reference](../doc/api/)

## Support

For issues or questions:
1. Check this guide
2. Review test logs
3. Check GitHub Issues
4. Contact development team

---

**Last Updated**: 2026-01-06
**Version**: 2.0.0
**Platform Support**: iOS 15+, Android API 29+
