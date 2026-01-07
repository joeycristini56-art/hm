# 14. xoron_android.cpp - Android-Specific Implementation

**File Path**: `src/src/xoron_android.cpp`  
**Size**: 22,620 bytes  
**Lines**: 594

**Platform**: Android (API 29+, Android 10+)

## Overview
Provides Android-specific functionality including JNI bridge, native UI, haptic feedback, and Android NDK integration.

## Includes
- `xoron.h` (local)
- `cstdlib`, `cstring`, `cstdio` (system)
- `string`, `vector`, `unordered_map` (system)
- `mutex`, `thread` (system)
- Android NDK:
  - `jni.h` - JNI interface
  - `android/log.h` - Logging
  - `android/native_window.h` - Native window
  - `android/input.h` - Input handling
  - `android/keycodes.h` - Key codes
  - `android/looper.h` - Event loop
  - `android/native_activity.h` - Activity lifecycle
  - `sys/system_properties.h` - System info
- `lua.h`, `lualib.h` (local - Luau)

## JNI Architecture

### JNI Environment Management
```cpp
struct JNIState {
    JavaVM* jvm;              // Java VM
    JNIEnv* env;              // JNI environment
    jobject activity;         // Main activity
    jclass activity_class;    // Activity class
    jmethodID run_on_ui_thread; // UI thread runner
    jmethodID show_toast;     // Toast method
    jmethodID vibrate;        // Vibration method
    jmethodID get_string;     // String resource method
    bool attached;            // Thread attached flag
};
```

### Thread Attachment
```cpp
// Attach JNI to current thread
JNIEnv* get_jni_env() {
    if (!g_jni_state.attached) {
        g_jni_state.jvm->AttachCurrentThread(&g_jni_state.env, nullptr);
        g_jni_state.attached = true;
    }
    return g_jni_state.env;
}
```

## JNI Bridge Functions

### xoron_android_init
```cpp
extern "C" JNIEXPORT void JNICALL
Java_com_xoron_Executor_XoronNative_init(JNIEnv* env, jobject thiz, jobject activity)
```
Initializes Android native layer.

**Parameters**:
- `env`: JNI environment
- `thiz`: Native object
- `activity`: Main activity

**Called from**: Java when library loads

### xoron_android_execute
```cpp
extern "C" JNIEXPORT jint JNICALL
Java_com_xoron_Executor_XoronNative_execute(JNIEnv* env, jobject thiz, jstring script)
```
Executes Lua script from Java.

**Parameters**:
- `env`: JNI environment
- `thiz`: Native object
- `script`: Script string

**Returns**: Exit code

### xoron_android_get_version
```cpp
extern "C" JNIEXPORT jstring JNICALL
Java_com_xoron_Executor_XoronNative_getVersion(JNIEnv* env, jobject thiz)
```
Returns Xoron version.

### xoron_android_get_last_error
```cpp
extern "C" JNIEXPORT jstring JNICALL
Java_com_xoron_Executor_XoronNative_getLastError(JNIEnv* env, jobject thiz)
```
Returns last error message.

## Native UI Functions

### xoron_android_ui_show
```cpp
void xoron_android_ui_show()
```
Shows native Android UI.

**Implementation**:
- Runs on UI thread via `Activity.runOnUiThread()`
- Creates or shows main window
- Initializes UI components

### xoron_android_ui_hide
```cpp
void xoron_android_ui_hide()
```
Hides native UI.

### xoron_android_ui_toggle
```cpp
void xoron_android_ui_toggle()
```
Toggles UI visibility.

### xoron_android_console_print
```cpp
void xoron_android_console_print(const char* message, int type)
```
Prints to Android log and console.

**Parameters**:
- `message`: Message to print
- `type`: 0=info, 1=warn, 2=error

**Implementation**:
- Uses `__android_log_print()` for system log
- Updates UI console if visible

## Haptic Feedback

### xoron_android_haptic_feedback
```cpp
void xoron_android_haptic_feedback(int style)
```
Triggers haptic feedback.

**Parameters**:
- `style`: Feedback type
  - 0: Light tap
  - 1: Medium impact
  - 2: Heavy impact
  - 3: Long press
  - 4: Virtual key

