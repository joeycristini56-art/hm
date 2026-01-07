/*
 * xoron_filesystem.cpp - File I/O operations for executor
 * Provides: readfile, writefile, appendfile, listfiles, isfile, isfolder, etc.
 * Platforms: iOS 15+ (.dylib) and Android 10+ (.so)
 * 
 * Folder Structure:
 *   iOS:     ~/Documents/Xoron/
 *   Android: /data/data/<package>/files/Xoron/ (internal storage)
 *            or /storage/emulated/0/Xoron/ (external storage)
 *
 *   Structure:
 *     ├── autoexecute/   - Scripts that run automatically on injection
 *     ├── scripts/       - Saved scripts
 *     └── workspace/     - File operations workspace
 */

#include "xoron.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <mutex>
#include <algorithm>

/* Platform-specific includes */
#if defined(XORON_PLATFORM_IOS)
    #include <Foundation/Foundation.h>
    #include <objc/objc.h>
    #include <objc/runtime.h>
    #include <objc/message.h>
    #include <unistd.h>
    #include <sys/stat.h>
    #define FS_LOG(...) NSLog(@__VA_ARGS__)
#elif defined(XORON_PLATFORM_ANDROID) || defined(__ANDROID__)
    #include <jni.h>
    #include <android/log.h>
    #include <sys/stat.h>
    #include <unistd.h>
    #define FS_LOG_TAG "XoronFS"
    #define FS_LOG(...) __android_log_print(ANDROID_LOG_INFO, FS_LOG_TAG, __VA_ARGS__)
#else
    #include <unistd.h>
    #include <sys/stat.h>
    #define FS_LOG(...) printf(__VA_ARGS__)
#endif

#include "lua.h"
#include "lualib.h"
#include "luacode.h"
#include "Luau/Compiler.h"

namespace fs = std::filesystem;

static std::mutex g_fs_mutex;
static std::string g_base_path;
static std::string g_workspace_path;
static std::string g_autoexecute_path;
static std::string g_scripts_path;

/* Android-specific: Store the app's internal files directory path */
#if defined(XORON_PLATFORM_ANDROID) || defined(__ANDROID__)
static std::string g_android_internal_path;
static std::string g_android_external_path;
static bool g_use_external_storage = false;
#endif

extern void xoron_set_error(const char* fmt, ...);

