# Android Integration Tests

## Overview

This directory contains Android-specific integration tests for Xoron Executor Engine. These tests verify functionality on Android devices running API 29+ (Android 10+).

## Test Architecture

```
android/
├── test_android_integration.cpp  # Main test implementation
├── XoronTestRunner.java          # Android test runner
├── AndroidManifest.xml           # App manifest
├── build.gradle                  # Build configuration
└── CMakeLists.txt                # CMake build file
```

## Prerequisites

### Software Requirements
- **Android NDK**: r25.2.9519653 or newer
- **Android SDK**: API 29+ (Android 10)
- **CMake**: 3.16+
- **OpenSSL**: Built for Android (arm64-v8a, armeabi-v7a, x86, x86_64)
- **Java**: JDK 11+
- **Android Studio**: Optional, for debugging

### Hardware Requirements
- **Device**: Android 10+ device or emulator
- **Architecture**: ARM64 (preferred) or ARM32, x86, x86_64
- **RAM**: 4GB minimum, 8GB recommended
- **Storage**: 2GB free space

## Build Configuration

### CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.16)
project(xoron_test_android)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Android-specific settings
set(CMAKE_TOOLCHAIN_FILE $ENV{ANDROID_NDK}/build/cmake/android.toolchain.cmake)
set(ANDROID_ABI arm64-v8a)
set(ANDROID_PLATFORM android-29)
set(ANDROID_STL c++_shared)

# Find Xoron library
find_library(XORON_LIBRARY xoron PATHS ${CMAKE_SOURCE_DIR}/../../build)

# Test sources
set(TEST_SOURCES
    test_android_integration.cpp
)

# Test executable
add_executable(xoron_test_android ${TEST_SOURCES})

# Link libraries
target_link_libraries(xoron_test_android
    ${XORON_LIBRARY}
    log       # Android logging
    android   # Android NDK
)

# Include directories
target_include_directories(xoron_test_android PRIVATE
    ${CMAKE_SOURCE_DIR}/../../
    ${CMAKE_SOURCE_DIR}/../common
)
```

### build.gradle

```gradle
apply plugin: 'com.android.application'

android {
    compileSdkVersion 33
    buildToolsVersion "33.0.0"

    defaultConfig {
        applicationId "com.xoron.test"
        minSdkVersion 29
        targetSdkVersion 33
        versionCode 1
        versionName "1.0"
        
        externalNativeBuild {
            cmake {
                arguments "-DANDROID_STL=c++_shared"
            }
        }
    }

    buildTypes {
        release {
            minifyEnabled false
            proguardFiles getDefaultProguardFile('proguard-android.txt'), 'proguard-rules.pro'
        }
    }

    externalNativeBuild {
        cmake {
            path "CMakeLists.txt"
        }
    }
}

dependencies {
    implementation fileTree(dir: 'libs', include: ['*.jar'])
}
```

### AndroidManifest.xml

```xml
<?xml version="1.0" encoding="utf-8"?>
<manifest xmlns:android="http://schemas.android.com/apk/res/android"
    package="com.xoron.test">

    <uses-permission android:name="android.permission.INTERNET" />
    <uses-permission android:name="android.permission.ACCESS_NETWORK_STATE" />
    <uses-permission android:name="android.permission.VIBRATE" />
    <uses-permission android:name="android.permission.WRITE_EXTERNAL_STORAGE" />
    <uses-permission android:name="android.permission.READ_EXTERNAL_STORAGE" />

    <application
        android:allowBackup="true"
        android:label="Xoron Test Runner"
        android:theme="@android:style/Theme.Material.Light">
        
        <activity android:name=".TestRunner"
            android:exported="true">
            <intent-filter>
                <action android:name="android.intent.action.MAIN" />
                <category android:name="android.intent.category.LAUNCHER" />
            </intent-filter>
        </activity>
    </application>
</manifest>
```

## Test Implementation

### Main Test File: test_android_integration.cpp

```cpp
#include "xoron.h"
#include "test_utils.h"
#include <android/log.h>
#include <jni.h>

// Android logging macro
#define TEST_LOG(...) __android_log_print(ANDROID_LOG_INFO, "XoronTest", __VA_ARGS__)