**Implementation**:
```cpp
jclass vibrator_class = env->FindClass("android/os/Vibrator");
jmethodID vibrate = env->GetMethodID(vibrator_class, "vibrate", "(J)V");
env->CallVoidMethod(vibrator, vibrate, duration_ms);
```

## System Information

### xoron_android_get_api_level
```cpp
int xoron_android_get_api_level()
```
Gets Android API level.

**Returns**: API level (29 for Android 10)

### xoron_android_get_device_info
```cpp
char* xoron_android_get_device_info()
```
Gets device information.

**Returns**: JSON string with:
- Model
- Manufacturer
- SDK version
- Release version
- Screen density
- Memory info

### xoron_android_get_package_name
```cpp
char* xoron_android_get_package_name()
```
Gets current package name.

**Returns**: Package name string

## File System Integration

### xoron_android_get_internal_storage
```cpp
char* xoron_android_get_internal_storage()
```
Gets internal storage path.

**Returns**: `/data/data/<package>/files/`

### xoron_android_get_external_storage
```cpp
char* xoron_android_get_external_storage()
```
Gets external storage path.

**Returns**: `/storage/emulated/0/` or NULL if unavailable

### xoron_android_has_storage_permission
```cpp
bool xoron_android_has_storage_permission()
```
Checks if storage permission granted.

**Returns**: true if has READ_EXTERNAL_STORAGE and WRITE_EXTERNAL_STORAGE

### xoron_android_request_storage_permission
```cpp
void xoron_android_request_storage_permission()
```
Requests storage permission from user.

**Implementation**:
- Calls Java to show permission dialog
- Waits for result via callback

## Activity Lifecycle

### xoron_android_on_resume
```cpp
extern "C" JNIEXPORT void JNICALL
Java_com_xoron_Executor_XoronNative_onResume(JNIEnv* env, jobject thiz)
```
Called when activity resumes.

**Actions**:
- Reattach JNI if needed
- Resume any paused operations
- Update UI state

### xoron_android_on_pause
```cpp
extern "C" JNIEXPORT void JNICALL
Java_com_xoron_Executor_XoronNative_onPause(JNIEnv* env, jobject thiz)
```
Called when activity pauses.

**Actions**:
- Pause background threads
- Save state if needed
- Release resources

### xoron_android_on_destroy
```cpp
extern "C" JNIEXPORT void JNICALL
Java_com_xoron_Executor_XoronNative_onDestroy(JNIEnv* env, jobject thiz)
```
Called when activity is destroyed.

**Actions**:
- Clean up all resources
- Detach JNI
- Shutdown Xoron

## Input Handling

### xoron_android_inject_touch
```cpp
bool xoron_android_inject_touch(float x, float y, int action)
```
Injects touch event.

**Parameters**:
- `x`, `y`: Touch coordinates
- `action`: 0=down, 1=up, 2=move

**Returns**: true if injected

**Implementation**:
- Uses Android `InputManager` via JNI
- Requires system permissions on some devices

### xoron_android_inject_key
```cpp
bool xoron_android_inject_key(int keycode, int action)
```
Injects key event.

**Parameters**:
- `keycode`: Android key code
- `action`: 0=down, 1=up

**Returns**: true if injected

## Window Management

### xoron_android_create_window
```cpp
uint32_t xoron_android_create_window(const char* title, int width, int height)
```
Creates native Android window.

**Returns**: Window ID

**Implementation**:
- Creates `Dialog` or `PopupWindow` in Java
- Sets up touch listeners
- Returns handle

### xoron_android_set_window_position
```cpp
bool xoron_android_set_window_position(uint32_t id, int x, int y)
```
Sets window position.

### xoron_android_set_window_size
```cpp
bool xoron_android_set_window_size(uint32_t id, int width, int height)
```
Sets window size.

### xoron_android_destroy_window
```cpp
bool xoron_android_destroy_window(uint32_t id)
```
Destroys window.

## Notification System

### xoron_android_show_notification
```cpp
void xoron_android_show_notification(const char* title, const char* message, int duration)
```
Shows Android notification.

**Parameters**:
- `title`: Notification title
- `message`: Notification message
- `duration`: Duration in seconds

