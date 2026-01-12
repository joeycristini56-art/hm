/*
 * xoron_cache.cpp - Cache library and instance functions for executor
 * Provides: cache.invalidate, cache.iscached, cache.replace
 * Also includes: decompile, saveinstance, gethiddenproperty, etc.
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
#include <sstream>
#include <fstream>
#include <filesystem>

#include "lua.h"
#include "lualib.h"
#include "luacode.h"  // For luau_compile
#include "Luau/Compiler.h"
#include "Luau/BytecodeBuilder.h"

extern void xoron_set_error(const char* fmt, ...);
namespace fs = std::filesystem;

// Cache storage
static std::mutex g_cache_mutex;
static std::unordered_map<void*, int> g_instance_cache;  // Maps instance pointer to ref

// cache.invalidate(instance) - Invalidates cached instance
static int lua_cache_invalidate(lua_State* L) {
    luaL_checkany(L, 1);
    
    void* ptr = const_cast<void*>(lua_topointer(L, 1));
    if (!ptr) {
        return 0;
    }
    
    std::lock_guard<std::mutex> lock(g_cache_mutex);
    auto it = g_instance_cache.find(ptr);
    if (it != g_instance_cache.end()) {
        lua_unref(L, it->second);
        g_instance_cache.erase(it);
    }
    
    return 0;
}

// cache.iscached(instance) - Checks if instance is cached
static int lua_cache_iscached(lua_State* L) {
    luaL_checkany(L, 1);
    
    void* ptr = const_cast<void*>(lua_topointer(L, 1));
    if (!ptr) {
        lua_pushboolean(L, false);
        return 1;
    }
    
    std::lock_guard<std::mutex> lock(g_cache_mutex);
    lua_pushboolean(L, g_instance_cache.count(ptr) > 0);
    return 1;
}

// cache.replace(instance, replacement) - Replaces cached instance
static int lua_cache_replace(lua_State* L) {
    luaL_checkany(L, 1);
    luaL_checkany(L, 2);
    
    void* ptr = const_cast<void*>(lua_topointer(L, 1));
    if (!ptr) {
        return 0;
    }
    
    std::lock_guard<std::mutex> lock(g_cache_mutex);
    
    // Remove old cache entry
    auto it = g_instance_cache.find(ptr);
    if (it != g_instance_cache.end()) {
        lua_unref(L, it->second);
        g_instance_cache.erase(it);
    }
    
    // Add new cache entry
    lua_pushvalue(L, 2);
    int ref = lua_ref(L, LUA_REGISTRYINDEX);
    g_instance_cache[ptr] = ref;
    
    return 0;
}

// Luau bytecode opcodes for disassembly
static const char* LUAU_OPCODES[] = {
    "NOP", "BREAK", "LOADNIL", "LOADB", "LOADN", "LOADK", "MOVE", "GETGLOBAL",
    "SETGLOBAL", "GETUPVAL", "SETUPVAL", "CLOSEUPVALS", "GETIMPORT", "GETTABLE",
    "SETTABLE", "GETTABLEKS", "SETTABLEKS", "GETTABLEN", "SETTABLEN", "NEWCLOSURE",
    "NAMECALL", "CALL", "RETURN", "JUMP", "JUMPBACK", "JUMPIF", "JUMPIFNOT",
    "JUMPIFEQ", "JUMPIFLE", "JUMPIFLT", "JUMPIFNOTEQ", "JUMPIFNOTLE", "JUMPIFNOTLT",
    "ADD", "SUB", "MUL", "DIV", "MOD", "POW", "ADDK", "SUBK", "MULK", "DIVK",
    "MODK", "POWK", "AND", "OR", "ANDK", "ORK", "CONCAT", "NOT", "MINUS", "LENGTH",
    "NEWTABLE", "DUPTABLE", "SETLIST", "FORNPREP", "FORNLOOP", "FORGLOOP", "FORGPREP_INEXT",
    "FORGLOOP_INEXT", "FORGPREP_NEXT", "FORGLOOP_NEXT", "GETVARARGS", "DUPCLOSURE",
    "PREPVARARGS", "LOADKX", "JUMPX", "FASTCALL", "COVERAGE", "CAPTURE", "JUMPIFEQK",
    "JUMPIFNOTEQK", "FASTCALL1", "FASTCALL2", "FASTCALL2K", "FORGPREP", "JUMPXEQKNIL",
    "JUMPXEQKB", "JUMPXEQKN", "JUMPXEQKS", "IDIV", "IDIVK"
};

// decompile(script) - Disassembles bytecode (full decompilation requires external tools)
static int lua_decompile(lua_State* L) {
    luaL_checkany(L, 1);
    
    std::stringstream result;
    result << "-- Xoron Decompiler Output\n";
    result << "-- Note: Full decompilation requires bytecode analysis\n\n";
    
    // Check if it's a function
    if (lua_isfunction(L, 1)) {
        // Get function info using debug library
        lua_Debug ar;
        lua_pushvalue(L, 1);
        
        if (lua_getinfo(L, 1, "nSluf", &ar)) {
            result << "-- Function Info:\n";
            if (ar.name) {
                result << "-- Name: " << ar.name << "\n";
            }
            if (ar.source) {
                result << "-- Source: " << ar.source << "\n";
            }
            result << "-- Line Defined: " << ar.linedefined << "\n";
            result << "-- Parameters: " << (int)ar.nparams << "\n";
            result << "-- Is Vararg: " << (ar.isvararg ? "yes" : "no") << "\n";
            result << "\n";
            
            // Try to get upvalue names
            int upvalueCount = 0;
            while (lua_getupvalue(L, 1, upvalueCount + 1) != nullptr) {
                lua_pop(L, 1);
                upvalueCount++;
            }
            
            if (upvalueCount > 0) {
                result << "-- Upvalues: " << upvalueCount << "\n";
                for (int i = 1; i <= upvalueCount; i++) {
                    const char* name = lua_getupvalue(L, 1, i);
                    if (name) {
                        result << "--   [" << i << "] " << name << "\n";
                        lua_pop(L, 1);
                    }
                }
                result << "\n";
            }
        }
        
        // Generate pseudo-code based on what we know
        result << "-- Pseudo-code reconstruction:\n";
        result << "local function ";
        if (ar.name) {
            result << ar.name;
        } else {
            result << "anonymous";
        }
        result << "(";
        for (int i = 0; i < ar.nparams; i++) {
            if (i > 0) result << ", ";
            result << "arg" << (i + 1);
        }
        if (ar.isvararg) {
            if (ar.nparams > 0) result << ", ";
            result << "...";
        }
        result << ")\n";
        result << "    -- Function body not accessible\n";
        result << "    -- Use getscriptbytecode for raw bytecode\n";
        result << "end\n";
        
        lua_pushstring(L, result.str().c_str());
        return 1;
    }
    
    // For userdata (script instances), try to get info
    if (lua_isuserdata(L, 1)) {
        result << "-- Script Instance\n";
        result << "-- Source code is not directly accessible\n";
        result << "-- In Roblox, use getscriptbytecode() to get raw bytecode\n\n";
        result << "-- To decompile Roblox scripts:\n";
        result << "-- 1. Get bytecode with getscriptbytecode(script)\n";
        result << "-- 2. Use external decompiler tools\n";
    }
    
    lua_pushstring(L, result.str().c_str());
    return 1;
}

// getscriptbytecode(script) - Gets bytecode of a script
static int lua_getscriptbytecode(lua_State* L) {
    luaL_checkany(L, 1);
    
    // For functions, we can dump them
    if (lua_isfunction(L, 1) && !lua_iscfunction(L, 1)) {
        // Get function info
        lua_Debug ar;
        lua_pushvalue(L, 1);
        if (lua_getinfo(L, 1, "S", &ar)) {
            // Return source info as bytecode isn't directly accessible
            std::string info = "-- Bytecode not directly accessible\n";
            info += "-- Source: ";
            info += ar.source ? ar.source : "unknown";
            lua_pushstring(L, info.c_str());
            return 1;
        }
    }
    
    // For script instances, this would require Roblox integration
    lua_pushnil(L);
    lua_pushstring(L, "Script bytecode requires Roblox integration");
    return 2;
}

// getscripthash(script) - Gets hash of a script
static int lua_getscripthash(lua_State* L) {
    luaL_checkany(L, 1);
    
    // Generate a hash based on the pointer and some additional info
    void* ptr = const_cast<void*>(lua_topointer(L, 1));
    
    // Use a simple hash combining pointer and type
    uint64_t hash = (uint64_t)(uintptr_t)ptr;
    hash ^= (hash >> 33);
    hash *= 0xff51afd7ed558ccdULL;
    hash ^= (hash >> 33);
    hash *= 0xc4ceb9fe1a85ec53ULL;
    hash ^= (hash >> 33);
    
    char hashStr[17];
    snprintf(hashStr, sizeof(hashStr), "%016llx", (unsigned long long)hash);
    
    lua_pushstring(L, hashStr);
    return 1;
}

// getscriptclosure(script) - Gets the closure of a script
static int lua_getscriptclosure(lua_State* L) {
    luaL_checkany(L, 1);
    
    // If it's already a function, return it
    if (lua_isfunction(L, 1)) {
        lua_pushvalue(L, 1);
        return 1;
    }
    
    // For script instances, this would require Roblox integration
    lua_pushnil(L);
    return 1;
}

// Helper to escape XML special characters
static std::string escapeXML(const std::string& str) {
    std::string result;
    result.reserve(str.size() * 1.1);
    for (char c : str) {
        switch (c) {
            case '&': result += "&amp;"; break;
            case '<': result += "&lt;"; break;
            case '>': result += "&gt;"; break;
            case '"': result += "&quot;"; break;
            case '\'': result += "&apos;"; break;
            default: result += c; break;
        }
    }
    return result;
}

// Embedded Lua saveinstance script (loaded on demand)
static const char* SAVEINSTANCE_LUA = R"LUA(
-- Xoron SaveInstance - Embedded Version
local SaveInstance = {}

local function escapeXML(str)
    if type(str) ~= "string" then str = tostring(str) end
    return str:gsub("&", "&amp;"):gsub("<", "&lt;"):gsub(">", "&gt;"):gsub("\"", "&quot;"):gsub("'", "&apos;")
end

local referentCounter = 0
local referentCache = {}

local function getRef(instance)
    if referentCache[instance] then return referentCache[instance] end
    referentCounter = referentCounter + 1
    local ref = "RBX" .. string.format("%08X", referentCounter)
    referentCache[instance] = ref
    return ref
end

local function serializeProperty(name, value)
    local t = typeof(value)
    if t == "string" then
        return string.format('\t\t\t<string name="%s">%s</string>\n', name, escapeXML(value))
    elseif t == "boolean" then
        return string.format('\t\t\t<bool name="%s">%s</bool>\n', name, value and "true" or "false")
    elseif t == "number" then
        if value == math.floor(value) then
            return string.format('\t\t\t<int name="%s">%d</int>\n', name, value)
        end
        return string.format('\t\t\t<double name="%s">%s</double>\n', name, tostring(value))
    elseif t == "Vector3" then
        return string.format('\t\t\t<Vector3 name="%s"><X>%s</X><Y>%s</Y><Z>%s</Z></Vector3>\n',
            name, value.X, value.Y, value.Z)
    elseif t == "CFrame" then
        local x,y,z,r00,r01,r02,r10,r11,r12,r20,r21,r22 = value:GetComponents()
        return string.format('\t\t\t<CoordinateFrame name="%s"><X>%s</X><Y>%s</Y><Z>%s</Z><R00>%s</R00><R01>%s</R01><R02>%s</R02><R10>%s</R10><R11>%s</R11><R12>%s</R12><R20>%s</R20><R21>%s</R21><R22>%s</R22></CoordinateFrame>\n',
            name, x,y,z,r00,r01,r02,r10,r11,r12,r20,r21,r22)
    elseif t == "Color3" then
        local r,g,b = math.floor(value.R*255), math.floor(value.G*255), math.floor(value.B*255)
        return string.format('\t\t\t<Color3uint8 name="%s">%d</Color3uint8>\n', name, r*65536+g*256+b)
    elseif t == "UDim2" then
        return string.format('\t\t\t<UDim2 name="%s"><XS>%s</XS><XO>%d</XO><YS>%s</YS><YO>%d</YO></UDim2>\n',
            name, value.X.Scale, value.X.Offset, value.Y.Scale, value.Y.Offset)
    elseif t == "Instance" then
        return string.format('\t\t\t<Ref name="%s">%s</Ref>\n', name, getRef(value))
    elseif t == "EnumItem" then
        return string.format('\t\t\t<token name="%s">%d</token>\n', name, value.Value)
    end
    return ""
end

local function getProps(inst)
    local props = {Name = inst.Name}
    local common = {"Anchored","CanCollide","Size","CFrame","Position","Color","BrickColor",
        "Material","Transparency","Enabled","Visible","Text","TextColor3","BackgroundColor3",
        "BackgroundTransparency","Value","Brightness","Range"}
    for _,p in ipairs(common) do
        local ok,v = pcall(function() return inst[p] end)
        if ok and v ~= nil then props[p] = v end
    end
    return props
end

local function serializeInstance(inst, opts, depth)
    depth = depth or 1
    local indent = string.rep("\t", depth)
    local out = {}
    
    if opts.IgnoreNotArchivable then
        local ok,arch = pcall(function() return inst.Archivable end)
        if ok and not arch then return "" end
    end
    
    table.insert(out, string.format('%s<Item class="%s" referent="%s">\n', indent, inst.ClassName, getRef(inst)))
    table.insert(out, indent.."\t<Properties>\n")
    
    for name,value in pairs(getProps(inst)) do
        local s = serializeProperty(name, value)
        if s ~= "" then table.insert(out, s) end
    end
    
    if inst:IsA("LuaSourceContainer") then
        local src = ""
        if opts.DecompileScripts and decompile then
            local ok,d = pcall(decompile, inst)
            if ok then src = d end
        else
            local ok,s = pcall(function() return inst.Source end)
            if ok then src = s or "" end
        end
        table.insert(out, string.format('\t\t\t<ProtectedString name="Source"><![CDATA[%s]]></ProtectedString>\n', src))
    end
    
    table.insert(out, indent.."\t</Properties>\n")
    
    for _,child in ipairs(inst:GetChildren()) do
        local c = serializeInstance(child, opts, depth+1)
        if c ~= "" then table.insert(out, c) end
    end
    
    table.insert(out, indent.."</Item>\n")
    return table.concat(out)
end

function SaveInstance.Save(opts)
    opts = opts or {}
    opts.FileName = opts.FileName or "game"
    opts.DecompileScripts = opts.DecompileScripts or false
    opts.IgnoreNotArchivable = opts.IgnoreNotArchivable ~= false
    
    referentCounter = 0
    referentCache = {}
    
    local xml = {'<?xml version="1.0" encoding="utf-8"?>\n',
        '<roblox xmlns:xmime="http://www.w3.org/2005/05/xmlmime" ',
        'xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" ',
        'xsi:noNamespaceSchemaLocation="http://www.roblox.com/roblox.xsd" version="4">\n',
        '\t<Meta name="ExplicitAutoJoints">true</Meta>\n',
        '\t<External>null</External>\n<External>nil</External>\n'}
    
    local toSave = {}
    if opts.Object then
        table.insert(toSave, opts.Object)
    else
        for _,svc in ipairs({"Workspace","Lighting","ReplicatedFirst","ReplicatedStorage",
            "StarterGui","StarterPack","StarterPlayer","Teams","SoundService"}) do
            local ok,s = pcall(function() return game:GetService(svc) end)
            if ok and s then table.insert(toSave, s) end
        end
    end
    
    for _,inst in ipairs(toSave) do
        local s = serializeInstance(inst, opts, 1)
        if s ~= "" then table.insert(xml, s) end
    end
    
    table.insert(xml, '</roblox>\n')
    
    local content = table.concat(xml)
    local filename = opts.FileName
    if not filename:match("%.rbxm?x?$") then filename = filename..".rbxmx" end
    
    if writefile then
        local path = filename
        if getworkspace then path = getworkspace().."/"..filename end
        writefile(path, content)
        return path
    end
    return content
end

return SaveInstance
)LUA";

// saveinstance(options) - Saves game instance to RBXMX format
// This function loads and executes the embedded Lua saveinstance script
static int lua_saveinstance(lua_State* L) {
    // First, try to use the Lua implementation if game is available
    lua_getglobal(L, "game");
    bool hasGame = !lua_isnil(L, -1);
    lua_pop(L, 1);
    
    if (hasGame) {
        // Load the embedded saveinstance script
        lua_getglobal(L, "_XORON_SAVEINSTANCE");
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            
            // Compile and cache the saveinstance module
            size_t bc_len = 0;
            char* bc = luau_compile(SAVEINSTANCE_LUA, strlen(SAVEINSTANCE_LUA), nullptr, &bc_len);
            if (bc && bc_len > 0) {
                if (luau_load(L, "saveinstance", bc, bc_len, 0) == 0) {
                    lua_call(L, 0, 1);  // Execute to get the module table
                    lua_pushvalue(L, -1);
                    lua_setglobal(L, "_XORON_SAVEINSTANCE");
                } else {
                    free(bc);
                    luaL_error(L, "Failed to load saveinstance module");
                    return 0;
                }
                free(bc);
            }
        }
        
        // Call SaveInstance.Save(options)
        lua_getfield(L, -1, "Save");
        lua_pushvalue(L, 1);  // Push options table
        lua_call(L, 1, 1);
        return 1;
    }
    
    // Fallback: Create a basic RBXMX file without game access
    std::string filename = "game.rbxmx";
    
    if (lua_istable(L, 1)) {
        lua_getfield(L, 1, "FileName");
        if (lua_isstring(L, -1)) {
            filename = lua_tostring(L, -1);
        }
        lua_pop(L, 1);
    }
    
    // Ensure .rbxmx extension
    if (filename.find(".rbxmx") == std::string::npos && 
        filename.find(".rbxm") == std::string::npos) {
        filename += ".rbxmx";
    }
    
    // Get workspace path
    extern const char* xoron_get_workspace(void);
    std::string workspace = xoron_get_workspace();
    std::string filepath = workspace + "/" + filename;
    
    // Create RBXMX with info about Xoron
    std::stringstream xml;
    xml << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    xml << "<roblox xmlns:xmime=\"http://www.w3.org/2005/05/xmlmime\" ";
    xml << "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" ";
    xml << "xsi:noNamespaceSchemaLocation=\"http://www.roblox.com/roblox.xsd\" ";
    xml << "version=\"4\">\n";
    xml << "\t<Meta name=\"ExplicitAutoJoints\">true</Meta>\n";
    xml << "\t<External>null</External>\n";
    xml << "\t<External>nil</External>\n";
    xml << "\t<Item class=\"Folder\" referent=\"RBX00000001\">\n";
    xml << "\t\t<Properties>\n";
    xml << "\t\t\t<string name=\"Name\">XoronSaveInstance</string>\n";
    xml << "\t\t</Properties>\n";
    xml << "\t\t<Item class=\"StringValue\" referent=\"RBX00000002\">\n";
    xml << "\t\t\t<Properties>\n";
    xml << "\t\t\t\t<string name=\"Name\">Info</string>\n";
    xml << "\t\t\t\t<string name=\"Value\">Saved with Xoron Executor v" << XORON_VERSION << "</string>\n";
    xml << "\t\t\t</Properties>\n";
    xml << "\t\t</Item>\n";
    xml << "\t\t<Item class=\"StringValue\" referent=\"RBX00000003\">\n";
    xml << "\t\t\t<Properties>\n";
    xml << "\t\t\t\t<string name=\"Name\">Note</string>\n";
    xml << "\t\t\t\t<string name=\"Value\">Full saveinstance requires game integration. Use this in-game.</string>\n";
    xml << "\t\t\t</Properties>\n";
    xml << "\t\t</Item>\n";
    xml << "\t</Item>\n";
    xml << "</roblox>\n";
    
    // Write to file
    std::ofstream file(filepath);
    if (!file.is_open()) {
        luaL_error(L, "Failed to create file: %s", filepath.c_str());
        return 0;
    }
    
    file << xml.str();
    file.close();
    
    lua_pushstring(L, filepath.c_str());
    return 1;
}

// gethiddenproperty(instance, property) - Gets a hidden property
static int lua_gethiddenproperty(lua_State* L) {
    luaL_checkany(L, 1);
    const char* property = luaL_checkstring(L, 2);
    
    // This would require access to Roblox's property system
    // Return nil for now
    (void)property;
    lua_pushnil(L);
    lua_pushboolean(L, false);  // wasHidden
    return 2;
}

// sethiddenproperty(instance, property, value) - Sets a hidden property
static int lua_sethiddenproperty(lua_State* L) {
    luaL_checkany(L, 1);
    const char* property = luaL_checkstring(L, 2);
    luaL_checkany(L, 3);
    
    // This would require access to Roblox's property system
    (void)property;
    lua_pushboolean(L, false);  // success
    return 1;
}

// getspecialinfo(instance) - Gets special info about an instance
static int lua_getspecialinfo(lua_State* L) {
    luaL_checkany(L, 1);
    
    // Return empty table
    lua_newtable(L);
    return 1;
}

// isnetworkowner(part) - Checks if local player owns the network for a part
static int lua_isnetworkowner(lua_State* L) {
    luaL_checkany(L, 1);
    
    // Would require game integration
    lua_pushboolean(L, false);
    return 1;
}

// setsimulationradius(radius, maxRadius) - Sets simulation radius
static int lua_setsimulationradius(lua_State* L) {
    float radius = (float)luaL_checknumber(L, 1);
    float maxRadius = (float)luaL_optnumber(L, 2, radius);
    
    // Would require game integration
    (void)radius;
    (void)maxRadius;
    return 0;
}

// getsimulationradius() - Gets current simulation radius
static int lua_getsimulationradius(lua_State* L) {
    // Default Roblox simulation radius
    lua_pushnumber(L, 1000);
    return 1;
}

// Register cache library
void xoron_register_cache(lua_State* L) {
    // Create cache table
    lua_newtable(L);
    
    lua_pushcfunction(L, lua_cache_invalidate, "invalidate");
    lua_setfield(L, -2, "invalidate");
    
    lua_pushcfunction(L, lua_cache_iscached, "iscached");
    lua_setfield(L, -2, "iscached");
    
    lua_pushcfunction(L, lua_cache_replace, "replace");
    lua_setfield(L, -2, "replace");
    
    lua_setglobal(L, "cache");
    
    // Decompile/script functions
    lua_pushcfunction(L, lua_decompile, "decompile");
    lua_setglobal(L, "decompile");
    
    lua_pushcfunction(L, lua_getscriptbytecode, "getscriptbytecode");
    lua_setglobal(L, "getscriptbytecode");
    
    lua_pushcfunction(L, lua_getscripthash, "getscripthash");
    lua_setglobal(L, "getscripthash");
    
    lua_pushcfunction(L, lua_getscriptclosure, "getscriptclosure");
    lua_setglobal(L, "getscriptclosure");
    
    // Save instance
    lua_pushcfunction(L, lua_saveinstance, "saveinstance");
    lua_setglobal(L, "saveinstance");
    
    // Hidden property functions
    lua_pushcfunction(L, lua_gethiddenproperty, "gethiddenproperty");
    lua_setglobal(L, "gethiddenproperty");
    
    lua_pushcfunction(L, lua_sethiddenproperty, "sethiddenproperty");
    lua_setglobal(L, "sethiddenproperty");
    
    // Special info
    lua_pushcfunction(L, lua_getspecialinfo, "getspecialinfo");
    lua_setglobal(L, "getspecialinfo");
    
    // Network functions
    lua_pushcfunction(L, lua_isnetworkowner, "isnetworkowner");
    lua_setglobal(L, "isnetworkowner");
    
    lua_pushcfunction(L, lua_setsimulationradius, "setsimulationradius");
    lua_setglobal(L, "setsimulationradius");
    
    lua_pushcfunction(L, lua_getsimulationradius, "getsimulationradius");
    lua_setglobal(L, "getsimulationradius");
}
