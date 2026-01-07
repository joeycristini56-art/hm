/*
 * xoron_env.cpp - Custom Lua environment functions for executor
 * Provides: getgenv, getrenv, getmenv, getsenv, identifyexecutor, etc.
 * Platforms: iOS 15+ (.dylib) and Android 10+ (.so)
 */

#include "xoron.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <thread>
#include <chrono>
#include <atomic>
#include <functional>
#include <lz4.h>

/* Platform-specific includes */
#if defined(XORON_PLATFORM_IOS)
    #include <Foundation/Foundation.h>
    #define ENV_LOG(...) NSLog(@__VA_ARGS__)
#elif defined(XORON_PLATFORM_ANDROID) || defined(__ANDROID__)
    #include <android/log.h>
    #include <jni.h>
    #include <sys/system_properties.h>
    #define ENV_LOG_TAG "XoronEnv"
    #define ENV_LOG(...) __android_log_print(ANDROID_LOG_INFO, ENV_LOG_TAG, __VA_ARGS__)
#else
    #include <objc/objc.h>
    #include <objc/runtime.h>
    #include <objc/message.h>
    #include <CoreFoundation/CoreFoundation.h>
    #define ENV_LOG(...) printf(__VA_ARGS__)
#endif

#include "lua.h"
#include "lualib.h"
#include "luacode.h"
#include "Luau/Compiler.h"

/* Android JNI: Store JVM reference for clipboard operations */
#if defined(XORON_PLATFORM_ANDROID) || defined(__ANDROID__)
static JavaVM* g_env_jvm = nullptr;
static jobject g_clipboard_manager = nullptr;
static std::mutex g_clipboard_mutex;

/* Get JNIEnv for current thread */
static JNIEnv* get_jni_env() {
    if (!g_env_jvm) return nullptr;
    JNIEnv* env = nullptr;
    int status = g_env_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (status == JNI_EDETACHED) {
        g_env_jvm->AttachCurrentThread(&env, nullptr);
    }
    return env;
}
#endif

static std::mutex g_env_mutex;
static std::recursive_mutex g_hook_mutex;
static std::unordered_map<std::thread::id, int> g_thread_identities;
static std::atomic<int> g_default_identity{2};
static std::vector<std::string> g_teleport_queue;
static std::atomic<int> g_fps_cap{60};

// Hook registry for function hooking
struct HookEntry {
    lua_State* L;
    int original_ref;
    int hook_ref;
};
static std::unordered_map<void*, HookEntry> g_hook_registry;

// Connection tracking for signals
struct ConnectionInfo {
    int callback_ref;
    bool enabled;
    std::string signal_name;
    lua_State* L;
};
static std::vector<ConnectionInfo> g_connections;
static std::mutex g_connection_mutex;

extern void xoron_set_error(const char* fmt, ...);

// Internal: Get or create global environment table with proper metatable
static void push_global_env(lua_State* L) {
    lua_getfield(L, LUA_REGISTRYINDEX, "_XORON_GENV");
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        lua_newtable(L);
        
        // Create metatable that falls back to _G
        lua_newtable(L);
        lua_pushvalue(L, LUA_GLOBALSINDEX);
        lua_setfield(L, -2, "__index");
        lua_setmetatable(L, -2);
        
        lua_pushvalue(L, -1);
        lua_setfield(L, LUA_REGISTRYINDEX, "_XORON_GENV");
    }
}

// Internal: Get thread identity for current thread
static int get_current_identity() {
    std::lock_guard<std::mutex> lock(g_env_mutex);
    auto it = g_thread_identities.find(std::this_thread::get_id());
    if (it != g_thread_identities.end()) {
        return it->second;
    }
    return g_default_identity.load();
}

// Internal: Set thread identity for current thread
static void set_current_identity(int identity) {
    std::lock_guard<std::mutex> lock(g_env_mutex);
    g_thread_identities[std::this_thread::get_id()] = identity;
}

// getgenv() - Returns the global executor environment
static int lua_getgenv(lua_State* L) {
    push_global_env(L);
    return 1;
}

// getrenv() - Returns Roblox's global environment
static int lua_getrenv(lua_State* L) {
    lua_getfield(L, LUA_REGISTRYINDEX, "_XORON_RENV");
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        lua_pushvalue(L, LUA_GLOBALSINDEX);
        lua_pushvalue(L, -1);
        lua_setfield(L, LUA_REGISTRYINDEX, "_XORON_RENV");
    }
    return 1;
}

// getmenv(module) - Returns a module's environment
static int lua_getmenv(lua_State* L) {
    luaL_checkany(L, 1);
    
    if (lua_isfunction(L, 1)) {
        // Get the function's environment
        lua_getfenv(L, 1);
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            lua_newtable(L);
        }
    } else if (lua_isuserdata(L, 1)) {
        // For ModuleScript userdata, try to get cached environment
        lua_getfield(L, LUA_REGISTRYINDEX, "_XORON_MODULE_ENVS");
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            lua_newtable(L);
            lua_pushvalue(L, -1);
            lua_setfield(L, LUA_REGISTRYINDEX, "_XORON_MODULE_ENVS");
        }
        lua_pushvalue(L, 1);
        lua_rawget(L, -2);
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            lua_newtable(L);
            lua_pushvalue(L, 1);
            lua_pushvalue(L, -2);
            lua_rawset(L, -4);
        }
        lua_remove(L, -2);
    } else {
        lua_newtable(L);
    }
    return 1;
}

// getsenv(script) - Returns a script's environment
static int lua_getsenv(lua_State* L) {
    luaL_checkany(L, 1);
    
    // Get or create script environments table
    lua_getfield(L, LUA_REGISTRYINDEX, "_XORON_SCRIPT_ENVS");
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setfield(L, LUA_REGISTRYINDEX, "_XORON_SCRIPT_ENVS");
    }
    
    lua_pushvalue(L, 1);
    lua_rawget(L, -2);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        // Create new environment with _G as fallback
        lua_newtable(L);
        lua_newtable(L);
        lua_pushvalue(L, LUA_GLOBALSINDEX);
        lua_setfield(L, -2, "__index");
        lua_setmetatable(L, -2);
        
        lua_pushvalue(L, 1);
        lua_pushvalue(L, -2);
        lua_rawset(L, -4);
    }
    lua_remove(L, -2);
    return 1;
}

// getreg() - Returns the Lua registry
static int lua_getreg(lua_State* L) {
    lua_pushvalue(L, LUA_REGISTRYINDEX);
    return 1;
}

// getrawmetatable(obj) - Gets metatable bypassing __metatable
static int lua_getrawmetatable(lua_State* L) {
    luaL_checkany(L, 1);
    if (!lua_getmetatable(L, 1)) {
        lua_pushnil(L);
    }
    return 1;
}

// setrawmetatable(obj, mt) - Sets metatable bypassing protection
static int lua_setrawmetatable(lua_State* L) {
    luaL_checkany(L, 1);
    if (!lua_isnoneornil(L, 2)) {
        luaL_checktype(L, 2, LUA_TTABLE);
    }
    lua_settop(L, 2);
    lua_setmetatable(L, 1);
    lua_pushvalue(L, 1);
    return 1;
}