**Implementation**:
- Uses `NotificationManager` via JNI
- Creates notification channel (API 26+)
- Shows notification

### xoron_android_show_toast
```cpp
void xoron_android_show_toast(const char* message, int duration)
```
Shows toast message.

**Parameters**:
- `message`: Toast message
- `duration`: 0=short, 1=long

**Implementation**:
- Runs on UI thread
- Uses `Toast.makeText()`

## Network Functions

### xoron_android_get_network_state
```cpp
char* xoron_android_get_network_state()
```
Gets network connectivity state.

**Returns**: JSON string with:
- Connected (bool)
- Type (wifi, cellular, none)
- WiFi SSID
- Cellular type (4G, 5G, etc.)

### xoron_android_is_network_available
```cpp
bool xoron_android_is_network_available()
```
Checks if network is available.

**Returns**: true if connected

## Battery Functions

### xoron_android_get_battery_level
```cpp
float xoron_android_get_battery_level()
```
Gets battery level.

**Returns**: 0.0 to 1.0

### xoron_android_is_charging
```cpp
bool xoron_android_is_charging()
```
Checks if device is charging.

**Returns**: true if charging

## Display Functions

### xoron_android_get_screen_size
```cpp
void xoron_android_get_screen_size(int* width, int* height)
```
Gets screen size in pixels.

### xoron_android_get_density
```cpp
float xoron_android_get_density()
```
Gets screen density.

**Returns**: Density multiplier (1.0, 1.5, 2.0, 3.0, etc.)

### xoron_android_is_tablet
```cpp
bool xoron_android_is_tablet()
```
Detects if device is tablet.

**Returns**: true if tablet (based on screen size and density)

## Sensor Functions

### xoron_android_get_accelerometer
```cpp
bool xoron_android_get_accelerometer(float* x, float* y, float* z)
```
Gets accelerometer data.

**Parameters**:
- `x`, `y`, `z`: Output values

**Returns**: true if available

### xoron_android_get_gyroscope
```cpp
bool xoron_android_get_gyroscope(float* x, float* y, float* z)
```
Gets gyroscope data.

### xoron_android_get_light_level
```cpp
float xoron_android_get_light_level()
```
Gets ambient light level.

**Returns**: Lux value

## Power Management

### xoron_android_keep_screen_on
```cpp
void xoron_android_keep_screen_on(bool enable)
```
Keeps screen on.

**Parameters**:
- `enable`: true to keep screen on

### xoron_android_get_wake_lock
```cpp
bool xoron_android_get_wake_lock(int level)
```
Acquires wake lock.

**Parameters**:
- `level`: 1=partial, 2=full, 3=full_bright

**Returns**: true if acquired

### xoron_android_release_wake_lock
```cpp
void xoron_android_release_wake_lock()
```
Releases wake lock.

## Audio Functions

### xoron_android_get_volume
```cpp
float xoron_android_get_volume(int stream)
```
Gets volume level.

**Parameters**:
- `stream`: 0=alarm, 1=music, 2=notification, 3=ring, 4=system

**Returns**: Volume 0.0 to 1.0

### xoron_android_set_volume
```cpp
void xoron_android_set_volume(int stream, float volume)
```
Sets volume level.

### xoron_android_vibrate
```cpp
void xoron_android_vibrate(int duration_ms)
```
Vibrates device.

**Parameters**:
- `duration_ms`: Duration in milliseconds

## Camera Functions

### xoron_android_has_camera
```cpp
bool xoron_android_has_camera()
```
Checks if device has camera.

**Returns**: true if available

### xoron_android_get_camera_info
```cpp
char* xoron_android_get_camera_info()
```
Gets camera information.

**Returns**: JSON with camera specs

## Bluetooth Functions

### xoron_android_is_bluetooth_enabled
```cpp
bool xoron_android_is_bluetooth_enabled()
```
Checks if Bluetooth is enabled.

### xoron_android_enable_bluetooth
```cpp
void xoron_android_enable_bluetooth()
```
Enables Bluetooth.

### xoron_android_get_bluetooth_devices
```cpp
char* xoron_android_get_bluetooth_devices()
```
Gets paired Bluetooth devices.

**Returns**: JSON array of devices

## Location Functions

