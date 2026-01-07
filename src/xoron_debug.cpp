/*
 * xoron_debug.cpp - Full Debug library for executor
 * Provides: debug.getinfo, debug.getupvalue, debug.setupvalue, debug.getconstant,
 *           debug.setconstant, debug.getconstants, debug.getproto, debug.getprotos,
 *           debug.getstack, debug.setstack, and more
 * 
 * This implementation uses the Luau public API where possible and accesses
 * VM internals through the proper headers when needed.
 */

#include "xoron.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>

#include "lua.h"
#include "lualib.h"

// Include Luau internal headers for VM access
// These are needed for getconstant, getproto, etc.
#include "lstate.h"
#include "lobject.h"
#include "lfunc.h"
#include "lgc.h"
#include "lapi.h"

extern void xoron_set_error(const char* fmt, ...);

// Get the Proto from a Lua function at stack index
static Proto* getproto_at(lua_State* L, int idx) {
    const TValue* o = luaA_toobject(L, idx);
    if (!o || !ttisfunction(o)) return nullptr;
    Closure* cl = clvalue(o);
    if (cl->isC) return nullptr;
    return cl->l.p;
}

// Push a TValue constant to the stack
static void pushconstant(lua_State* L, Proto* p, int idx) {
    if (idx < 0 || idx >= p->sizek) {
        lua_pushnil(L);
        return;
    }
    
    TValue* k = &p->k[idx];
    luaA_pushobject(L, k);
}

// debug.getinfo(func_or_level, what) - Gets debug info about a function
static int lua_debug_getinfo(lua_State* L) {
    lua_Debug ar;
    int arg = 1;
    
    if (lua_isnumber(L, arg)) {
        int level = lua_tointeger(L, arg);
        if (!lua_getinfo(L, level, "nSluf", &ar)) {
            lua_pushnil(L);
            return 1;
        }
    } else if (lua_isfunction(L, arg)) {
        lua_pushvalue(L, arg);
        if (!lua_getinfo(L, -1, "nSluf", &ar)) {
            lua_pop(L, 1);
            lua_pushnil(L);
            return 1;
        }
        lua_pop(L, 1);
    } else {
        luaL_argerror(L, arg, "function or level expected");
        return 0;
    }
    
    const char* what = luaL_optstring(L, arg + 1, "nSluf");
    
    lua_newtable(L);
    
    for (const char* p = what; *p; p++) {
        switch (*p) {
            case 'n':
                lua_pushstring(L, ar.name ? ar.name : "");
                lua_setfield(L, -2, "name");
                lua_pushstring(L, ar.what ? ar.what : "");
                lua_setfield(L, -2, "what");
                break;
            case 'S':
                lua_pushstring(L, ar.source ? ar.source : "");
                lua_setfield(L, -2, "source");
                lua_pushstring(L, ar.short_src);
                lua_setfield(L, -2, "short_src");
                lua_pushinteger(L, ar.linedefined);
                lua_setfield(L, -2, "linedefined");
                break;
            case 'l':
                lua_pushinteger(L, ar.currentline);
                lua_setfield(L, -2, "currentline");
                break;
            case 'u':
                lua_pushinteger(L, ar.nupvals);
                lua_setfield(L, -2, "nups");
                lua_pushinteger(L, ar.nparams);
                lua_setfield(L, -2, "numparams");
                lua_pushboolean(L, ar.isvararg);
                lua_setfield(L, -2, "is_vararg");
                break;
            case 'f':
                // Function is already on stack from getinfo
                break;
        }
    }
    
    return 1;
}

// debug.getupvalue(func, index) - Gets an upvalue from a function
static int lua_debug_getupvalue(lua_State* L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);
    int idx = luaL_checkinteger(L, 2);
    
    const char* name = lua_getupvalue(L, 1, idx);
    if (name == nullptr) {
        lua_pushnil(L);
        return 1;
    }
    
    // Return name and value
    lua_pushstring(L, name);
    lua_insert(L, -2);
    return 2;
}

// debug.setupvalue(func, index, value) - Sets an upvalue on a function
static int lua_debug_setupvalue(lua_State* L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);
    int idx = luaL_checkinteger(L, 2);
    luaL_checkany(L, 3);
    
    lua_pushvalue(L, 3);
    const char* name = lua_setupvalue(L, 1, idx);
    
    if (name == nullptr) {
        lua_pushnil(L);
    } else {
        lua_pushstring(L, name);
    }
    return 1;
}

// debug.getupvalues(func) - Gets all upvalues from a function
static int lua_debug_getupvalues(lua_State* L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);
    
    lua_newtable(L);
    int idx = 1;
    
    while (true) {
        const char* name = lua_getupvalue(L, 1, idx);
        if (name == nullptr) break;
        
        lua_rawseti(L, -2, idx);
        idx++;
    }
    
    return 1;
}