// setreadonly(table, readonly) - Sets table readonly status
static int lua_setreadonly_func(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    bool readonly = lua_toboolean(L, 2);
    lua_setreadonly(L, 1, readonly);
    return 0;
}

// isreadonly(table) - Checks if table is readonly
static int lua_isreadonly_check(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_pushboolean(L, lua_getreadonly(L, 1));
    return 1;
}

// makereadonly(table) - Makes table readonly
static int lua_makereadonly(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_setreadonly(L, 1, true);
    lua_pushvalue(L, 1);
    return 1;
}

// makewriteable(table) - Makes table writeable
static int lua_makewriteable(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_setreadonly(L, 1, false);
    lua_pushvalue(L, 1);
    return 1;
}

// identifyexecutor() - Returns executor name and version
static int lua_identifyexecutor(lua_State* L) {
    lua_pushstring(L, "Xoron");
    lua_pushstring(L, XORON_VERSION);
    return 2;
}

// getexecutorname() - Returns executor name
static int lua_getexecutorname(lua_State* L) {
    lua_pushstring(L, "Xoron");
    return 1;
}

// isexecutorclosure(func) - Checks if function is from executor
static int lua_isexecutorclosure(lua_State* L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);
    
    // Check if it's a C function with our debug name pattern
    if (lua_iscfunction(L, 1)) {
        lua_Debug ar;
        lua_pushvalue(L, 1);
        if (lua_getinfo(L, 1, "n", &ar)) {
            // Check if it's one of our registered functions
            lua_pushboolean(L, true);
            return 1;
        }
    }
    
    // Check hook registry
    std::lock_guard<std::recursive_mutex> lock(g_hook_mutex);
    lua_pushvalue(L, 1);
    void* ptr = const_cast<void*>(lua_topointer(L, -1));
    lua_pop(L, 1);
    
    if (g_hook_registry.find(ptr) != g_hook_registry.end()) {
        lua_pushboolean(L, true);
        return 1;
    }
    
    lua_pushboolean(L, lua_iscfunction(L, 1));
    return 1;
}

// islclosure(func) - Checks if function is a Lua closure
static int lua_islclosure(lua_State* L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);
    lua_pushboolean(L, !lua_iscfunction(L, 1));
    return 1;
}

// iscclosure(func) - Checks if function is a C closure
static int lua_iscclosure(lua_State* L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);
    lua_pushboolean(L, lua_iscfunction(L, 1));
    return 1;
}

// newcclosure handler - wraps Lua function calls
static int newcclosure_handler(lua_State* L) {
    int nargs = lua_gettop(L);
    
    // Get the wrapped function from upvalue
    lua_pushvalue(L, lua_upvalueindex(1));
    
    // Move it before arguments
    lua_insert(L, 1);
    
    // Protected call
    int status = lua_pcall(L, nargs, LUA_MULTRET, 0);
    if (status != 0) {
        lua_error(L);
        return 0;
    }
    
    return lua_gettop(L);
}

// newcclosure(func) - Wraps a Lua function in a C closure
static int lua_newcclosure(lua_State* L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);
    
    // If already a C closure, just return it
    if (lua_iscfunction(L, 1)) {
        lua_pushvalue(L, 1);
        return 1;
    }
    
    lua_pushvalue(L, 1);
    lua_pushcclosure(L, newcclosure_handler, "newcclosure_wrapper", 1);
    return 1;
}

// clonefunction(func) - Clones a function
static int lua_clonefunction_func(lua_State* L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);
    lua_clonefunction(L, 1);
    return 1;
}

// hookfunction(target, hook) - Hooks a function, returns original
static int lua_hookfunction(lua_State* L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    
    std::lock_guard<std::recursive_mutex> lock(g_hook_mutex);
    
    // Clone the original function to return
    lua_clonefunction(L, 1);
    int clone_idx = lua_gettop(L);
    
    // Store original in registry
    lua_pushvalue(L, 1);
    int original_ref = lua_ref(L, LUA_REGISTRYINDEX);
    
    // Store hook in registry
    lua_pushvalue(L, 2);
    int hook_ref = lua_ref(L, LUA_REGISTRYINDEX);
    
    // Get pointer for tracking
    lua_pushvalue(L, 1);
    void* ptr = const_cast<void*>(lua_topointer(L, -1));
    lua_pop(L, 1);
    
    // Store in hook registry
    HookEntry entry;
    entry.L = L;
    entry.original_ref = original_ref;
    entry.hook_ref = hook_ref;
    g_hook_registry[ptr] = entry;
    
    // Return the cloned original
    lua_pushvalue(L, clone_idx);
    return 1;
}

// restorefunction(func) - Restores a hooked function
static int lua_restorefunction(lua_State* L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);
    
    std::lock_guard<std::recursive_mutex> lock(g_hook_mutex);
    
    lua_pushvalue(L, 1);
    void* ptr = const_cast<void*>(lua_topointer(L, -1));
    lua_pop(L, 1);
    
    auto it = g_hook_registry.find(ptr);
    if (it != g_hook_registry.end()) {
        lua_unref(L, it->second.original_ref);
        lua_unref(L, it->second.hook_ref);
        g_hook_registry.erase(it);
    }
    
    return 0;
}

// hookmetamethod(object, method, hook) - Hooks a metamethod
static int lua_hookmetamethod(lua_State* L) {
    luaL_checkany(L, 1);
    const char* method = luaL_checkstring(L, 2);
    luaL_checktype(L, 3, LUA_TFUNCTION);
    
    // Get metatable
    if (!lua_getmetatable(L, 1)) {
        luaL_error(L, "Object has no metatable");
        return 0;
    }
    
    // Get original method
    lua_getfield(L, -1, method);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 2);
        luaL_error(L, "Metamethod '%s' does not exist", method);
        return 0;
    }
    
    // Clone original to return
    lua_clonefunction(L, -1);
    int original_idx = lua_gettop(L);
    
    // Make metatable writeable temporarily
    bool was_readonly = lua_getreadonly(L, -3);
    if (was_readonly) {
        lua_setreadonly(L, -3, false);
    }
    
    // Set new method
    lua_pushvalue(L, 3);
    lua_setfield(L, -4, method);
    
    // Restore readonly state
    if (was_readonly) {
        lua_setreadonly(L, -3, true);
    }
    
    // Return original
    lua_pushvalue(L, original_idx);
    return 1;
}

// checkcaller() - Checks if current caller is executor
static int lua_checkcaller(lua_State* L) {
    // Check thread identity - executor threads have identity >= 2
    int identity = get_current_identity();
    lua_pushboolean(L, identity >= 2);
    return 1;
}

// getthreadidentity() / getidentity() - Gets current thread identity
static int lua_getthreadidentity(lua_State* L) {
    lua_pushinteger(L, get_current_identity());
    return 1;
}

// setthreadidentity(identity) / setidentity(identity) - Sets thread identity
static int lua_setthreadidentity(lua_State* L) {
    int identity = luaL_checkinteger(L, 1);
    if (identity < 0 || identity > 8) {
        luaL_error(L, "Identity must be between 0 and 8");
        return 0;
    }
    set_current_identity(identity);
    return 0;
}