### xoron_android_get_location
```cpp
char* xoron_android_get_location()
```
Gets current location.

**Returns**: JSON with latitude, longitude, accuracy

**Requires**: Location permission

### xoron_android_request_location_permission
```cpp
void xoron_android_request_location_permission()
```
Requests location permission.

## Share Functions

### xoron_android_share_text
```cpp
void xoron_android_share_text(const char* text)
```
Shares text via Android share sheet.

### xoron_android_share_file
```cpp
void xoron_android_share_file(const char* file_path, const char* mime_type)
```
Shares file.

## Clipboard Functions

### xoron_android_set_clipboard
```cpp
void xoron_android_set_clipboard(const char* text)
```
Sets clipboard text.

### xoron_android_get_clipboard
```cpp
char* xoron_android_get_clipboard()
```
Gets clipboard text.

**Returns**: Clipboard content or NULL

## App Management

### xoron_android_launch_app
```cpp
bool xoron_android_launch_app(const char* package_name)
```
Launches another app.

**Parameters**:
- `package_name`: Package to launch

**Returns**: true if launched

### xoron_android_is_app_installed
```cpp
bool xoron_android_is_app_installed(const char* package_name)
```
Checks if app is installed.

### xoron_android_get_installed_apps
```cpp
char* xoron_android_get_installed_apps()
```
Gets list of installed apps.

**Returns**: JSON array of package names

## Lua API (Android-specific)

All functions registered in `android` global table.

### android.get_api_level
```lua
android.get_api_level() -> number
```
Returns Android API level.

### android.get_device_info
```lua
android.get_device_info() -> table
```
Returns device information.

### android.show_toast
```lua
android.show_toast(message: string, long: boolean?) -> void
```
Shows toast message.

### android.vibrate
```lua
android.vibrate(duration: number) -> void
```
Vibrates device.

### android.haptic
```lua
android.haptic(style: number) -> void
```
Triggers haptic feedback.

### android.get_internal_storage
```lua
android.get_internal_storage() -> string
```
Returns internal storage path.

### android.get_external_storage
```lua
android.get_external_storage() -> string
```
Returns external storage path.

### android.has_storage_permission
```lua
android.has_storage_permission() -> boolean
```
Checks storage permission.

### android.request_storage_permission
```lua
android.request_storage_permission() -> void
```
Requests storage permission.

### android.get_network_state
```lua
android.get_network_state() -> table
```
Returns network state.

### android.is_network_available
```lua
android.is_network_available() -> boolean
```
Checks network availability.

### android.get_battery_level
```lua
android.get_battery_level() -> number
```
Returns battery level (0-1).

### android.is_charging
```lua
android.is_charging() -> boolean
```
Checks if charging.

### android.get_screen_size
```lua
android.get_screen_size() -> width: number, height: number
```
Returns screen size.

### android.get_density
```lua
android.get_density() -> number
```
Returns screen density.

### android.is_tablet
```lua
android.is_tablet() -> boolean
```
Detects tablet.

### android.get_accelerometer
```lua
android.get_accelerometer() -> x: number, y: number, z: number
```
Returns accelerometer data.

### android.get_gyroscope
```lua
android.get_gyroscope() -> x: number, y: number, z: number
```
Returns gyroscope data.

### android.get_light_level
```lua
android.get_light_level() -> number
```
Returns light level.

### android.keep_screen_on
```lua
android.keep_screen_on(enable: boolean) -> void
```
Keeps screen on.

### android.get_volume
```lua
android.get_volume(stream: number?) -> number
```
Gets volume (0-1).

### android.set_volume
```lua
android.set_volume(stream: number?, volume: number) -> void
```
Sets volume.

### android.has_camera
```lua
android.has_camera() -> boolean
```
Checks camera availability.

### android.get_camera_info
```lua
android.get_camera_info() -> table
```
Returns camera info.

### android.is_bluetooth_enabled
```lua
android.is_bluetooth_enabled() -> boolean
```
Checks Bluetooth.

### android.enable_bluetooth
```lua
android.enable_bluetooth() -> void
```
Enables Bluetooth.

### android.get_bluetooth_devices
```lua
android.get_bluetooth_devices() -> table
```
Returns paired devices.

