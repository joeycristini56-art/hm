# 17. saveinstance.lua - RBXMX Serializer

**File Path**: `src/src/lua/saveinstance.lua`  
**Size**: 17,000 bytes  
**Lines**: 496

**Platform**: Roblox Lua (Luau)

## Overview
Provides instance serialization to RBXMX format for Roblox. Saves game instances, scripts, and properties in XML format compatible with Roblox Studio.

## Architecture

### Core Components
```lua
SaveInstance = {
    -- Main API
    Save(options) -> string
    
    -- Internal
    PropertySerializers = {},
    DefaultOptions = {},
    -- Helper functions
}
```

### RBXMX Format
XML-based format:
```xml
<?xml version="1.0" encoding="utf-8"?>
<roblox ... version="4">
    <Item class="Folder" referent="RBX00000001">
        <Properties>
            <string name="Name">MyFolder</string>
        </Properties>
        <Item class="Part" referent="RBX00000002">
            ...
        </Item>
    </Item>
</roblox>
```

## Configuration Options

### Default Options
```lua
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
    Mode = "optimized",  -- "optimized", "full", "scripts"
}
```

### Option Descriptions

#### FileName
**Type**: string  
**Default**: `"game"`  
**Description**: Output filename (without extension)

**Example**:
```lua
saveinstance({FileName = "my_game"})
-- Creates: my_game.rbxmx
```

#### DecompileScripts
**Type**: boolean  
**Default**: `false`  
**Description**: Decompile Lua scripts instead of saving source

**Requirements**: `decompile` function must be available

**Example**:
```lua
saveinstance({DecompileScripts = true})
```

#### NilInstances
**Type**: boolean  
**Default**: `false`  
**Description**: Include instances with nil parent

**Requirements**: `getnilinstances` function must be available

**Example**:
```lua
saveinstance({NilInstances = true})
```

#### RemovePlayerCharacters
**Type**: boolean  
**Default**: `true`  
**Description**: Remove player characters from save

**Example**:
```lua
saveinstance({RemovePlayerCharacters = false})
```

#### SavePlayers
**Type**: boolean  
**Default**: `false`  
**Description**: Include Players service and characters

**Example**:
```lua
saveinstance({SavePlayers = true})
```

#### IsolateStarterPlayer
**Type**: boolean  
**Default**: `false`  
**Description**: Only save StarterPlayer children

**Example**:
```lua
saveinstance({IsolateStarterPlayer = true})
```

#### IsolateLocalPlayer
**Type**: boolean  
**Default**: `false`  
**Description**: Only save local player's objects

**Example**:
```lua
saveinstance({IsolateLocalPlayer = true})
```

#### IsolateLocalPlayerCharacter
**Type**: boolean  
**Default**: `false`  
**Description**: Only save local player's character

**Example**:
```lua
saveinstance({IsolateLocalPlayerCharacter = true})
```

#### ShowStatus
**Type**: boolean  
**Default**: `true`  
**Description**: Show progress messages

**Example**:
```lua
saveinstance({ShowStatus = false})
```

#### IgnoreDefaultProperties
**Type**: boolean  
**Default**: `true`  
**Description**: Skip properties with default values

**Example**:
```lua
saveinstance({IgnoreDefaultProperties = false})
```

#### IgnoreNotArchivable
**Type**: boolean  
**Default**: `true`  
**Description**: Skip instances with Archivable = false

**Example**:
```lua
saveinstance({IgnoreNotArchivable = false})
```

#### IgnorePropertiesOfNotScriptsOnScriptsMode
**Type**: boolean  
**Default**: `false`  
**Description**: Skip non-script properties in scripts mode

**Example**:
```lua
saveinstance({IgnorePropertiesOfNotScriptsOnScriptsMode = true})
```

#### SaveNonCreatable
**Type**: boolean  
**Default**: `false`  
**Description**: Save non-creatable instances

**Example**:
```lua
saveinstance({SaveNonCreatable = true})
```

#### Mode
**Type**: string  
**Default**: `"optimized"`  
**Description**: Save mode

**Modes**:
- `"optimized"`: Skip default properties, optimized size
- `"full"`: Save all properties
- `"scripts"`: Only save scripts

**Example**:
```lua
saveinstance({Mode = "full"})
```

