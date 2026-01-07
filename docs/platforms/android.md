# Android Integration Guide

## Overview

This guide covers complete integration of Xoron into Android applications (Android 10+ / API 29+).

## Prerequisites

### Software Requirements
- **Android Studio**: 2022.3.1+ (Giraffe or later)
- **Android NDK**: r26.1.10909125+
- **Android SDK**: API 29+ (Android 10)
- **CMake**: 3.16+
- **Java JDK**: 11+
- **OpenSSL**: Built for Android (arm64-v8a, armeabi-v7a, x86, x86_64)

### Hardware Requirements
- **RAM**: 8GB minimum, 16GB recommended
- **Storage**: 10GB free space
- **Device**: Android 10+ device or emulator for testing

## Building Xoron for Android

### Step 1: Prepare Environment

```bash
# Set environment variables
export ANDROID_NDK_HOME=/path/to/android-ndk-r26
export ANDROID_HOME=/path/to/android-sdk
export PATH=$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin:$PATH

# Verify
echo $ANDROID_NDK_HOME
echo $ANDROID_HOME
```

### Step 2: Prepare OpenSSL

```bash
# Option 1: Use pre-built OpenSSL for Android
wget https://github.com/leenjewel/openssl-for-android/releases/download/1.1.1w/openssl-1.1.1w-android.tar.gz
tar xzf openssl-1.1.1w-android.tar.gz
mv openssl-1.1.1w openssl-android

# Option 2: Build from source
git clone https://github.com/openssl/openssl.git
cd openssl

# Set up NDK toolchain
export ANDROID_NDK_ROOT=$ANDROID_NDK_HOME
export PATH=$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/bin:$PATH

# Configure for arm64-v8a
./Configure android-arm64 \
  -D__ANDROID_API__=29 \
  --prefix=$(pwd)/../openssl-android-arm64 \
  no-shared \
  no-tests \
  no-ui-console \
  no-engine

make -j$(nproc)
make install_sw
```

### Step 3: Build Xoron

```bash
# Clone repository
git clone https://github.com/yourusername/xoron.git
cd xoron/src

# Create build directory
mkdir build-android && cd build-android

# Configure with CMake
cmake .. \
  -DXORON_ANDROID_BUILD=ON \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-29 \
  -DANDROID_STL=c++_shared \
  -DXORON_OPENSSL_ROOT=/path/to/openssl-android-arm64

# Build
cmake --build . --config Release

# Verify output
ls -la libxoron.so
file libxoron.so
```

**Expected Output**:
```
libxoron.so: ELF 64-bit LSB shared object, ARM aarch64
```

### Step 4: Verify Architecture

```bash
# Check supported architectures
readelf -h libxoron.so | grep Machine
# Should show: ARM aarch64

# Check dependencies
readelf -d libxoron.so | grep NEEDED
# Should show: libc.so, libm.so, libdl.so, etc.
```

## Integration Methods

### Method 1: CMake Integration (Recommended)

#### 1. Add Xoron to Your Project

**Project Structure**:
```
app/
├── src/
│   └── main/
│       ├── cpp/
│       │   ├── CMakeLists.txt
│       │   └── xoron_bridge.cpp
│       ├── java/
│       │   └── com/yourapp/xoron/
│       │       ├── XoronManager.java
│       │       └── XoronBridge.java
│       └── jniLibs/
│           └── arm64-v8a/
│               └── libxoron.so
├── build.gradle
└── CMakeLists.txt
```

#### 2. Configure CMakeLists.txt

**app/build.gradle**:
```gradle
android {
    compileSdk 33
    
    defaultConfig {
        minSdk 29
        targetSdk 33
        
        externalNativeBuild {
            cmake {
                arguments "-DANDROID_STL=c++_shared"
            }
        }
        
        ndk {
            abiFilters 'arm64-v8a', 'armeabi-v7a', 'x86_64', 'x86'
        }
    }
    
    externalNativeBuild {
        cmake {
            path "src/main/cpp/CMakeLists.txt"
            version "3.22.1"
        }
    }
    
    sourceSets {
        main {
            jniLibs.srcDirs = ['src/main/jniLibs']
        }
    }
}

dependencies {
    implementation fileTree(dir: 'libs', include: ['*.jar'])
}
```

