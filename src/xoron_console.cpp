/*
 * xoron_console.cpp - Console output functions for executor
 * Provides: rconsole functions, print variants, warn, info, etc.
 * Platforms: iOS 15+ (.dylib) and Android 10+ (.so)
 */

#include "xoron.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
#include <mutex>
#include <queue>
#include <thread>
#include <atomic>
#include <condition_variable>

/* Platform-specific includes */
#if defined(XORON_PLATFORM_IOS) || defined(__APPLE__)
    #include <objc/objc.h>
    #include <objc/runtime.h>
    #include <objc/message.h>
    #define CONSOLE_LOG(...) printf(__VA_ARGS__)
#elif defined(XORON_PLATFORM_ANDROID) || defined(__ANDROID__)
    #include <android/log.h>
    #define CONSOLE_LOG_TAG "XoronConsole"
    #define CONSOLE_LOG(...) __android_log_print(ANDROID_LOG_INFO, CONSOLE_LOG_TAG, __VA_ARGS__)
    #define CONSOLE_LOG_WARN(...) __android_log_print(ANDROID_LOG_WARN, CONSOLE_LOG_TAG, __VA_ARGS__)
    #define CONSOLE_LOG_ERROR(...) __android_log_print(ANDROID_LOG_ERROR, CONSOLE_LOG_TAG, __VA_ARGS__)
#else
    #define CONSOLE_LOG(...) printf(__VA_ARGS__)
#endif

/* Define missing macros for non-Android platforms */
#if !defined(XORON_PLATFORM_ANDROID) && !defined(__ANDROID__)
    #define CONSOLE_LOG_WARN(...) printf(__VA_ARGS__)
    #define CONSOLE_LOG_ERROR(...) fprintf(stderr, __VA_ARGS__)
#endif

#include "lua.h"
#include "lualib.h"

extern void xoron_set_error(const char* fmt, ...);

// Console state
static std::mutex g_console_mutex;
static std::atomic<bool> g_console_created{false};
static std::string g_console_title = "Xoron Console";
static std::vector<std::string> g_console_buffer;
static std::queue<std::string> g_input_queue;
static std::condition_variable g_input_cv;
static std::mutex g_input_mutex;

// Output callback
static xoron_output_fn g_print_callback = nullptr;
static xoron_output_fn g_error_callback = nullptr;
static void* g_callback_userdata = nullptr;

// Console colors
enum ConsoleColor {
    COLOR_DEFAULT = 0,
    COLOR_BLACK = 30,
    COLOR_RED = 31,
    COLOR_GREEN = 32,
    COLOR_YELLOW = 33,
    COLOR_BLUE = 34,
    COLOR_MAGENTA = 35,
    COLOR_CYAN = 36,
    COLOR_WHITE = 37
};

static void console_output(const char* text, ConsoleColor color = COLOR_DEFAULT) {
    std::lock_guard<std::mutex> lock(g_console_mutex);
    
    std::string output;
    if (color != COLOR_DEFAULT) {
        output = "\033[" + std::to_string(color) + "m" + text + "\033[0m";
    } else {
        output = text;
    }
    
    g_console_buffer.push_back(output);
    
    if (g_print_callback) {
        g_print_callback(output.c_str(), g_callback_userdata);
    } else {
#if defined(XORON_PLATFORM_ANDROID) || defined(__ANDROID__)
        /* Use Android logcat for console output */
        switch (color) {
            case COLOR_RED:
                CONSOLE_LOG_ERROR("%s", text);
                break;
            case COLOR_YELLOW:
                CONSOLE_LOG_WARN("%s", text);
                break;
            default:
                CONSOLE_LOG("%s", text);
                break;
        }
#else
        printf("%s\n", output.c_str());
        fflush(stdout);
#endif
    }
}

// rconsolecreate() - Creates a console window
static int lua_rconsolecreate(lua_State* L) {
    (void)L;
    
    if (g_console_created.exchange(true)) {
        return 0; // Already created
    }
    
    {
        std::lock_guard<std::mutex> lock(g_console_mutex);
        g_console_buffer.clear();
    }
    
#ifdef __APPLE__
    // On macOS, we could create an NSWindow for console
    // For now, we use stdout
#endif
    
    console_output("=== Xoron Console ===", COLOR_CYAN);
    return 0;
}

// rconsoledestroy() - Destroys the console window
static int lua_rconsoledestroy(lua_State* L) {
    (void)L;
    
    if (!g_console_created.exchange(false)) {
        return 0; // Not created
    }
    
    std::lock_guard<std::mutex> lock(g_console_mutex);
    g_console_buffer.clear();
    
    return 0;
}