#### Object
**Type**: Instance  
**Default**: `nil`  
**Description**: Specific object to save (instead of game)

**Example**:
```lua
local part = workspace.Part
saveinstance({Object = part, FileName = "part"})
```

## Property Serializers

### Supported Types

#### string
```xml
<string name="Name">Value</string>
```

#### bool
```xml
<bool name="Visible">true</bool>
```

#### int / int64
```xml
<int name="Value">42</int>
<int64 name="LargeValue">9223372036854775807</int64>
```

#### float / double
```xml
<float name="Transparency">0.5</float>
<double name="Precision">3.141592653589793</double>
```

#### BinaryString
```xml
<BinaryString name="Data">SGVsbG8gV29ybGQ=</BinaryString>
```

#### ProtectedString (Scripts)
```xml
<ProtectedString name="Source"><![CDATA[print("Hello")]]></ProtectedString>
```

#### Content
```xml
<Content name="Texture"><url>rbxassetid://12345</url></Content>
<Content name="Empty"><null></null></Content>
```

#### Vector2
```xml
<Vector2 name="Size">
    <X>10</X>
    <Y>20</Y>
</Vector2>
```

#### Vector3
```xml
<Vector3 name="Position">
    <X>10</X>
    <Y>20</Y>
    <Z>30</Z>
</Vector3>
```

#### CFrame (CoordinateFrame)
```xml
<CoordinateFrame name="CFrame">
    <X>0</X><Y>0</Y><Z>0</Z>
    <R00>1</R00><R01>0</R01><R02>0</R02>
    <R10>0</R10><R11>1</R11><R12>0</R12>
    <R20>0</R20><R21>0</R21><R22>1</R22>
</CoordinateFrame>
```

#### Color3
```xml
<Color3uint8 name="Color">16711680</Color3uint8>
```

#### BrickColor
```xml
<int name="BrickColor">1</int>
```

#### UDim
```xml
<UDim name="Size">
    <S>1</S>
    <O>0</O>
</UDim>
```

#### UDim2
```xml
<UDim2 name="Position">
    <XS>0.5</XS><XO>0</XO>
    <YS>0.5</YS><YO>0</YO>
</UDim2>
```

#### Rect
```xml
<Rect2D name="Rect">
    <min><X>0</X><Y>0</Y></min>
    <max><X>100</X><Y>100</Y></max>
</Rect2D>
```

#### NumberSequence
```xml
<NumberSequence name="Transparency">0 1 0 0.5 1 0 1 1 0</NumberSequence>
```

#### ColorSequence
```xml
<ColorSequence name="Color">0 1 0 0 0 0.5 1 0 0 0 1 1 0 0 0</ColorSequence>
```

#### NumberRange
```xml
<NumberRange name="Size">5 10</NumberRange>
```

#### Ref
```xml
<Ref name="Parent">RBX00000001</Ref>
<Ref name="Empty">null</Ref>
```

## Usage Examples

### Basic Usage
```lua
-- Save entire game
saveinstance()

-- Save with custom name
saveinstance({FileName = "my_awesome_game"})

-- Save specific object
local model = workspace.MyModel
saveinstance({Object = model, FileName = "model"})
```

### Advanced Options
```lua
-- Full save with decompilation
saveinstance({
    FileName = "full_game",
    DecompileScripts = true,
    NilInstances = true,
    SavePlayers = true,
    Mode = "full"
})

-- Scripts only
saveinstance({
    FileName = "scripts_only",
    Mode = "scripts"
})

-- Optimized (default)
saveinstance({
    FileName = "optimized",
    Mode = "optimized"
})
```

### Custom Object
```lua
-- Save workspace only
saveinstance({
    Object = workspace,
    FileName = "workspace"
})

-- Save specific folder
local folder = workspace.Map
saveinstance({
    Object = folder,
    FileName = "map"
})
```

### Nil Instances
```lua
-- Include nil instances
local nilInstances = getnilinstances()
saveinstance({
    NilInstances = true,
    FileName = "all_instances"
})
```

### Decompile Scripts
```lua
-- Decompile instead of source
saveinstance({
    DecompileScripts = true,
    FileName = "decompiled"
})
```