**app/src/main/cpp/CMakeLists.txt**:
```cmake
cmake_minimum_required(VERSION 3.18)

project(xoron_bridge)

# Add Xoron library
add_library(xoron SHARED IMPORTED)
set_target_properties(xoron PROPERTIES
    IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/../jniLibs/${ANDROID_ABI}/libxoron.so
)

# Add bridge library
add_library(xoron_bridge SHARED
    xoron_bridge.cpp
)

# Link libraries
target_link_libraries(xoron_bridge
    xoron
    log
    android
)

# Include directories
target_include_directories(xoron_bridge PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
```

#### 3. Create JNI Bridge

**app/src/main/cpp/xoron_bridge.cpp**:
```cpp
#include <jni.h>
#include <string>
#include "xoron.h"

extern "C" {

// Initialize Xoron
JNIEXPORT jboolean JNICALL
Java_com_yourapp_xoron_XoronBridge_initXoron(JNIEnv* env, jobject thiz) {
    return xoron_init() == XORON_OK;
}

// Shutdown Xoron
JNIEXPORT void JNICALL
Java_com_yourapp_xoron_XoronBridge_shutdownXoron(JNIEnv* env, jobject thiz) {
    xoron_shutdown();
}

// Execute Lua script
JNIEXPORT jstring JNICALL
Java_com_yourapp_xoron_XoronBridge_executeScript(
    JNIEnv* env, jobject thiz, jstring script, jstring name) {
    
    const char* script_str = env->GetStringUTFChars(script, nullptr);
    const char* name_str = env->GetStringUTFChars(name, nullptr);
    
    xoron_vm_t* vm = xoron_vm_new();
    if (!vm) {
        env->ReleaseStringUTFChars(script, script_str);
        env->ReleaseStringUTFChars(name, name_str);
        return env->NewStringUTF("Failed to create VM");
    }
    
    int result = xoron_dostring(vm, script_str, name_str);
    
    const char* error = xoron_last_error();
    jstring result_str = env->NewStringUTF(error ? error : "Success");
    
    xoron_vm_free(vm);
    
    env->ReleaseStringUTFChars(script, script_str);
    env->ReleaseStringUTFChars(name, name_str);
    
    return result_str;
}

// Haptic feedback
JNIEXPORT void JNICALL
Java_com_yourapp_xoron_XoronBridge_hapticFeedback(JNIEnv* env, jobject thiz, jint style) {
    xoron_android_haptic_feedback(style);
}

// Console print
JNIEXPORT void JNICALL
Java_com_yourapp_xoron_XoronBridge_consolePrint(
    JNIEnv* env, jobject thiz, jstring message, jint type) {
    
    const char* msg = env->GetStringUTFChars(message, nullptr);
    xoron_android_console_print(msg, type);
    env->ReleaseStringUTFChars(message, msg);
}

}
```

#### 4. Create Java Wrapper

**app/src/main/java/com/yourapp/xoron/XoronBridge.java**:
```java
package com.yourapp.xoron;

public class XoronBridge {
    static {
        System.loadLibrary("xoron_bridge");
    }
    
    // Native methods
    public static native boolean initXoron();
    public static native void shutdownXoron();
    public static native String executeScript(String script, String name);
    public static native void hapticFeedback(int style);
    public static native void consolePrint(String message, int type);
    
    // Helper methods
    public static boolean initialize() {
        return initXoron();
    }
    
    public static String run(String script) {
        return executeScript(script, "android_script");
    }
    
    public static void vibrateLight() {
        hapticFeedback(0);
    }
    
    public static void vibrateMedium() {
        hapticFeedback(1);
    }
    
    public static void vibrateHeavy() {
        hapticFeedback(2);
    }
}
```

**app/src/main/java/com/yourapp/xoron/XoronManager.java**:
```java
package com.yourapp.xoron;

import android.content.Context;
import android.util.Log;

public class XoronManager {
    private static final String TAG = "XoronManager";
    private static XoronManager instance;
    private Context context;
    
    private XoronManager(Context context) {
        this.context = context.getApplicationContext();
    }
    
    public static synchronized XoronManager getInstance(Context context) {
        if (instance == null) {
            instance = new XoronManager(context);
        }
        return instance;
    }
    
    public boolean initialize() {
        boolean success = XoronBridge.initialize();
        if (success) {
            Log.i(TAG, "Xoron initialized successfully");
        } else {
            Log.e(TAG, "Failed to initialize Xoron");
        }
        return success;
    }
    
    public String executeScript(String script) {
        try {
            String result = XoronBridge.run(script);
            Log.d(TAG, "Script execution result: " + result);
            return result;
        } catch (Exception e) {
            Log.e(TAG, "Script execution error", e);
            return "Error: " + e.getMessage();
        }
    }
    
    public void cleanup() {
        XoronBridge.shutdownXoron();
        Log.i(TAG, "Xoron shutdown");
    }
}
```