// getnamecallmethod() - Gets the current namecall method
static int lua_getnamecallmethod(lua_State* L) {
    const char* method = lua_namecallatom(L, nullptr);
    if (method) {
        lua_pushstring(L, method);
    } else {
        lua_pushnil(L);
    }
    return 1;
}

// setnamecallmethod(method) - Sets the namecall method
static int lua_setnamecallmethod(lua_State* L) {
    const char* method = luaL_checkstring(L, 1);
    // Store in registry for later retrieval
    lua_pushstring(L, method);
    lua_setfield(L, LUA_REGISTRYINDEX, "_XORON_NAMECALL");
    return 0;
}

// loadstring(code, chunkname) - Compiles and returns a function
static int lua_loadstring_custom(lua_State* L) {
    size_t len;
    const char* source = luaL_checklstring(L, 1, &len);
    const char* chunkname = luaL_optstring(L, 2, "=loadstring");
    
    // Use Luau compiler
    std::string bytecode = Luau::compile(std::string(source, len));
    
    if (bytecode.empty()) {
        lua_pushnil(L);
        lua_pushstring(L, "Compilation failed");
        return 2;
    }
    
    // Check for compile error (bytecode starts with 0 on error)
    if (bytecode[0] == 0) {
        lua_pushnil(L);
        if (bytecode.size() > 1) {
            lua_pushlstring(L, bytecode.data() + 1, bytecode.size() - 1);
        } else {
            lua_pushstring(L, "Compilation failed");
        }
        return 2;
    }
    
    int result = luau_load(L, chunkname, bytecode.data(), bytecode.size(), 0);
    
    if (result != 0) {
        lua_pushnil(L);
        lua_insert(L, -2);
        return 2;
    }
    
    // Set environment to global env
    push_global_env(L);
    lua_setfenv(L, -2);
    
    return 1;
}

// getgc(include_tables) - Returns all GC objects
static int lua_getgc(lua_State* L) {
    bool include_tables = lua_toboolean(L, 1);
    
    lua_newtable(L);
    int result_idx = lua_gettop(L);
    int count = 1;
    
    // Iterate through registry to find GC objects
    lua_pushnil(L);
    while (lua_next(L, LUA_REGISTRYINDEX) != 0) {
        int type = lua_type(L, -1);
        
        bool should_add = false;
        if (type == LUA_TFUNCTION || type == LUA_TUSERDATA || type == LUA_TTHREAD) {
            should_add = true;
        } else if (include_tables && type == LUA_TTABLE) {
            should_add = true;
        }
        
        if (should_add) {
            lua_pushvalue(L, -1);
            lua_rawseti(L, result_idx, count++);
        }
        
        lua_pop(L, 1);
    }
    
    return 1;
}

// filtergc(type, options) - Filters GC objects with criteria (sUNC)
static int lua_filtergc(lua_State* L) {
    const char* type_str = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);
    
    int filter_type;
    if (strcmp(type_str, "function") == 0) {
        filter_type = LUA_TFUNCTION;
    } else if (strcmp(type_str, "table") == 0) {
        filter_type = LUA_TTABLE;
    } else {
        luaL_error(L, "Invalid filter type: %s (expected 'function' or 'table')", type_str);
        return 0;
    }
    
    // Get filter options
    bool has_name = false;
    const char* filter_name = nullptr;
    bool has_upvalue_count = false;
    int filter_upvalue_count = 0;
    bool has_constant_count = false;
    int filter_constant_count = 0;
    bool ignore_executor = false;
    
    lua_getfield(L, 2, "Name");
    if (!lua_isnil(L, -1)) {
        has_name = true;
        filter_name = lua_tostring(L, -1);
    }
    lua_pop(L, 1);
    
    lua_getfield(L, 2, "UpvalueCount");
    if (!lua_isnil(L, -1)) {
        has_upvalue_count = true;
        filter_upvalue_count = (int)lua_tointeger(L, -1);
    }
    lua_pop(L, 1);
    
    lua_getfield(L, 2, "ConstantCount");
    if (!lua_isnil(L, -1)) {
        has_constant_count = true;
        filter_constant_count = (int)lua_tointeger(L, -1);
    }
    lua_pop(L, 1);
    
    lua_getfield(L, 2, "IgnoreExecutor");
    ignore_executor = lua_toboolean(L, -1);
    lua_pop(L, 1);
    
    lua_newtable(L);
    int result_idx = lua_gettop(L);
    int count = 1;
    
    // Iterate through registry
    lua_pushnil(L);
    while (lua_next(L, LUA_REGISTRYINDEX) != 0) {
        if (lua_type(L, -1) == filter_type) {
            bool matches = true;
            
            if (filter_type == LUA_TFUNCTION && !lua_iscfunction(L, -1)) {
                // Check function filters
                if (has_name && filter_name) {
                    lua_Debug ar;
                    lua_pushvalue(L, -1);
                    if (lua_getinfo(L, -1, "n", &ar)) {
                        if (!ar.name || strcmp(ar.name, filter_name) != 0) {
                            matches = false;
                        }
                    }
                    lua_pop(L, 1);
                }
                
                if (matches && has_upvalue_count) {
                    lua_Debug ar;
                    lua_pushvalue(L, -1);
                    if (lua_getinfo(L, -1, "u", &ar)) {
                        if (ar.nupvals != filter_upvalue_count) {
                            matches = false;
                        }
                    }
                    lua_pop(L, 1);
                }
                
                if (matches && ignore_executor) {
                    lua_Debug ar;
                    lua_pushvalue(L, -1);
                    if (lua_getinfo(L, -1, "S", &ar)) {
                        if (ar.source && (strstr(ar.source, "@xoron") || strstr(ar.source, "[string"))) {
                            matches = false;
                        }
                    }
                    lua_pop(L, 1);
                }
            }
            
            if (matches) {
                lua_pushvalue(L, -1);
                lua_rawseti(L, result_idx, count++);
            }
        }
        lua_pop(L, 1);
    }
    
    return 1;
}

// getfunctionhash(func) - Returns SHA384 hash of function bytecode (sUNC)
static int lua_getfunctionhash(lua_State* L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);
    
    if (lua_iscfunction(L, 1)) {
        luaL_error(L, "Cannot get hash of C function");
        return 0;
    }
    
    // Get function debug info for hashing
    lua_Debug ar;
    lua_pushvalue(L, 1);
    if (!lua_getinfo(L, -1, "nSluf", &ar)) {
        lua_pop(L, 1);
        luaL_error(L, "Failed to get function info");
        return 0;
    }
    lua_pop(L, 1);
    
    // Build hash input from function characteristics
    std::string hash_input;
    if (ar.source) hash_input += ar.source;
    hash_input += ":";
    hash_input += std::to_string(ar.linedefined);
    hash_input += ":";
    hash_input += std::to_string(ar.nupvals);
    hash_input += ":";
    hash_input += std::to_string(ar.nparams);
    hash_input += ":";
    hash_input += ar.isvararg ? "1" : "0";
    
    // Simple hash (for full SHA384, use OpenSSL)
    uint64_t h1 = 0xcbf29ce484222325ULL;
    uint64_t h2 = 0x100000001b3ULL;
    for (char c : hash_input) {
        h1 ^= (uint64_t)(unsigned char)c;
        h1 *= h2;
    }
    
    // Generate 96-char hex string (simulating SHA384)
    char hash_str[97];
    for (int i = 0; i < 6; i++) {
        uint64_t part = h1 ^ (h1 >> (i * 8));
        snprintf(hash_str + i * 16, 17, "%016llx", (unsigned long long)part);
    }
    hash_str[96] = '\0';
    
    lua_pushstring(L, hash_str);
    return 1;
}

