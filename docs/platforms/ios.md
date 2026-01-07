# iOS Integration Guide

## Overview

This guide covers complete integration of Xoron into iOS applications (iOS 15.0+).

## Prerequisites

### Software Requirements
- **Xcode**: 14.0+ (15.0 recommended)
- **iOS SDK**: 15.0+
- **CMake**: 3.16+
- **iOS CMake toolchain**: https://github.com/leetal/ios-cmake
- **OpenSSL**: Built for iOS (arm64, arm64e)

### Hardware Requirements
- **Mac**: macOS 13.0+ (Ventura or later)
- **Xcode Command Line Tools**: Installed
- **iOS Device**: iPhone/iPad with iOS 15+ (for testing)

## Building Xoron for iOS

### Step 1: Prepare OpenSSL

```bash
# Option 1: Use pre-built OpenSSL
wget https://github.com/leenjewel/openssl-for-ios/releases/download/1.1.1w/openssl-1.1.1w-ios.tar.gz
tar xzf openssl-1.1.1w-ios.tar.gz
mv openssl-1.1.1w openssl-ios

# Option 2: Build from source
git clone https://github.com/openssl/openssl.git
cd openssl
./Configure ios64-xcrun --prefix=$(pwd)/../openssl-ios no-shared no-tests
make -j$(nproc)
make install_sw
```

### Step 2: Build Xoron

```bash
# Clone repository
git clone https://github.com/yourusername/xoron.git
cd xoron/src

# Create build directory
mkdir build-ios && cd build-ios

# Configure with CMake
cmake .. \
  -DXORON_IOS_BUILD=ON \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/ios-cmake/ios.toolchain.cmake \
  -DPLATFORM=OS64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 \
  -DXORON_OPENSSL_ROOT=/path/to/openssl-ios

# Build
cmake --build . --config Release

# Verify output
ls -la libxoron.dylib
file libxoron.dylib
```

**Expected Output**:
```
libxoron.dylib: Mach-O 64-bit dynamically linked shared library arm64
```

### Step 3: Verify Architecture

```bash
# Check supported architectures
lipo -info libxoron.dylib

# Output should show:
# Architectures in the fat file: libxoron.dylib are: arm64
```

## Integration Methods

### Method 1: Dynamic Library (Recommended)

#### 1. Add Library to Xcode Project

1. Open your Xcode project
2. Drag `libxoron.dylib` into your project
3. In the dialog:
   - **Copy items if needed**: ✓
   - **Add to targets**: Select your app target
   - **Create folder references**: ✓

#### 2. Configure Build Settings

In your target's Build Settings:

**Linking**:
- **Other Linker Flags**: Add `-l"xoron"`
- **Runpath Search Paths**: Add `@executable_path/Frameworks`

**Search Paths**:
- **Library Search Paths**: Add path to directory containing `libxoron.dylib`

#### 3. Embed Library

In Build Phases:
- **Copy Files**: Add `libxoron.dylib`
- **Destination**: Frameworks
- **Code Sign On Copy**: ✓

#### 4. Import in Code

```objective-c
// In your Objective-C file
#import <Foundation/Foundation.h>

// Import Xoron header
extern "C" {
    #include "xoron.h"
}

// Use Xoron
- (void)initializeXoron {
    if (xoron_init() != XORON_OK) {
        NSLog(@"Failed to initialize Xoron: %s", xoron_last_error());
        return;
    }
    
    xoron_vm_t* vm = xoron_vm_new();
    if (!vm) {
        NSLog(@"Failed to create VM: %s", xoron_last_error());
        return;
    }
    
    // Execute script
    const char* script = "print('Hello from Xoron!')";
    int result = xoron_dostring(vm, script, "main");
    
    if (result != XORON_OK) {
        NSLog(@"Execution failed: %s", xoron_last_error());
    }
    
    xoron_vm_free(vm);
    xoron_shutdown();
}
```

### Method 2: Framework Bundle

#### 1. Create Framework Structure

```bash
mkdir -p Xoron.framework
mkdir -p Xoron.framework/Headers
mkdir -p Xoron.framework/Modules

# Copy library
cp libxoron.dylib Xoron.framework/Xoron

# Copy header
cp xoron.h Xoron.framework/Headers/

# Create module map
cat > Xoron.framework/Modules/module.modulemap << EOF
framework module Xoron {
    header "xoron.h"
    export *
}
EOF

# Create Info.plist
cat > Xoron.framework/Info.plist << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleDevelopmentRegion</key>
    <string>en</string>
    <key>CFBundleExecutable</key>
    <string>Xoron</string>
    <key>CFBundleIdentifier</key>
    <string>com.xoron.framework</string>
    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>
    <key>CFBundleName</key>
    <string>Xoron</string>
    <key>CFBundlePackageType</key>
    <string>FMWK</string>
    <key>CFBundleShortVersionString</key>
    <string>2.0.0</string>
    <key>CFBundleVersion</key>
    <string>1</string>
</dict>
</plist>
EOF
```

