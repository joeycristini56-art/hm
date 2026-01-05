/*
 * xoron_drawing.cpp - Drawing library for executor
 * Provides: Drawing.new, Line, Circle, Square, Text, Triangle, Quad, Image
 */

#include "xoron.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <cmath>

#include "lua.h"
#include "lualib.h"

extern void xoron_set_error(const char* fmt, ...);

// Drawing object types
enum DrawingType {
    DRAWING_LINE = 0,
    DRAWING_CIRCLE,
    DRAWING_SQUARE,
    DRAWING_TEXT,
    DRAWING_TRIANGLE,
    DRAWING_QUAD,
    DRAWING_IMAGE
};

// Color structure
struct Color3 {
    float r, g, b;
    Color3() : r(1), g(1), b(1) {}
    Color3(float r_, float g_, float b_) : r(r_), g(g_), b(b_) {}
};

// Vector2 structure
struct Vector2 {
    float x, y;
    Vector2() : x(0), y(0) {}
    Vector2(float x_, float y_) : x(x_), y(y_) {}
};

// Base drawing object
struct DrawingObject {
    DrawingType type;
    bool visible;
    float transparency;
    Color3 color;
    int zindex;
    uint32_t id;
    
    // Type-specific properties
    Vector2 from, to;           // Line
    Vector2 position;           // Circle, Square, Text, Image
    float radius;               // Circle
    Vector2 size;               // Square, Image
    std::string text;           // Text
    float textSize;             // Text
    bool center;                // Text
    bool outline;               // Text
    Color3 outlineColor;        // Text
    bool filled;                // Circle, Square, Triangle, Quad
    float thickness;            // Line, Circle, Square, Triangle, Quad
    Vector2 pointA, pointB, pointC, pointD; // Triangle, Quad
    std::string imageData;      // Image
    float rounding;             // Square
    std::string font;           // Text
    
    DrawingObject() : type(DRAWING_LINE), visible(true), transparency(0), 
                      zindex(0), id(0), radius(0), textSize(16), center(false),
                      outline(false), filled(false), thickness(1), rounding(0) {}
};

// Drawing state
static std::mutex g_drawing_mutex;
static std::unordered_map<uint32_t, DrawingObject*> g_drawings;
static std::atomic<uint32_t> g_next_id{1};
static std::vector<std::string> g_fonts = {"UI", "System", "RobotoMono", "Legacy", "Plex"};

// Metatable name
static const char* DRAWING_MT = "XoronDrawing";

// Get drawing object from userdata
static DrawingObject* get_drawing(lua_State* L, int idx) {
    DrawingObject** ud = (DrawingObject**)luaL_checkudata(L, idx, DRAWING_MT);
    if (!ud || !*ud) {
        luaL_error(L, "Invalid drawing object");
        return nullptr;
    }
    return *ud;
}

// Push Color3 to Lua
static void push_color3(lua_State* L, const Color3& c) {
    lua_newtable(L);
    lua_pushnumber(L, c.r);
    lua_setfield(L, -2, "R");
    lua_pushnumber(L, c.g);
    lua_setfield(L, -2, "G");
    lua_pushnumber(L, c.b);
    lua_setfield(L, -2, "B");
}

// Get Color3 from Lua
static Color3 get_color3(lua_State* L, int idx) {
    Color3 c;
    if (lua_istable(L, idx)) {
        lua_getfield(L, idx, "R");
        if (lua_isnumber(L, -1)) c.r = lua_tonumber(L, -1);
        lua_pop(L, 1);
        
        lua_getfield(L, idx, "G");
        if (lua_isnumber(L, -1)) c.g = lua_tonumber(L, -1);
        lua_pop(L, 1);
        
        lua_getfield(L, idx, "B");
        if (lua_isnumber(L, -1)) c.b = lua_tonumber(L, -1);
        lua_pop(L, 1);
    }
    return c;
}

// Push Vector2 to Lua
static void push_vector2(lua_State* L, const Vector2& v) {
    lua_newtable(L);
    lua_pushnumber(L, v.x);
    lua_setfield(L, -2, "X");
    lua_pushnumber(L, v.y);
    lua_setfield(L, -2, "Y");
}

// Get Vector2 from Lua
static Vector2 get_vector2(lua_State* L, int idx) {
    Vector2 v;
    if (lua_istable(L, idx)) {
        lua_getfield(L, idx, "X");
        if (lua_isnumber(L, -1)) v.x = lua_tonumber(L, -1);
        lua_pop(L, 1);
        
        lua_getfield(L, idx, "Y");
        if (lua_isnumber(L, -1)) v.y = lua_tonumber(L, -1);
        lua_pop(L, 1);
    }
    return v;
}