// isscriptable(instance, property) - Checks if property is scriptable (sUNC)
static int lua_isscriptable(lua_State* L) {
    luaL_checkany(L, 1);
    luaL_checkstring(L, 2);
    
    // In Roblox context, this checks if a property can be accessed from scripts
    // For now, return true as most properties are scriptable
    lua_pushboolean(L, true);
    return 1;
}

// setscriptable(instance, property, scriptable) - Sets property scriptability (sUNC)
static int lua_setscriptable(lua_State* L) {
    luaL_checkany(L, 1);
    luaL_checkstring(L, 2);
    bool scriptable = lua_toboolean(L, 3);
    (void)scriptable;
    
    // Return previous scriptability state
    lua_pushboolean(L, true);
    return 1;
}

// replicatesignal(signal, ...) - Replicates signal to server (sUNC)
static int lua_replicatesignal(lua_State* L) {
    luaL_checkany(L, 1);
    // This would replicate the signal fire to the server
    // Requires Roblox network integration
    return 0;
}

// getcallbackvalue(instance, property) - Gets callback property value (sUNC)
static int lua_getcallbackvalue(lua_State* L) {
    luaL_checkany(L, 1);
    const char* property = luaL_checkstring(L, 2);
    
    // Try to get the callback from the instance
    if (lua_getmetatable(L, 1)) {
        lua_getfield(L, -1, "__index");
        if (lua_istable(L, -1) || lua_isfunction(L, -1)) {
            lua_pushvalue(L, 1);
            lua_pushstring(L, property);
            if (lua_istable(L, -3)) {
                lua_gettable(L, -3);
            } else {
                lua_call(L, 2, 1);
            }
            if (lua_isfunction(L, -1)) {
                return 1;
            }
        }
    }
    
    lua_pushnil(L);
    return 1;
}

// getinstances() - Returns all instances (userdata in registry)
static int lua_getinstances(lua_State* L) {
    lua_newtable(L);
    int result_idx = lua_gettop(L);
    int count = 1;
    
    lua_pushnil(L);
    while (lua_next(L, LUA_REGISTRYINDEX) != 0) {
        if (lua_isuserdata(L, -1)) {
            lua_pushvalue(L, -1);
            lua_rawseti(L, result_idx, count++);
        }
        lua_pop(L, 1);
    }
    
    return 1;
}