/* Platform-specific path initialization */
static std::string get_platform_base_path() {
#if defined(XORON_PLATFORM_ANDROID) || defined(__ANDROID__)
    /* Android: Try external storage first, fall back to internal */
    
    /* Check if external storage path was set via JNI */
    if (!g_android_external_path.empty()) {
        struct stat st;
        if (stat(g_android_external_path.c_str(), &st) == 0 || 
            mkdir(g_android_external_path.c_str(), 0755) == 0) {
            /* Check if we can write to external storage */
            std::string test_file = g_android_external_path + "/.xoron_test";
            FILE* f = fopen(test_file.c_str(), "w");
            if (f) {
                fclose(f);
                remove(test_file.c_str());
                g_use_external_storage = true;
                FS_LOG("Using external storage: %s", g_android_external_path.c_str());
                return g_android_external_path;
            }
        }
    }
    
    /* Try default external storage path */
    const char* ext_storage = getenv("EXTERNAL_STORAGE");
    if (ext_storage) {
        std::string ext_path = std::string(ext_storage) + "/Xoron";
        struct stat st;
        if (stat(ext_path.c_str(), &st) == 0 || mkdir(ext_path.c_str(), 0755) == 0) {
            std::string test_file = ext_path + "/.xoron_test";
            FILE* f = fopen(test_file.c_str(), "w");
            if (f) {
                fclose(f);
                remove(test_file.c_str());
                g_use_external_storage = true;
                FS_LOG("Using EXTERNAL_STORAGE: %s", ext_path.c_str());
                return ext_path;
            }
        }
    }
    
    /* Try /storage/emulated/0/Xoron */
    {
        std::string ext_path = "/storage/emulated/0/Xoron";
        struct stat st;
        if (stat(ext_path.c_str(), &st) == 0 || mkdir(ext_path.c_str(), 0755) == 0) {
            std::string test_file = ext_path + "/.xoron_test";
            FILE* f = fopen(test_file.c_str(), "w");
            if (f) {
                fclose(f);
                remove(test_file.c_str());
                g_use_external_storage = true;
                FS_LOG("Using /storage/emulated/0: %s", ext_path.c_str());
                return ext_path;
            }
        }
    }
    
    /* Fall back to internal storage */
    if (!g_android_internal_path.empty()) {
        FS_LOG("Using internal storage: %s", g_android_internal_path.c_str());
        return g_android_internal_path;
    }
    
    /* Last resort: use app data directory pattern */
    const char* data_dir = getenv("ANDROID_DATA");
    if (data_dir) {
        std::string internal_path = std::string(data_dir) + "/files/Xoron";
        FS_LOG("Using ANDROID_DATA: %s", internal_path.c_str());
        return internal_path;
    }
    
    /* Absolute fallback */
    FS_LOG("Using fallback path: /data/local/tmp/Xoron");
    return "/data/local/tmp/Xoron";
    
#elif defined(XORON_PLATFORM_IOS) || defined(__APPLE__)
    /* iOS: Use ~/Documents/Xoron */
    const char* home = getenv("HOME");
    if (home) {
        return std::string(home) + "/Documents/Xoron";
    }
    /* Fallback for iOS sandbox */
    return "./Documents/Xoron";
    
#else
    /* Development/Unknown platform */
    const char* home = getenv("HOME");
    if (home) {
        return std::string(home) + "/Xoron";
    }
    return "./Xoron";
#endif
}

/* Initialize all executor directories */
static void ensure_directories() {
    std::lock_guard<std::mutex> lock(g_fs_mutex);
    
    if (g_base_path.empty()) {
        g_base_path = get_platform_base_path();
        g_workspace_path = g_base_path + "/workspace";
        g_autoexecute_path = g_base_path + "/autoexecute";
        g_scripts_path = g_base_path + "/scripts";
        
#if defined(XORON_PLATFORM_ANDROID) || defined(__ANDROID__)
        FS_LOG("Xoron filesystem initialized:");
        FS_LOG("  Base: %s", g_base_path.c_str());
        FS_LOG("  Workspace: %s", g_workspace_path.c_str());
        FS_LOG("  Autoexecute: %s", g_autoexecute_path.c_str());
        FS_LOG("  Scripts: %s", g_scripts_path.c_str());
#endif
    }
    
    /* Create all directories if they don't exist */
    std::error_code ec;
    if (!fs::exists(g_base_path, ec)) {
        fs::create_directories(g_base_path, ec);
        if (ec) {
#if defined(XORON_PLATFORM_ANDROID) || defined(__ANDROID__)
            FS_LOG("Failed to create base path: %s - %s", g_base_path.c_str(), ec.message().c_str());
#endif
        }
    }
    if (!fs::exists(g_workspace_path, ec)) {
        fs::create_directories(g_workspace_path, ec);
    }
    if (!fs::exists(g_autoexecute_path, ec)) {
        fs::create_directories(g_autoexecute_path, ec);
    }
    if (!fs::exists(g_scripts_path, ec)) {
        fs::create_directories(g_scripts_path, ec);
    }
}