// Drawing object __index
static int drawing_index(lua_State* L) {
    DrawingObject* obj = get_drawing(L, 1);
    const char* key = luaL_checkstring(L, 2);
    
    if (strcmp(key, "Visible") == 0) {
        lua_pushboolean(L, obj->visible);
    } else if (strcmp(key, "Color") == 0) {
        push_color3(L, obj->color);
    } else if (strcmp(key, "Transparency") == 0) {
        lua_pushnumber(L, obj->transparency);
    } else if (strcmp(key, "ZIndex") == 0) {
        lua_pushinteger(L, obj->zindex);
    } else if (strcmp(key, "From") == 0) {
        push_vector2(L, obj->from);
    } else if (strcmp(key, "To") == 0) {
        push_vector2(L, obj->to);
    } else if (strcmp(key, "Position") == 0) {
        push_vector2(L, obj->position);
    } else if (strcmp(key, "Radius") == 0) {
        lua_pushnumber(L, obj->radius);
    } else if (strcmp(key, "Size") == 0) {
        push_vector2(L, obj->size);
    } else if (strcmp(key, "Text") == 0) {
        lua_pushstring(L, obj->text.c_str());
    } else if (strcmp(key, "TextBounds") == 0) {
        // Calculate text bounds
        Vector2 bounds;
        bounds.x = obj->text.length() * obj->textSize * 0.6f;
        bounds.y = obj->textSize;
        push_vector2(L, bounds);
    } else if (strcmp(key, "TextSize") == 0 || strcmp(key, "Size") == 0) {
        lua_pushnumber(L, obj->textSize);
    } else if (strcmp(key, "Center") == 0) {
        lua_pushboolean(L, obj->center);
    } else if (strcmp(key, "Outline") == 0) {
        lua_pushboolean(L, obj->outline);
    } else if (strcmp(key, "OutlineColor") == 0) {
        push_color3(L, obj->outlineColor);
    } else if (strcmp(key, "Filled") == 0) {
        lua_pushboolean(L, obj->filled);
    } else if (strcmp(key, "Thickness") == 0) {
        lua_pushnumber(L, obj->thickness);
    } else if (strcmp(key, "PointA") == 0) {
        push_vector2(L, obj->pointA);
    } else if (strcmp(key, "PointB") == 0) {
        push_vector2(L, obj->pointB);
    } else if (strcmp(key, "PointC") == 0) {
        push_vector2(L, obj->pointC);
    } else if (strcmp(key, "PointD") == 0) {
        push_vector2(L, obj->pointD);
    } else if (strcmp(key, "Data") == 0) {
        lua_pushstring(L, obj->imageData.c_str());
    } else if (strcmp(key, "Rounding") == 0) {
        lua_pushnumber(L, obj->rounding);
    } else if (strcmp(key, "Font") == 0) {
        lua_pushinteger(L, 0); // Font enum
    } else if (strcmp(key, "Remove") == 0 || strcmp(key, "Destroy") == 0) {
        // Return the remove function
        lua_pushvalue(L, 1);
        lua_pushcclosure(L, [](lua_State* L) -> int {
            DrawingObject* obj = get_drawing(L, lua_upvalueindex(1));
            if (obj) {
                std::lock_guard<std::mutex> lock(g_drawing_mutex);
                g_drawings.erase(obj->id);
                delete obj;
            }
            return 0;
        }, "Remove", 1);
    } else {
        lua_pushnil(L);
    }
    
    return 1;
}