### Method 2: Prebuilt Library (Alternative)

#### 1. Add Library to Project

**app/build.gradle**:
```gradle
android {
    sourceSets {
        main {
            jniLibs.srcDirs = ['src/main/jniLibs']
        }
    }
}

dependencies {
    // No special dependencies needed
}
```

#### 2. Copy Libraries

```
app/src/main/jniLibs/
├── arm64-v8a/
│   └── libxoron.so
├── armeabi-v7a/
│   └── libxoron.so
├── x86_64/
│   └── libxoron.so
└── x86/
    └── libxoron.so
```

#### 3. Load in Java

```java
public class XoronLoader {
    static {
        System.loadLibrary("xoron");
    }
    
    // Native methods from xoron.h
    public static native int xoron_init();
    public static native void xoron_shutdown();
    // ... other methods
}
```

### Method 3: Gradle CMake Integration

**settings.gradle**:
```gradle
pluginManagement {
    repositories {
        google()
        mavenCentral()
        gradlePluginPortal()
    }
}

dependencyResolutionManagement {
    repositoriesMode.set(RepositoriesMode.FAIL_ON_PROJECT_REPOS)
    repositories {
        google()
        mavenCentral()
    }
}
```

**app/build.gradle.kts**:
```kotlin
plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}

android {
    namespace = "com.yourapp.xoron"
    compileSdk = 33

    defaultConfig {
        applicationId = "com.yourapp.xoron"
        minSdk = 29
        targetSdk = 33
        versionCode = 1
        versionName = "1.0"

        externalNativeBuild {
            cmake {
                arguments "-DANDROID_STL=c++_shared"
            }
        }

        ndk {
            abiFilters.addAll(listOf("arm64-v8a", "armeabi-v7a", "x86_64", "x86"))
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = true
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
    }

    kotlinOptions {
        jvmTarget = "11"
    }

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }

    sourceSets {
        getByName("main") {
            jniLibs.srcDirs("src/main/jniLibs")
        }
    }
}

dependencies {
    implementation("androidx.core:core-ktx:1.12.0")
    implementation("androidx.appcompat:appcompat:1.6.1")
    implementation("com.google.android.material:material:1.11.0")
    implementation("androidx.constraintlayout:constraintlayout:2.1.4")
}
```

## Android-Specific Features

### Haptic Feedback

```java
// In Java
public class HapticUtils {
    public static void vibrate(Context context, int style) {
        Vibrator vibrator = (Vibrator) context.getSystemService(Context.VIBRATOR_SERVICE);
        
        if (vibrator == null || !vibrator.hasVibrator()) {
            return;
        }
        
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.O) {
            VibrationEffect effect;
            switch (style) {
                case 0: // Light
                    effect = VibrationEffect.createOneShot(50, VibrationEffect.DEFAULT_AMPLITUDE);
                    break;
                case 1: // Medium
                    effect = VibrationEffect.createOneShot(100, VibrationEffect.DEFAULT_AMPLITUDE);
                    break;
                case 2: // Heavy
                    effect = VibrationEffect.createOneShot(200, VibrationEffect.DEFAULT_AMPLITUDE);
                    break;
                default:
                    effect = VibrationEffect.createOneShot(50, VibrationEffect.DEFAULT_AMPLITUDE);
            }
            vibrator.vibrate(effect);
        } else {
            // Deprecated in API 26
            vibrator.vibrate(50);
        }
    }
}
```