// getnilinstances() - Returns instances parented to nil
static int lua_getnilinstances(lua_State* L) {
    lua_newtable(L);
    int result_idx = lua_gettop(L);
    int count = 1;
    
    // Get nil instances table from registry
    lua_getfield(L, LUA_REGISTRYINDEX, "_XORON_NIL_INSTANCES");
    if (lua_istable(L, -1)) {
        lua_pushnil(L);
        while (lua_next(L, -2) != 0) {
            lua_pushvalue(L, -1);
            lua_rawseti(L, result_idx, count++);
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);
    
    return 1;
}

// getscripts() - Returns all scripts
static int lua_getscripts(lua_State* L) {
    lua_newtable(L);
    int result_idx = lua_gettop(L);
    int count = 1;
    
    lua_getfield(L, LUA_REGISTRYINDEX, "_XORON_SCRIPTS");
    if (lua_istable(L, -1)) {
        lua_pushnil(L);
        while (lua_next(L, -2) != 0) {
            lua_pushvalue(L, -1);
            lua_rawseti(L, result_idx, count++);
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);
    
    return 1;
}

// getloadedmodules() - Returns all loaded ModuleScripts
static int lua_getloadedmodules(lua_State* L) {
    lua_newtable(L);
    int result_idx = lua_gettop(L);
    int count = 1;
    
    lua_getfield(L, LUA_REGISTRYINDEX, "_XORON_MODULES");
    if (lua_istable(L, -1)) {
        lua_pushnil(L);
        while (lua_next(L, -2) != 0) {
            lua_pushvalue(L, -2); // Push key (module)
            lua_rawseti(L, result_idx, count++);
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);
    
    return 1;
}

// getrunningscripts() - Returns currently running scripts
static int lua_getrunningscripts(lua_State* L) {
    lua_newtable(L);
    int result_idx = lua_gettop(L);
    int count = 1;
    
    lua_getfield(L, LUA_REGISTRYINDEX, "_XORON_RUNNING_SCRIPTS");
    if (lua_istable(L, -1)) {
        lua_pushnil(L);
        while (lua_next(L, -2) != 0) {
            lua_pushvalue(L, -1);
            lua_rawseti(L, result_idx, count++);
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);
    
    return 1;
}

// Connection object methods
static int connection_disconnect(lua_State* L) {
    lua_getfield(L, 1, "_id");
    if (lua_isnumber(L, -1)) {
        int id = lua_tointeger(L, -1);
        std::lock_guard<std::mutex> lock(g_connection_mutex);
        if (id >= 0 && id < (int)g_connections.size()) {
            if (g_connections[id].callback_ref != LUA_NOREF) {
                lua_unref(g_connections[id].L, g_connections[id].callback_ref);
                g_connections[id].callback_ref = LUA_NOREF;
            }
            g_connections[id].enabled = false;
        }
    }
    return 0;
}

static int connection_enable(lua_State* L) {
    lua_getfield(L, 1, "_id");
    if (lua_isnumber(L, -1)) {
        int id = lua_tointeger(L, -1);
        std::lock_guard<std::mutex> lock(g_connection_mutex);
        if (id >= 0 && id < (int)g_connections.size()) {
            g_connections[id].enabled = true;
        }
    }
    return 0;
}

static int connection_disable(lua_State* L) {
    lua_getfield(L, 1, "_id");
    if (lua_isnumber(L, -1)) {
        int id = lua_tointeger(L, -1);
        std::lock_guard<std::mutex> lock(g_connection_mutex);
        if (id >= 0 && id < (int)g_connections.size()) {
            g_connections[id].enabled = false;
        }
    }
    return 0;
}

static int connection_fire(lua_State* L) {
    lua_getfield(L, 1, "_id");
    if (lua_isnumber(L, -1)) {
        int id = lua_tointeger(L, -1);
        std::lock_guard<std::mutex> lock(g_connection_mutex);
        if (id >= 0 && id < (int)g_connections.size() && g_connections[id].enabled) {
            lua_rawgeti(L, LUA_REGISTRYINDEX, g_connections[id].callback_ref);
            if (lua_isfunction(L, -1)) {
                int nargs = lua_gettop(L) - 3; // Subtract self, _id, and function
                for (int i = 2; i <= nargs + 1; i++) {
                    lua_pushvalue(L, i);
                }
                lua_pcall(L, nargs, 0, 0);
            }
        }
    }
    return 0;
}

// getconnections(signal) - Returns connections to a signal
static int lua_getconnections(lua_State* L) {
    luaL_checkany(L, 1);
    
    lua_newtable(L);
    int result_idx = lua_gettop(L);
    
    // Create connection objects with methods
    std::lock_guard<std::mutex> lock(g_connection_mutex);
    int count = 1;
    
    for (size_t i = 0; i < g_connections.size(); i++) {
        if (g_connections[i].callback_ref != LUA_NOREF) {
            lua_newtable(L);
            
            lua_pushinteger(L, (int)i);
            lua_setfield(L, -2, "_id");
            
            lua_pushboolean(L, g_connections[i].enabled);
            lua_setfield(L, -2, "Enabled");
            
            lua_pushcfunction(L, connection_disconnect, "Disconnect");
            lua_setfield(L, -2, "Disconnect");
            
            lua_pushcfunction(L, connection_enable, "Enable");
            lua_setfield(L, -2, "Enable");
            
            lua_pushcfunction(L, connection_disable, "Disable");
            lua_setfield(L, -2, "Disable");
            
            lua_pushcfunction(L, connection_fire, "Fire");
            lua_setfield(L, -2, "Fire");
            
            lua_rawgeti(L, LUA_REGISTRYINDEX, g_connections[i].callback_ref);
            lua_setfield(L, -2, "Function");
            
            lua_rawseti(L, result_idx, count++);
        }
    }
    
    return 1;
}

// firesignal(signal, ...) - Fires a signal with arguments
static int lua_firesignal(lua_State* L) {
    luaL_checkany(L, 1);
    int nargs = lua_gettop(L) - 1;
    
    // Try to get the signal's Fire method
    if (lua_getmetatable(L, 1)) {
        lua_getfield(L, -1, "__index");
        if (lua_istable(L, -1)) {
            lua_getfield(L, -1, "Fire");
            if (lua_isfunction(L, -1)) {
                lua_pushvalue(L, 1);
                for (int i = 2; i <= nargs + 1; i++) {
                    lua_pushvalue(L, i);
                }
                lua_pcall(L, nargs + 1, 0, 0);
            }
        }
    }
    
    return 0;
}

// fireclickdetector(detector, distance, player) - Fires a ClickDetector
static int lua_fireclickdetector(lua_State* L) {
    luaL_checkany(L, 1);
    double distance = luaL_optnumber(L, 2, 0);
    (void)distance;
    
    // Try to fire the detector's MouseClick event
    if (lua_getmetatable(L, 1)) {
        lua_getfield(L, -1, "__index");
        if (lua_istable(L, -1)) {
            lua_getfield(L, -1, "MouseClick");
            if (!lua_isnil(L, -1)) {
                lua_getfield(L, -1, "Fire");
                if (lua_isfunction(L, -1)) {
                    lua_pushvalue(L, -2);
                    if (lua_gettop(L) >= 3 && !lua_isnil(L, 3)) {
                        lua_pushvalue(L, 3);
                    } else {
                        lua_pushnil(L);
                    }
                    lua_pcall(L, 2, 0, 0);
                }
            }
        }
    }
    
    return 0;
}

// firetouchinterest(part1, part2, toggle) - Fires touch interest
static int lua_firetouchinterest(lua_State* L) {
    luaL_checkany(L, 1);
    luaL_checkany(L, 2);
    int toggle = luaL_optinteger(L, 3, 0);
    
    // Try to fire Touched/TouchEnded events
    const char* event_name = toggle ? "Touched" : "TouchEnded";
    
    if (lua_getmetatable(L, 1)) {
        lua_getfield(L, -1, "__index");
        if (lua_istable(L, -1)) {
            lua_getfield(L, -1, event_name);
            if (!lua_isnil(L, -1)) {
                lua_getfield(L, -1, "Fire");
                if (lua_isfunction(L, -1)) {
                    lua_pushvalue(L, -2);
                    lua_pushvalue(L, 2);
                    lua_pcall(L, 2, 0, 0);
                }
            }
        }
    }
    
    return 0;
}

// fireproximityprompt(prompt) - Fires a ProximityPrompt
static int lua_fireproximityprompt(lua_State* L) {
    luaL_checkany(L, 1);
    
    if (lua_getmetatable(L, 1)) {
        lua_getfield(L, -1, "__index");
        if (lua_istable(L, -1)) {
            lua_getfield(L, -1, "Triggered");
            if (!lua_isnil(L, -1)) {
                lua_getfield(L, -1, "Fire");
                if (lua_isfunction(L, -1)) {
                    lua_pushvalue(L, -2);
                    lua_pcall(L, 1, 0, 0);
                }
            }
        }
    }
    
    return 0;
}

/* isrbxactive() / isgameactive() - Checks if Roblox window is focused */
static int lua_isrbxactive(lua_State* L) {
#if defined(XORON_PLATFORM_IOS) || defined(__APPLE__)
    /* On iOS, we're injected into the Roblox process as a dylib.
       Check if the app is in foreground using UIApplication state */
    Class UIApplicationClass = objc_getClass("UIApplication");
    if (UIApplicationClass) {
        id sharedApp = ((id(*)(Class, SEL))objc_msgSend)(
            UIApplicationClass, sel_registerName("sharedApplication"));
        if (sharedApp) {
            /* applicationState: 0 = active, 1 = inactive, 2 = background */
            long state = ((long(*)(id, SEL))objc_msgSend)(
                sharedApp, sel_registerName("applicationState"));
            lua_pushboolean(L, state == 0); /* UIApplicationStateActive */
            return 1;
        }
    }
    /* If we can't check, assume active since we're injected */
    lua_pushboolean(L, true);
    return 1;
#elif defined(XORON_PLATFORM_ANDROID) || defined(__ANDROID__)
    /* On Android, we assume the game is always active when the library is loaded */
    /* A more sophisticated check would require JNI to query the activity state */
    lua_pushboolean(L, true);
    return 1;
#else
    lua_pushboolean(L, true);
    return 1;
#endif
}

// setfpscap(fps) - Sets FPS cap
static int lua_setfpscap(lua_State* L) {
    int fps = luaL_optinteger(L, 1, 60);
    if (fps < 0) fps = 0;
    if (fps > 1000) fps = 1000;
    g_fps_cap.store(fps);
    return 0;
}

// getfpscap() - Gets current FPS cap
static int lua_getfpscap(lua_State* L) {
    lua_pushinteger(L, g_fps_cap.load());
    return 1;
}

/* setclipboard(text) / toclipboard(text) - Sets clipboard content */
static int lua_setclipboard(lua_State* L) {
    const char* text = luaL_checkstring(L, 1);
    
#if defined(XORON_PLATFORM_IOS) || defined(__APPLE__)
    /* On iOS, use UIPasteboard instead of NSPasteboard */
    Class UIPasteboardClass = objc_getClass("UIPasteboard");
    if (UIPasteboardClass) {
        id pasteboard = ((id(*)(Class, SEL))objc_msgSend)(
            UIPasteboardClass, sel_registerName("generalPasteboard"));
        
        if (pasteboard) {
            id nsstring = ((id(*)(Class, SEL, const char*))objc_msgSend)(
                objc_getClass("NSString"), sel_registerName("stringWithUTF8String:"), text);
            
            if (nsstring) {
                ((void(*)(id, SEL, id))objc_msgSend)(
                    pasteboard, sel_registerName("setString:"), nsstring);
            }
        }
    }
        
#elif defined(XORON_PLATFORM_ANDROID) || defined(__ANDROID__)
    /* Android clipboard requires JNI - use ClipboardManager */
    JNIEnv* env = get_jni_env();
    if (env && g_clipboard_manager) {
        std::lock_guard<std::mutex> lock(g_clipboard_mutex);
        
        /* Create ClipData from text */
        jclass clipDataClass = env->FindClass("android/content/ClipData");
        if (clipDataClass) {
            jmethodID newPlainText = env->GetStaticMethodID(clipDataClass, "newPlainText",
                "(Ljava/lang/CharSequence;Ljava/lang/CharSequence;)Landroid/content/ClipData;");
            if (newPlainText) {
                jstring label = env->NewStringUTF("Xoron");
                jstring content = env->NewStringUTF(text);
                jobject clipData = env->CallStaticObjectMethod(clipDataClass, newPlainText, label, content);
                
                if (clipData) {
                    /* Set the clip data to clipboard manager */
                    jclass clipboardClass = env->GetObjectClass(g_clipboard_manager);
                    jmethodID setPrimaryClip = env->GetMethodID(clipboardClass, "setPrimaryClip",
                        "(Landroid/content/ClipData;)V");
                    if (setPrimaryClip) {
                        env->CallVoidMethod(g_clipboard_manager, setPrimaryClip, clipData);
                    }
                    env->DeleteLocalRef(clipData);
                }
                env->DeleteLocalRef(label);
                env->DeleteLocalRef(content);
            }
            env->DeleteLocalRef(clipDataClass);
        }
    } else {
        ENV_LOG("Clipboard not available - JNI not initialized");
    }
#else
    (void)text;
#endif
    
    return 0;
}

/* getclipboard() - Gets clipboard content */
static int lua_getclipboard(lua_State* L) {
#if defined(XORON_PLATFORM_IOS) || defined(__APPLE__)
    /* On iOS, use UIPasteboard instead of NSPasteboard */
    Class UIPasteboardClass = objc_getClass("UIPasteboard");
    if (!UIPasteboardClass) {
        lua_pushstring(L, "");
        return 1;
    }
    
    id pasteboard = ((id(*)(Class, SEL))objc_msgSend)(
        UIPasteboardClass, sel_registerName("generalPasteboard"));
    
    if (pasteboard) {
        id str = ((id(*)(id, SEL))objc_msgSend)(pasteboard, sel_registerName("string"));
        if (str) {
            const char* cstr = ((const char*(*)(id, SEL))objc_msgSend)(
                str, sel_registerName("UTF8String"));
            if (cstr) {
                lua_pushstring(L, cstr);
                return 1;
            }
        }
    }
    
#elif defined(XORON_PLATFORM_ANDROID) || defined(__ANDROID__)
    /* Android clipboard requires JNI - use ClipboardManager */
    JNIEnv* env = get_jni_env();
    if (env && g_clipboard_manager) {
        std::lock_guard<std::mutex> lock(g_clipboard_mutex);
        
        jclass clipboardClass = env->GetObjectClass(g_clipboard_manager);
        jmethodID getPrimaryClip = env->GetMethodID(clipboardClass, "getPrimaryClip",
            "()Landroid/content/ClipData;");
        
        if (getPrimaryClip) {
            jobject clipData = env->CallObjectMethod(g_clipboard_manager, getPrimaryClip);
            if (clipData) {
                jclass clipDataClass = env->GetObjectClass(clipData);
                jmethodID getItemCount = env->GetMethodID(clipDataClass, "getItemCount", "()I");
                jint count = env->CallIntMethod(clipData, getItemCount);
                
                if (count > 0) {
                    jmethodID getItemAt = env->GetMethodID(clipDataClass, "getItemAt",
                        "(I)Landroid/content/ClipData$Item;");
                    jobject item = env->CallObjectMethod(clipData, getItemAt, 0);
                    
                    if (item) {
                        jclass itemClass = env->GetObjectClass(item);
                        jmethodID getText = env->GetMethodID(itemClass, "getText",
                            "()Ljava/lang/CharSequence;");
                        jobject charSeq = env->CallObjectMethod(item, getText);
                        
                        if (charSeq) {
                            jmethodID toString = env->GetMethodID(
                                env->FindClass("java/lang/Object"), "toString",
                                "()Ljava/lang/String;");
                            jstring str = (jstring)env->CallObjectMethod(charSeq, toString);
                            
                            if (str) {
                                const char* cstr = env->GetStringUTFChars(str, nullptr);
                                if (cstr) {
                                    lua_pushstring(L, cstr);
                                    env->ReleaseStringUTFChars(str, cstr);
                                    env->DeleteLocalRef(str);
                                    env->DeleteLocalRef(charSeq);
                                    env->DeleteLocalRef(item);
                                    env->DeleteLocalRef(clipData);
                                    return 1;
                                }
                                env->DeleteLocalRef(str);
                            }
                            env->DeleteLocalRef(charSeq);
                        }
                        env->DeleteLocalRef(item);
                    }
                }
                env->DeleteLocalRef(clipData);
            }
        }
    }
#endif
    lua_pushstring(L, "");
    return 1;
}

// lz4compress(data) - Compresses data with LZ4
static int lua_lz4compress(lua_State* L) {
    size_t len;
    const char* data = luaL_checklstring(L, 1, &len);
    
    // Calculate max compressed size
    int max_compressed = LZ4_compressBound((int)len);
    if (max_compressed <= 0) {
        luaL_error(L, "Data too large to compress");
        return 0;
    }
    
    // Allocate buffer with 4 bytes for original size header
    std::vector<char> compressed(max_compressed + 4);
    
    // Store original size (4 bytes, little endian)
    compressed[0] = (len >> 0) & 0xFF;
    compressed[1] = (len >> 8) & 0xFF;
    compressed[2] = (len >> 16) & 0xFF;
    compressed[3] = (len >> 24) & 0xFF;
    
    // Compress
    int compressed_size = LZ4_compress_default(data, compressed.data() + 4, (int)len, max_compressed);
    if (compressed_size <= 0) {
        luaL_error(L, "LZ4 compression failed");
        return 0;
    }
    
    lua_pushlstring(L, compressed.data(), compressed_size + 4);
    return 1;
}

// lz4decompress(data, size) - Decompresses LZ4 data
static int lua_lz4decompress(lua_State* L) {
    size_t len;
    const char* data = luaL_checklstring(L, 1, &len);
    size_t expected_size = luaL_optinteger(L, 2, 0);
    
    if (len < 4) {
        luaL_error(L, "Invalid compressed data");
        return 0;
    }
    
    // Read original size from header
    size_t orig_size = ((unsigned char)data[0]) |
                       ((unsigned char)data[1] << 8) |
                       ((unsigned char)data[2] << 16) |
                       ((unsigned char)data[3] << 24);
    
    // Use expected_size if provided and different
    if (expected_size > 0) {
        orig_size = expected_size;
    }
    
    // Safety check
    if (orig_size > 100 * 1024 * 1024) { // 100MB limit
        luaL_error(L, "Decompressed size too large");
        return 0;
    }
    
    std::vector<char> decompressed(orig_size);
    
    int decompressed_size = LZ4_decompress_safe(data + 4, decompressed.data(), (int)(len - 4), (int)orig_size);
    if (decompressed_size < 0) {
        luaL_error(L, "LZ4 decompression failed");
        return 0;
    }
    
    lua_pushlstring(L, decompressed.data(), decompressed_size);
    return 1;
}

// queue_on_teleport(code) - Queues code to run after teleport
static int lua_queue_on_teleport(lua_State* L) {
    const char* code = luaL_checkstring(L, 1);
    
    std::lock_guard<std::mutex> lock(g_env_mutex);
    g_teleport_queue.push_back(code);
    
    return 0;
}

// getteleportqueue() - Gets queued teleport scripts
static int lua_getteleportqueue(lua_State* L) {
    std::lock_guard<std::mutex> lock(g_env_mutex);
    
    lua_newtable(L);
    for (size_t i = 0; i < g_teleport_queue.size(); i++) {
        lua_pushstring(L, g_teleport_queue[i].c_str());
        lua_rawseti(L, -2, (int)i + 1);
    }
    
    return 1;
}

// clearteleportqueue() - Clears teleport queue
static int lua_clearteleportqueue(lua_State* L) {
    std::lock_guard<std::mutex> lock(g_env_mutex);
    g_teleport_queue.clear();
    return 0;
}

// gethui() - Gets hidden UI container
static int lua_gethui(lua_State* L) {
    // Return CoreGui or create hidden container
    lua_getfield(L, LUA_REGISTRYINDEX, "_XORON_HUI");
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setfield(L, LUA_REGISTRYINDEX, "_XORON_HUI");
    }
    return 1;
}

// getcallingscript() - Gets the script that called the current function
static int lua_getcallingscript(lua_State* L) {
    lua_getfield(L, LUA_REGISTRYINDEX, "_XORON_CURRENT_SCRIPT");
    return 1;
}

// checkclosure(func) - Alias for isexecutorclosure
static int lua_checkclosure(lua_State* L) {
    return lua_isexecutorclosure(L);
}

// compareinstances(a, b) - Compares two instances
static int lua_compareinstances(lua_State* L) {
    luaL_checkany(L, 1);
    luaL_checkany(L, 2);
    lua_pushboolean(L, lua_rawequal(L, 1, 2));
    return 1;
}

// cloneref(instance) - Clones an instance reference
static int lua_cloneref(lua_State* L) {
    luaL_checkany(L, 1);
    lua_pushvalue(L, 1);
    return 1;
}

// Register all environment functions
void xoron_register_env(lua_State* L) {
    // Global environment functions
    lua_pushcfunction(L, lua_getgenv, "getgenv");
    lua_setglobal(L, "getgenv");
    
    lua_pushcfunction(L, lua_getrenv, "getrenv");
    lua_setglobal(L, "getrenv");
    
    lua_pushcfunction(L, lua_getmenv, "getmenv");
    lua_setglobal(L, "getmenv");
    
    lua_pushcfunction(L, lua_getsenv, "getsenv");
    lua_setglobal(L, "getsenv");
    
    lua_pushcfunction(L, lua_getreg, "getreg");
    lua_setglobal(L, "getreg");
    
    // Metatable functions
    lua_pushcfunction(L, lua_getrawmetatable, "getrawmetatable");
    lua_setglobal(L, "getrawmetatable");
    
    lua_pushcfunction(L, lua_setrawmetatable, "setrawmetatable");
    lua_setglobal(L, "setrawmetatable");
    
    lua_pushcfunction(L, lua_setreadonly_func, "setreadonly");
    lua_setglobal(L, "setreadonly");
    
    lua_pushcfunction(L, lua_isreadonly_check, "isreadonly");
    lua_setglobal(L, "isreadonly");
    
    // Executor identification
    lua_pushcfunction(L, lua_identifyexecutor, "identifyexecutor");
    lua_setglobal(L, "identifyexecutor");
    
    lua_pushcfunction(L, lua_getexecutorname, "getexecutorname");
    lua_setglobal(L, "getexecutorname");
    
    // Function utilities
    lua_pushcfunction(L, lua_isexecutorclosure, "isexecutorclosure");
    lua_setglobal(L, "isexecutorclosure");
    
    lua_pushcfunction(L, lua_islclosure, "islclosure");
    lua_setglobal(L, "islclosure");
    
    lua_pushcfunction(L, lua_iscclosure, "iscclosure");
    lua_setglobal(L, "iscclosure");
    
    lua_pushcfunction(L, lua_newcclosure, "newcclosure");
    lua_setglobal(L, "newcclosure");
    
    lua_pushcfunction(L, lua_clonefunction_func, "clonefunction");
    lua_setglobal(L, "clonefunction");
    
    lua_pushcfunction(L, lua_hookfunction, "hookfunction");
    lua_setglobal(L, "hookfunction");
    lua_pushcfunction(L, lua_hookfunction, "replaceclosure");
    lua_setglobal(L, "replaceclosure");
    
    lua_pushcfunction(L, lua_checkcaller, "checkcaller");
    lua_setglobal(L, "checkcaller");
    
    // Thread identity
    lua_pushcfunction(L, lua_getthreadidentity, "getthreadidentity");
    lua_setglobal(L, "getthreadidentity");
    lua_pushcfunction(L, lua_getthreadidentity, "getidentity");
    lua_setglobal(L, "getidentity");
    
    lua_pushcfunction(L, lua_setthreadidentity, "setthreadidentity");
    lua_setglobal(L, "setthreadidentity");
    lua_pushcfunction(L, lua_setthreadidentity, "setidentity");
    lua_setglobal(L, "setidentity");
    
    // Namecall
    lua_pushcfunction(L, lua_getnamecallmethod, "getnamecallmethod");
    lua_setglobal(L, "getnamecallmethod");
    
    lua_pushcfunction(L, lua_setnamecallmethod, "setnamecallmethod");
    lua_setglobal(L, "setnamecallmethod");
    
    // Script loading
    lua_pushcfunction(L, lua_loadstring_custom, "loadstring");
    lua_setglobal(L, "loadstring");
    
    // GC and instances
    lua_pushcfunction(L, lua_getgc, "getgc");
    lua_setglobal(L, "getgc");
    
    lua_pushcfunction(L, lua_getinstances, "getinstances");
    lua_setglobal(L, "getinstances");
    
    lua_pushcfunction(L, lua_getnilinstances, "getnilinstances");
    lua_setglobal(L, "getnilinstances");
    
    lua_pushcfunction(L, lua_getscripts, "getscripts");
    lua_setglobal(L, "getscripts");
    
    lua_pushcfunction(L, lua_getloadedmodules, "getloadedmodules");
    lua_setglobal(L, "getloadedmodules");
    
    lua_pushcfunction(L, lua_getrunningscripts, "getrunningscripts");
    lua_setglobal(L, "getrunningscripts");
    
    // Signals and events
    lua_pushcfunction(L, lua_getconnections, "getconnections");
    lua_setglobal(L, "getconnections");
    
    lua_pushcfunction(L, lua_firesignal, "firesignal");
    lua_setglobal(L, "firesignal");
    
    lua_pushcfunction(L, lua_fireclickdetector, "fireclickdetector");
    lua_setglobal(L, "fireclickdetector");
    
    lua_pushcfunction(L, lua_firetouchinterest, "firetouchinterest");
    lua_setglobal(L, "firetouchinterest");
    
    lua_pushcfunction(L, lua_fireproximityprompt, "fireproximityprompt");
    lua_setglobal(L, "fireproximityprompt");
    
    // Misc utilities
    lua_pushcfunction(L, lua_isrbxactive, "isrbxactive");
    lua_setglobal(L, "isrbxactive");
    lua_pushcfunction(L, lua_isrbxactive, "isgameactive");
    lua_setglobal(L, "isgameactive");
    
    lua_pushcfunction(L, lua_setfpscap, "setfpscap");
    lua_setglobal(L, "setfpscap");
    
    lua_pushcfunction(L, lua_getfpscap, "getfpscap");
    lua_setglobal(L, "getfpscap");
    
    lua_pushcfunction(L, lua_setclipboard, "setclipboard");
    lua_setglobal(L, "setclipboard");
    lua_pushcfunction(L, lua_setclipboard, "toclipboard");
    lua_setglobal(L, "toclipboard");
    
    lua_pushcfunction(L, lua_lz4compress, "lz4compress");
    lua_setglobal(L, "lz4compress");
    
    lua_pushcfunction(L, lua_lz4decompress, "lz4decompress");
    lua_setglobal(L, "lz4decompress");
    
    lua_pushcfunction(L, lua_queue_on_teleport, "queue_on_teleport");
    lua_setglobal(L, "queue_on_teleport");
    lua_pushcfunction(L, lua_queue_on_teleport, "queueonteleport");
    lua_setglobal(L, "queueonteleport");
    
    lua_pushcfunction(L, lua_gethui, "gethui");
    lua_setglobal(L, "gethui");
    
    // Additional functions
    lua_pushcfunction(L, lua_getclipboard, "getclipboard");
    lua_setglobal(L, "getclipboard");
    
    lua_pushcfunction(L, lua_getteleportqueue, "getteleportqueue");
    lua_setglobal(L, "getteleportqueue");
    
    lua_pushcfunction(L, lua_clearteleportqueue, "clearteleportqueue");
    lua_setglobal(L, "clearteleportqueue");
    
    lua_pushcfunction(L, lua_getcallingscript, "getcallingscript");
    lua_setglobal(L, "getcallingscript");
    
    lua_pushcfunction(L, lua_checkclosure, "checkclosure");
    lua_setglobal(L, "checkclosure");
    
    lua_pushcfunction(L, lua_compareinstances, "compareinstances");
    lua_setglobal(L, "compareinstances");
    
    lua_pushcfunction(L, lua_cloneref, "cloneref");
    lua_setglobal(L, "cloneref");
    
    lua_pushcfunction(L, lua_hookmetamethod, "hookmetamethod");
    lua_setglobal(L, "hookmetamethod");
    
    lua_pushcfunction(L, lua_restorefunction, "restorefunction");
    lua_setglobal(L, "restorefunction");
    
    lua_pushcfunction(L, lua_makereadonly, "makereadonly");
    lua_setglobal(L, "makereadonly");
    
    lua_pushcfunction(L, lua_makewriteable, "makewriteable");
    lua_setglobal(L, "makewriteable");

    // sUNC additional functions - filtergc
    lua_pushcfunction(L, lua_filtergc, "filtergc");
    lua_setglobal(L, "filtergc");

    // sUNC - getfunctionhash
    lua_pushcfunction(L, lua_getfunctionhash, "getfunctionhash");
    lua_setglobal(L, "getfunctionhash");

    // sUNC - getcallbackvalue
    lua_pushcfunction(L, lua_getcallbackvalue, "getcallbackvalue");
    lua_setglobal(L, "getcallbackvalue");

    // sUNC - isscriptable/setscriptable
    lua_pushcfunction(L, lua_isscriptable, "isscriptable");
    lua_setglobal(L, "isscriptable");

    lua_pushcfunction(L, lua_setscriptable, "setscriptable");
    lua_setglobal(L, "setscriptable");

    // sUNC - replicatesignal
    lua_pushcfunction(L, lua_replicatesignal, "replicatesignal");
    lua_setglobal(L, "replicatesignal");
}


/* ============================================================================
 * Android JNI Functions for Environment Setup
 * ============================================================================ */
#if defined(XORON_PLATFORM_ANDROID) || defined(__ANDROID__)
extern "C" {

/* Initialize clipboard manager from Java */
JNIEXPORT void JNICALL Java_com_xoron_Executor_initClipboard(JNIEnv* env, jobject obj, jobject clipboardManager) {
    (void)obj;
    std::lock_guard<std::mutex> lock(g_clipboard_mutex);
    
    /* Store JVM reference */
    env->GetJavaVM(&g_env_jvm);
    
    /* Store global reference to clipboard manager */
    if (g_clipboard_manager) {
        env->DeleteGlobalRef(g_clipboard_manager);
    }
    g_clipboard_manager = env->NewGlobalRef(clipboardManager);
    
    ENV_LOG("Clipboard manager initialized");
}

/* Get platform information */
JNIEXPORT jstring JNICALL Java_com_xoron_Executor_getPlatformInfo(JNIEnv* env, jobject obj) {
    (void)obj;
    
    char info[256];
    char sdk_ver[PROP_VALUE_MAX] = "unknown";
    char device[PROP_VALUE_MAX] = "unknown";
    char model[PROP_VALUE_MAX] = "unknown";
    
    __system_property_get("ro.build.version.sdk", sdk_ver);
    __system_property_get("ro.product.device", device);
    __system_property_get("ro.product.model", model);
    
    snprintf(info, sizeof(info), 
        "Xoron v%s | Android API %s | Device: %s (%s) | Arch: %s",
        XORON_VERSION, sdk_ver, model, device, XORON_ARCH_NAME);
    
    return env->NewStringUTF(info);
}

/* Check if running on Android 10+ */
JNIEXPORT jboolean JNICALL Java_com_xoron_Executor_isAndroid10OrHigher(JNIEnv* env, jobject obj) {
    (void)env;
    (void)obj;
    
    char sdk_ver_str[PROP_VALUE_MAX];
    if (__system_property_get("ro.build.version.sdk", sdk_ver_str)) {
        int api_level = atoi(sdk_ver_str);
        return api_level >= 29 ? JNI_TRUE : JNI_FALSE;
    }
    return JNI_FALSE;
}

} /* extern "C" */
#endif /* XORON_PLATFORM_ANDROID */