// Drawing object __newindex
static int drawing_newindex(lua_State* L) {
    DrawingObject* obj = get_drawing(L, 1);
    const char* key = luaL_checkstring(L, 2);
    
    if (strcmp(key, "Visible") == 0) {
        obj->visible = lua_toboolean(L, 3);
    } else if (strcmp(key, "Color") == 0) {
        obj->color = get_color3(L, 3);
    } else if (strcmp(key, "Transparency") == 0) {
        obj->transparency = lua_tonumber(L, 3);
    } else if (strcmp(key, "ZIndex") == 0) {
        obj->zindex = lua_tointeger(L, 3);
    } else if (strcmp(key, "From") == 0) {
        obj->from = get_vector2(L, 3);
    } else if (strcmp(key, "To") == 0) {
        obj->to = get_vector2(L, 3);
    } else if (strcmp(key, "Position") == 0) {
        obj->position = get_vector2(L, 3);
    } else if (strcmp(key, "Radius") == 0) {
        obj->radius = lua_tonumber(L, 3);
    } else if (strcmp(key, "Size") == 0) {
        if (lua_istable(L, 3)) {
            obj->size = get_vector2(L, 3);
        } else {
            obj->textSize = lua_tonumber(L, 3);
        }
    } else if (strcmp(key, "Text") == 0) {
        obj->text = luaL_checkstring(L, 3);
    } else if (strcmp(key, "TextSize") == 0) {
        obj->textSize = lua_tonumber(L, 3);
    } else if (strcmp(key, "Center") == 0) {
        obj->center = lua_toboolean(L, 3);
    } else if (strcmp(key, "Outline") == 0) {
        obj->outline = lua_toboolean(L, 3);
    } else if (strcmp(key, "OutlineColor") == 0) {
        obj->outlineColor = get_color3(L, 3);
    } else if (strcmp(key, "Filled") == 0) {
        obj->filled = lua_toboolean(L, 3);
    } else if (strcmp(key, "Thickness") == 0) {
        obj->thickness = lua_tonumber(L, 3);
    } else if (strcmp(key, "PointA") == 0) {
        obj->pointA = get_vector2(L, 3);
    } else if (strcmp(key, "PointB") == 0) {
        obj->pointB = get_vector2(L, 3);
    } else if (strcmp(key, "PointC") == 0) {
        obj->pointC = get_vector2(L, 3);
    } else if (strcmp(key, "PointD") == 0) {
        obj->pointD = get_vector2(L, 3);
    } else if (strcmp(key, "Data") == 0) {
        obj->imageData = luaL_checkstring(L, 3);
    } else if (strcmp(key, "Rounding") == 0) {
        obj->rounding = lua_tonumber(L, 3);
    } else if (strcmp(key, "Font") == 0) {
        // Font enum or index
        if (lua_isnumber(L, 3)) {
            int idx = lua_tointeger(L, 3);
            if (idx >= 0 && idx < (int)g_fonts.size()) {
                obj->font = g_fonts[idx];
            }
        }
    }
    
    return 0;
}

// Drawing object __gc
static int drawing_gc(lua_State* L) {
    DrawingObject** ud = (DrawingObject**)lua_touserdata(L, 1);
    if (ud && *ud) {
        std::lock_guard<std::mutex> lock(g_drawing_mutex);
        g_drawings.erase((*ud)->id);
        delete *ud;
        *ud = nullptr;
    }
    return 0;
}

// Create drawing object
static DrawingObject* create_drawing(lua_State* L, DrawingType type) {
    DrawingObject* obj = new DrawingObject();
    obj->type = type;
    obj->id = g_next_id++;
    
    {
        std::lock_guard<std::mutex> lock(g_drawing_mutex);
        g_drawings[obj->id] = obj;
    }
    
    // Create userdata
    DrawingObject** ud = (DrawingObject**)lua_newuserdata(L, sizeof(DrawingObject*));
    *ud = obj;
    
    // Set metatable
    luaL_getmetatable(L, DRAWING_MT);
    lua_setmetatable(L, -2);
    
    return obj;
}

// Drawing.new(type) - Creates a new drawing object
static int lua_drawing_new(lua_State* L) {
    const char* type_str = luaL_checkstring(L, 1);
    
    DrawingType type;
    if (strcmp(type_str, "Line") == 0) {
        type = DRAWING_LINE;
    } else if (strcmp(type_str, "Circle") == 0) {
        type = DRAWING_CIRCLE;
    } else if (strcmp(type_str, "Square") == 0) {
        type = DRAWING_SQUARE;
    } else if (strcmp(type_str, "Text") == 0) {
        type = DRAWING_TEXT;
    } else if (strcmp(type_str, "Triangle") == 0) {
        type = DRAWING_TRIANGLE;
    } else if (strcmp(type_str, "Quad") == 0) {
        type = DRAWING_QUAD;
    } else if (strcmp(type_str, "Image") == 0) {
        type = DRAWING_IMAGE;
    } else {
        luaL_error(L, "Invalid drawing type: %s", type_str);
        return 0;
    }
    
    create_drawing(L, type);
    return 1;
}

// Drawing.Fonts - Table of available fonts
static int lua_drawing_fonts(lua_State* L) {
    lua_newtable(L);
    for (size_t i = 0; i < g_fonts.size(); i++) {
        lua_pushinteger(L, (int)i);
        lua_setfield(L, -2, g_fonts[i].c_str());
    }
    return 1;
}