**C++ Implementation**:
```cpp
void xoron_android_haptic_feedback(int style) {
    JNIEnv* env = get_jni_env();
    if (!env) return;
    
    // Get Vibrator service
    jclass context_class = env->FindClass("android/content/Context");
    jmethodID get_system_service = env->GetMethodID(
        context_class, "getSystemService", 
        "(Ljava/lang/String;)Ljava/lang/Object;");
    
    jfieldID vibrator_service_field = env->GetStaticFieldID(
        context_class, "VIBRATOR_SERVICE", "Ljava/lang/String;");
    jstring vibrator_service = (jstring)env->GetStaticObjectField(
        context_class, vibrator_service_field);
    
    jobject vibrator = env->CallObjectMethod(
        g_context, get_system_service, vibrator_service);
    
    if (!vibrator) return;
    
    // Check if has vibrator
    jclass vibrator_class = env->FindClass("android/os/Vibrator");
    jmethodID has_vibrator = env->GetMethodID(vibrator_class, "hasVibrator", "()Z");
    jboolean has = env->CallBooleanMethod(vibrator, has_vibrator);
    
    if (!has) return;
    
    // Create vibration effect
    if (android_get_device_api_level() >= 26) {
        jclass vibration_effect_class = env->FindClass("android/os/VibrationEffect");
        jmethodID create_one_shot = env->GetStaticMethodID(
            vibration_effect_class, "createOneShot", "(JI)Landroid/os/VibrationEffect;");
        
        jlong duration;
        jint amplitude;
        switch (style) {
            case 0: duration = 50; amplitude = 1; break;
            case 1: duration = 100; amplitude = 100; break;
            case 2: duration = 200; amplitude = 255; break;
            default: duration = 50; amplitude = 1;
        }
        
        jobject effect = env->CallStaticObjectMethod(
            vibration_effect_class, create_one_shot, duration, amplitude);
        
        jmethodID vibrate_effect = env->GetMethodID(
            vibrator_class, "vibrate", "(Landroid/os/VibrationEffect;)V");
        env->CallVoidMethod(vibrator, vibrate_effect, effect);
    } else {
        jmethodID vibrate_long = env->GetMethodID(
            vibrator_class, "vibrate", "(J)V");
        env->CallLongMethod(vibrator, vibrate_long, 50L);
    }
}
```

### Android Logging

```java
// Java wrapper
public class LogUtils {
    public static void print(String message) {
        Log.i("Xoron", message);
    }
    
    public static void warn(String message) {
        Log.w("Xoron", message);
    }
    
    public static void error(String message) {
        Log.e("Xoron", message);
    }
}
```

**C++ Implementation**:
```cpp
void xoron_android_console_print(const char* message, int type) {
    switch (type) {
        case 0:  // Info
            __android_log_print(ANDROID_LOG_INFO, "Xoron", "%s", message);
            break;
        case 1:  // Warning
            __android_log_print(ANDROID_LOG_WARN, "Xoron", "%s", message);
            break;
        case 2:  // Error
            __android_log_print(ANDROID_LOG_ERROR, "Xoron", "%s", message);
            break;
    }
}
```

### JNI Context Management

```cpp
// Store JVM and Context
static JavaVM* g_jvm = nullptr;
static jobject g_context = nullptr;

// Initialize in JNI_OnLoad
JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    g_jvm = vm;
    
    JNIEnv* env;
    if (vm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }
    
    // Store context (passed from Java)
    return JNI_VERSION_1_6;
}

// Get JNIEnv for current thread
JNIEnv* get_jni_env() {
    if (!g_jvm) return nullptr;
    
    JNIEnv* env;
    int status = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    
    if (status == JNI_EDETACHED) {
        if (g_jvm->AttachCurrentThread(&env, nullptr) != JNI_OK) {
            return nullptr;
        }
    }
    
    return env;
}

// Set context from Java
extern "C" JNIEXPORT void JNICALL
Java_com_yourapp_xoron_XoronBridge_setContext(JNIEnv* env, jobject thiz, jobject context) {
    if (g_context) {
        env->DeleteGlobalRef(g_context);
    }
    g_context = env->NewGlobalRef(context);
}
```

### File System Integration

```java
// Get internal storage path
public class FilePaths {
    public static String getInternalPath(Context context) {
        return context.getFilesDir().getAbsolutePath() + "/Xoron";
    }
    
    public static String getExternalPath(Context context) {
        File[] external = context.getExternalFilesDirs(null);
        if (external.length > 1 && external[1] != null) {
            return external[1].getAbsolutePath() + "/Xoron";
        }
        return null;
    }
    
    public static void ensureDirectory(String path) {
        File dir = new File(path);
        if (!dir.exists()) {
            dir.mkdirs();
        }
    }
}
```

