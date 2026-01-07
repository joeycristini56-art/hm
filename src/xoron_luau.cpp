/*
 * xoron_luau.cpp - Luau VM wrapper
 * Platforms: iOS (.dylib) and Android (.so)
 */

#include "xoron.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <string>
#include <mutex>
#include <fstream>
#include <sstream>

#include "lua.h"
#include "lualib.h"
#include "luacode.h"

#ifdef __ANDROID__
#include <android/log.h>
#include <jni.h>
#include <dlfcn.h>
#define XORON_LOG(...) __android_log_print(ANDROID_LOG_INFO, "Xoron", __VA_ARGS__)
#elif defined(XORON_PLATFORM_IOS)
#include <Foundation/Foundation.h>
#define XORON_LOG(...) NSLog(@__VA_ARGS__)
#else
#define XORON_LOG(...) printf(__VA_ARGS__)
#endif

static struct {
    bool initialized = false;
    std::mutex mutex;
    xoron_output_fn print_fn = nullptr;
    xoron_output_fn error_fn = nullptr;
    void* output_ud = nullptr;
    std::string last_error;
} g_state;

struct xoron_vm { lua_State* L; };
struct xoron_bytecode { std::string data; std::string name; };

void xoron_set_error(const char* fmt, ...) {
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    std::lock_guard<std::mutex> lock(g_state.mutex);
    g_state.last_error = buf;
    if (g_state.error_fn) g_state.error_fn(buf, g_state.output_ud);
}

static void* luau_alloc(void* ud, void* ptr, size_t osize, size_t nsize) {
    (void)ud; (void)osize;
    if (nsize == 0) { free(ptr); return nullptr; }
    return realloc(ptr, nsize);
}

static int luau_print(lua_State* L) {
    std::string output;
    int n = lua_gettop(L);
    for (int i = 1; i <= n; i++) {
        size_t len;
        const char* s = luaL_tolstring(L, i, &len);
        if (i > 1) output += "\t";
        if (s) output += s;
        lua_pop(L, 1);
    }
    if (g_state.print_fn) g_state.print_fn(output.c_str(), g_state.output_ud);
    else XORON_LOG("%s", output.c_str());
    return 0;
}

static int lua_http_get(lua_State* L) {
    const char* url = luaL_checkstring(L, 1);
    int status = 0; size_t len = 0;
    char* response = xoron_http_get(url, &status, &len);
    if (response) {
        lua_newtable(L);
        lua_pushinteger(L, status); lua_setfield(L, -2, "status");
        lua_pushlstring(L, response, len); lua_setfield(L, -2, "body");
        xoron_http_free(response);
        return 1;
    }
    lua_pushnil(L);
    lua_pushstring(L, xoron_last_error());
    return 2;
}

static int lua_http_post(lua_State* L) {
    const char* url = luaL_checkstring(L, 1);
    size_t body_len;
    const char* body = luaL_optlstring(L, 2, "", &body_len);
    const char* ct = luaL_optstring(L, 3, "application/json");
    int status = 0; size_t len = 0;
    char* response = xoron_http_post(url, body, body_len, ct, &status, &len);
    if (response) {
        lua_newtable(L);
        lua_pushinteger(L, status); lua_setfield(L, -2, "status");
        lua_pushlstring(L, response, len); lua_setfield(L, -2, "body");
        xoron_http_free(response);
        return 1;
    }
    lua_pushnil(L);
    lua_pushstring(L, xoron_last_error());
    return 2;
}

static int lua_sha256(lua_State* L) {
    size_t len;
    const char* data = luaL_checklstring(L, 1, &len);
    uint8_t hash[32];
    xoron_sha256(data, len, hash);
    char hex[65];
    for (int i = 0; i < 32; i++) snprintf(hex + i * 2, 3, "%02x", hash[i]);
    lua_pushlstring(L, hex, 64);
    return 1;
}

static int lua_sha384(lua_State* L) {
    size_t len;
    const char* data = luaL_checklstring(L, 1, &len);
    uint8_t hash[48];
    xoron_sha384(data, len, hash);
    char hex[97];
    for (int i = 0; i < 48; i++) snprintf(hex + i * 2, 3, "%02x", hash[i]);
    lua_pushlstring(L, hex, 96);
    return 1;
}

static int lua_sha512(lua_State* L) {
    size_t len;
    const char* data = luaL_checklstring(L, 1, &len);
    uint8_t hash[64];
    xoron_sha512(data, len, hash);
    char hex[129];
    for (int i = 0; i < 64; i++) snprintf(hex + i * 2, 3, "%02x", hash[i]);
    lua_pushlstring(L, hex, 128);
    return 1;
}

