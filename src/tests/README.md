# Xoron Integration Tests

This directory contains comprehensive integration tests for Xoron on iOS and Android platforms.

## Structure

```
tests/
├── ios/           # iOS-specific integration tests
├── android/       # Android-specific integration tests
├── common/        # Cross-platform test utilities
└── README.md      # This file
```

## Test Categories

### 1. Platform Detection Tests
- Verify correct platform identification
- Check minimum version requirements
- Validate framework availability

### 2. Logging Tests
- iOS: NSLog integration
- Android: android_log integration
- Verify log output and formatting

### 3. Console Tests
- Console output functions
- Console clear and naming
- Platform-specific console handling

### 4. Environment Tests
- Environment variable access
- Platform information retrieval
- System configuration

### 5. Filesystem Tests
- File I/O operations
- Directory management
- Path resolution

### 6. Memory Tests
- Memory scanning
- Pattern matching
- Anti-detection mechanisms

### 7. Luau Integration Tests
- Script execution
- API binding
- Error handling

### 8. Drawing Tests
- Graphics operations
- UI rendering
- Platform-specific drawing

### 9. UI Tests
- View management
- Event handling
- Interface interactions

### 10. Network Tests
- HTTP requests
- WebSocket connections
- Network error handling

## Running Tests

### iOS Tests
```bash
# Build the iOS library first
cd /workspace/project/src/src
cmake -B build-ios -S . -G Ninja \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 \
  -DXORON_IOS_BUILD=ON

cmake --build build-ios

# Run iOS tests (requires Xcode and iOS device/simulator)
xcodebuild -project tests/ios/XoronTests.xcodeproj \
  -scheme XoronTests \
  -destination 'platform=iOS Simulator,name=iPhone 15' \
  test
```

### Android Tests
```bash
# Build the Android library first
cd /workspace/project/src/src
cmake -B build-android -S . -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-29 \
  -DXORON_ANDROID_BUILD=ON

cmake --build build-android

# Run Android tests (requires Android device/emulator)
cd tests/android
./gradlew connectedAndroidTest
```

## Test Coverage

All tests verify:
- ✅ Platform-specific logging (NSLog vs android_log)
- ✅ Framework usage (UIKit vs Android NDK)
- ✅ No debug output in production code
- ✅ Proper error handling
- ✅ Memory safety
- ✅ Thread safety
- ✅ API compatibility

## CI/CD Integration

Integration tests are automatically run via GitHub Actions:
- `.github/workflows/test-ios.yml` - iOS integration tests
- `.github/workflows/test-android.yml` - Android integration tests

See the `workflows/` directory for workflow definitions.
