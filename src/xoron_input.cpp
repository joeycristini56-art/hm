/*
 * xoron_input.cpp - Input library for executor
 * Provides: mouse/keyboard input functions, keypress detection
 * 
 * On iOS, input simulation requires integration with the game's input system.
 * The functions track state locally and can be connected to actual input
 * handlers when injected into the game process.
 */

#include "xoron.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <mutex>

#include "lua.h"
#include "lualib.h"

extern void xoron_set_error(const char* fmt, ...);

// Input state tracking
static std::mutex g_input_mutex;
static std::unordered_set<int> g_pressed_keys;
static std::unordered_set<int> g_pressed_mouse;
static float g_mouse_x = 0;
static float g_mouse_y = 0;
static float g_scroll_delta = 0;

// Key code mapping (Roblox Enum.KeyCode values)
static std::unordered_map<std::string, int> g_keycode_map = {
    {"Unknown", 0},
    {"Backspace", 8},
    {"Tab", 9},
    {"Clear", 12},
    {"Return", 13},
    {"Pause", 19},
    {"Escape", 27},
    {"Space", 32},
    {"QuotedDouble", 34},
    {"Hash", 35},
    {"Dollar", 36},
    {"Percent", 37},
    {"Ampersand", 38},
    {"Quote", 39},
    {"LeftParenthesis", 40},
    {"RightParenthesis", 41},
    {"Asterisk", 42},
    {"Plus", 43},
    {"Comma", 44},
    {"Minus", 45},
    {"Period", 46},
    {"Slash", 47},
    {"Zero", 48},
    {"One", 49},
    {"Two", 50},
    {"Three", 51},
    {"Four", 52},
    {"Five", 53},
    {"Six", 54},
    {"Seven", 55},
    {"Eight", 56},
    {"Nine", 57},
    {"Colon", 58},
    {"Semicolon", 59},
    {"LessThan", 60},
    {"Equals", 61},
    {"GreaterThan", 62},
    {"Question", 63},
    {"At", 64},
    {"A", 97},
    {"B", 98},
    {"C", 99},
    {"D", 100},
    {"E", 101},
    {"F", 102},
    {"G", 103},
    {"H", 104},
    {"I", 105},
    {"J", 106},
    {"K", 107},
    {"L", 108},
    {"M", 109},
    {"N", 110},
    {"O", 111},
    {"P", 112},
    {"Q", 113},
    {"R", 114},
    {"S", 115},
    {"T", 116},
    {"U", 117},
    {"V", 118},
    {"W", 119},
    {"X", 120},
    {"Y", 121},
    {"Z", 122},
    {"LeftBracket", 91},
    {"BackSlash", 92},
    {"RightBracket", 93},
    {"Caret", 94},
    {"Underscore", 95},
    {"Backquote", 96},
    {"Delete", 127},
    {"KeypadZero", 256},
    {"KeypadOne", 257},
    {"KeypadTwo", 258},
    {"KeypadThree", 259},
    {"KeypadFour", 260},
    {"KeypadFive", 261},
    {"KeypadSix", 262},
    {"KeypadSeven", 263},
    {"KeypadEight", 264},
    {"KeypadNine", 265},
    {"KeypadPeriod", 266},
    {"KeypadDivide", 267},
    {"KeypadMultiply", 268},
    {"KeypadMinus", 269},
    {"KeypadPlus", 270},
    {"KeypadEnter", 271},
    {"KeypadEquals", 272},
    {"Up", 273},
    {"Down", 274},
    {"Right", 275},
    {"Left", 276},
    {"Insert", 277},
    {"Home", 278},
    {"End", 279},
    {"PageUp", 280},
    {"PageDown", 281},
    {"F1", 282},
    {"F2", 283},
    {"F3", 284},
    {"F4", 285},
    {"F5", 286},
    {"F6", 287},
    {"F7", 288},
    {"F8", 289},
    {"F9", 290},
    {"F10", 291},
    {"F11", 292},
    {"F12", 293},
    {"NumLock", 300},
    {"CapsLock", 301},
    {"ScrollLock", 302},
    {"RightShift", 303},
    {"LeftShift", 304},
    {"RightControl", 305},
    {"LeftControl", 306},
    {"RightAlt", 307},
    {"LeftAlt", 308},
    {"RightMeta", 309},
    {"LeftMeta", 310},
    {"LeftSuper", 311},
    {"RightSuper", 312},
    {"Mode", 313},
    {"Compose", 314},
    {"Help", 315},
    {"Print", 316},
    {"SysReq", 317},
    {"Break", 318},
    {"Menu", 319},
    {"Power", 320},
    {"Euro", 321},
    {"Undo", 322},
};

