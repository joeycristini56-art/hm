/*
 * xoron.h - Xoron Executor Engine
 * Full-featured Luau executor with HTTP, Crypto, WebSocket, Drawing, and more
 * Platforms: iOS 15+ (.dylib) and Android 10+ (.so)
 */

#ifndef XORON_H
#define XORON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ============== Version Information ============== */
#define XORON_VERSION "2.0.0"
#define XORON_NAME "Xoron"

/* ============== Platform Detection ============== */
/* 
 * Platform macros:
 *   XORON_PLATFORM_IOS     - iOS (iPhone/iPad) ARM64
 *   XORON_PLATFORM_ANDROID - Android ARM64/ARM32/x86/x86_64
 *   XORON_PLATFORM_UNKNOWN - Unknown/unsupported platform
 * 
 * Minimum supported versions:
 *   iOS: 15.0+
 *   Android: API 29+ (Android 10+)
 */

/* iOS Detection */
#if defined(__APPLE__)
    #include <TargetConditionals.h>
    #if TARGET_OS_IPHONE || TARGET_OS_IOS || TARGET_IPHONE_SIMULATOR
        #define XORON_PLATFORM_IOS 1
        #define XORON_PLATFORM_NAME "iOS"
        #define XORON_MIN_IOS_VERSION 15.0
        /* iOS uses .dylib shared libraries */
        #define XORON_LIBRARY_EXTENSION ".dylib"
    #endif
#endif

/* Android Detection */
#if defined(__ANDROID__) || defined(ANDROID) || defined(XORON_ANDROID)
    #ifndef XORON_PLATFORM_ANDROID
        #define XORON_PLATFORM_ANDROID 1
    #endif
    #define XORON_PLATFORM_NAME "Android"
    /* Minimum Android API level (Android 10 = API 29) */
    #define XORON_MIN_ANDROID_API 29
    /* Android uses .so shared libraries */
    #define XORON_LIBRARY_EXTENSION ".so"
    
    /* Android architecture detection */
    #if defined(__aarch64__)
        #define XORON_ANDROID_ARM64 1
        #define XORON_ARCH_NAME "arm64-v8a"
    #elif defined(__arm__)
        #define XORON_ANDROID_ARM32 1
        #define XORON_ARCH_NAME "armeabi-v7a"
    #elif defined(__x86_64__)
        #define XORON_ANDROID_X86_64 1
        #define XORON_ARCH_NAME "x86_64"
    #elif defined(__i386__)
        #define XORON_ANDROID_X86 1
        #define XORON_ARCH_NAME "x86"
    #else
        #define XORON_ARCH_NAME "unknown"
    #endif
#endif

/* Unknown platform fallback */
#if !defined(XORON_PLATFORM_IOS) && !defined(XORON_PLATFORM_ANDROID)
    #define XORON_PLATFORM_UNKNOWN 1
    #define XORON_PLATFORM_NAME "Unknown"
    #define XORON_LIBRARY_EXTENSION ""
#endif

/* Mobile platform check (iOS or Android) */
#if defined(XORON_PLATFORM_IOS) || defined(XORON_PLATFORM_ANDROID)
    #define XORON_PLATFORM_MOBILE 1
#endif

/* ============== Platform-Specific Path Definitions ============== */
/*
 * iOS paths:
 *   Base: ~/Documents/Xoron/
 *   Workspace: ~/Documents/Xoron/workspace/
 *   Autoexecute: ~/Documents/Xoron/autoexecute/
 *   Scripts: ~/Documents/Xoron/scripts/
 *
 * Android paths:
 *   Base: /data/data/<package>/files/Xoron/ (internal)
 *      or /storage/emulated/0/Xoron/ (external, if available)
 *   Workspace: <base>/workspace/
 *   Autoexecute: <base>/autoexecute/
 *   Scripts: <base>/scripts/
 */
#if defined(XORON_PLATFORM_IOS)
    #define XORON_DEFAULT_BASE_PATH_SUFFIX "/Documents/Xoron"
#elif defined(XORON_PLATFORM_ANDROID)
    #define XORON_DEFAULT_BASE_PATH_SUFFIX "/files/Xoron"
    #define XORON_EXTERNAL_STORAGE_PATH "/storage/emulated/0/Xoron"
#else
    #define XORON_DEFAULT_BASE_PATH_SUFFIX "/Xoron"