// debug.setstack(level, index, value) - Sets a value on the stack
static int lua_debug_setstack(lua_State* L) {
    int level = luaL_checkinteger(L, 1);
    int idx = luaL_checkinteger(L, 2);
    luaL_checkany(L, 3);
    
    lua_Debug ar;
    if (!lua_getinfo(L, level, "f", &ar)) {
        luaL_error(L, "Invalid level");
        return 0;
    }
    
    lua_pushvalue(L, 3);
    const char* name = lua_setlocal(L, level, idx);
    
    if (name == nullptr) {
        lua_pushnil(L);
    } else {
        lua_pushstring(L, name);
    }
    return 1;
}

// debug.getstack(level, index) - Gets a value from the stack
static int lua_debug_getstack(lua_State* L) {
    int level = luaL_checkinteger(L, 1);
    int idx = luaL_optinteger(L, 2, 0);
    
    lua_Debug ar;
    if (!lua_getinfo(L, level, "f", &ar)) {
        lua_pushnil(L);
        return 1;
    }
    
    if (idx == 0) {
        // Return all locals as a table
        lua_newtable(L);
        int i = 1;
        while (true) {
            const char* name = lua_getlocal(L, level, i);
            if (name == nullptr) break;
            lua_setfield(L, -2, name);
            i++;
        }
        return 1;
    }
    
    const char* name = lua_getlocal(L, level, idx);
    if (name == nullptr) {
        lua_pushnil(L);
        return 1;
    }
    
    return 1;
}

// debug.getconstant(func, index) - Gets a constant from a function
// Accesses the Proto's constant table (k array)
static int lua_debug_getconstant(lua_State* L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);
    int idx = luaL_checkinteger(L, 2);
    
    if (lua_iscfunction(L, 1)) {
        lua_pushnil(L);
        return 1;
    }
    
    Proto* proto = getproto_at(L, 1);
    if (!proto) {
        lua_pushnil(L);
        return 1;
    }
    
    // Check bounds (1-indexed in Lua)
    if (idx < 1 || idx > proto->sizek) {
        lua_pushnil(L);
        return 1;
    }
    
    // Get the constant at index (convert to 0-indexed)
    pushconstant(L, proto, idx - 1);
    return 1;
}

// debug.setconstant(func, index, value) - Sets a constant on a function
static int lua_debug_setconstant(lua_State* L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);
    int idx = luaL_checkinteger(L, 2);
    luaL_checkany(L, 3);
    
    if (lua_iscfunction(L, 1)) {
        lua_pushboolean(L, false);
        return 1;
    }
    
    Proto* proto = getproto_at(L, 1);
    if (!proto) {
        lua_pushboolean(L, false);
        return 1;
    }
    
    // Check bounds
    if (idx < 1 || idx > proto->sizek) {
        lua_pushboolean(L, false);
        return 1;
    }
    
    // Get the constant slot (0-indexed)
    TValue* k = &proto->k[idx - 1];
    
    // Copy the value from stack position 3 to the constant
    const TValue* newval = luaA_toobject(L, 3);
    if (newval) {
        // Direct copy - this modifies the proto's constant table
        *k = *newval;
        lua_pushboolean(L, true);
    } else {
        lua_pushboolean(L, false);
    }
    return 1;
}

// debug.getconstants(func) - Gets all constants from a function
static int lua_debug_getconstants(lua_State* L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);
    
    lua_newtable(L);
    
    if (lua_iscfunction(L, 1)) {
        return 1;
    }
    
    Proto* proto = getproto_at(L, 1);
    if (!proto) {
        return 1;
    }
    
    // Iterate through all constants
    for (int i = 0; i < proto->sizek; i++) {
        pushconstant(L, proto, i);
        lua_rawseti(L, -2, i + 1);
    }
    
    return 1;
}

// Helper to create a closure from a proto
static void pushproto(lua_State* L, Proto* p, Closure* parent) {
    // Create a new Lua closure for this proto
    Closure* cl = luaF_newLclosure(L, p->nups, parent->env, p);
    
    // Initialize upvalues (they'll be nil/closed)
    for (int i = 0; i < p->nups; i++) {
        cl->l.uprefs[i].tt = LUA_TNIL;
    }
    
    // Push the closure
    setclvalue(L, L->top, cl);
    L->top++;
}