### Filtered Save
```lua
-- Ignore non-archivable
saveinstance({
    IgnoreNotArchivable = true,
    FileName = "archivable_only"
})

-- Ignore default properties
saveinstance({
    IgnoreDefaultProperties = true,
    FileName = "minimal"
})
```

### Isolation
```lua
-- Save only StarterPlayer
saveinstance({
    IsolateStarterPlayer = true,
    FileName = "starter_player"
})

-- Save only local player
saveinstance({
    IsolateLocalPlayer = true,
    FileName = "local_player"
})
```

## Implementation Details

### Referent System
```lua
-- Generates unique IDs for instances
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
```

### Property Filtering
```lua
-- Gets instance properties
local function getInstanceProperties(instance)
    local properties = {}
    
    -- Get default properties
    for _, prop in ipairs(instance:GetProperties()) do
        properties[prop.Name] = instance[prop.Name]
    end
    
    -- Get hidden properties (if available)
    if gethiddenproperty then
        local hiddenProps = {
            "AbsoluteSize", "AbsolutePosition", "AbsoluteRotation",
            "Player", "LocalPlayer", "Character"
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
```

### Script Handling
```lua
-- Special handling for scripts
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
    -- Serialize as ProtectedString
end
```

### XML Generation
```lua
-- Builds XML structure
local xml = {}

-- Header
table.insert(xml, '<?xml version="1.0" encoding="utf-8"?>\n')
table.insert(xml, '<roblox ... version="4">\n')

-- Meta
table.insert(xml, '\t<Meta name="ExplicitAutoJoints">true</Meta>\n')
table.insert(xml, '\t<External>null</External>\n')
table.insert(xml, '\t<External>nil</External>\n')

-- Items (recursive)
for _, instance in ipairs(toSave) do
    table.insert(xml, serializeInstance(instance, options, 1))
end

-- Footer
table.insert(xml, '</roblox>\n')

return table.concat(xml)
```

### Recursive Serialization
```lua
local function serializeInstance(instance, options, depth)
    local indent = string.rep("\t", depth)
    local output = {}
    
    -- Start tag
    table.insert(output, string.format('%s<Item class="%s" referent="%s">\n',
        indent, instance.ClassName, getRef(instance)))
    
    -- Properties
    table.insert(output, indent .. "\t<Properties>\n")
    for name, value in pairs(getInstanceProperties(instance)) do
        local serialized = serializeProperty(name, value)
        if serialized ~= "" then
            table.insert(output, serialized)
        end
    end
    table.insert(output, indent .. "\t</Properties>\n")
    
    -- Children (recursive)
    for _, child in ipairs(instance:GetChildren()) do
        table.insert(output, serializeInstance(child, options, depth + 1))
    end
    
    -- End tag
    table.insert(output, indent .. "</Item>\n")
    
    return table.concat(output)
end
```

## Dependencies

### Required Functions
```lua
-- Core Roblox functions
Instance:GetChildren()
Instance:GetProperties()
Instance:IsA()
Instance.Archivable
Instance.ClassName
Instance.Source  -- For scripts

-- Optional functions
getnilinstances()  -- For NilInstances option
decompile()        -- For DecompileScripts option
gethiddenproperty() -- For hidden properties
writefile()        -- For saving to file
getworkspace()     -- For path resolution
```

### Optional Dependencies
```lua
-- Cryptography (for base64)
crypt.base64encode()  -- If available, used for BinaryString

-- Bit manipulation
bit32.bor()
bit32.lshift()
```

## File Output

### Location
```lua
-- If writefile and getworkspace available:
local path = getworkspace() .. "/" .. filename
writefile(path, content)

-- Otherwise returns content string
return content
```

### Format
```
game.rbxmx
├── <?xml ...> (header)
├── <roblox ...>
│   ├── <Meta ...>
│   ├── <External>null</External>
│   ├── <External>nil</External>
│   ├── <Item ...> (instances)
│   │   ├── <Properties>
│   │   │   ├── <string name="Name">...</string>
│   │   │   └── ...
│   │   ├── <Item ...> (children)
│   │   └── ...
│   └── ...
└── </roblox>
```

## Error Handling

### Safe Property Access
```lua
-- All property access wrapped in pcall
local success, value = pcall(function() return instance[prop] end)
if success then
    -- Use value
end
```