// Test suite for Android
void run_android_tests() {
    TEST_LOG("Starting Android Integration Tests");
    
    TestSuite suite("Android Integration");
    Timer timer;
    
    // Initialize Xoron
    if (xoron_init() != XORON_OK) {
        TEST_LOG("Failed to initialize Xoron: %s", xoron_last_error());
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
        bool hash_ok = (hash[0] != 0);  // Basic check
        suite.recordResult("SHA256", hash_ok, "", timer.elapsed_ms());
        
        // Test 6: Base64 Encode
        timer.reset();
        char* b64 = xoron_base64_encode("test", 4);
        bool b64_ok = (b64 != nullptr && strcmp(b64, "dGVzdA==") == 0);
        if (b64) xoron_free(b64);
        suite.recordResult("Base64 Encode", b64_ok, "", timer.elapsed_ms());
        
        // Test 7: File Operations
        timer.reset();
        const char* test_file = "/data/data/com.xoron.test/files/test.txt";
        FILE* fp = fopen(test_file, "w");
        bool file_write = (fp != nullptr);
        if (fp) {
            fprintf(fp, "test data");
            fclose(fp);
        }
        
        if (file_write) {
            fp = fopen(test_file, "r");
            if (fp) {
                char buffer[100];
                file_write = (fgets(buffer, 100, fp) != nullptr);
                fclose(fp);
            }
            remove(test_file);
        }
        suite.recordResult("File Operations", file_write, "", timer.elapsed_ms());
        
        // Test 8: Haptic Feedback
        timer.reset();
        xoron_android_haptic_feedback(0);  // Light
        xoron_android_haptic_feedback(1);  // Medium
        suite.recordResult("Haptic Feedback", true, "", timer.elapsed_ms());
        
        // Test 9: Android Logging
        timer.reset();
        xoron_android_console_print("Test message", 0);
        suite.recordResult("Android Logging", true, "", timer.elapsed_ms());
        
        // Test 10: Lua Script with Android APIs
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
        result = xoron_dostring(vm, lua_script, "android_test");
        bool lua_android_ok = (result == XORON_OK);
        suite.recordResult("Lua + Android APIs", lua_android_ok, 
                          xoron_last_error(), timer.elapsed_ms());
        
        // Cleanup VM
        xoron_vm_free(vm);
    }
    
    // Shutdown
    xoron_shutdown();
    
    // Print summary
    suite.printSummary();
    
    // Log to Android console
    TEST_LOG("Test Summary: %d passed, %d failed", 
             suite.getResults().size(), 
             suite.getResults().size());
}

// JNI entry point
extern "C" JNIEXPORT void JNICALL
Java_com_xoron_test_TestRunner_runTests(JNIEnv* env, jobject thiz) {
    run_android_tests();
}
```

### XoronTestRunner.java

```java
package com.xoron.test;

import android.app.Activity;
import android.os.Bundle;
import android.widget.TextView;
import android.widget.ScrollView;
import android.util.Log;

public class TestRunner extends Activity {
    private static final String TAG = "XoronTestRunner";
    
    // Load native library
    static {
        System.loadLibrary("xoron_test_android");
    }
    
    // Native method
    private native void runTests();
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        // Create UI
        ScrollView scrollView = new ScrollView(this);
        TextView textView = new TextView(this);
        textView.setText("Xoron Test Runner\nRunning tests...\n");
        textView.setPadding(20, 20, 20, 20);
        scrollView.addView(textView);
        setContentView(scrollView);
        
        // Run tests in background thread
        new Thread(() -> {
            try {
                // Redirect output to UI
                System.setOut(new java.io.PrintStream(new java.io.OutputStream() {
                    @Override
                    public void write(int b) {
                        runOnUiThread(() -> {
                            textView.append(String.valueOf((char)b));
                        });
                    }
                }));
                
                // Run native tests
                runTests();
                
                runOnUiThread(() -> {
                    textView.append("\n\nTests completed!");
                });
                
            } catch (Exception e) {
                Log.e(TAG, "Test error", e);
                runOnUiThread(() -> {
                    textView.append("\n\nError: " + e.getMessage());
                });
            }
        }).start();
    }
}
```

## Building and Running

### Step 1: Set Up Environment

```bash
# Set environment variables
export ANDROID_NDK_HOME=/path/to/android-ndk-r26
export ANDROID_HOME=/path/to/android-sdk
export PATH=$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin:$PATH

# Verify
echo $ANDROID_NDK_HOME
echo $ANDROID_HOME
```

### Step 2: Build Xoron Library

```bash
cd /workspace/project/src
mkdir -p build-android
cd build-android