#endif

/* ============== Export/Import Macros ============== */
#if defined(XORON_PLATFORM_ANDROID)
    #define XORON_API __attribute__((visibility("default")))
    #define XORON_LOCAL __attribute__((visibility("hidden")))
#elif defined(XORON_PLATFORM_IOS)
    #define XORON_API __attribute__((visibility("default")))
    #define XORON_LOCAL __attribute__((visibility("hidden")))
#else
    #define XORON_API
    #define XORON_LOCAL
#endif

/* ============== Logging Macros ============== */
#if defined(XORON_PLATFORM_ANDROID)
    #include <android/log.h>
    #define XORON_LOG(...) __android_log_print(ANDROID_LOG_INFO, "Xoron", __VA_ARGS__)
    #define CONSOLE_LOG(...) __android_log_print(ANDROID_LOG_INFO, "XoronConsole", __VA_ARGS__)
    #define CONSOLE_LOG_WARN(...) __android_log_print(ANDROID_LOG_WARN, "XoronConsole", __VA_ARGS__)
    #define CONSOLE_LOG_ERROR(...) __android_log_print(ANDROID_LOG_ERROR, "XoronConsole", __VA_ARGS__)
    #define ENV_LOG(...) __android_log_print(ANDROID_LOG_INFO, "XoronEnv", __VA_ARGS__)
    #define FS_LOG(...) __android_log_print(ANDROID_LOG_INFO, "XoronFS", __VA_ARGS__)
    #define MEM_LOG(...) __android_log_print(ANDROID_LOG_INFO, "XoronMem", __VA_ARGS__)
#elif defined(XORON_PLATFORM_IOS)
    #include <Foundation/Foundation.h>
    #define XORON_LOG(...) NSLog(@__VA_ARGS__)
    #define CONSOLE_LOG(...) NSLog(@__VA_ARGS__)
    #define CONSOLE_LOG_WARN(...) NSLog(@__VA_ARGS__)
    #define CONSOLE_LOG_ERROR(...) NSLog(@__VA_ARGS__)
    #define ENV_LOG(...) NSLog(@__VA_ARGS__)
    #define FS_LOG(...) NSLog(@__VA_ARGS__)
    #define MEM_LOG(...) NSLog(@__VA_ARGS__)
#else
    #include <stdio.h>
    #define XORON_LOG(...) printf(__VA_ARGS__); printf("\n")
    #define CONSOLE_LOG(...) printf(__VA_ARGS__); printf("\n")
    #define CONSOLE_LOG_WARN(...) printf("[WARN] "); printf(__VA_ARGS__); printf("\n")
    #define CONSOLE_LOG_ERROR(...) printf("[ERROR] "); printf(__VA_ARGS__); printf("\n")
    #define ENV_LOG(...) printf("[ENV] "); printf(__VA_ARGS__); printf("\n")
    #define FS_LOG(...) printf("[FS] "); printf(__VA_ARGS__); printf("\n")
    #define MEM_LOG(...) printf("[MEM] "); printf(__VA_ARGS__); printf("\n")
#endif

typedef enum {
    XORON_OK = 0,
    XORON_ERR_INIT = -1,
    XORON_ERR_MEMORY = -2,
    XORON_ERR_COMPILE = -3,
    XORON_ERR_RUNTIME = -4,
    XORON_ERR_HTTP = -5,
    XORON_ERR_INVALID = -6,
    XORON_ERR_IO = -7,
    XORON_ERR_WEBSOCKET = -8,
    XORON_ERR_SECURITY = -9
} xoron_error_t;

typedef struct xoron_vm xoron_vm_t;
typedef struct xoron_bytecode xoron_bytecode_t;
typedef void (*xoron_output_fn)(const char* msg, void* ud);

/* ============== Core API ============== */
int xoron_init(void);
void xoron_shutdown(void);
const char* xoron_version(void);
const char* xoron_last_error(void);
void xoron_set_output(xoron_output_fn print_fn, xoron_output_fn error_fn, void* ud);

/* ============== VM API ============== */
xoron_vm_t* xoron_vm_new(void);
void xoron_vm_free(xoron_vm_t* vm);
void xoron_vm_reset(xoron_vm_t* vm);