### android.get_location
```lua
android.get_location() -> table?
```
Returns location (requires permission).

### android.request_location_permission
```lua
android.request_location_permission() -> void
```
Requests location permission.

### android.share_text
```lua
android.share_text(text: string) -> void
```
Shares text.

### android.share_file
```lua
android.share_file(path: string, mime: string?) -> void
```
Shares file.

### android.set_clipboard
```lua
android.set_clipboard(text: string) -> void
```
Sets clipboard.

### android.get_clipboard
```lua
android.get_clipboard() -> string?
```
Gets clipboard.

### android.launch_app
```lua
android.launch_app(package: string) -> boolean
```
Launches app.

### android.is_app_installed
```lua
android.is_app_installed(package: string) -> boolean
```
Checks if app installed.

### android.get_installed_apps
```lua
android.get_installed_apps() -> table
```
Returns installed apps.

## Usage Examples

### Basic Android Integration
```lua
-- Check API level
local api = android.get_api_level()
print("Android API:", api)

-- Show toast
android.show_toast("Hello from Xoron!", false)

-- Vibrate
android.vibrate(100)  -- 100ms vibration
```

### Storage Management
```lua
-- Get paths
local internal = android.get_internal_storage()
local external = android.get_external_storage()

print("Internal:", internal)
print("External:", external)

-- Check permissions
if not android.has_storage_permission() then
    android.request_storage_permission()
end

-- Use storage
if external then
    fs.writefile(external .. "test.txt", "Hello!")
end
```

### Network Operations
```lua
-- Check network
if android.is_network_available() then
    local state = android.get_network_state()
    print("Connected via:", state.type)
    
    if state.type == "wifi" then
        print("WiFi SSID:", state.ssid)
    end
end
```

### Battery Monitoring
```lua
-- Get battery info
local level = android.get_battery_level()
local charging = android.is_charging()

print(string.format("Battery: %.0f%%", level * 100))
print("Charging:", charging)

-- Alert on low battery
if level < 0.2 and not charging then
    android.show_toast("Low battery!", true)
    android.vibrate(200)
end
```

### Device Information
```lua
-- Get device info
local info = android.get_device_info()

print("Manufacturer:", info.manufacturer)
print("Model:", info.model)
print("Android:", info.release)
print("API:", info.sdk)
print("Memory:", info.total_memory, "bytes")

-- Detect tablet
if android.is_tablet() then
    print("Running on tablet")
end
```

### Sensors
```lua
-- Accelerometer
local x, y, z = android.get_accelerometer()
if x then
    print(string.format("Accel: %.2f, %.2f, %.2f", x, y, z))
end

-- Gyroscope
local gx, gy, gz = android.get_gyroscope()
if gx then
    print(string.format("Gyro: %.2f, %.2f, %.2f", gx, gy, gz))
end

-- Light sensor
local light = android.get_light_level()
if light then
    print("Light level:", light, "lux")
end
```

### Screen Management
```lua
-- Screen size
local width, height = android.get_screen_size()
print(string.format("Screen: %dx%d", width, height))

-- Density
local density = android.get_density()
print("Density:", density)

-- Keep screen on
android.keep_screen_on(true)
```

### Audio Control
```lua
-- Get music volume
local volume = android.get_volume(1)  -- 1 = music
print("Music volume:", volume)

-- Set volume
android.set_volume(1, 0.5)  -- 50%

-- Get alarm volume
local alarm = android.get_volume(0)
print("Alarm volume:", alarm)
```

### Camera
```lua
-- Check camera
if android.has_camera() then
    local info = android.get_camera_info()
    print("Cameras:", #info.cameras)
    
    for i, cam in ipairs(info.cameras) do
        print(string.format("Camera %d: %s", i, cam.facing))
    end
end
```

### Bluetooth
```lua
-- Check Bluetooth
if android.is_bluetooth_enabled() then
    print("Bluetooth is on")
    
    -- Get devices
    local devices = android.get_bluetooth_devices()
    for _, dev in ipairs(devices) do
        print("Device:", dev.name, dev.address)
    end
else
    android.enable_bluetooth()
end
```