static int lua_md5(lua_State* L) {
    size_t len;
    const char* data = luaL_checklstring(L, 1, &len);
    uint8_t hash[16];
    xoron_md5(data, len, hash);
    char hex[33];
    for (int i = 0; i < 16; i++) snprintf(hex + i * 2, 3, "%02x", hash[i]);
    lua_pushlstring(L, hex, 32);
    return 1;
}

static int lua_base64_encode(lua_State* L) {
    size_t len;
    const char* data = luaL_checklstring(L, 1, &len);
    char* encoded = xoron_base64_encode(data, len);
    if (encoded) { lua_pushstring(L, encoded); xoron_free(encoded); return 1; }
    lua_pushnil(L);
    return 1;
}

static int lua_base64_decode(lua_State* L) {
    const char* str = luaL_checkstring(L, 1);
    size_t len;
    uint8_t* decoded = xoron_base64_decode(str, &len);
    if (decoded) { lua_pushlstring(L, (const char*)decoded, len); xoron_free(decoded); return 1; }
    lua_pushnil(L);
    return 1;
}

static int lua_hex_encode(lua_State* L) {
    size_t len;
    const char* data = luaL_checklstring(L, 1, &len);
    char* encoded = xoron_hex_encode(data, len);
    if (encoded) { lua_pushstring(L, encoded); xoron_free(encoded); return 1; }
    lua_pushnil(L);
    return 1;
}

static int lua_hex_decode(lua_State* L) {
    const char* str = luaL_checkstring(L, 1);
    size_t len;
    uint8_t* decoded = xoron_hex_decode(str, &len);
    if (decoded) { lua_pushlstring(L, (const char*)decoded, len); xoron_free(decoded); return 1; }
    lua_pushnil(L);
    return 1;
}

static void register_xoron_lib(lua_State* L) {
    // Main xoron table
    lua_newtable(L);
    lua_pushstring(L, XORON_VERSION); lua_setfield(L, -2, "version");
    lua_pushstring(L, XORON_NAME); lua_setfield(L, -2, "name");
    
    // HTTP subtable
    lua_newtable(L);
    lua_pushcfunction(L, lua_http_get, "http.get"); lua_setfield(L, -2, "get");
    lua_pushcfunction(L, lua_http_post, "http.post"); lua_setfield(L, -2, "post");
    lua_setfield(L, -2, "http");
    
    // Crypto subtable (basic, full crypt library registered separately)
    lua_newtable(L);
    lua_pushcfunction(L, lua_sha256, "crypto.sha256"); lua_setfield(L, -2, "sha256");
    lua_pushcfunction(L, lua_sha384, "crypto.sha384"); lua_setfield(L, -2, "sha384");
    lua_pushcfunction(L, lua_sha512, "crypto.sha512"); lua_setfield(L, -2, "sha512");
    lua_pushcfunction(L, lua_md5, "crypto.md5"); lua_setfield(L, -2, "md5");
    lua_pushcfunction(L, lua_base64_encode, "crypto.base64encode"); lua_setfield(L, -2, "base64encode");
    lua_pushcfunction(L, lua_base64_decode, "crypto.base64decode"); lua_setfield(L, -2, "base64decode");
    lua_pushcfunction(L, lua_hex_encode, "crypto.hexencode"); lua_setfield(L, -2, "hexencode");
    lua_pushcfunction(L, lua_hex_decode, "crypto.hexdecode"); lua_setfield(L, -2, "hexdecode");
    lua_setfield(L, -2, "crypto");
    
    lua_setglobal(L, "xoron");
    
    // Override print
    lua_pushcfunction(L, luau_print, "print"); lua_setglobal(L, "print");
    
    // Register all executor libraries
    xoron_register_env(L);
    xoron_register_filesystem(L);
    xoron_register_memory(L);
    xoron_register_debug(L);
    xoron_register_console(L);
    xoron_register_drawing(L);
    xoron_register_websocket(L);
    xoron_register_http(L);
    xoron_register_crypt(L);  // Full crypt library with AES, HMAC, etc.
    xoron_register_input(L);  // Input library
    xoron_register_cache(L);  // Cache library
    xoron_register_ui(L);     // UI library for executor menu
    
    // Platform-specific libraries
#if defined(XORON_PLATFORM_IOS) || (defined(__APPLE__) && defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE)
    xoron_register_ios(L);    // iOS-specific: clipboard, screen, haptics, etc.
#elif defined(XORON_PLATFORM_ANDROID) || defined(__ANDROID__)
    xoron_register_android(L); // Android-specific: native UI, haptics, etc.
#endif
    
    // Create syn table for compatibility
    lua_newtable(L);
    lua_pushcfunction(L, lua_http_get, "request"); lua_setfield(L, -2, "request");
    lua_setglobal(L, "syn");
    
    // Create request function (httpget style)
    lua_pushcfunction(L, lua_http_get, "request");
    lua_setglobal(L, "request");
    
    lua_pushcfunction(L, lua_http_get, "http_request");
    lua_setglobal(L, "http_request");
    
    lua_pushcfunction(L, lua_http_get, "httpget");
    lua_setglobal(L, "httpget");
    
    // game global - initialized as empty table
    // When injected into Roblox, this will be replaced with the actual game reference
    // Scripts can check if game is properly initialized by checking for GetService
    lua_newtable(L);
    lua_setglobal(L, "game");
}