// debug.getproto(func, index, activated) - Gets a proto (inner function) from a function
static int lua_debug_getproto(lua_State* L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);
    int idx = luaL_checkinteger(L, 2);
    bool activated = lua_toboolean(L, 3);
    
    if (lua_iscfunction(L, 1)) {
        lua_pushnil(L);
        return 1;
    }
    
    Proto* proto = getproto_at(L, 1);
    if (!proto) {
        lua_pushnil(L);
        return 1;
    }
    
    // Check bounds (1-indexed)
    if (idx < 1 || idx > proto->sizep) {
        lua_pushnil(L);
        return 1;
    }
    
    Proto* innerProto = proto->p[idx - 1];
    if (!innerProto) {
        lua_pushnil(L);
        return 1;
    }
    
    if (activated) {
        // Return an activated closure
        const TValue* o = luaA_toobject(L, 1);
        Closure* parentCl = clvalue(o);
        pushproto(L, innerProto, parentCl);
    } else {
        // Return proto info as a table
        lua_newtable(L);
        lua_pushinteger(L, innerProto->numparams);
        lua_setfield(L, -2, "numparams");
        lua_pushboolean(L, innerProto->is_vararg);
        lua_setfield(L, -2, "is_vararg");
        lua_pushinteger(L, innerProto->sizek);
        lua_setfield(L, -2, "sizek");
        lua_pushinteger(L, innerProto->sizep);
        lua_setfield(L, -2, "sizep");
        lua_pushinteger(L, innerProto->linedefined);
        lua_setfield(L, -2, "linedefined");
        lua_pushinteger(L, innerProto->nups);
        lua_setfield(L, -2, "nups");
    }
    
    return 1;
}

// debug.getprotos(func) - Gets all protos (inner functions) from a function
static int lua_debug_getprotos(lua_State* L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);
    
    lua_newtable(L);
    
    if (lua_iscfunction(L, 1)) {
        return 1;
    }
    
    Proto* proto = getproto_at(L, 1);
    if (!proto) {
        return 1;
    }
    
    const TValue* o = luaA_toobject(L, 1);
    Closure* parentCl = clvalue(o);
    
    // Return activated closures for each inner proto
    for (int i = 0; i < proto->sizep; i++) {
        Proto* innerProto = proto->p[i];
        if (innerProto) {
            pushproto(L, innerProto, parentCl);
            lua_rawseti(L, -2, i + 1);
        }
    }
    
    return 1;
}

// debug.info(func_or_level, what) - Alternative to getinfo with different format
static int lua_debug_info(lua_State* L) {
    lua_Debug ar;
    int arg = 1;
    
    if (lua_isnumber(L, arg)) {
        int level = lua_tointeger(L, arg);
        if (!lua_getinfo(L, level, "nSluf", &ar)) {
            return 0;
        }
    } else if (lua_isfunction(L, arg)) {
        lua_pushvalue(L, arg);
        if (!lua_getinfo(L, -1, "nSluf", &ar)) {
            lua_pop(L, 1);
            return 0;
        }
        lua_pop(L, 1);
    } else {
        return 0;
    }
    
    const char* what = luaL_optstring(L, arg + 1, "slnaf");
    int nresults = 0;
    
    for (const char* p = what; *p; p++) {
        switch (*p) {
            case 's':
                lua_pushstring(L, ar.source ? ar.source : "");
                nresults++;
                break;
            case 'l':
                lua_pushinteger(L, ar.currentline);
                nresults++;
                break;
            case 'n':
                lua_pushstring(L, ar.name ? ar.name : "");
                nresults++;
                break;
            case 'a':
                lua_pushinteger(L, ar.nparams);
                lua_pushboolean(L, ar.isvararg);
                nresults += 2;
                break;
            case 'f':
                // Function would be pushed by getinfo
                break;
        }
    }
    
    return nresults;
}

// debug.setmetatable(obj, mt) - Sets metatable (bypassing __metatable)
static int lua_debug_setmetatable(lua_State* L) {
    luaL_checkany(L, 1);
    if (!lua_isnoneornil(L, 2)) {
        luaL_checktype(L, 2, LUA_TTABLE);
    }
    lua_settop(L, 2);
    lua_setmetatable(L, 1);
    lua_pushvalue(L, 1);
    return 1;
}

// debug.getmetatable(obj) - Gets metatable (bypassing __metatable)
static int lua_debug_getmetatable(lua_State* L) {
    luaL_checkany(L, 1);
    if (!lua_getmetatable(L, 1)) {
        lua_pushnil(L);
    }
    return 1;
}

// debug.getregistry() - Gets the registry table
static int lua_debug_getregistry(lua_State* L) {
    lua_pushvalue(L, LUA_REGISTRYINDEX);
    return 1;
}