// Get keycode from string or number
static int get_keycode(lua_State* L, int idx) {
    if (lua_isnumber(L, idx)) {
        return lua_tointeger(L, idx);
    } else if (lua_isstring(L, idx)) {
        const char* name = lua_tostring(L, idx);
        auto it = g_keycode_map.find(name);
        if (it != g_keycode_map.end()) {
            return it->second;
        }
    } else if (lua_istable(L, idx)) {
        // Handle Enum.KeyCode userdata/table
        lua_getfield(L, idx, "Value");
        if (lua_isnumber(L, -1)) {
            int val = lua_tointeger(L, -1);
            lua_pop(L, 1);
            return val;
        }
        lua_pop(L, 1);
        
        lua_getfield(L, idx, "Name");
        if (lua_isstring(L, -1)) {
            const char* name = lua_tostring(L, -1);
            lua_pop(L, 1);
            auto it = g_keycode_map.find(name);
            if (it != g_keycode_map.end()) {
                return it->second;
            }
        }
        lua_pop(L, 1);
    }
    return 0;
}

// iskeypressed(keycode) - Check if a key is currently pressed
static int lua_iskeypressed(lua_State* L) {
    int keycode = get_keycode(L, 1);
    
    std::lock_guard<std::mutex> lock(g_input_mutex);
    lua_pushboolean(L, g_pressed_keys.count(keycode) > 0);
    return 1;
}

// iskeydown(keycode) - Alias for iskeypressed
static int lua_iskeydown(lua_State* L) {
    return lua_iskeypressed(L);
}

// mouse1click() - Simulate left mouse click
// Performs a press and release sequence for left mouse button
static int lua_mouse1click(lua_State* L) {
    (void)L;
    std::lock_guard<std::mutex> lock(g_input_mutex);
    // Simulate click by briefly pressing and releasing
    g_pressed_mouse.insert(1);
    // In actual game integration, this would trigger the input event
    // The release happens immediately for a click
    g_pressed_mouse.erase(1);
    return 0;
}

// mouse1press() - Simulate left mouse press
static int lua_mouse1press(lua_State* L) {
    (void)L;
    std::lock_guard<std::mutex> lock(g_input_mutex);
    g_pressed_mouse.insert(1);
    return 0;
}

// mouse1release() - Simulate left mouse release
static int lua_mouse1release(lua_State* L) {
    (void)L;
    std::lock_guard<std::mutex> lock(g_input_mutex);
    g_pressed_mouse.erase(1);
    return 0;
}

// mouse2click() - Simulate right mouse click
static int lua_mouse2click(lua_State* L) {
    (void)L;
    return 0;
}

// mouse2press() - Simulate right mouse press
static int lua_mouse2press(lua_State* L) {
    (void)L;
    std::lock_guard<std::mutex> lock(g_input_mutex);
    g_pressed_mouse.insert(2);
    return 0;
}

// mouse2release() - Simulate right mouse release
static int lua_mouse2release(lua_State* L) {
    (void)L;
    std::lock_guard<std::mutex> lock(g_input_mutex);
    g_pressed_mouse.erase(2);
    return 0;
}

// mousemoverel(dx, dy) - Move mouse relative
static int lua_mousemoverel(lua_State* L) {
    float dx = (float)luaL_checknumber(L, 1);
    float dy = (float)luaL_checknumber(L, 2);
    
    std::lock_guard<std::mutex> lock(g_input_mutex);
    g_mouse_x += dx;
    g_mouse_y += dy;
    return 0;
}

// mousemoveabs(x, y) - Move mouse absolute
static int lua_mousemoveabs(lua_State* L) {
    float x = (float)luaL_checknumber(L, 1);
    float y = (float)luaL_checknumber(L, 2);
    
    std::lock_guard<std::mutex> lock(g_input_mutex);
    g_mouse_x = x;
    g_mouse_y = y;
    return 0;
}

// mousescroll(delta) - Scroll mouse wheel
static int lua_mousescroll(lua_State* L) {
    float delta = (float)luaL_checknumber(L, 1);
    
    std::lock_guard<std::mutex> lock(g_input_mutex);
    g_scroll_delta = delta;
    return 0;
}

// getmouseposition() - Get current mouse position
static int lua_getmouseposition(lua_State* L) {
    std::lock_guard<std::mutex> lock(g_input_mutex);
    
    lua_newtable(L);
    lua_pushnumber(L, g_mouse_x);
    lua_setfield(L, -2, "X");
    lua_pushnumber(L, g_mouse_y);
    lua_setfield(L, -2, "Y");
    
    return 1;
}