cmake .. \
  -DXORON_ANDROID_BUILD=ON \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-29 \
  -DXORON_OPENSSL_ROOT=/path/to/openssl-android

cmake --build . --config Release
```

### Step 3: Build Test App

```bash
cd /workspace/project/src/tests/android
mkdir build
cd build

# Configure
cmake .. \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-29 \
  -DXORON_LIBRARY=/workspace/project/src/build-android/libxoron.so

# Build
cmake --build . --config Release

# Create APK
cd ..
$ANDROID_HOME/build-tools/33.0.0/aapt package -f -M AndroidManifest.xml \
  -I $ANDROID_HOME/platforms/android-33/android.jar \
  -F build/XoronTestRunner.apk build

# Sign APK (for release)
jarsigner -verbose -sigalg SHA256withRSA -digestalg SHA-256 \
  -keystore my-release-key.keystore build/XoronTestRunner.apk alias_name
```

### Step 4: Deploy and Run

```bash
# Connect device or start emulator
adb devices

# Install APK
adb install -r build/XoronTestRunner.apk

# Run tests
adb shell am start -n com.xoron.test/.TestRunner

# Monitor logs
adb logcat -s XoronTest:V XoronTestRunner:V

# Pull results
adb pull /data/data/com.xoron.test/files/test_results.txt
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
- File read/write
- Directory operations
- Path validation
- Permissions

### 5. Platform Integration
- Haptic feedback
- Android logging
- System properties
- UI integration

### 6. Lua Integration
- Android APIs in Lua
- Mixed C++/Lua tests
- Performance benchmarks

## Performance Benchmarks

### Target Performance (ARM64)

| Test | Target | Typical |
|------|--------|---------|
| VM Creation | < 5ms | 2-3ms |
| HTTP GET | < 100ms | 50-80ms |
| SHA256 (1KB) | < 1ms | 0.1ms |
| File Write (1KB) | < 5ms | 1-2ms |
| Haptic Feedback | < 10ms | 5ms |
| Lua Execution | < 10ms | 3-5ms |

### Benchmark Results

```bash
# Run benchmarks
adb shell am start -n com.xoron.test/.TestRunner --es benchmark true

# View results
adb logcat -s XoronTest:V | grep "Benchmark"
```

## Debugging

### Logcat Filtering

```bash
# Show only test logs
adb logcat -s XoronTest:V XoronTestRunner:V

# Save to file
adb logcat -s XoronTest:V > test_results.log

# Clear logs before test
adb logcat -c
```

### Common Issues

**1. Library Not Found**
```
java.lang.UnsatisfiedLinkError: dlopen failed: library "libxoron.so" not found
```
**Solution**: Ensure libxoron.so is in APK's lib/arm64-v8a/ directory

**2. Permission Denied**
```
E/XoronTest: Failed to write file: Permission denied
```
**Solution**: Check AndroidManifest.xml permissions

**3. Network Not Available**
```
E/XoronTest: HTTP request failed: Connection refused
```
**Solution**: Check device internet connection, add network security config

**4. OpenSSL Not Available**
```
E/XoronTest: HTTPS not supported (OpenSSL not available)
```
**Solution**: Build OpenSSL for Android and link properly

### Debug Build

```bash
# Build with debug symbols
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .

# Run with gdb
adb shell run-as com.xoron.test /data/data/com.xoron.test/lib/libxoron_test_android
```

## Test Results

### Output Format

```
[XoronTest] Starting Android Integration Tests
[XoronTest] [PASS] VM Creation (2.34 ms)
[XoronTest] [PASS] Basic Lua (0.12 ms)
[XoronTest] [PASS] HTTP GET (67.45 ms)
[XoronTest] [PASS] HTTP POST (71.23 ms)
[XoronTest] [PASS] SHA256 (0.08 ms)
[XoronTest] [PASS] Base64 Encode (0.05 ms)
[XoronTest] [PASS] File Operations (1.56 ms)
[XoronTest] [PASS] Haptic Feedback (5.21 ms)
[XoronTest] [PASS] Android Logging (0.03 ms)
[XoronTest] [PASS] Lua + Android APIs (68.12 ms)
[XoronTest] ========================================
[XoronTest] Test Suite: Android Integration
[XoronTest] Passed: 10, Failed: 0, Total: 10
[XoronTest] ========================================
```

