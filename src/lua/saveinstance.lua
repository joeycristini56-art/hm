--[[
    Xoron SaveInstance - Full RBXMX Serializer
    Based on UniversalSynSaveInstance patterns
    Serializes Roblox instances to RBXMX format
]]

local SaveInstance = {}

-- Configuration defaults
local DefaultOptions = {
    FileName = "game",
    DecompileScripts = false,
    NilInstances = false,
    RemovePlayerCharacters = true,
    SavePlayers = false,
    IsolateStarterPlayer = false,
    IsolateLocalPlayer = false,
    IsolateLocalPlayerCharacter = false,
    ShowStatus = true,
    IgnoreDefaultProperties = true,
    IgnoreNotArchivable = true,
    IgnorePropertiesOfNotScriptsOnScriptsMode = false,
    SaveNonCreatable = false,
    Mode = "optimized", -- "optimized", "full", "scripts"
}

-- Property type serializers
local PropertySerializers = {}

-- Escape XML special characters
local function escapeXML(str)
    if type(str) ~= "string" then
        str = tostring(str)
    end
    return str:gsub("&", "&amp;")
              :gsub("<", "&lt;")
              :gsub(">", "&gt;")
              :gsub("\"", "&quot;")
              :gsub("'", "&apos;")
end

-- Encode binary data as base64
local function base64Encode(data)
    if crypt and crypt.base64encode then
        return crypt.base64encode(data)
    end
    -- Fallback base64 implementation
    local b = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/'
    return ((data:gsub('.', function(x)
        local r, b = '', x:byte()
        for i = 8, 1, -1 do r = r .. (b % 2 ^ i - b % 2 ^ (i - 1) > 0 and '1' or '0') end
        return r
    end) .. '0000'):gsub('%d%d%d?%d?%d?%d?', function(x)
        if #x < 6 then return '' end
        local c = 0
        for i = 1, 6 do c = c + (x:sub(i, i) == '1' and 2 ^ (6 - i) or 0) end
        return b:sub(c + 1, c + 1)
    end) .. ({ '', '==', '=' })[#data % 3 + 1])
end

-- Generate unique referent ID
local referentCounter = 0
local referentCache = {}

local function getRef(instance)
    if referentCache[instance] then
        return referentCache[instance]
    end
    referentCounter = referentCounter + 1
    local ref = "RBX" .. string.format("%08X", referentCounter)
    referentCache[instance] = ref
    return ref
end

-- Property type serializers
PropertySerializers["string"] = function(name, value)
    return string.format('\t\t\t<string name="%s">%s</string>\n', name, escapeXML(value))
end

PropertySerializers["bool"] = function(name, value)
    return string.format('\t\t\t<bool name="%s">%s</bool>\n', name, value and "true" or "false")
end

PropertySerializers["int"] = function(name, value)
    return string.format('\t\t\t<int name="%s">%d</int>\n', name, value)
end

PropertySerializers["int64"] = function(name, value)
    return string.format('\t\t\t<int64 name="%s">%d</int64>\n', name, value)
end

PropertySerializers["float"] = function(name, value)
    return string.format('\t\t\t<float name="%s">%s</float>\n', name, tostring(value))
end

PropertySerializers["double"] = function(name, value)
    return string.format('\t\t\t<double name="%s">%s</double>\n', name, tostring(value))
end

PropertySerializers["BinaryString"] = function(name, value)
    return string.format('\t\t\t<BinaryString name="%s">%s</BinaryString>\n', name, base64Encode(value))
end

PropertySerializers["ProtectedString"] = function(name, value)
    return string.format('\t\t\t<ProtectedString name="%s"><![CDATA[%s]]></ProtectedString>\n', name, value)
end

PropertySerializers["Content"] = function(name, value)
    if value == "" then
        return string.format('\t\t\t<Content name="%s"><null></null></Content>\n', name)
    end
    return string.format('\t\t\t<Content name="%s"><url>%s</url></Content>\n', name, escapeXML(value))
end

PropertySerializers["Vector2"] = function(name, value)
    return string.format('\t\t\t<Vector2 name="%s">\n\t\t\t\t<X>%s</X>\n\t\t\t\t<Y>%s</Y>\n\t\t\t</Vector2>\n',
        name, tostring(value.X), tostring(value.Y))
end

PropertySerializers["Vector3"] = function(name, value)
    return string.format('\t\t\t<Vector3 name="%s">\n\t\t\t\t<X>%s</X>\n\t\t\t\t<Y>%s</Y>\n\t\t\t\t<Z>%s</Z>\n\t\t\t</Vector3>\n',
        name, tostring(value.X), tostring(value.Y), tostring(value.Z))
end

PropertySerializers["CFrame"] = function(name, value)
    local x, y, z, r00, r01, r02, r10, r11, r12, r20, r21, r22 = value:GetComponents()
    return string.format('\t\t\t<CoordinateFrame name="%s">\n' ..
        '\t\t\t\t<X>%s</X>\n\t\t\t\t<Y>%s</Y>\n\t\t\t\t<Z>%s</Z>\n' ..
        '\t\t\t\t<R00>%s</R00>\n\t\t\t\t<R01>%s</R01>\n\t\t\t\t<R02>%s</R02>\n' ..
        '\t\t\t\t<R10>%s</R10>\n\t\t\t\t<R11>%s</R11>\n\t\t\t\t<R12>%s</R12>\n' ..
        '\t\t\t\t<R20>%s</R20>\n\t\t\t\t<R21>%s</R21>\n\t\t\t\t<R22>%s</R22>\n' ..
        '\t\t\t</CoordinateFrame>\n',
        name, x, y, z, r00, r01, r02, r10, r11, r12, r20, r21, r22)
end

PropertySerializers["Color3"] = function(name, value)
    -- RBXMX uses uint32 color format
    local r = math.floor(value.R * 255)
    local g = math.floor(value.G * 255)
    local b = math.floor(value.B * 255)
    local color = bit32.bor(bit32.lshift(r, 16), bit32.lshift(g, 8), b)
    return string.format('\t\t\t<Color3uint8 name="%s">%d</Color3uint8>\n', name, color)
end

PropertySerializers["BrickColor"] = function(name, value)
    return string.format('\t\t\t<int name="%s">%d</int>\n', name, value.Number)
end

PropertySerializers["UDim"] = function(name, value)
    return string.format('\t\t\t<UDim name="%s">\n\t\t\t\t<S>%s</S>\n\t\t\t\t<O>%d</O>\n\t\t\t</UDim>\n',
        name, tostring(value.Scale), value.Offset)
end

PropertySerializers["UDim2"] = function(name, value)
    return string.format('\t\t\t<UDim2 name="%s">\n' ..
        '\t\t\t\t<XS>%s</XS>\n\t\t\t\t<XO>%d</XO>\n' ..
        '\t\t\t\t<YS>%s</YS>\n\t\t\t\t<YO>%d</YO>\n' ..
        '\t\t\t</UDim2>\n',
        name, tostring(value.X.Scale), value.X.Offset, tostring(value.Y.Scale), value.Y.Offset)
end

PropertySerializers["Rect"] = function(name, value)
    return string.format('\t\t\t<Rect2D name="%s">\n' ..
        '\t\t\t\t<min>\n\t\t\t\t\t<X>%s</X>\n\t\t\t\t\t<Y>%s</Y>\n\t\t\t\t</min>\n' ..
        '\t\t\t\t<max>\n\t\t\t\t\t<X>%s</X>\n\t\t\t\t\t<Y>%s</Y>\n\t\t\t\t</max>\n' ..
        '\t\t\t</Rect2D>\n',
        name, value.Min.X, value.Min.Y, value.Max.X, value.Max.Y)
end

PropertySerializers["NumberSequence"] = function(name, value)
    local keypoints = {}
    for _, kp in ipairs(value.Keypoints) do
        table.insert(keypoints, string.format("%s %s %s", kp.Time, kp.Value, kp.Envelope))
    end
    return string.format('\t\t\t<NumberSequence name="%s">%s</NumberSequence>\n',
        name, table.concat(keypoints, " "))
end

PropertySerializers["ColorSequence"] = function(name, value)
    local keypoints = {}
    for _, kp in ipairs(value.Keypoints) do
        table.insert(keypoints, string.format("%s %s %s %s 0",
            kp.Time, kp.Value.R, kp.Value.G, kp.Value.B))
    end
    return string.format('\t\t\t<ColorSequence name="%s">%s</ColorSequence>\n',
        name, table.concat(keypoints, " "))
end

PropertySerializers["NumberRange"] = function(name, value)
    return string.format('\t\t\t<NumberRange name="%s">%s %s</NumberRange>\n',
        name, value.Min, value.Max)
end

PropertySerializers["Ref"] = function(name, value)
    if value and typeof(value) == "Instance" then
        return string.format('\t\t\t<Ref name="%s">%s</Ref>\n', name, getRef(value))
    end
    return string.format('\t\t\t<Ref name="%s">null</Ref>\n', name)
end

PropertySerializers["Enum"] = function(name, value)
    return string.format('\t\t\t<token name="%s">%d</token>\n', name, value.Value)
end

PropertySerializers["Font"] = function(name, value)
    return string.format('\t\t\t<Font name="%s">\n' ..
        '\t\t\t\t<Family><url>%s</url></Family>\n' ..
        '\t\t\t\t<Weight>%d</Weight>\n' ..
        '\t\t\t\t<Style>%s</Style>\n' ..
        '\t\t\t</Font>\n',
        name, escapeXML(value.Family), value.Weight.Value, value.Style.Name)
end

-- Get property value type
local function getPropertyType(value)
    local t = typeof(value)
    if t == "string" then return "string"
    elseif t == "boolean" then return "bool"
    elseif t == "number" then
        if value == math.floor(value) and value >= -2147483648 and value <= 2147483647 then
            return "int"
        end
        return "double"
    elseif t == "Vector2" then return "Vector2"
    elseif t == "Vector3" then return "Vector3"
    elseif t == "CFrame" then return "CFrame"
    elseif t == "Color3" then return "Color3"
    elseif t == "BrickColor" then return "BrickColor"
    elseif t == "UDim" then return "UDim"
    elseif t == "UDim2" then return "UDim2"
    elseif t == "Rect" then return "Rect"
    elseif t == "NumberSequence" then return "NumberSequence"
    elseif t == "ColorSequence" then return "ColorSequence"
    elseif t == "NumberRange" then return "NumberRange"
    elseif t == "Instance" then return "Ref"
    elseif t == "EnumItem" then return "Enum"
    elseif t == "Font" then return "Font"
    end
    return nil
end

-- Serialize a property
local function serializeProperty(name, value)
    local propType = getPropertyType(value)
    if propType and PropertySerializers[propType] then
        return PropertySerializers[propType](name, value)
    end
    return ""
end

-- Get all properties of an instance
local function getInstanceProperties(instance)
    local properties = {}
    
    -- Always include Name
    properties.Name = instance.Name
    
    -- Try to get properties using API
    local success, props = pcall(function()
        -- This would use the reflection API if available
        return {}
    end)
    
    -- Common properties for different class types
    local commonProps = {
        "Anchored", "CanCollide", "CanTouch", "CanQuery", "CastShadow",
        "Color", "BrickColor", "Material", "Reflectance", "Transparency",
        "Size", "CFrame", "Position", "Orientation",
        "Velocity", "RotVelocity", "AssemblyLinearVelocity", "AssemblyAngularVelocity",
        "Massless", "RootPriority",
        "Shape", "MeshId", "TextureId",
        "Enabled", "Visible", "Active",
        "Text", "TextColor3", "TextSize", "Font", "TextScaled",
        "BackgroundColor3", "BackgroundTransparency", "BorderColor3", "BorderSizePixel",
        "Image", "ImageColor3", "ImageTransparency",
        "Value", "MaxValue", "MinValue",
        "Adornee", "AlwaysOnTop",
        "Brightness", "Range", "Shadows",
        "PlaybackSpeed", "Volume", "Looped", "Playing",
        "MaxDistance", "RollOffMode", "RollOffMaxDistance", "RollOffMinDistance",
    }
    
    for _, propName in ipairs(commonProps) do
        local success, value = pcall(function()
            return instance[propName]
        end)
        if success and value ~= nil then
            properties[propName] = value
        end
    end
    
    -- Try to get hidden properties if available
    if gethiddenproperty then
        local hiddenProps = {
            "StreamingEnabled", "Archivable", "RobloxLocked",
        }
        for _, propName in ipairs(hiddenProps) do
            local success, value = pcall(gethiddenproperty, instance, propName)
            if success and value ~= nil then
                properties[propName] = value
            end
        end
    end
    
    return properties
end

-- Serialize an instance and its descendants
local function serializeInstance(instance, options, depth)
    depth = depth or 1
    local indent = string.rep("\t", depth)
    local output = {}
    
    -- Check if we should skip this instance
    if options.IgnoreNotArchivable then
        local success, archivable = pcall(function() return instance.Archivable end)
        if success and not archivable then
            return ""
        end
    end
    
    -- Start item tag
    table.insert(output, string.format('%s<Item class="%s" referent="%s">\n',
        indent, instance.ClassName, getRef(instance)))
    
    -- Properties
    table.insert(output, indent .. "\t<Properties>\n")
    
    local properties = getInstanceProperties(instance)
    for name, value in pairs(properties) do
        local serialized = serializeProperty(name, value)
        if serialized ~= "" then
            table.insert(output, serialized)
        end
    end
    
    -- Handle scripts specially
    if instance:IsA("LuaSourceContainer") then
        local source = ""
        if options.DecompileScripts and decompile then
            local success, decompiled = pcall(decompile, instance)
            if success then
                source = decompiled
            end
        else
            local success, src = pcall(function() return instance.Source end)
            if success then
                source = src or ""
            end
        end
        table.insert(output, PropertySerializers["ProtectedString"]("Source", source))
    end
    
    table.insert(output, indent .. "\t</Properties>\n")
    
    -- Children
    local children = instance:GetChildren()
    for _, child in ipairs(children) do
        local childOutput = serializeInstance(child, options, depth + 1)
        if childOutput ~= "" then
            table.insert(output, childOutput)
        end
    end
    
    -- End item tag
    table.insert(output, indent .. "</Item>\n")
    
    return table.concat(output)
end

-- Main saveinstance function
function SaveInstance.Save(options)
    options = options or {}
    
    -- Merge with defaults
    for key, value in pairs(DefaultOptions) do
        if options[key] == nil then
            options[key] = value
        end
    end
    
    -- Reset state
    referentCounter = 0
    referentCache = {}
    
    -- Build XML
    local xml = {}
    
    -- Header
    table.insert(xml, '<?xml version="1.0" encoding="utf-8"?>\n')
    table.insert(xml, '<roblox xmlns:xmime="http://www.w3.org/2005/05/xmlmime" ')
    table.insert(xml, 'xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" ')
    table.insert(xml, 'xsi:noNamespaceSchemaLocation="http://www.roblox.com/roblox.xsd" ')
    table.insert(xml, 'version="4">\n')
    table.insert(xml, '\t<Meta name="ExplicitAutoJoints">true</Meta>\n')
    table.insert(xml, '\t<External>null</External>\n')
    table.insert(xml, '\t<External>nil</External>\n')
    
    -- Determine what to save
    local toSave = {}
    
    if options.Object then
        -- Save specific object
        table.insert(toSave, options.Object)
    else
        -- Save game services
        local services = {
            "Workspace",
            "Lighting",
            "ReplicatedFirst",
            "ReplicatedStorage",
            "StarterGui",
            "StarterPack",
            "StarterPlayer",
            "Teams",
            "SoundService",
            "Chat",
            "LocalizationService",
            "TestService",
        }
        
        for _, serviceName in ipairs(services) do
            local success, service = pcall(function()
                return game:GetService(serviceName)
            end)
            if success and service then
                table.insert(toSave, service)
            end
        end
        
        -- Optionally save players
        if options.SavePlayers then
            local success, players = pcall(function()
                return game:GetService("Players")
            end)
            if success and players then
                table.insert(toSave, players)
            end
        end
        
        -- Optionally save nil instances
        if options.NilInstances and getnilinstances then
            local success, nilInstances = pcall(getnilinstances)
            if success and nilInstances then
                -- Create a folder to hold nil instances
                table.insert(xml, '\t<Item class="Folder" referent="RBX_NIL_INSTANCES">\n')
                table.insert(xml, '\t\t<Properties>\n')
                table.insert(xml, '\t\t\t<string name="Name">NilInstances</string>\n')
                table.insert(xml, '\t\t</Properties>\n')
                for _, inst in ipairs(nilInstances) do
                    local serialized = serializeInstance(inst, options, 2)
                    if serialized ~= "" then
                        table.insert(xml, serialized)
                    end
                end
                table.insert(xml, '\t</Item>\n')
            end
        end
    end
    
    -- Serialize all instances
    for _, instance in ipairs(toSave) do
        local serialized = serializeInstance(instance, options, 1)
        if serialized ~= "" then
            table.insert(xml, serialized)
        end
    end
    
    -- Footer
    table.insert(xml, '</roblox>\n')
    
    -- Combine and save
    local content = table.concat(xml)
    local filename = options.FileName
    if not filename:match("%.rbxm?x?$") then
        filename = filename .. ".rbxmx"
    end
    
    -- Write to file
    if writefile then
        local path = filename
        if getworkspace then
            path = getworkspace() .. "/" .. filename
        end
        writefile(path, content)
        return path
    end
    
    return content
end

-- Global function
_G.saveinstance = SaveInstance.Save
saveinstance = SaveInstance.Save

return SaveInstance