// debug.traceback(thread, message, level) - Gets a traceback
static int lua_debug_traceback(lua_State* L) {
    lua_State* L1 = L;
    int arg = 1;
    
    if (lua_isthread(L, 1)) {
        L1 = lua_tothread(L, 1);
        arg++;
    }
    
    const char* msg = lua_tostring(L, arg);
    int level = luaL_optinteger(L, arg + 1, (L == L1) ? 1 : 0);
    
    // Build traceback manually since luaL_traceback doesn't exist in Luau
    std::string traceback;
    if (msg) {
        traceback = msg;
        traceback += "\n";
    }
    traceback += "stack traceback:\n";
    
    lua_Debug ar;
    for (int i = level; lua_getinfo(L1, i, "nSl", &ar); i++) {
        traceback += "\t";
        traceback += ar.short_src;
        traceback += ":";
        if (ar.currentline > 0) {
            traceback += std::to_string(ar.currentline);
            traceback += ":";
        }
        traceback += " in ";
        if (ar.name) {
            traceback += "function '";
            traceback += ar.name;
            traceback += "'";
        } else {
            traceback += "?";
        }
        traceback += "\n";
    }
    
    lua_pushstring(L, traceback.c_str());
    return 1;
}

// debug.profilebegin(label) - Begins a profiling section
static int lua_debug_profilebegin(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    (void)label;
    // Would integrate with profiler
    return 0;
}

// debug.profileend() - Ends a profiling section
static int lua_debug_profileend(lua_State* L) {
    (void)L;
    return 0;
}

// debug.resetmemorycategory() - Resets memory category
static int lua_debug_resetmemorycategory(lua_State* L) {
    (void)L;
    return 0;
}

// debug.setmemorycategory(category) - Sets memory category
static int lua_debug_setmemorycategory(lua_State* L) {
    const char* category = luaL_checkstring(L, 1);
    (void)category;
    return 0;
}

// Register debug library
void xoron_register_debug(lua_State* L) {
    // Get existing debug table or create new one
    lua_getglobal(L, "debug");
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        lua_newtable(L);
    }
    
    lua_pushcfunction(L, lua_debug_getinfo, "getinfo");
    lua_setfield(L, -2, "getinfo");
    
    lua_pushcfunction(L, lua_debug_getupvalue, "getupvalue");
    lua_setfield(L, -2, "getupvalue");
    
    lua_pushcfunction(L, lua_debug_setupvalue, "setupvalue");
    lua_setfield(L, -2, "setupvalue");
    
    lua_pushcfunction(L, lua_debug_getupvalues, "getupvalues");
    lua_setfield(L, -2, "getupvalues");
    
    lua_pushcfunction(L, lua_debug_setstack, "setstack");
    lua_setfield(L, -2, "setstack");
    
    lua_pushcfunction(L, lua_debug_getstack, "getstack");
    lua_setfield(L, -2, "getstack");
    
    lua_pushcfunction(L, lua_debug_getconstant, "getconstant");
    lua_setfield(L, -2, "getconstant");
    
    lua_pushcfunction(L, lua_debug_setconstant, "setconstant");
    lua_setfield(L, -2, "setconstant");
    
    lua_pushcfunction(L, lua_debug_getconstants, "getconstants");
    lua_setfield(L, -2, "getconstants");
    
    lua_pushcfunction(L, lua_debug_getproto, "getproto");
    lua_setfield(L, -2, "getproto");
    
    lua_pushcfunction(L, lua_debug_getprotos, "getprotos");
    lua_setfield(L, -2, "getprotos");
    
    lua_pushcfunction(L, lua_debug_setmetatable, "setmetatable");
    lua_setfield(L, -2, "setmetatable");
    
    lua_pushcfunction(L, lua_debug_getmetatable, "getmetatable");
    lua_setfield(L, -2, "getmetatable");
    
    lua_pushcfunction(L, lua_debug_getregistry, "getregistry");
    lua_setfield(L, -2, "getregistry");
    
    lua_pushcfunction(L, lua_debug_traceback, "traceback");
    lua_setfield(L, -2, "traceback");
    
    lua_pushcfunction(L, lua_debug_profilebegin, "profilebegin");
    lua_setfield(L, -2, "profilebegin");
    
    lua_pushcfunction(L, lua_debug_profileend, "profileend");
    lua_setfield(L, -2, "profileend");
    
    lua_pushcfunction(L, lua_debug_resetmemorycategory, "resetmemorycategory");
    lua_setfield(L, -2, "resetmemorycategory");
    
    lua_pushcfunction(L, lua_debug_setmemorycategory, "setmemorycategory");
    lua_setfield(L, -2, "setmemorycategory");
    
    lua_pushcfunction(L, lua_debug_info, "info");
    lua_setfield(L, -2, "info");
    
    lua_setglobal(L, "debug");
}