### Success Criteria
- All tests pass
- No crashes or ANRs
- Memory usage < 100MB
- Execution time within targets
- No permission errors

## Continuous Integration

### GitHub Actions

```yaml
name: Android Tests

on: [push, pull_request]

jobs:
  test-android:
    runs-on: ubuntu-latest
    
    steps:
      - uses: actions/checkout@v4
      
      - name: Setup Android NDK
        uses: android-actions/setup-android@v3
        with:
          packages: 'ndk;25.2.9519653'
      
      - name: Setup OpenSSL
        run: |
          # Download pre-built OpenSSL for Android
          wget https://github.com/leenjewel/openssl-for-android/releases/download/1.1.1w/openssl-1.1.1w-android.tar.gz
          tar xzf openssl-1.1.1w-android.tar.gz
          mv openssl-1.1.1w openssl-android
      
      - name: Build Xoron
        run: |
          cd src
          mkdir build-android && cd build-android
          cmake .. \
            -DXORON_ANDROID_BUILD=ON \
            -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
            -DANDROID_ABI=arm64-v8a \
            -DANDROID_PLATFORM=android-29 \
            -DXORON_OPENSSL_ROOT=$GITHUB_WORKSPACE/openssl-android
          cmake --build .
      
      - name: Build Tests
        run: |
          cd src/tests/android
          mkdir build && cd build
          cmake .. \
            -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
            -DANDROID_ABI=arm64-v8a \
            -DANDROID_PLATFORM=android-29
          cmake --build .
      
      - name: Run Tests
        uses: reactivecircus/android-emulator-runner@v2
        with:
          api-level: 29
          arch: arm64-v8a
          script: |
            cd src/tests/android/build
            adb install XoronTestRunner.apk
            adb shell am start -n com.xoron.test/.TestRunner
            sleep 10
            adb logcat -d | grep XoronTest
```

## Platform-Specific Features

### Haptic Feedback

```cpp
// Light feedback
xoron_android_haptic_feedback(0);

// Medium feedback
xoron_android_haptic_feedback(1);

// Heavy feedback
xoron_android_haptic_feedback(2);
```

### Android Logging

```cpp
// Log to Android logcat
xoron_android_console_print("Test message", 0);  // Info
xoron_android_console_print("Warning", 1);       // Warning
xoron_android_console_print("Error", 2);         // Error
```

### System Properties

```cpp
// Get system property
char value[PROP_VALUE_MAX];
__system_property_get("ro.build.version.sdk", value);
```

### JNI Integration

```cpp
// Get JNIEnv for current thread
JNIEnv* env = get_jni_env();

// Call Java method
jclass cls = env->FindClass("com/xoron/test/TestRunner");
jmethodID method = env->GetMethodID(cls, "onTestComplete", "(Z)V");
env->CallVoidMethod(java_object, method, success);
```

## Memory Management

### Leak Detection

```cpp
// Enable leak detection
#ifdef DEBUG
#include <malloc.h>
#include <unistd.h>

void check_memory() {
    struct mallinfo info = mallinfo();
    TEST_LOG("Memory used: %d bytes", info.uordblks);
}
#endif
```

### Memory Limits

```cpp
// Set memory limit for VM
lua_gc(vm->L, LUA_GCSETALLOCF, 0, 50 * 1024 * 1024);  // 50MB
```

## Security Considerations

### Network Security

```xml
<!-- network_security_config.xml -->
<network-security-config>
    <domain-config cleartextTrafficPermitted="false">
        <domain includeSubdomains="true">httpbin.org</domain>
    </domain-config>
</network-security-config>
```

### Permission Handling

```java
// Runtime permission check (Android 6.0+)
if (ContextCompat.checkSelfPermission(this, Manifest.permission.VIBRATE)
    != PackageManager.PERMISSION_GRANTED) {
    ActivityCompat.requestPermissions(this,
        new String[]{Manifest.permission.VIBRATE}, 0);
}
```

## Summary

The Android integration tests provide comprehensive coverage of:
- Core engine functionality
- Network operations
- Cryptographic functions
- File system operations
- Platform-specific features
- Performance benchmarks

These tests ensure Xoron works correctly on Android devices running API 29+.

---

**Related:**
- [iOS Tests](../ios/README.md)
- [Test Utilities](../common/README.md)
- [Main Test Suite](../README.md)