extern "C" {

int xoron_init(void) {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    if (g_state.initialized) return XORON_OK;
    g_state.initialized = true;
    return XORON_OK;
}

void xoron_shutdown(void) {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    g_state.initialized = false;
    g_state.print_fn = nullptr;
    g_state.error_fn = nullptr;
    g_state.output_ud = nullptr;
}

const char* xoron_version(void) { return XORON_VERSION; }
const char* xoron_last_error(void) { return g_state.last_error.c_str(); }

void xoron_set_output(xoron_output_fn print_fn, xoron_output_fn error_fn, void* ud) {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    g_state.print_fn = print_fn;
    g_state.error_fn = error_fn;
    g_state.output_ud = ud;
}

xoron_vm_t* xoron_vm_new(void) {
    xoron_vm_t* vm = new (std::nothrow) xoron_vm_t;
    if (!vm) { xoron_set_error("Failed to allocate VM"); return nullptr; }
    vm->L = lua_newstate(luau_alloc, nullptr);
    if (!vm->L) { delete vm; xoron_set_error("Failed to create Lua state"); return nullptr; }
    luaL_openlibs(vm->L);
    register_xoron_lib(vm->L);
    return vm;
}

void xoron_vm_free(xoron_vm_t* vm) {
    if (vm) { if (vm->L) lua_close(vm->L); delete vm; }
}

void xoron_vm_reset(xoron_vm_t* vm) {
    if (!vm) return;
    if (vm->L) lua_close(vm->L);
    vm->L = lua_newstate(luau_alloc, nullptr);
    if (vm->L) { luaL_openlibs(vm->L); register_xoron_lib(vm->L); }
}

xoron_bytecode_t* xoron_compile(const char* source, size_t len, const char* name) {
    if (!source) { xoron_set_error("Source is null"); return nullptr; }
    if (len == 0) len = strlen(source);
    if (!name) name = "chunk";
    
    size_t bc_len = 0;
    char* bc = luau_compile(source, len, nullptr, &bc_len);
    if (!bc || bc_len == 0) { xoron_set_error("Compilation failed"); if (bc) free(bc); return nullptr; }
    
    xoron_bytecode_t* result = new (std::nothrow) xoron_bytecode_t;
    if (!result) { free(bc); xoron_set_error("Failed to allocate bytecode"); return nullptr; }
    result->data.assign(bc, bc_len);
    result->name = name;
    free(bc);
    return result;
}

xoron_bytecode_t* xoron_compile_file(const char* path) {
    if (!path) { xoron_set_error("Path is null"); return nullptr; }
    std::ifstream file(path, std::ios::binary);
    if (!file) { xoron_set_error("Failed to open file: %s", path); return nullptr; }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    std::string name = path;
    size_t pos = name.find_last_of("/\\");
    if (pos != std::string::npos) name = name.substr(pos + 1);
    return xoron_compile(source.c_str(), source.size(), name.c_str());
}

void xoron_bytecode_free(xoron_bytecode_t* bc) { delete bc; }

const char* xoron_bytecode_data(xoron_bytecode_t* bc, size_t* len) {
    if (!bc) return nullptr;
    if (len) *len = bc->data.size();
    return bc->data.c_str();
}

int xoron_run(xoron_vm_t* vm, xoron_bytecode_t* bc) {
    if (!vm || !vm->L || !bc) { xoron_set_error("Invalid arguments"); return XORON_ERR_INVALID; }
    int result = luau_load(vm->L, bc->name.c_str(), bc->data.c_str(), bc->data.size(), 0);
    if (result != 0) {
        const char* err = lua_tostring(vm->L, -1);
        xoron_set_error("Load error: %s", err ? err : "unknown");
        lua_pop(vm->L, 1);
        return XORON_ERR_RUNTIME;
    }
    result = lua_pcall(vm->L, 0, 0, 0);
    if (result != 0) {
        const char* err = lua_tostring(vm->L, -1);
        xoron_set_error("Runtime error: %s", err ? err : "unknown");
        lua_pop(vm->L, 1);
        return XORON_ERR_RUNTIME;
    }
    return XORON_OK;
}

int xoron_dostring(xoron_vm_t* vm, const char* source, const char* name) {
    xoron_bytecode_t* bc = xoron_compile(source, 0, name);
    if (!bc) return XORON_ERR_COMPILE;
    int result = xoron_run(vm, bc);
    xoron_bytecode_free(bc);
    return result;
}

int xoron_dofile(xoron_vm_t* vm, const char* path) {
    xoron_bytecode_t* bc = xoron_compile_file(path);
    if (!bc) return XORON_ERR_COMPILE;
    int result = xoron_run(vm, bc);
    xoron_bytecode_free(bc);
    return result;
}

} // extern "C"