#### 2. Add Framework to Xcode

1. Drag `Xoron.framework` into your project
2. In Build Settings:
   - **Framework Search Paths**: Add path to framework
   - **Other Linker Flags**: Add `-framework Xoron`

#### 3. Import and Use

```swift
// In Swift
import Xoron

class XoronManager {
    func runScript() {
        let script = "print('Hello from Swift!')"
        let result = xoron_dostring(vm, script, "main")
        // Handle result
    }
}
```

### Method 3: Static Linking

For apps that need to be self-contained:

```bash
# Build static library
cmake .. -DBUILD_SHARED_LIBS=OFF
cmake --build .

# Link in Xcode
# Add libxoron.a to "Link Binary With Libraries"
# Add -l"xoron" to Other Linker Flags
```

## iOS-Specific Features

### Haptic Feedback

```objective-c
#import <Foundation/Foundation.h>

extern "C" {
    void xoron_ios_haptic_feedback(int style);
}

// Usage
- (void)triggerHaptic {
    // 0 = Light, 1 = Medium, 2 = Heavy
    xoron_ios_haptic_feedback(1);
}
```

**Implementation**:
```objective-c
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

### Console Logging

```objective-c
void xoron_ios_console_print(const char* message, int type) {
    NSString* msg = [NSString stringWithUTF8String:message];
    switch (type) {
        case 0: NSLog(@"%@", msg); break;
        case 1: NSLog(@"⚠️ %@", msg); break;
        case 2: NSLog(@"❌ %@", msg); break;
    }
}
```

### Clipboard Operations

```objective-c
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

### File System Integration

```objective-c
// Get Documents directory
NSString* get_documents_directory() {
    NSArray* paths = NSSearchPathForDirectoriesInDomains(
        NSDocumentDirectory, NSUserDomainMask, YES);
    return [paths firstObject];
}

// Example: Save file
- (void)saveFile:(NSString*)filename content:(NSString*)content {
    NSString* dir = get_documents_directory();
    NSString* path = [dir stringByAppendingPathComponent:filename];
    
    NSError* error = nil;
    BOOL success = [content writeToFile:path 
                             atomically:YES 
                               encoding:NSUTF8StringEncoding 
                                  error:&error];
    
    if (!success) {
        NSLog(@"Error saving file: %@", error);
    }
}
```

## SwiftUI Integration

### Basic Setup

```swift
import SwiftUI
import Xoron

struct XoronView: View {
    @State private var output: String = ""
    
    var body: some View {
        VStack {
            Text("Xoron Executor")
                .font(.title)
            
            TextEditor(text: $output)
                .frame(height: 200)
            
            Button("Run Script") {
                runScript()
            }
        }
        .padding()
    }
    
    func runScript() {
        // Initialize Xoron
        if xoron_init() != XORON_OK {
            output = "Init failed"
            return
        }
        
        let vm = xoron_vm_new()
        let script = """
        local response = http.get("https://api.github.com")
        if response then
            return "Status: " .. response.status
        end
        return "Failed"
        """
        
        // Execute
        let result = xoron_dostring(vm, script, "swiftui_test")
        
        if result == XORON_OK {
            // Get result from Lua stack
            output = "Execution successful"
        } else {
            output = String(cString: xoron_last_error())
        }
        
        xoron_vm_free(vm)
        xoron_shutdown()
    }
}
```

### UIKit Integration

```objective-c
// In your ViewController
@interface XoronViewController : UIViewController
@property (nonatomic) xoron_vm_t* vm;
@end

@implementation XoronViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    
    // Initialize Xoron
    xoron_init();
    self.vm = xoron_vm_new();
    
    // Setup UI
    [self setupUI];
}

- (void)setupUI {
    UIButton* button = [UIButton buttonWithType:UIButtonTypeSystem];
    [button setTitle:@"Run Script" forState:UIControlStateNormal];
    [button addTarget:self 
               action:@selector(runScript) 
     forControlEvents:UIControlEventTouchUpInside];
    
    button.frame = CGRectMake(100, 100, 200, 50);
    [self.view addSubview:button];
}

- (void)runScript {
    const char* script = "print('Hello from UIKit!')";
    int result = xoron_dostring(self.vm, script, "uikit_test");
    
    if (result != XORON_OK) {
        UIAlertController* alert = [UIAlertController 
            alertControllerWithTitle:@"Error"
                             message:[NSString stringWithUTF8String:xoron_last_error()]
                      preferredStyle:UIAlertControllerStyleAlert];
        [self presentViewController:alert animated:YES completion:nil];
    }
}

- (void)dealloc {
    if (self.vm) {
        xoron_vm_free(self.vm);
    }
    xoron_shutdown();
}

@end
```