// keypress(keycode) - Simulate key press
static int lua_keypress(lua_State* L) {
    int keycode = get_keycode(L, 1);
    
    std::lock_guard<std::mutex> lock(g_input_mutex);
    g_pressed_keys.insert(keycode);
    return 0;
}

// keyrelease(keycode) - Simulate key release
static int lua_keyrelease(lua_State* L) {
    int keycode = get_keycode(L, 1);
    
    std::lock_guard<std::mutex> lock(g_input_mutex);
    g_pressed_keys.erase(keycode);
    return 0;
}

// keyclick(keycode) - Simulate key click (press + release)
static int lua_keyclick(lua_State* L) {
    int keycode = get_keycode(L, 1);
    
    std::lock_guard<std::mutex> lock(g_input_mutex);
    g_pressed_keys.insert(keycode);
    g_pressed_keys.erase(keycode);
    return 0;
}

// ismousebuttonpressed(button) - Check if mouse button is pressed
static int lua_ismousebuttonpressed(lua_State* L) {
    int button = luaL_checkinteger(L, 1);
    
    std::lock_guard<std::mutex> lock(g_input_mutex);
    lua_pushboolean(L, g_pressed_mouse.count(button) > 0);
    return 1;
}

// Internal: Update input state (called from game integration)
extern "C" void xoron_input_set_key(int keycode, bool pressed) {
    std::lock_guard<std::mutex> lock(g_input_mutex);
    if (pressed) {
        g_pressed_keys.insert(keycode);
    } else {
        g_pressed_keys.erase(keycode);
    }
}

extern "C" void xoron_input_set_mouse(int button, bool pressed) {
    std::lock_guard<std::mutex> lock(g_input_mutex);
    if (pressed) {
        g_pressed_mouse.insert(button);
    } else {
        g_pressed_mouse.erase(button);
    }
}

extern "C" void xoron_input_set_mouse_pos(float x, float y) {
    std::lock_guard<std::mutex> lock(g_input_mutex);
    g_mouse_x = x;
    g_mouse_y = y;
}

// Register input library
void xoron_register_input(lua_State* L) {
    // Key functions
    lua_pushcfunction(L, lua_iskeypressed, "iskeypressed");
    lua_setglobal(L, "iskeypressed");
    
    lua_pushcfunction(L, lua_iskeydown, "iskeydown");
    lua_setglobal(L, "iskeydown");
    
    lua_pushcfunction(L, lua_keypress, "keypress");
    lua_setglobal(L, "keypress");
    
    lua_pushcfunction(L, lua_keyrelease, "keyrelease");
    lua_setglobal(L, "keyrelease");
    
    lua_pushcfunction(L, lua_keyclick, "keyclick");
    lua_setglobal(L, "keyclick");
    
    // Mouse functions
    lua_pushcfunction(L, lua_mouse1click, "mouse1click");
    lua_setglobal(L, "mouse1click");
    
    lua_pushcfunction(L, lua_mouse1press, "mouse1press");
    lua_setglobal(L, "mouse1press");
    
    lua_pushcfunction(L, lua_mouse1release, "mouse1release");
    lua_setglobal(L, "mouse1release");
    
    lua_pushcfunction(L, lua_mouse2click, "mouse2click");
    lua_setglobal(L, "mouse2click");
    
    lua_pushcfunction(L, lua_mouse2press, "mouse2press");
    lua_setglobal(L, "mouse2press");
    
    lua_pushcfunction(L, lua_mouse2release, "mouse2release");
    lua_setglobal(L, "mouse2release");
    
    lua_pushcfunction(L, lua_mousemoverel, "mousemoverel");
    lua_setglobal(L, "mousemoverel");
    
    lua_pushcfunction(L, lua_mousemoveabs, "mousemoveabs");
    lua_setglobal(L, "mousemoveabs");
    
    lua_pushcfunction(L, lua_mousescroll, "mousescroll");
    lua_setglobal(L, "mousescroll");
    
    lua_pushcfunction(L, lua_getmouseposition, "getmouseposition");
    lua_setglobal(L, "getmouseposition");
    
    lua_pushcfunction(L, lua_ismousebuttonpressed, "ismousebuttonpressed");
    lua_setglobal(L, "ismousebuttonpressed");
    
    // Create Input table for compatibility
    lua_newtable(L);
    
    lua_pushcfunction(L, lua_iskeypressed, "IsKeyDown");
    lua_setfield(L, -2, "IsKeyDown");
    
    lua_pushcfunction(L, lua_ismousebuttonpressed, "IsMouseButtonPressed");
    lua_setfield(L, -2, "IsMouseButtonPressed");
    
    lua_pushcfunction(L, lua_getmouseposition, "GetMouseLocation");
    lua_setfield(L, -2, "GetMouseLocation");
    
    lua_setglobal(L, "Input");
}