// ============================================================================
// Android JNI Entry Point - Called when library is loaded via System.loadLibrary
// ============================================================================
#ifdef __ANDROID__

static JavaVM* g_jvm = nullptr;
static xoron_vm_t* g_default_vm = nullptr;

extern "C" {

// JNI_OnLoad - Called automatically when the library is loaded
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    (void)reserved;
    g_jvm = vm;
    
    XORON_LOG("Xoron v%s loaded!", XORON_VERSION);
    
    // Initialize xoron
    xoron_init();
    
    // Create default VM
    g_default_vm = xoron_vm_new();
    if (g_default_vm) {
        XORON_LOG("Xoron VM initialized successfully");
    } else {
        XORON_LOG("Failed to create Xoron VM");
    }
    
    return JNI_VERSION_1_6;
}

// JNI_OnUnload - Called when library is unloaded
JNIEXPORT void JNICALL JNI_OnUnload(JavaVM* vm, void* reserved) {
    (void)vm;
    (void)reserved;
    
    XORON_LOG("Xoron unloading...");
    
    if (g_default_vm) {
        xoron_vm_free(g_default_vm);
        g_default_vm = nullptr;
    }
    
    xoron_shutdown();
    g_jvm = nullptr;
}

// Execute Lua script from Java
JNIEXPORT jint JNICALL Java_com_xoron_Executor_execute(JNIEnv* env, jobject obj, jstring script) {
    (void)obj;
    
    if (!g_default_vm) {
        XORON_LOG("VM not initialized");
        return -1;
    }
    
    const char* script_str = env->GetStringUTFChars(script, nullptr);
    if (!script_str) {
        XORON_LOG("Failed to get script string");
        return -1;
    }
    
    XORON_LOG("Executing script...");
    int result = xoron_dostring(g_default_vm, script_str, "script");
    
    env->ReleaseStringUTFChars(script, script_str);
    
    if (result != XORON_OK) {
        XORON_LOG("Script error: %s", xoron_last_error());
    }
    
    return result;
}

// Get Xoron version from Java
JNIEXPORT jstring JNICALL Java_com_xoron_Executor_getVersion(JNIEnv* env, jobject obj) {
    (void)obj;
    return env->NewStringUTF(XORON_VERSION);
}

// Get last error from Java
JNIEXPORT jstring JNICALL Java_com_xoron_Executor_getLastError(JNIEnv* env, jobject obj) {
    (void)obj;
    return env->NewStringUTF(xoron_last_error());
}

// Get default VM for native code
xoron_vm_t* xoron_get_default_vm(void) {
    return g_default_vm;
}

} // extern "C"

#endif // __ANDROID__

// ============================================================================
// iOS Entry Point - Constructor attribute for automatic initialization
// ============================================================================
#if defined(__APPLE__) && defined(TARGET_OS_IPHONE)

__attribute__((constructor))
static void xoron_ios_init(void) {
    XORON_LOG("Xoron v%s loaded on iOS!\n", XORON_VERSION);
    xoron_init();
}

__attribute__((destructor))
static void xoron_ios_cleanup(void) {
    XORON_LOG("Xoron unloading from iOS...\n");
    xoron_shutdown();
}

#endif // iOS