**C++ Implementation**:
```cpp
const char* xoron_get_workspace() {
    // Return internal storage path
    return "/data/data/com.yourapp/files/Xoron/workspace";
}

const char* xoron_get_external_workspace() {
    // Return external storage path (if available)
    return "/storage/emulated/0/Xoron/workspace";
}
```

### System Properties

```cpp
// Get Android version
int get_android_api_level() {
    char value[PROP_VALUE_MAX];
    __system_property_get("ro.build.version.sdk", value);
    return atoi(value);
}

// Get device model
std::string get_device_model() {
    char model[PROP_VALUE_MAX];
    __system_property_get("ro.product.model", model);
    return std::string(model);
}
```

## Activity Integration

### Main Activity Setup

```java
package com.yourapp.xoron;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.widget.Toast;

public class MainActivity extends Activity {
    private static final String TAG = "MainActivity";
    private XoronManager xoronManager;
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        
        // Initialize Xoron
        xoronManager = XoronManager.getInstance(this);
        
        if (xoronManager.initialize()) {
            Toast.makeText(this, "Xoron initialized", Toast.LENGTH_SHORT).show();
            runTestScript();
        } else {
            Toast.makeText(this, "Failed to initialize Xoron", Toast.LENGTH_SHORT).show();
        }
    }
    
    private void runTestScript() {
        String script = """
            local response = http.get("https://api.github.com")
            if response then
                print("Status: " .. response.status)
                return "Success"
            end
            return "Failed"
            """;
        
        String result = xoronManager.executeScript(script);
        Log.i(TAG, "Script result: " + result);
    }
    
    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (xoronManager != null) {
            xoronManager.cleanup();
        }
    }
}
```

### Fragment Integration

```java
public class XoronFragment extends Fragment {
    private XoronManager xoronManager;
    
    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        xoronManager = XoronManager.getInstance(requireContext());
        xoronManager.initialize();
    }
    
    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        
        // Setup UI
        view.findViewById(R.id.run_script).setOnClickListener(v -> {
            runScript();
        });
    }
    
    private void runScript() {
        String script = """
            -- Drawing example
            local draw = Drawing.new()
            draw:Circle(100, 100, 50, Color3.new(1, 0, 0), true)
            return "Drawn"
            """;
        
        String result = xoronManager.executeScript(script);
        Toast.makeText(requireContext(), result, Toast.LENGTH_SHORT).show();
    }
    
    @Override
    public void onDestroy() {
        super.onDestroy();
        xoronManager.cleanup();
    }
}
```

## Kotlin Integration

### Kotlin Wrapper

```kotlin
package com.yourapp.xoron

import android.content.Context

class XoronKotlin(private val context: Context) {
    
    companion object {
        init {
            System.loadLibrary("xoron_bridge")
        }
    }
    
    // Native methods
    private external fun initXoron(): Boolean
    private external fun shutdownXoron()
    private external fun executeScript(script: String, name: String): String
    private external fun hapticFeedback(style: Int)
    private external fun consolePrint(message: String, type: Int)
    
    fun initialize(): Boolean {
        return initXoron()
    }
    
    fun execute(script: String): String {
        return executeScript(script, "kotlin_script")
    }
    
    fun vibrate(style: Int) {
        hapticFeedback(style)
    }
    
    fun log(message: String, type: Int = 0) {
        consolePrint(message, type)
    }
    
    fun cleanup() {
        shutdownXoron()
    }
}

// Usage in Activity
class MainActivity : AppCompatActivity() {
    private lateinit var xoron: XoronKotlin
    
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        xoron = XoronKotlin(this)
        if (xoron.initialize()) {
            // Run script
            val result = xoron.execute("""
                return "Hello from Kotlin!"
            """)
            Toast.makeText(this, result, Toast.LENGTH_SHORT).show()
        }
    }
    
    override fun onDestroy() {
        super.onDestroy()
        xoron.cleanup()
    }
}
```

### Coroutines Integration

```kotlin
class XoronViewModel : ViewModel() {
    private val xoron = XoronKotlin(getApplication())
    
    init {
        xoron.initialize()
    }
    
    fun runScriptAsync(script: String): LiveData<String> {
        val result = MutableLiveData<String>()
        
        viewModelScope.launch(Dispatchers.IO) {
            val scriptResult = xoron.execute(script)
            result.postValue(scriptResult)
        }
        
        return result
    }
    
    override fun onCleared() {
        super.onCleared()
        xoron.cleanup()
    }
}
```