/* Android JNI: Set internal storage path from Java */
#if defined(XORON_PLATFORM_ANDROID) || defined(__ANDROID__)
extern "C" {
    
JNIEXPORT void JNICALL Java_com_xoron_Executor_setInternalStoragePath(JNIEnv* env, jobject obj, jstring path) {
    (void)obj;
    const char* path_str = env->GetStringUTFChars(path, nullptr);
    if (path_str) {
        std::lock_guard<std::mutex> lock(g_fs_mutex);
        g_android_internal_path = std::string(path_str) + "/Xoron";
        env->ReleaseStringUTFChars(path, path_str);
        FS_LOG("Internal storage path set: %s", g_android_internal_path.c_str());
    }
}

JNIEXPORT void JNICALL Java_com_xoron_Executor_setExternalStoragePath(JNIEnv* env, jobject obj, jstring path) {
    (void)obj;
    const char* path_str = env->GetStringUTFChars(path, nullptr);
    if (path_str) {
        std::lock_guard<std::mutex> lock(g_fs_mutex);
        g_android_external_path = std::string(path_str) + "/Xoron";
        env->ReleaseStringUTFChars(path, path_str);
        FS_LOG("External storage path set: %s", g_android_external_path.c_str());
    }
}

JNIEXPORT jstring JNICALL Java_com_xoron_Executor_getBasePath(JNIEnv* env, jobject obj) {
    (void)obj;
    ensure_directories();
    return env->NewStringUTF(g_base_path.c_str());
}

JNIEXPORT jboolean JNICALL Java_com_xoron_Executor_isUsingExternalStorage(JNIEnv* env, jobject obj) {
    (void)env;
    (void)obj;
    return g_use_external_storage ? JNI_TRUE : JNI_FALSE;
}

} /* extern "C" */
#endif /* XORON_PLATFORM_ANDROID */

// Legacy function for compatibility
static void ensure_workspace() {
    ensure_directories();
}

// Resolve path relative to workspace
static std::string resolve_path(const char* path) {
    ensure_directories();
    
    std::string p(path);
    
    // If path is empty, return workspace
    if (p.empty()) {
        return g_workspace_path;
    }
    
    // Check if it's an absolute path within our allowed directories
    if (p[0] == '/' || p[0] == '\\') {
        // Check if it's within our base directory
        if (p.find(g_base_path) == 0) {
            // Prevent directory traversal
            if (p.find("..") != std::string::npos) {
                return "";
            }
            return p;
        }
        // Strip leading slashes for relative path handling
        while (!p.empty() && (p[0] == '/' || p[0] == '\\')) {
            p = p.substr(1);
        }
    }
    
    // Prevent directory traversal
    if (p.find("..") != std::string::npos) {
        return "";
    }
    
    return g_workspace_path + "/" + p;
}