### Safe Serialization
```lua
-- Each property serialized safely
local serialized = serializeProperty(name, value)
if serialized ~= "" then
    table.insert(output, serialized)
end
```

### Safe Instance Access
```lua
-- All instance operations wrapped
local success, children = pcall(function() return instance:GetChildren() end)
if success then
    for _, child in ipairs(children) do
        -- Process child
    end
end
```

## Performance Considerations

### Memory Usage
- Builds XML in memory using table.concat
- Referent cache prevents duplicate IDs
- Property filtering reduces size

### Speed
- Recursive traversal is O(n) where n = instances
- Property serialization is O(p) where p = properties
- XML building is O(x) where x = output size

### Optimization
```lua
-- Mode "optimized" skips:
-- 1. Default properties
-- 2. Non-archivable instances
-- 3. Unnecessary properties

-- Mode "scripts" only saves:
-- 1. Script instances
-- 2. Script properties
-- 3. Script hierarchy
```

## Limitations

### Roblox Restrictions
- Cannot save non-creatable instances (unless option enabled)
- Cannot save services that don't support it
- Some properties may be read-only
- Hidden properties require special functions

### Script Limitations
- Decompilation requires external library
- Some scripts may not decompile correctly
- Obfuscated scripts may fail
- Plugin restrictions may apply

### Property Limitations
- Some custom property types may not be supported
- Binary data must be base64 encoded
- References must be valid instances
- Complex types need custom serializers

## Best Practices

### 1. Use Appropriate Mode
```lua
-- For scripts only
saveinstance({Mode = "scripts"})

-- For full game
saveinstance({Mode = "full"})

-- For optimized (default)
saveinstance({Mode = "optimized"})
```

### 2. Filter Unnecessary Data
```lua
saveinstance({
    IgnoreNotArchivable = true,
    IgnoreDefaultProperties = true,
    RemovePlayerCharacters = true
})
```

### 3. Handle Errors
```lua
local success, path = pcall(function()
    return saveinstance({FileName = "game"})
end)

if success then
    print("Saved to:", path)
else
    warn("Save failed:", path)
end
```

### 4. Check Dependencies
```lua
if not writefile then
    warn("writefile not available")
    return
end

if options.DecompileScripts and not decompile then
    warn("decompile not available, falling back to source")
    options.DecompileScripts = false
end
```

### 5. Use Specific Objects
```lua
-- Instead of saving entire game
local model = workspace:FindFirstChild("ImportantModel")
if model then
    saveinstance({Object = model, FileName = "important"})
end
```

## Testing

### Basic Test
```lua
-- Test save
local path = saveinstance({FileName = "test"})
print("Saved to:", path)

-- Verify file exists
if isfile and isfile(path) then
    print("File created successfully")
end
```

### Options Test
```lua
-- Test all options
local options = {
    FileName = "test_all",
    DecompileScripts = false,
    NilInstances = false,
    RemovePlayerCharacters = true,
    SavePlayers = false,
    ShowStatus = true,
    IgnoreDefaultProperties = true,
    IgnoreNotArchivable = true,
    Mode = "optimized"
}

local path = saveinstance(options)
print("Test completed:", path)
```

### Object Test
```lua
-- Test with specific object
local testPart = Instance.new("Part")
testPart.Name = "TestPart"
testPart.Parent = workspace

local path = saveinstance({
    Object = testPart,
    FileName = "single_part"
})

print("Single object saved:", path)
```

## Integration

### With File System
```lua
-- Save and load
local path = saveinstance({FileName = "backup"})

-- Later, load back (requires loadstring or similar)
local content = readfile(path)
-- Load into game...
```

### With HTTP
```lua
-- Upload saved game
local path = saveinstance({FileName = "game"})
local content = readfile(path)

http.post("https://example.com/upload", {
    body = content,
    headers = {["Content-Type"] = "application/xml"}
})
```

### With Compression
```lua
-- Compress saved game
local path = saveinstance({FileName = "game"})
local content = readfile(path)
local compressed = lz4_compress(content)
writefile("game.lz4", compressed)
```

## Related Files
- `xoron.h` (C++ API)
- `xoron_luau.cpp` (Lua bindings)
- `xoron_filesystem.cpp` (file operations)
- `xoron_http.cpp` (HTTP upload)
- `xoron_crypto.cpp` (compression)