// rconsoleprint(text) - Prints text to console
static int lua_rconsoleprint(lua_State* L) {
    const char* text = luaL_checkstring(L, 1);
    console_output(text);
    return 0;
}

// rconsoleinput() - Gets input from console
static int lua_rconsoleinput(lua_State* L) {
    if (!g_console_created) {
        lua_pushstring(L, "");
        return 1;
    }
    
    // For now, use stdin
    char buffer[4096];
    if (fgets(buffer, sizeof(buffer), stdin)) {
        // Remove newline
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        }
        lua_pushstring(L, buffer);
    } else {
        lua_pushstring(L, "");
    }
    
    return 1;
}

// rconsoleinfo(text) - Prints info message
static int lua_rconsoleinfo(lua_State* L) {
    const char* text = luaL_checkstring(L, 1);
    std::string msg = "[INFO] " + std::string(text);
    console_output(msg.c_str(), COLOR_CYAN);
    return 0;
}

// rconsolewarn(text) - Prints warning message
static int lua_rconsolewarn(lua_State* L) {
    const char* text = luaL_checkstring(L, 1);
    std::string msg = "[WARN] " + std::string(text);
    console_output(msg.c_str(), COLOR_YELLOW);
    return 0;
}

// rconsoleerr(text) - Prints error message
static int lua_rconsoleerr(lua_State* L) {
    const char* text = luaL_checkstring(L, 1);
    std::string msg = "[ERROR] " + std::string(text);
    console_output(msg.c_str(), COLOR_RED);
    return 0;
}

// rconsoleclear() - Clears the console
static int lua_rconsoleclear(lua_State* L) {
    (void)L;
    
    std::lock_guard<std::mutex> lock(g_console_mutex);
    g_console_buffer.clear();
    
    // ANSI clear screen
    printf("\033[2J\033[H");
    fflush(stdout);
    
    return 0;
}

// rconsolename(title) - Sets console title
static int lua_rconsolename(lua_State* L) {
    const char* title = luaL_checkstring(L, 1);
    
    std::lock_guard<std::mutex> lock(g_console_mutex);
    g_console_title = title;
    
    // ANSI set title
    printf("\033]0;%s\007", title);
    fflush(stdout);
    
    return 0;
}

// rconsoleclose() - Alias for rconsoledestroy
static int lua_rconsoleclose(lua_State* L) {
    return lua_rconsoledestroy(L);
}

// printconsole(text, r, g, b) - Prints colored text
static int lua_printconsole(lua_State* L) {
    const char* text = luaL_checkstring(L, 1);
    int r = luaL_optinteger(L, 2, 255);
    int g = luaL_optinteger(L, 3, 255);
    int b = luaL_optinteger(L, 4, 255);
    
    // Convert RGB to closest ANSI color
    ConsoleColor color = COLOR_DEFAULT;
    int brightness = (r + g + b) / 3;
    
    if (brightness < 64) {
        color = COLOR_BLACK;
    } else if (r > g && r > b) {
        color = (r > 200) ? COLOR_RED : COLOR_MAGENTA;
    } else if (g > r && g > b) {
        color = COLOR_GREEN;
    } else if (b > r && b > g) {
        color = (b > 200) ? COLOR_BLUE : COLOR_CYAN;
    } else if (r > 200 && g > 200) {
        color = COLOR_YELLOW;
    } else {
        color = COLOR_WHITE;
    }
    
    console_output(text, color);
    return 0;
}

// warn(...) - Prints warning message
static int lua_warn_func(lua_State* L) {
    std::string output;
    int n = lua_gettop(L);
    
    for (int i = 1; i <= n; i++) {
        size_t len;
        const char* s = luaL_tolstring(L, i, &len);
        if (i > 1) output += "\t";
        if (s) output += s;
        lua_pop(L, 1);
    }
    
    std::string msg = "[WARN] " + output;
    console_output(msg.c_str(), COLOR_YELLOW);
    return 0;
}

// info(...) - Prints info message
static int lua_info_func(lua_State* L) {
    std::string output;
    int n = lua_gettop(L);
    
    for (int i = 1; i <= n; i++) {
        size_t len;
        const char* s = luaL_tolstring(L, i, &len);
        if (i > 1) output += "\t";
        if (s) output += s;
        lua_pop(L, 1);
    }
    
    std::string msg = "[INFO] " + output;
    console_output(msg.c_str(), COLOR_CYAN);
    return 0;
}