// readfile(path) - Reads file contents
static int lua_readfile(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    std::string resolved = resolve_path(path);
    
    if (resolved.empty()) {
        luaL_error(L, "Invalid path");
        return 0;
    }
    
    std::ifstream file(resolved, std::ios::binary);
    if (!file) {
        luaL_error(L, "Unable to read file: %s", path);
        return 0;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    lua_pushlstring(L, content.c_str(), content.size());
    return 1;
}

// writefile(path, content) - Writes content to file
static int lua_writefile(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    size_t len;
    const char* content = luaL_checklstring(L, 2, &len);
    
    std::string resolved = resolve_path(path);
    if (resolved.empty()) {
        luaL_error(L, "Invalid path");
        return 0;
    }
    
    // Create parent directories if needed
    fs::path p(resolved);
    if (p.has_parent_path()) {
        fs::create_directories(p.parent_path());
    }
    
    std::ofstream file(resolved, std::ios::binary);
    if (!file) {
        luaL_error(L, "Unable to write file: %s", path);
        return 0;
    }
    
    file.write(content, len);
    return 0;
}

// appendfile(path, content) - Appends content to file
static int lua_appendfile(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    size_t len;
    const char* content = luaL_checklstring(L, 2, &len);
    
    std::string resolved = resolve_path(path);
    if (resolved.empty()) {
        luaL_error(L, "Invalid path");
        return 0;
    }
    
    std::ofstream file(resolved, std::ios::binary | std::ios::app);
    if (!file) {
        luaL_error(L, "Unable to append to file: %s", path);
        return 0;
    }
    
    file.write(content, len);
    return 0;
}

// loadfile(path) - Loads and compiles a Lua file
static int lua_loadfile_custom(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    std::string resolved = resolve_path(path);
    
    if (resolved.empty()) {
        lua_pushnil(L);
        lua_pushstring(L, "Invalid path");
        return 2;
    }
    
    std::ifstream file(resolved, std::ios::binary);
    if (!file) {
        lua_pushnil(L);
        lua_pushfstring(L, "Unable to read file: %s", path);
        return 2;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    
    // Use Luau compiler
    std::string bytecode = Luau::compile(source);
    
    if (bytecode.empty() || bytecode[0] == 0) {
        lua_pushnil(L);
        if (bytecode.size() > 1) {
            lua_pushlstring(L, bytecode.data() + 1, bytecode.size() - 1);
        } else {
            lua_pushstring(L, "Compilation failed");
        }
        return 2;
    }
    
    int result = luau_load(L, path, bytecode.data(), bytecode.size(), 0);
    
    if (result != 0) {
        lua_pushnil(L);
        lua_insert(L, -2);
        return 2;
    }
    
    return 1;
}

// dofile(path) - Loads and executes a Lua file
static int lua_dofile_custom(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    std::string resolved = resolve_path(path);
    
    if (resolved.empty()) {
        luaL_error(L, "Invalid path");
        return 0;
    }
    
    std::ifstream file(resolved, std::ios::binary);
    if (!file) {
        luaL_error(L, "Unable to read file: %s", path);
        return 0;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    
    // Use Luau compiler
    std::string bytecode = Luau::compile(source);
    
    if (bytecode.empty() || bytecode[0] == 0) {
        if (bytecode.size() > 1) {
            luaL_error(L, "Compilation failed: %s", bytecode.data() + 1);
        } else {
            luaL_error(L, "Compilation failed");
        }
        return 0;
    }
    
    int result = luau_load(L, path, bytecode.data(), bytecode.size(), 0);
    
    if (result != 0) {
        lua_error(L);
        return 0;
    }
    
    lua_call(L, 0, LUA_MULTRET);
    return lua_gettop(L);
}

// listfiles(path) - Lists files in directory
static int lua_listfiles(lua_State* L) {
    const char* path = luaL_optstring(L, 1, "");
    std::string resolved = resolve_path(path);
    
    if (resolved.empty()) {
        luaL_error(L, "Invalid path");
        return 0;
    }
    
    if (!fs::exists(resolved) || !fs::is_directory(resolved)) {
        luaL_error(L, "Directory does not exist: %s", path);
        return 0;
    }
    
    lua_newtable(L);
    int idx = 1;
    
    for (const auto& entry : fs::directory_iterator(resolved)) {
        std::string name = entry.path().filename().string();
        std::string relative = std::string(path);
        if (!relative.empty() && relative.back() != '/') {
            relative += "/";
        }
        relative += name;
        
        lua_pushstring(L, relative.c_str());
        lua_rawseti(L, -2, idx++);
    }
    
    return 1;
}

// isfile(path) - Checks if path is a file
static int lua_isfile(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    std::string resolved = resolve_path(path);
    
    if (resolved.empty()) {
        lua_pushboolean(L, false);
        return 1;
    }
    
    lua_pushboolean(L, fs::exists(resolved) && fs::is_regular_file(resolved));
    return 1;
}

// isfolder(path) - Checks if path is a folder
static int lua_isfolder(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    std::string resolved = resolve_path(path);
    
    if (resolved.empty()) {
        lua_pushboolean(L, false);
        return 1;
    }
    
    lua_pushboolean(L, fs::exists(resolved) && fs::is_directory(resolved));
    return 1;
}

// makefolder(path) - Creates a folder
static int lua_makefolder(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    std::string resolved = resolve_path(path);
    
    if (resolved.empty()) {
        luaL_error(L, "Invalid path");
        return 0;
    }
    
    fs::create_directories(resolved);
    return 0;
}

// delfolder(path) - Deletes a folder
static int lua_delfolder(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    std::string resolved = resolve_path(path);
    
    if (resolved.empty()) {
        luaL_error(L, "Invalid path");
        return 0;
    }
    
    if (!fs::exists(resolved)) {
        luaL_error(L, "Folder does not exist: %s", path);
        return 0;
    }
    
    fs::remove_all(resolved);
    return 0;
}

// delfile(path) - Deletes a file
static int lua_delfile(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    std::string resolved = resolve_path(path);
    
    if (resolved.empty()) {
        luaL_error(L, "Invalid path");
        return 0;
    }
    
    if (!fs::exists(resolved)) {
        luaL_error(L, "File does not exist: %s", path);
        return 0;
    }
    
    fs::remove(resolved);
    return 0;
}

// getcustomasset(path) - Gets a custom asset URL
static int lua_getcustomasset(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    std::string resolved = resolve_path(path);
    
    if (resolved.empty() || !fs::exists(resolved)) {
        luaL_error(L, "Asset does not exist: %s", path);
        return 0;
    }
    
    // Return a file:// URL for the asset
    std::string url = "file://" + resolved;
    lua_pushstring(L, url.c_str());
    return 1;
}

// getworkspace() - Gets the workspace path
static int lua_getworkspace(lua_State* L) {
    ensure_directories();
    lua_pushstring(L, g_workspace_path.c_str());
    return 1;
}

// getautoexecutepath() - Gets the autoexecute folder path
static int lua_getautoexecutepath(lua_State* L) {
    ensure_directories();
    lua_pushstring(L, g_autoexecute_path.c_str());
    return 1;
}

// getscriptspath() - Gets the scripts folder path
static int lua_getscriptspath(lua_State* L) {
    ensure_directories();
    lua_pushstring(L, g_scripts_path.c_str());
    return 1;
}

// getbasepath() - Gets the base Xoron folder path
static int lua_getbasepath(lua_State* L) {
    ensure_directories();
    lua_pushstring(L, g_base_path.c_str());
    return 1;
}

// movefile(from, to) - Moves a file
static int lua_movefile(lua_State* L) {
    const char* from_path = luaL_checkstring(L, 1);
    const char* to_path = luaL_checkstring(L, 2);
    
    std::string resolved_from = resolve_path(from_path);
    std::string resolved_to = resolve_path(to_path);
    
    if (resolved_from.empty() || resolved_to.empty()) {
        luaL_error(L, "Invalid path");
        return 0;
    }
    
    if (!fs::exists(resolved_from)) {
        luaL_error(L, "Source file does not exist: %s", from_path);
        return 0;
    }
    
    // Create parent directories if needed
    fs::path to_p(resolved_to);
    if (to_p.has_parent_path()) {
        std::error_code ec;
        fs::create_directories(to_p.parent_path(), ec);
    }
    
    std::error_code ec;
    fs::rename(resolved_from, resolved_to, ec);
    
    if (ec) {
        // If rename fails (cross-device), try copy + delete
        fs::copy(resolved_from, resolved_to, fs::copy_options::overwrite_existing, ec);
        if (!ec) {
            fs::remove(resolved_from, ec);
        }
    }
    
    if (ec) {
        luaL_error(L, "Failed to move file: %s", ec.message().c_str());
        return 0;
    }
    
    return 0;
}

// copyfile(from, to) - Copies a file
static int lua_copyfile(lua_State* L) {
    const char* from_path = luaL_checkstring(L, 1);
    const char* to_path = luaL_checkstring(L, 2);
    
    std::string resolved_from = resolve_path(from_path);
    std::string resolved_to = resolve_path(to_path);
    
    if (resolved_from.empty() || resolved_to.empty()) {
        luaL_error(L, "Invalid path");
        return 0;
    }
    
    if (!fs::exists(resolved_from)) {
        luaL_error(L, "Source file does not exist: %s", from_path);
        return 0;
    }
    
    // Create parent directories if needed
    fs::path to_p(resolved_to);
    if (to_p.has_parent_path()) {
        std::error_code ec;
        fs::create_directories(to_p.parent_path(), ec);
    }
    
    std::error_code ec;
    fs::copy(resolved_from, resolved_to, fs::copy_options::overwrite_existing, ec);
    
    if (ec) {
        luaL_error(L, "Failed to copy file: %s", ec.message().c_str());
        return 0;
    }
    
    return 0;
}

// getfilesize(path) - Gets file size in bytes
static int lua_getfilesize(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    std::string resolved = resolve_path(path);
    
    if (resolved.empty() || !fs::exists(resolved)) {
        lua_pushinteger(L, 0);
        return 1;
    }
    
    std::error_code ec;
    auto size = fs::file_size(resolved, ec);
    
    if (ec) {
        lua_pushinteger(L, 0);
    } else {
        lua_pushinteger(L, (lua_Integer)size);
    }
    return 1;
}

// listfolders(path) - Lists only folders in directory
static int lua_listfolders(lua_State* L) {
    const char* path = luaL_optstring(L, 1, "");
    std::string resolved = resolve_path(path);
    
    if (resolved.empty()) {
        luaL_error(L, "Invalid path");
        return 0;
    }
    
    if (!fs::exists(resolved) || !fs::is_directory(resolved)) {
        luaL_error(L, "Directory does not exist: %s", path);
        return 0;
    }
    
    lua_newtable(L);
    int idx = 1;
    
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(resolved, ec)) {
        if (entry.is_directory()) {
            std::string name = entry.path().filename().string();
            std::string relative = std::string(path);
            if (!relative.empty() && relative.back() != '/') {
                relative += "/";
            }
            relative += name;
            
            lua_pushstring(L, relative.c_str());
            lua_rawseti(L, -2, idx++);
        }
    }
    
    return 1;
}

// getautoexecutescripts() - Gets list of scripts in autoexecute folder
static int lua_getautoexecutescripts(lua_State* L) {
    ensure_directories();
    
    lua_newtable(L);
    int idx = 1;
    
    std::error_code ec;
    if (fs::exists(g_autoexecute_path, ec) && fs::is_directory(g_autoexecute_path, ec)) {
        for (const auto& entry : fs::directory_iterator(g_autoexecute_path, ec)) {
            if (entry.is_regular_file()) {
                std::string ext = entry.path().extension().string();
                // Only include Lua files
                if (ext == ".lua" || ext == ".luau" || ext == ".txt") {
                    lua_pushstring(L, entry.path().filename().string().c_str());
                    lua_rawseti(L, -2, idx++);
                }
            }
        }
    }
    
    return 1;
}

// getsavedscripts() - Gets list of scripts in scripts folder
static int lua_getsavedscripts(lua_State* L) {
    ensure_directories();
    
    lua_newtable(L);
    int idx = 1;
    
    std::error_code ec;
    if (fs::exists(g_scripts_path, ec) && fs::is_directory(g_scripts_path, ec)) {
        for (const auto& entry : fs::directory_iterator(g_scripts_path, ec)) {
            if (entry.is_regular_file()) {
                std::string ext = entry.path().extension().string();
                if (ext == ".lua" || ext == ".luau" || ext == ".txt") {
                    lua_pushstring(L, entry.path().filename().string().c_str());
                    lua_rawseti(L, -2, idx++);
                }
            }
        }
    }
    
    return 1;
}

// savescript(name, content) - Saves a script to the scripts folder
static int lua_savescript(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    size_t len;
    const char* content = luaL_checklstring(L, 2, &len);
    
    ensure_directories();
    
    std::string filename = name;
    // Add .lua extension if not present
    if (filename.find('.') == std::string::npos) {
        filename += ".lua";
    }
    
    // Sanitize filename
    for (char& c : filename) {
        if (c == '/' || c == '\\' || c == ':' || c == '*' || 
            c == '?' || c == '"' || c == '<' || c == '>' || c == '|') {
            c = '_';
        }
    }
    
    std::string filepath = g_scripts_path + "/" + filename;
    
    std::ofstream file(filepath, std::ios::binary);
    if (!file) {
        luaL_error(L, "Unable to save script: %s", name);
        return 0;
    }
    
    file.write(content, len);
    lua_pushboolean(L, true);
    return 1;
}

// loadscript(name) - Loads a script from the scripts folder
static int lua_loadscript(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    
    ensure_directories();
    
    std::string filename = name;
    std::string filepath = g_scripts_path + "/" + filename;
    
    // Try with .lua extension if file doesn't exist
    if (!fs::exists(filepath)) {
        filepath = g_scripts_path + "/" + filename + ".lua";
    }
    
    if (!fs::exists(filepath)) {
        lua_pushnil(L);
        lua_pushstring(L, "Script not found");
        return 2;
    }
    
    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        lua_pushnil(L);
        lua_pushstring(L, "Unable to read script");
        return 2;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    lua_pushlstring(L, content.c_str(), content.size());
    return 1;
}

// deletescript(name) - Deletes a script from the scripts folder
static int lua_deletescript(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    
    ensure_directories();
    
    std::string filename = name;
    std::string filepath = g_scripts_path + "/" + filename;
    
    if (!fs::exists(filepath)) {
        filepath = g_scripts_path + "/" + filename + ".lua";
    }
    
    if (!fs::exists(filepath)) {
        luaL_error(L, "Script not found: %s", name);
        return 0;
    }
    
    std::error_code ec;
    fs::remove(filepath, ec);
    
    if (ec) {
        luaL_error(L, "Failed to delete script: %s", ec.message().c_str());
        return 0;
    }
    
    return 0;
}

// runautoexecute() - Runs all scripts in the autoexecute folder
static int lua_runautoexecute(lua_State* L) {
    ensure_directories();
    
    std::error_code ec;
    if (!fs::exists(g_autoexecute_path, ec) || !fs::is_directory(g_autoexecute_path, ec)) {
        lua_pushinteger(L, 0);
        return 1;
    }
    
    // Collect scripts first to avoid iterator invalidation
    std::vector<std::string> scripts;
    for (const auto& entry : fs::directory_iterator(g_autoexecute_path, ec)) {
        if (entry.is_regular_file()) {
            std::string ext = entry.path().extension().string();
            if (ext == ".lua" || ext == ".luau" || ext == ".txt") {
                scripts.push_back(entry.path().string());
            }
        }
    }
    
    // Sort scripts alphabetically for consistent execution order
    std::sort(scripts.begin(), scripts.end());
    
    int executed = 0;
    for (const auto& script_path : scripts) {
        std::ifstream file(script_path, std::ios::binary);
        if (!file) continue;
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string source = buffer.str();
        
        // Compile the script
        std::string bytecode = Luau::compile(source);
        
        if (bytecode.empty() || bytecode[0] == 0) {
            // Compilation error - skip this script
            continue;
        }
        
        // Get script name for error reporting
        std::string name = fs::path(script_path).filename().string();
        
        // Load and execute
        int result = luau_load(L, name.c_str(), bytecode.data(), bytecode.size(), 0);
        if (result == 0) {
            // Execute with pcall to catch errors
            if (lua_pcall(L, 0, 0, 0) == 0) {
                executed++;
            } else {
                // Pop error message
                lua_pop(L, 1);
            }
        } else {
            // Pop error message
            lua_pop(L, 1);
        }
    }
    
    lua_pushinteger(L, executed);
    return 1;
}

// Register all filesystem functions
void xoron_register_filesystem(lua_State* L) {
    // Initialize directories on registration
    ensure_directories();
    
    // Basic file operations
    lua_pushcfunction(L, lua_readfile, "readfile");
    lua_setglobal(L, "readfile");
    
    lua_pushcfunction(L, lua_writefile, "writefile");
    lua_setglobal(L, "writefile");
    
    lua_pushcfunction(L, lua_appendfile, "appendfile");
    lua_setglobal(L, "appendfile");
    
    lua_pushcfunction(L, lua_loadfile_custom, "loadfile");
    lua_setglobal(L, "loadfile");
    
    lua_pushcfunction(L, lua_dofile_custom, "dofile");
    lua_setglobal(L, "dofile");
    
    // Directory operations
    lua_pushcfunction(L, lua_listfiles, "listfiles");
    lua_setglobal(L, "listfiles");
    
    lua_pushcfunction(L, lua_listfolders, "listfolders");
    lua_setglobal(L, "listfolders");
    
    lua_pushcfunction(L, lua_isfile, "isfile");
    lua_setglobal(L, "isfile");
    
    lua_pushcfunction(L, lua_isfolder, "isfolder");
    lua_setglobal(L, "isfolder");
    
    lua_pushcfunction(L, lua_makefolder, "makefolder");
    lua_setglobal(L, "makefolder");
    
    lua_pushcfunction(L, lua_delfolder, "delfolder");
    lua_setglobal(L, "delfolder");
    
    lua_pushcfunction(L, lua_delfile, "delfile");
    lua_setglobal(L, "delfile");
    
    // File manipulation
    lua_pushcfunction(L, lua_movefile, "movefile");
    lua_setglobal(L, "movefile");
    
    lua_pushcfunction(L, lua_copyfile, "copyfile");
    lua_setglobal(L, "copyfile");
    
    lua_pushcfunction(L, lua_getfilesize, "getfilesize");
    lua_setglobal(L, "getfilesize");
    
    // Asset functions
    lua_pushcfunction(L, lua_getcustomasset, "getcustomasset");
    lua_setglobal(L, "getcustomasset");
    lua_pushcfunction(L, lua_getcustomasset, "getsynasset");
    lua_setglobal(L, "getsynasset");
    
    // Path getters
    lua_pushcfunction(L, lua_getworkspace, "getworkspace");
    lua_setglobal(L, "getworkspace");
    
    lua_pushcfunction(L, lua_getautoexecutepath, "getautoexecutepath");
    lua_setglobal(L, "getautoexecutepath");
    
    lua_pushcfunction(L, lua_getscriptspath, "getscriptspath");
    lua_setglobal(L, "getscriptspath");
    
    lua_pushcfunction(L, lua_getbasepath, "getbasepath");
    lua_setglobal(L, "getbasepath");
    
    // Script management
    lua_pushcfunction(L, lua_getautoexecutescripts, "getautoexecutescripts");
    lua_setglobal(L, "getautoexecutescripts");
    
    lua_pushcfunction(L, lua_getsavedscripts, "getsavedscripts");
    lua_setglobal(L, "getsavedscripts");
    
    lua_pushcfunction(L, lua_savescript, "savescript");
    lua_setglobal(L, "savescript");
    
    lua_pushcfunction(L, lua_loadscript, "loadscript");
    lua_setglobal(L, "loadscript");
    
    lua_pushcfunction(L, lua_deletescript, "deletescript");
    lua_setglobal(L, "deletescript");
    
    // Autoexecute
    lua_pushcfunction(L, lua_runautoexecute, "runautoexecute");
    lua_setglobal(L, "runautoexecute");
}

// C API for workspace path
extern "C" {

const char* xoron_get_workspace(void) {
    ensure_directories();
    return g_workspace_path.c_str();
}

void xoron_set_workspace(const char* path) {
    if (path) {
        std::lock_guard<std::mutex> lock(g_fs_mutex);
        g_workspace_path = path;
        std::error_code ec;
        if (!fs::exists(g_workspace_path, ec)) {
            fs::create_directories(g_workspace_path, ec);
        }
    }
}

const char* xoron_get_autoexecute_path(void) {
    ensure_directories();
    return g_autoexecute_path.c_str();
}

const char* xoron_get_scripts_path(void) {
    ensure_directories();
    return g_scripts_path.c_str();
}

}