## Advanced Features

### WebSocket Integration

```kotlin
class WebSocketManager {
    fun connect(url: String) {
        val script = """
            local ws = WebSocket.connect("$url")
            ws:on_message(function(msg)
                print("Received: " .. msg)
            end)
            ws:on_close(function()
                print("Disconnected")
            end)
            ws:send("Hello from Android!")
        """
        
        xoron.execute(script)
    }
}
```

### Background Service

```java
public class XoronService extends Service {
    private XoronManager xoronManager;
    
    @Override
    public void onCreate() {
        super.onCreate();
        xoronManager = XoronManager.getInstance(this);
        xoronManager.initialize();
    }
    
    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        // Run background script
        new Thread(() -> {
            String script = """
                -- Long-running task
                for i = 1, 1000000 do
                    local x = math.sqrt(i)
                end
                return "Background task complete"
                """;
            
            String result = xoronManager.executeScript(script);
            Log.i("XoronService", result);
            
            // Send result via broadcast
            Intent resultIntent = new Intent("XORON_RESULT");
            resultIntent.putExtra("result", result);
            sendBroadcast(resultIntent);
        }).start();
        
        return START_STICKY;
    }
    
    @Override
    public void onDestroy() {
        super.onDestroy();
        xoronManager.cleanup();
    }
    
    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }
}
```

### Content Provider Integration

```java
public class XoronProvider extends ContentProvider {
    @Override
    public boolean onCreate() {
        XoronManager.getInstance(getContext()).initialize();
        return true;
    }
    
    @Nullable
    @Override
    public Cursor query(@NonNull Uri uri, @Nullable String[] projection, 
                       @Nullable String selection, @Nullable String[] selectionArgs, 
                       @Nullable String sortOrder) {
        // Execute script based on URI
        String script = "return 'query executed'";
        String result = XoronManager.getInstance(getContext()).executeScript(script);
        
        MatrixCursor cursor = new MatrixCursor(new String[]{"result"});
        cursor.addRow(new Object[]{result});
        return cursor;
    }
    
    // Other methods...
}
```

## Memory Management

### Memory Limits

```java
// Set memory limit in Java
public class MemoryManager {
    public static void setMemoryLimit(XoronManager manager, int mb) {
        String script = "collectgarbage('setpause', " + mb * 1024 * 1024 + ")";
        manager.executeScript(script);
    }
    
    public static long getMemoryUsage(XoronManager manager) {
        String result = manager.executeScript("return collectgarbage('count')");
        try {
            return Long.parseLong(result);
        } catch (NumberFormatException e) {
            return -1;
        }
    }
}
```

### Memory Monitoring

```kotlin
class MemoryMonitor(private val xoron: XoronKotlin) {
    fun startMonitoring() {
        val timer = Timer()
        timer.scheduleAtFixedRate(object : TimerTask() {
            override fun run() {
                val usage = xoron.execute("return collectgarbage('count')")
                Log.d("MemoryMonitor", "Lua memory: $usage KB")
            }
        }, 0, 5000) // Every 5 seconds
    }
}
```

## Security

### Permissions

**AndroidManifest.xml**:
```xml
<manifest xmlns:android="http://schemas.android.com/apk/res/android">
    
    <!-- Required for network access -->
    <uses-permission android:name="android.permission.INTERNET" />
    <uses-permission android:name="android.permission.ACCESS_NETWORK_STATE" />
    
    <!-- Optional for haptic feedback -->
    <uses-permission android:name="android.permission.VIBRATE" />
    
    <!-- Optional for external storage -->
    <uses-permission android:name="android.permission.WRITE_EXTERNAL_STORAGE"
                     android:maxSdkVersion="28" />
    <uses-permission android:name="android.permission.READ_EXTERNAL_STORAGE"
                     android:maxSdkVersion="28" />
    
    <!-- For Android 13+ -->
    <uses-permission android:name="android.permission.POST_NOTIFICATIONS" />
    
    <application
        android:allowBackup="true"
        android:icon="@mipmap/ic_launcher"
        android:label="@string/app_name"
        android:theme="@style/Theme.AppCompat.Light.DarkActionBar">
        
        <!-- Network security config -->
        <application
            android:networkSecurityConfig="@xml/network_security_config"
            ...>
        
        <activity android:name=".MainActivity"
            android:exported="true">
            <intent-filter>
                <action android:name="android.intent.action.MAIN" />
                <category android:name="android.intent.category.LAUNCHER" />
            </intent-filter>
        </activity>
        
        <service android:name=".XoronService"
            android:exported="false" />
    </application>
</manifest>
```