## Advanced Integration

### WebSocket Integration

```objective-c
// Connect to WebSocket
- (void)connectWebSocket {
    const char* url = "wss://echo.websocket.org";
    
    // In Lua script
    const char* script = R"(
        local ws = WebSocket.connect("wss://echo.websocket.org")
        ws:on_message(function(msg)
            print("Received:", msg)
        end)
        ws:send("Hello from iOS!")
    )";
    
    xoron_dostring(self.vm, script, "websocket");
}
```

### HTTP with Callbacks

```objective-c
// Custom HTTP handler
static int lua_ios_http_get(lua_State* L) {
    const char* url = luaL_checkstring(L, 1);
    
    // Use iOS URLSession for better integration
    NSURL* nsUrl = [NSURL URLWithString:[NSString stringWithUTF8String:url]];
    NSURLSession* session = [NSURLSession sharedSession];
    
    dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);
    __block char* response_data = nullptr;
    __block int status = 0;
    
    NSURLSessionDataTask* task = [session dataTaskWithURL:nsUrl 
        completionHandler:^(NSData* data, NSURLResponse* response, NSError* error) {
        if (error) {
            status = -1;
        } else {
            NSHTTPURLResponse* httpResponse = (NSHTTPURLResponse*)response;
            status = (int)httpResponse.statusCode;
            
            if (data) {
                response_data = (char*)malloc(data.length + 1);
                memcpy(response_data, data.bytes, data.length);
                response_data[data.length] = '\0';
            }
        }
        dispatch_semaphore_signal(semaphore);
    }];
    
    [task resume];
    dispatch_semaphore_wait(semaphore, DISPATCH_TIME_FOREVER);
    
    if (response_data) {
        lua_newtable(L);
        lua_pushinteger(L, status);
        lua_setfield(L, -2, "status");
        lua_pushstring(L, response_data);
        lua_setfield(L, -2, "body");
        free(response_data);
        return 1;
    }
    
    return 0;
}
```

### Background Execution

```objective-c
// Run Xoron in background
- (void)runInBackground {
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        xoron_vm_t* vm = xoron_vm_new();
        
        // Long-running script
        const char* script = R"(
            for i = 1, 1000000 do
                -- Heavy computation
                local x = math.sqrt(i)
            end
            return "Done"
        )";
        
        int result = xoron_dostring(vm, script, "background");
        
        // Update UI on main thread
        dispatch_async(dispatch_get_main_queue(), ^{
            if (result == XORON_OK) {
                [self updateStatus:@"Completed"];
            }
        });
        
        xoron_vm_free(vm);
    });
}
```

## Memory Management

### ARC Considerations

```objective-c
// ARC handles Objective-C objects
// But C structures need manual management

@interface XoronManager : NSObject
@property (nonatomic) xoron_vm_t* vm;  // C structure
@end

@implementation XoronManager

- (instancetype)init {
    self = [super init];
    if (self) {
        xoron_init();
        _vm = xoron_vm_new();
    }
    return self;
}

- (void)dealloc {
    // ARC calls this automatically
    if (_vm) {
        xoron_vm_free(_vm);
    }
    xoron_shutdown();
}

@end
```

### Memory Limits

```objective-c
// Set memory limit for VM
- (void)setMemoryLimit {
    // 50MB limit
    lua_gc(self.vm->L, LUA_GCSETALLOCF, 0, 50 * 1024 * 1024);
}

// Monitor memory
- (void)checkMemory {
    int bytes = lua_gc(self.vm->L, LUA_GCCOUNT, 0);
    NSLog(@"Memory usage: %d KB", bytes / 1024);
}
```

## Security

### App Transport Security

```xml
<!-- Info.plist -->
<key>NSAppTransportSecurity</key>
<dict>
    <key>NSAllowsArbitraryLoads</key>
    <true/>
    <!-- Or for specific domains -->
    <key>NSExceptionDomains</key>
    <dict>
        <key>example.com</key>
        <dict>
            <key>NSIncludesSubdomains</key>
            <true/>
            <key>NSExceptionRequiresForwardSecrecy</key>
            <false/>
        </dict>
    </dict>
</dict>
```

### Keychain for Secrets