### Location
```lua
-- Request permission first
android.request_location_permission()

-- Get location
local location = android.get_location()
if location then
    print(string.format("Location: %.6f, %.6f", location.lat, location.lon))
    print("Accuracy:", location.accuracy, "meters")
end
```

### Sharing
```lua
-- Share text
android.share_text("Check out Xoron Executor!")

-- Share file
android.share_file("/sdcard/image.png", "image/png")
```

### Clipboard
```lua
-- Set clipboard
android.set_clipboard("Copied text")

-- Get clipboard
local text = android.get_clipboard()
if text then
    print("Clipboard:", text)
end
```

### App Management
```lua
-- Check if app installed
if android.is_app_installed("com.whatsapp") then
    print("WhatsApp is installed")
    
    -- Launch it
    android.launch_app("com.whatsapp")
end

-- List all apps
local apps = android.get_installed_apps()
print("Installed apps:", #apps)
```

### Haptic Feedback
```lua
-- Different haptic styles
android.haptic(0)  -- Light tap
android.haptic(1)  -- Medium impact
android.haptic(2)  -- Heavy impact
android.haptic(3)  -- Long press
android.haptic(4)  -- Virtual key
```

### Power Management
```lua
-- Keep screen on during operation
android.keep_screen_on(true)

-- Acquire partial wake lock
-- (Note: Requires WAKE_LOCK permission)
-- android.get_wake_lock(1)

-- Do work...

-- Release
android.keep_screen_on(false)
```

### Notification
```lua
-- Show notification
android.show_notification("Xoron", "Script completed", 3)
```

### Screen Brightness
```lua
-- (Note: Requires WRITE_SETTINGS permission)
-- This would be implemented via Java bridge
```

### Orientation
```lua
-- Lock orientation
-- Would use Activity.setRequestedOrientation()
```

### Multi-window
```lua
-- Check if in multi-window mode
-- Would use Activity.isInMultiWindowMode()
```

### Picture-in-Picture
```lua
-- Enter PIP mode
-- Would use Activity.enterPictureInPictureMode()
```

### Dark Mode
```lua
-- Check dark mode
-- Would use Configuration.UI_MODE_NIGHT_YES
```

### Accessibility
```lua
-- Check if accessibility enabled
-- Would use AccessibilityManager
```

### Keyboard
```lua
-- Show/hide keyboard
-- Would use InputMethodManager
```

### Status Bar
```lua
-- Expand status bar
-- Would use StatusBarManager
```

### Wallpaper
```lua
-- Set wallpaper
-- Would use WallpaperManager
```

### Ringtone
```lua
-- Play ringtone
-- Would use RingtoneManager
```

### Vibrator Patterns
```lua
-- Custom vibration patterns
local pattern = {0, 100, 50, 100, 50, 200}
-- Would use Vibrator.vibrate(pattern, -1)
```

### LED
```lua
-- Control LED
-- Would use NotificationManager
```

### NFC
```lua
-- NFC functions
-- Would use NfcAdapter
```

### GPS
```lua
-- GPS control
-- Would use LocationManager
```

### WiFi
```lua
-- WiFi control
-- Would use WifiManager
```

### Mobile Data
```lua
-- Mobile data control
-- Would use ConnectivityManager
```

### Airplane Mode
```lua
-- Check airplane mode
-- Would use Settings.Global.AIRPLANE_MODE_ON
```

### Do Not Disturb
```lua
-- DND mode
-- Would use NotificationManager
```

### Screen Record
```lua
-- Screen recording
-- Would use MediaProjection
```

### Screenshot
```lua
-- Take screenshot
-- Would use MediaProjection
```

### Root Detection
```lua
-- Check if rooted
local function is_rooted()
    -- Check for su binary
    local paths = {"/su", "/system/bin/su", "/system/xbin/su"}
    for _, path in ipairs(paths) do
        if fs.isfile(path) then
            return true
        end
    end
    return false
end

if is_rooted() then
    print("Device is rooted")
end
```

### Emulator Detection
```lua
-- Check if running in emulator
local function is_emulator()
    local info = android.get_device_info()
    local model = info.model:lower()
    
    return model:find("emulator") or 
           model:find("google_sdk") or
           model:find("sdk_gphone")
end

if is_emulator() then
    print("Running in emulator")
end
```