**res/xml/network_security_config.xml**:
```xml
<?xml version="1.0" encoding="utf-8"?>
<network-security-config>
    <domain-config cleartextTrafficPermitted="false">
        <domain includeSubdomains="true">api.example.com</domain>
    </domain-config>
    
    <!-- For debugging -->
    <base-config cleartextTrafficPermitted="false">
        <trust-anchors>
            <certificates src="system" />
            <certificates src="user" />
        </trust-anchors>
    </base-config>
</network-security-config>
```

### Runtime Permissions (Android 6.0+)

```kotlin
class PermissionManager(private val activity: Activity) {
    private val PERMISSIONS = arrayOf(
        Manifest.permission.VIBRATE,
        Manifest.permission.INTERNET
    )
    
    fun requestPermissions(callback: (Boolean) -> Unit) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            val notGranted = PERMISSIONS.filter {
                activity.checkSelfPermission(it) != PackageManager.PERMISSION_GRANTED
            }
            
            if (notGranted.isNotEmpty()) {
                activity.requestPermissions(notGranted.toTypedArray(), 100)
            } else {
                callback(true)
            }
        } else {
            callback(true)
        }
    }
    
    fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray,
        callback: (Boolean) -> Unit
    ) {
        if (requestCode == 100) {
            val allGranted = grantResults.all { it == PackageManager.PERMISSION_GRANTED }
            callback(allGranted)
        }
    }
}
```

### Secure Storage

```kotlin
class SecureStorage(private val context: Context) {
    private val masterKey = MasterKey.Builder(context)
        .setKeyScheme(MasterKey.KeyScheme.AES256_GCM)
        .build()
    
    private val sharedPreferences = EncryptedSharedPreferences.create(
        context,
        "xoron_prefs",
        masterKey,
        EncryptedSharedPreferences.PrefKeyEncryptionScheme.AES256_SIV,
        EncryptedSharedPreferences.PrefValueEncryptionScheme.AES256_GCM
    )
    
    fun saveToken(token: String) {
        sharedPreferences.edit().putString("api_token", token).apply()
    }
    
    fun getToken(): String? {
        return sharedPreferences.getString("api_token", null)
    }
}
```

## Debugging

### Logcat Filtering

```bash
# Filter Xoron logs
adb logcat -s Xoron:V XoronManager:V

# Save to file
adb logcat -s Xoron:V > xoron_logs.txt

# Clear logs before test
adb logcat -c
```

### Native Debugging

```bash
# Start app
adb shell am start -n com.yourapp/.MainActivity

# Get PID
PID=$(adb shell pidof com.yourapp)

# Attach gdb
adb shell run-as com.yourapp /system/bin/gdb --attach=$PID

# In gdb
(gdb) break xoron_bridge.cpp:50
(gdb) continue
```

### Memory Leak Detection

```bash
# Use Android Studio Profiler
# Or use LeakCanary in debug builds

dependencies {
    debugImplementation 'com.squareup.leakcanary:leakcanary-android:2.12'
}
```

### Crash Analysis

```bash
# Pull tombstone
adb shell ls -lt /data/tombstones/ | head -1
adb pull /data/tombstones/tombstone_00

# Analyze with ndk-stack
$ANDROID_NDK/ndk-stack -sym ./obj/local/arm64-v8a -dump tombstone_00
```

## Distribution

### AAB (Android App Bundle)

**Build**:
```bash
./gradlew bundleRelease
```

**Sign**:
```bash
jarsigner -verbose -sigalg SHA256withRSA -digestalg SHA-256 \
  -keystore my-release-key.keystore \
  app/build/outputs/bundle/release/app-release.aab \
  alias_name
```

**Upload**: Use Google Play Console

### APK (Direct Distribution)

**Build**:
```bash
./gradlew assembleRelease
```

**Sign**:
```bash
apksigner sign --ks my-release-key.keystore --ks-key-alias alias_name \
  app/build/outputs/apk/release/app-release.apk
```