/* ============== Compilation API ============== */
xoron_bytecode_t* xoron_compile(const char* source, size_t len, const char* name);
xoron_bytecode_t* xoron_compile_file(const char* path);
void xoron_bytecode_free(xoron_bytecode_t* bc);
const char* xoron_bytecode_data(xoron_bytecode_t* bc, size_t* len);

/* ============== Execution API ============== */
int xoron_run(xoron_vm_t* vm, xoron_bytecode_t* bc);
int xoron_dostring(xoron_vm_t* vm, const char* source, const char* name);
int xoron_dofile(xoron_vm_t* vm, const char* path);

/* ============== HTTP API ============== */
char* xoron_http_get(const char* url, int* status, size_t* len);
char* xoron_http_post(const char* url, const char* body, size_t body_len, 
                      const char* content_type, int* status, size_t* len);
void xoron_http_free(char* response);

/* ============== Crypto API ============== */
void xoron_sha256(const void* data, size_t len, uint8_t out[32]);
void xoron_sha384(const void* data, size_t len, uint8_t out[48]);
void xoron_sha512(const void* data, size_t len, uint8_t out[64]);
void xoron_md5(const void* data, size_t len, uint8_t out[16]);
char* xoron_base64_encode(const void* data, size_t len);
uint8_t* xoron_base64_decode(const char* str, size_t* out_len);
char* xoron_hex_encode(const void* data, size_t len);
uint8_t* xoron_hex_decode(const char* str, size_t* out_len);
void xoron_free(void* ptr);

/* ============== Filesystem API ============== */
const char* xoron_get_workspace(void);
void xoron_set_workspace(const char* path);
const char* xoron_get_autoexecute_path(void);
const char* xoron_get_scripts_path(void);

/* ============== Security API ============== */
bool xoron_check_environment(void);
void xoron_enable_anti_detection(bool enable);

/* ============== Console API ============== */
void xoron_set_console_callbacks(xoron_output_fn print_fn, xoron_output_fn error_fn, void* ud);
void xoron_console_print(const char* text);
void xoron_console_warn(const char* text);
void xoron_console_error(const char* text);

#ifdef __cplusplus
}

/* C++ only declarations */
struct lua_State;

/* Library registration functions */
void xoron_register_env(lua_State* L);
void xoron_register_filesystem(lua_State* L);
void xoron_register_memory(lua_State* L);
void xoron_register_debug(lua_State* L);
void xoron_register_console(lua_State* L);
void xoron_register_drawing(lua_State* L);
void xoron_register_websocket(lua_State* L);
void xoron_register_http(lua_State* L);
void xoron_register_crypt(lua_State* L);
void xoron_register_input(lua_State* L);
void xoron_register_cache(lua_State* L);
void xoron_register_ui(lua_State* L);

/* Platform-specific registration (iOS only) */
#if defined(XORON_PLATFORM_IOS) || (defined(__APPLE__) && defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE)
void xoron_register_ios(lua_State* L);

/* iOS native UI functions */
void xoron_ios_ui_init(void);
void xoron_ios_ui_show(void);
void xoron_ios_ui_hide(void);
void xoron_ios_ui_toggle(void);
void xoron_ios_haptic_feedback(int style);
void xoron_ios_console_print(const char* message, int type);
void xoron_ios_set_lua_state(lua_State* L);

/* iOS drawing functions */
#if defined(__OBJC__) || defined(__cplusplus)
/* Forward declare CGContextRef for C++ */
typedef struct CGContext* CGContextRef;
#ifdef __cplusplus
extern "C" {
#endif
void xoron_drawing_render_ios(CGContextRef ctx);
#ifdef __cplusplus
}
#endif
#endif
#endif

/* Platform-specific registration (Android only) */
#if defined(XORON_PLATFORM_ANDROID) || defined(__ANDROID__)
/* Android native UI functions */
void xoron_android_haptic_feedback(int style);
void xoron_android_ui_show(void);
void xoron_android_ui_hide(void);
void xoron_android_ui_toggle(void);
void xoron_android_console_print(const char* message, int type);
void xoron_android_set_lua_state(lua_State* L);
lua_State* xoron_android_get_lua_state(void);

/* Android initialization */
void xoron_register_android(lua_State* L);
#endif

/* Global drawing functions - available on both iOS and Android */
#ifdef __cplusplus
extern "C" {
#endif
void xoron_drawing_set_screen_size(float width, float height);
#ifdef __cplusplus
}
#endif

#endif

#endif