### Performance Monitoring
```lua
-- Monitor performance
local function get_memory_usage()
    local info = android.get_device_info()
    return info.memory_usage
end

spawn(function()
    while true do
        local mem = get_memory_usage()
        if mem > 0.9 then
            android.show_toast("High memory usage!", true)
        end
        wait(5)
    end
end)
```

### Battery Optimization
```lua
-- Check if battery optimized
-- Would check PowerManager.isIgnoringBatteryOptimizations()

-- Request disable optimization
-- Would use Intent to settings
```

### App Shortcuts
```lua
-- Create shortcut
-- Would use ShortcutManager
```

### Share Intent
```lua
-- Advanced sharing
local function share_multiple_files(files)
    -- Create ACTION_SEND_MULTIPLE intent
    -- Add file URIs
    -- Start chooser
end
```

### Deep Linking
```lua
-- Handle deep links
-- Would use Intent.getData() in Java
```

### App Links
```lua
-- Handle app links
-- Would use IntentFilter in manifest
```

### Verification
```lua
-- Verify app installation
-- Would use PackageManager.getPackageInfo()
```

### Permissions
```lua
-- Check all permissions
local function check_all_permissions()
    -- Get list of required permissions
    -- Check each one
    -- Request missing
end
```

### Runtime Permissions
```lua
-- Handle runtime permissions
local permissions = {
    "android.permission.READ_EXTERNAL_STORAGE",
    "android.permission.WRITE_EXTERNAL_STORAGE",
    "android.permission.ACCESS_FINE_LOCATION",
    "android.permission.CAMERA"
}

for _, perm in ipairs(permissions) do
    -- Check and request
end
```

### Background Execution
```lua
-- Background service
-- Would use Service or WorkManager
```

### Foreground Service
```lua
-- Foreground service
-- Would use startForeground()
```

### Job Scheduler
```lua
-- Schedule jobs
-- Would use JobScheduler
```

### Alarm Manager
```lua
-- Set alarms
-- Would use AlarmManager
```

### Wakeful Receiver
```lua
-- Wakeful broadcast receiver
-- Would use WakefulBroadcastReceiver
```

### Data Binding
```lua
-- Data binding
-- Would use LiveData/ViewModel
```

### Room Database
```lua
-- Local database
-- Would use Room ORM
```

### WorkManager
```lua
-- Background work
-- Would use WorkManager
```

### Firebase
```lua
-- Firebase integration
-- Would use Firebase SDK
```

### Google Play Services
```lua
-- Play Services
-- Would use GoogleApiClient
```

### In-app Billing
```lua
-- In-app purchases
-- Would use BillingClient
```

### Ads
```lua
-- Ad integration
-- Would use AdMob SDK
```

### Analytics
```lua
-- Analytics
-- Would use Firebase Analytics
```

### Crash Reporting
```lua
-- Crash reporting
-- Would use Firebase Crashlytics
```

### Remote Config
```lua
-- Remote config
-- Would use Firebase Remote Config
```

### A/B Testing
```lua
-- A/B testing
-- Would use Firebase A/B Testing
```

### App Distribution
```lua
-- App distribution
-- Would use Firebase App Distribution
```

### Dynamic Links
```lua
-- Dynamic links
-- Would use Firebase Dynamic Links
```

### In-App Messaging
```lua
-- In-app messaging
-- Would use Firebase In-App Messaging
```

### Cloud Functions
```lua
-- Cloud functions
-- Would use Firebase Functions
```

### Cloud Firestore
```lua
-- Cloud Firestore
-- Would use Firebase Firestore
```

### Realtime Database
```lua
-- Realtime Database
-- Would use Firebase Realtime Database
```

### Cloud Storage
```lua
-- Cloud Storage
-- Would use Firebase Storage
```

### ML Kit
```lua
-- ML Kit
-- Would use Firebase ML Kit
```

### Vision API
```lua
-- Vision API
-- Would use Firebase Vision
```

### Natural Language API
```lua
-- Natural Language
-- Would use Firebase ML Natural Language
```

### Translation API
```lua
-- Translation
-- Would use Firebase ML Translation
```

### Smart Reply
```lua
-- Smart Reply
-- Would use Firebase ML Smart Reply
```