// cleardrawcache() - Clears all drawings
static int lua_cleardrawcache(lua_State* L) {
    (void)L;
    
    std::lock_guard<std::mutex> lock(g_drawing_mutex);
    for (auto& pair : g_drawings) {
        delete pair.second;
    }
    g_drawings.clear();
    
    return 0;
}

// Drawing.clear() - Alias for cleardrawcache
static int lua_drawing_clear(lua_State* L) {
    return lua_cleardrawcache(L);
}

// getrenderproperty(obj, property) - Gets a render property
static int lua_getrenderproperty_impl(lua_State* L) {
    DrawingObject* obj = get_drawing(L, 1);
    const char* prop = luaL_checkstring(L, 2);
    
    // Reuse index logic
    lua_pushvalue(L, 1);
    lua_pushstring(L, prop);
    return drawing_index(L);
}

// setrenderproperty(obj, property, value) - Sets a render property
static int lua_setrenderproperty_impl(lua_State* L) {
    // Reuse __newindex
    return drawing_newindex(L);
}

// isrenderobj(obj) - Checks if object is a drawing
static int lua_isrenderobj(lua_State* L) {
    void* ud = lua_touserdata(L, 1);
    if (ud) {
        lua_getmetatable(L, 1);
        luaL_getmetatable(L, DRAWING_MT);
        bool is_drawing = lua_rawequal(L, -1, -2);
        lua_pop(L, 2);
        lua_pushboolean(L, is_drawing);
    } else {
        lua_pushboolean(L, false);
    }
    return 1;
}

// getrenderproperty(obj, property) - Gets a render property
static int lua_getrenderproperty(lua_State* L) {
    DrawingObject* obj = get_drawing(L, 1);
    const char* prop = luaL_checkstring(L, 2);
    (void)obj;
    
    lua_pushvalue(L, 1);
    lua_pushstring(L, prop);
    return drawing_index(L);
}

// setrenderproperty(obj, property, value) - Sets a render property
static int lua_setrenderproperty(lua_State* L) {
    // Reuse __newindex
    return drawing_newindex(L);
}

// getscreensize() - Gets the screen size
static int lua_getscreensize(lua_State* L) {
    // Default to common iPhone landscape resolution
    // In actual use, this would be set by the game integration
    lua_newtable(L);
    lua_pushnumber(L, 844);
    lua_setfield(L, -2, "X");
    lua_pushnumber(L, 390);
    lua_setfield(L, -2, "Y");
    return 1;
}

// Register drawing library
void xoron_register_drawing(lua_State* L) {
    // Create metatable for drawing objects
    luaL_newmetatable(L, DRAWING_MT);
    
    lua_pushcfunction(L, drawing_index, "__index");
    lua_setfield(L, -2, "__index");
    
    lua_pushcfunction(L, drawing_newindex, "__newindex");
    lua_setfield(L, -2, "__newindex");
    
    lua_pushcfunction(L, drawing_gc, "__gc");
    lua_setfield(L, -2, "__gc");
    
    lua_pop(L, 1);
    
    // Create Drawing table
    lua_newtable(L);
    
    lua_pushcfunction(L, lua_drawing_new, "new");
    lua_setfield(L, -2, "new");
    
    lua_pushcfunction(L, lua_drawing_clear, "clear");
    lua_setfield(L, -2, "clear");
    
    // Add Fonts table
    lua_newtable(L);
    for (size_t i = 0; i < g_fonts.size(); i++) {
        lua_pushinteger(L, (int)i);
        lua_setfield(L, -2, g_fonts[i].c_str());
    }
    lua_setfield(L, -2, "Fonts");
    
    lua_setglobal(L, "Drawing");
    
    // Global functions
    lua_pushcfunction(L, lua_cleardrawcache, "cleardrawcache");
    lua_setglobal(L, "cleardrawcache");
    
    lua_pushcfunction(L, lua_isrenderobj, "isrenderobj");
    lua_setglobal(L, "isrenderobj");
    
    lua_pushcfunction(L, lua_getrenderproperty, "getrenderproperty");
    lua_setglobal(L, "getrenderproperty");
    
    lua_pushcfunction(L, lua_setrenderproperty, "setrenderproperty");
    lua_setglobal(L, "setrenderproperty");
    
    lua_pushcfunction(L, lua_getscreensize, "getscreensize");
    lua_setglobal(L, "getscreensize");
}