```objective-c
// Store API key in Keychain
- (void)storeAPIKey:(NSString*)key {
    NSDictionary* query = @{
        (__bridge id)kSecClass: (__bridge id)kSecClassGenericPassword,
        (__bridge id)kSecAttrAccount: @"APIKey",
        (__bridge id)kSecValueData: [key dataUsingEncoding:NSUTF8StringEncoding],
        (__bridge id)kSecAttrAccessible: (__bridge id)kSecAttrAccessibleWhenUnlocked
    };
    
    SecItemDelete((__bridge CFDictionaryRef)query);
    SecItemAdd((__bridge CFDictionaryRef)query, NULL);
}

// Retrieve API key
- (NSString*)getAPIKey {
    NSDictionary* query = @{
        (__bridge id)kSecClass: (__bridge id)kSecClassGenericPassword,
        (__bridge id)kSecAttrAccount: @"APIKey",
        (__bridge id)kSecReturnData: @YES
    };
    
    CFTypeRef result;
    OSStatus status = SecItemCopyMatching((__bridge CFDictionaryRef)query, &result);
    
    if (status == errSecSuccess) {
        NSData* data = (__bridge_transfer NSData*)result;
        return [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
    }
    
    return nil;
}
```

## Debugging

### Xcode Debugger

```bash
# Set breakpoints in xoron_ios.mm
# Use LLDB commands:
(lldb) breakpoint set --file xoron_ios.mm --line 50
(lldb) run
(lldb) print *vm
(lldb) print (char*)xoron_last_error()
```

### Console Output

```objective-c
// View logs in Xcode Console or Console.app
NSLog(@"Xoron: %s", message);

// Or use unified logging
os_log_with_type(OS_LOG_DEFAULT, OS_LOG_TYPE_INFO, "Xoron: %s", message);
```

### Instruments Profiling

1. Product → Profile
2. Choose "Leaks" or "Allocations"
3. Run your app
4. Analyze memory usage

## Distribution

### App Store Submission

1. **Build for Release**:
```bash
cmake --build . --config Release
```

2. **Sign Library**:
```bash
codesign -s "Apple Distribution" libxoron.dylib
```

3. **Embed in App**:
- Add to "Copy Files" build phase
- Destination: Frameworks
- Code Sign On Copy: ✓

4. **Archive and Upload**:
- Xcode: Product → Archive
- Organizer: Distribute App

### Enterprise Distribution

Same as App Store, but use Enterprise provisioning profile.

## Troubleshooting

### Common Issues

**1. Library Not Found**
```
dyld: Library not loaded: @rpath/libxoron.dylib
```
**Solution**:
- Check "Runpath Search Paths" includes `@executable_path/Frameworks`
- Verify library is in "Copy Files" build phase
- Use `otool -L` to check dependencies

**2. Linker Errors**
```
Undefined symbols for architecture arm64
```
**Solution**:
- Verify library architecture: `lipo -info`
- Check "Other Linker Flags"
- Clean build folder (Shift+Cmd+K)

**3. HTTPS Fails**
```
HTTP request failed: SSL error
```
**Solution**:
- Verify OpenSSL is linked
- Check Info.plist for ATS settings
- Test with httpbin.org

**4. Memory Issues**
```
Memory limit exceeded
```
**Solution**:
- Increase memory limit
- Use `lua_gc` to monitor
- Profile with Instruments

### Verification Checklist

```bash
# 1. Check library
file libxoron.dylib
# Should show: Mach-O 64-bit dynamically linked shared library arm64

# 2. Check dependencies
otool -L libxoron.dylib
# Should show: libSystem.B.dylib, libc++.1.dylib, etc.

# 3. Check symbols
nm -gU libxoron.dylib | grep xoron_init
# Should show: _xoron_init

# 4. Test in simulator
xcrun simctl boot "iPhone 14"
xcrun simctl install booted YourApp.app
xcrun simctl launch booted com.yourcompany.yourapp
```

## Performance Optimization

### Build Settings

**Release Configuration**:
- **Optimization Level**: Fastest [-O3]
- **Debug Information Format**: DWARF with dSYM File
- **Strip Debug Symbols**: Yes
- **Deployment Postprocessing**: Yes
- **Enable Bitcode**: Yes (for App Store)

### Runtime Optimization

```objective-c
// Pre-compile frequently used scripts
xoron_bytecode_t* bc = xoron_compile(script, strlen(script), "cached");

// Reuse VM instances
static xoron_vm_t* sharedVM = nil;
+ (xoron_vm_t*)sharedVM {
    if (!sharedVM) {
        sharedVM = xoron_vm_new();
    }
    return sharedVM;
}

// Batch operations
- (void)batchOperations {
    const char* script = R"(
        -- Multiple operations in one call
        local results = {}
        for i = 1, 100 do
            results[i] = math.sqrt(i)
        end
        return results
    )";
    
    xoron_dostring(self.vm, script, "batch");
}
```

## Summary

This guide covered:
- Building Xoron for iOS
- Multiple integration methods
- iOS-specific features
- SwiftUI and UIKit integration
- Memory management
- Security considerations
- Debugging techniques
- Distribution methods
- Performance optimization

**Next Steps**:
- Review [Android Integration Guide](android.md)
- Check [Cross-Platform Guide](cross_platform.md)
- See [iOS Tests](../../src/tests/ios/README.md) for examples