**Verify**:
```bash
apksigner verify app/build/outputs/apk/release/app-release.apk
```

### Enterprise Distribution

Same as APK, but use enterprise certificate and distribute via:
- MDM (Mobile Device Management)
- Direct download from website
- Email attachment

## Performance Optimization

### Build Optimization

**app/build.gradle**:
```gradle
android {
    buildTypes {
        release {
            minifyEnabled true
            shrinkResources true
            proguardFiles getDefaultProguardFile('proguard-android-optimize.txt'),
                         'proguard-rules.pro'
            
            // For C++ optimization
            externalNativeBuild {
                cmake {
                    arguments "-DCMAKE_BUILD_TYPE=Release"
                }
            }
        }
    }
}
```

**proguard-rules.pro**:
```
# Keep native methods
-keepclasseswithmembernames class * {
    native <methods>;
}

# Keep Xoron classes
-keep class com.yourapp.xoron.** { *; }
```

### Runtime Optimization

```kotlin
// Use background thread for heavy operations
class OptimizedXoron(private val xoron: XoronKotlin) {
    
    fun heavyComputation(callback: (String) -> Unit) {
        CoroutineScope(Dispatchers.IO).launch {
            val script = """
                -- Batch operations
                local results = {}
                for i = 1, 10000 do
                    results[i] = math.sqrt(i)
                end
                return #results
                """
            
            val result = xoron.execute(script)
            withContext(Dispatchers.Main) {
                callback(result)
            }
        }
    }
    
    // Reuse VM instances
    private var sharedVM: XoronKotlin? = null
    
    fun getSharedVM(): XoronKotlin {
        if (sharedVM == null) {
            sharedVM = XoronKotlin(getApplication())
            sharedVM?.initialize()
        }
        return sharedVM!!
    }
}
```

### Profiling

```bash
# CPU Profiling
adb shell perfetto -o /data/misc/perfetto.trace -t 10s sched freq idle am wm gfx view

# Memory Profiling
adb shell dumpsys meminfo com.yourapp

# Battery Profiling
adb shell dumpsys batterystats --reset
adb shell dumpsys batterystats --enable full-wake-history
# ... run app ...
adb shell dumpsys batterystats > batterystats.txt
```

## Troubleshooting

### Common Issues

**1. UnsatisfiedLinkError**
```
java.lang.UnsatisfiedLinkError: dlopen failed: library "libxoron.so" not found
```
**Solution**:
- Check `jniLibs` directory structure
- Verify ABI matches device
- Check `System.loadLibrary()` call

**2. JNI Signature Mismatch**
```
No such method: com.yourapp.xoron.XoronBridge.initXoron()Z
```
**Solution**:
- Verify method signatures match exactly
- Use `javap -s` to check signatures
- Clean and rebuild

**3. Permission Denied**
```
E/Xoron: Failed to write file: Permission denied
```
**Solution**:
- Check manifest permissions
- Request runtime permissions (Android 6.0+)
- Use internal storage for app data

**4. SSL/TLS Errors**
```
javax.net.ssl.SSLHandshakeException
```
**Solution**:
- Check network security config
- Update OpenSSL
- Verify certificate chain

**5. Memory Issues**
```
Out of memory: Kill process
```
**Solution**:
- Set memory limits
- Use `collectgarbage()` in Lua
- Profile with Android Studio

### Verification Checklist

```bash
# 1. Check library
adb shell ls -la /data/app/com.yourapp/lib/arm64/
# Should show: libxoron.so

# 2. Check dependencies
adb shell readelf -d /data/app/com.yourapp/lib/arm64/libxoron.so
# Should show: libc.so, libm.so, etc.

# 3. Test on device
adb shell am start -n com.yourapp/.MainActivity
adb logcat -s Xoron:V

# 4. Check ABI
adb shell getprop ro.product.cpu.abi
# Should match your build
```

## Summary

This guide covered:
- Building Xoron for Android
- CMake integration
- JNI bridge creation
- Java/Kotlin wrappers
- Android-specific features
- Activity/Fragment integration
- Security and permissions
- Debugging techniques
- Distribution methods
- Performance optimization

**Next Steps**:
- Review [iOS Integration Guide](ios.md)
- Check [Cross-Platform Guide](cross_platform.md)
- See [Android Tests](../../src/tests/android/README.md) for examples