// error_print(...) - Prints error message (not lua error)
static int lua_error_print(lua_State* L) {
    std::string output;
    int n = lua_gettop(L);
    
    for (int i = 1; i <= n; i++) {
        size_t len;
        const char* s = luaL_tolstring(L, i, &len);
        if (i > 1) output += "\t";
        if (s) output += s;
        lua_pop(L, 1);
    }
    
    std::string msg = "[ERROR] " + output;
    console_output(msg.c_str(), COLOR_RED);
    return 0;
}

// printidentity() - Prints current thread identity
static int lua_printidentity(lua_State* L) {
    (void)L;
    // Get identity from env module
    console_output("Current identity is 2", COLOR_DEFAULT);
    return 0;
}

// Register console functions
void xoron_register_console(lua_State* L) {
    // rconsole functions
    lua_pushcfunction(L, lua_rconsolecreate, "rconsolecreate");
    lua_setglobal(L, "rconsolecreate");
    lua_pushcfunction(L, lua_rconsolecreate, "consolecreate");
    lua_setglobal(L, "consolecreate");
    
    lua_pushcfunction(L, lua_rconsoledestroy, "rconsoledestroy");
    lua_setglobal(L, "rconsoledestroy");
    lua_pushcfunction(L, lua_rconsoledestroy, "consoledestroy");
    lua_setglobal(L, "consoledestroy");
    
    lua_pushcfunction(L, lua_rconsoleprint, "rconsoleprint");
    lua_setglobal(L, "rconsoleprint");
    lua_pushcfunction(L, lua_rconsoleprint, "consoleprint");
    lua_setglobal(L, "consoleprint");
    
    lua_pushcfunction(L, lua_rconsoleinput, "rconsoleinput");
    lua_setglobal(L, "rconsoleinput");
    lua_pushcfunction(L, lua_rconsoleinput, "consoleinput");
    lua_setglobal(L, "consoleinput");
    
    lua_pushcfunction(L, lua_rconsoleinfo, "rconsoleinfo");
    lua_setglobal(L, "rconsoleinfo");
    lua_pushcfunction(L, lua_rconsoleinfo, "consoleinfo");
    lua_setglobal(L, "consoleinfo");
    
    lua_pushcfunction(L, lua_rconsolewarn, "rconsolewarn");
    lua_setglobal(L, "rconsolewarn");
    lua_pushcfunction(L, lua_rconsolewarn, "consolewarn");
    lua_setglobal(L, "consolewarn");
    
    lua_pushcfunction(L, lua_rconsoleerr, "rconsoleerr");
    lua_setglobal(L, "rconsoleerr");
    lua_pushcfunction(L, lua_rconsoleerr, "consoleerror");
    lua_setglobal(L, "consoleerror");
    
    lua_pushcfunction(L, lua_rconsoleclear, "rconsoleclear");
    lua_setglobal(L, "rconsoleclear");
    lua_pushcfunction(L, lua_rconsoleclear, "consoleclear");
    lua_setglobal(L, "consoleclear");
    
    lua_pushcfunction(L, lua_rconsolename, "rconsolename");
    lua_setglobal(L, "rconsolename");
    lua_pushcfunction(L, lua_rconsolename, "rconsoletitle");
    lua_setglobal(L, "rconsoletitle");
    lua_pushcfunction(L, lua_rconsolename, "consolesettitle");
    lua_setglobal(L, "consolesettitle");
    
    lua_pushcfunction(L, lua_rconsoleclose, "rconsoleclose");
    lua_setglobal(L, "rconsoleclose");
    
    // Print variants
    lua_pushcfunction(L, lua_printconsole, "printconsole");
    lua_setglobal(L, "printconsole");
    
    lua_pushcfunction(L, lua_warn_func, "warn");
    lua_setglobal(L, "warn");
    
    lua_pushcfunction(L, lua_info_func, "info");
    lua_setglobal(L, "info");
    
    lua_pushcfunction(L, lua_error_print, "printerror");
    lua_setglobal(L, "printerror");
    
    lua_pushcfunction(L, lua_printidentity, "printidentity");
    lua_setglobal(L, "printidentity");
}

// C API for console callbacks
extern "C" {

void xoron_set_console_callbacks(xoron_output_fn print_fn, xoron_output_fn error_fn, void* ud) {
    std::lock_guard<std::mutex> lock(g_console_mutex);
    g_print_callback = print_fn;
    g_error_callback = error_fn;
    g_callback_userdata = ud;
}

void xoron_console_print(const char* text) {
    console_output(text);
}

void xoron_console_warn(const char* text) {
    std::string msg = "[WARN] " + std::string(text);
    console_output(msg.c_str(), COLOR_YELLOW);
}

void xoron_console_error(const char* text) {
    std::string msg = "[ERROR] " + std::string(text);
    console_output(msg.c_str(), COLOR_RED);
}

}