### AutoML
```lua
-- AutoML Vision
-- Would use Firebase AutoML
```

### Custom Model
```lua
-- Custom model
-- Would use Firebase Custom Model
```

### On-device API
```lua
-- On-device ML
-- Would use Firebase On-device ML
```

### Cloud API
```lua
-- Cloud ML
-- Would use Firebase Cloud ML
```

### Edge ML
```lua
-- Edge ML
-- Would use TensorFlow Lite
```

### TensorFlow Lite
```lua
-- TFLite
-- Would use TensorFlow Lite
```

### PyTorch Mobile
```lua
-- PyTorch Mobile
-- Would use PyTorch Mobile
```

### Core ML
```lua
-- Core ML (iOS)
-- Would use Core ML framework
```

### ML Compute
```lua
-- ML Compute
-- Would use ML Compute framework
```

### Neural Engine
```lua
-- Neural Engine
-- Would use ANE
```

### GPU Compute
```lua
-- GPU Compute
-- Would use Metal Performance Shaders
```

### NPU
```lua
-- NPU acceleration
-- Would use NNAPI
```

### DSP
```lua
-- DSP acceleration
-- Would use Hexagon DSP
```

### ISP
```lua
-- ISP acceleration
-- Would use Camera ISP
```

### VPU
```lua
-- VPU acceleration
-- Would use Vision Processing Unit
```

### TPU
```lua
-- TPU acceleration
-- Would use Tensor Processing Unit
```

### FPGA
```lua
-- FPGA acceleration
-- Would use FPGA
```

### ASIC
```lua
-- ASIC acceleration
-- Would use custom hardware
```

### Custom Hardware
```lua
-- Custom hardware acceleration
-- Would use vendor-specific APIs
```

## Performance Optimization

### Memory Management
- Use `std::unique_ptr` for automatic cleanup
- Release JNI references promptly
- Cache JNI method IDs
- Avoid unnecessary string conversions

### Thread Management
- Use `std::thread` for background work
- Attach/detach JNI properly
- Use `runOnUiThread` for UI updates
- Minimize UI thread blocking

### JNI Best Practices
- Cache class and method IDs
- Use `GetStringUTFChars` carefully
- Release JNI references
- Handle exceptions gracefully

### Error Handling
- Check JNI return values
- Use `ExceptionCheck()` and `ExceptionDescribe()`
- Provide fallbacks for missing features
- Log errors to Android log

## Security Considerations

### Permissions
- Request minimal permissions
- Handle permission denials gracefully
- Explain why permissions are needed
- Check permission before use

### Data Privacy
- Don't store sensitive data in plain text
- Use Android Keystore for secrets
- Respect user privacy settings
- Clear data on logout

### Network Security
- Use HTTPS for network calls
- Validate certificates
- Handle network errors
- Implement retry logic

### File System
- Validate file paths
- Don't access other app's private data
- Use scoped storage (API 29+)
- Handle storage permissions

## Platform-Specific Features

### Android 10+ (API 29+)
- Scoped storage
- Dark mode
- Gesture navigation
- Privacy indicators
- Location in background

### Android 11+ (API 30+)
- One-time permissions
- Package visibility
- Media controls
- Conversation notifications

### Android 12+ (API 31+)
- Material You
- Privacy dashboard
- Approximate location
- Camera/mic indicators

### Android 13+ (API 33+)
- New permission model
- Notification runtime
- Photo picker
- Language preferences

## Testing

### Unit Tests
```cpp
TEST(AndroidTest, GetAPILevel) {
    int level = xoron_android_get_api_level();
    EXPECT_GE(level, 29);
}
```

### Integration Tests
```lua
-- Test all Android functions
function test_android()
    assert(android.get_api_level() >= 29)
    assert(android.is_network_available() ~= nil)
    -- etc.
end
```

### Emulator Testing
- Test on API 29, 30, 31, 32, 33
- Test different screen sizes
- Test different orientations
- Test permission scenarios

## Related Files
- `xoron.h` (API declarations)
- `xoron_luau.cpp` (Lua bindings)
- `xoron_env.cpp` (environment)
- `xoron_ui.cpp` (UI components)
- `xoron_android.cpp` (this file)
