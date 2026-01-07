# 09. xoron_drawing.cpp - Graphics Drawing Library

**File Path**: `src/src/xoron_drawing.cpp`  
**Size**: 42,846 bytes  
**Lines**: 1,116

**Platform**: Cross-platform (iOS: CoreGraphics, Android: Native Canvas)

## Overview
Provides a comprehensive 2D graphics drawing library for the executor. Supports lines, circles, squares, text, triangles, quads, and images with real-time rendering.

## Includes
- `xoron.h` (local)
- `cstdlib`, `cstring`, `cstdio` (system)
- `string`, `vector`, `unordered_map` (system)
- `mutex`, `atomic` (system)
- `cmath` (system)
- Platform-specific:
  - iOS: `CoreGraphics.h`, `CoreFoundation.h`, `CoreText.h`, `objc/runtime.h`
  - Android: `android/native_window.h`, `jni.h`
- `lua.h`, `lualib.h` (local - Luau)

## Core Structures

### DrawingType
```cpp
enum DrawingType {
    DRAWING_LINE = 0,
    DRAWING_CIRCLE,
    DRAWING_SQUARE,
    DRAWING_TEXT,
    DRAWING_TRIANGLE,
    DRAWING_QUAD,
    DRAWING_IMAGE
};
```

### Color3
```cpp
struct Color3 {
    float r, g, b;  // 0.0 to 1.0
    Color3(float r = 1, float g = 1, float b = 1);
};
```

### Vector2
```cpp
struct Vector2 {
    float x, y;
    Vector2(float x = 0, float y = 0);
};
```

### DrawingObject
```cpp
struct DrawingObject {
    DrawingType type;
    bool visible;
    float transparency;  // 0.0 to 1.0
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
};
```

### DrawingState
```cpp
struct DrawingState {
    std::unordered_map<uint32_t, DrawingObject*> objects;
    std::mutex mutex;
    std::atomic<uint32_t> nextId{1};
    bool enabled;
    float screenWidth, screenHeight;
    bool needsRedraw;
    
    // Platform-specific context
    #if defined(XORON_IOS_DRAWING)
        CGContextRef context;
    #elif defined(XORON_ANDROID_DRAWING)
        // Android canvas handles
    #endif
};
```

## Drawing Types

### Line
```lua
local line = Drawing.new("Line")
line.From = Vector2.new(100, 100)
line.To = Vector2.new(200, 200)
line.Color = Color3.new(1, 0, 0)  -- Red
line.Thickness = 2
line.Visible = true
```

**Properties**:
- `From`: Vector2 - Start point
- `To`: Vector2 - End point
- `Color`: Color3 - Line color
- `Thickness`: number - Line width
- `Transparency`: number (0-1)
- `Visible`: boolean
- `ZIndex`: number

### Circle
```lua
local circle = Drawing.new("Circle")
circle.Position = Vector2.new(400, 300)
circle.Radius = 50
circle.Color = Color3.new(0, 1, 0)  -- Green
circle.Filled = true
circle.Thickness = 2
```

**Properties**:
- `Position`: Vector2 - Center position
- `Radius`: number - Circle radius
- `Color`: Color3 - Fill/border color
- `Filled`: boolean - Fill or outline
- `Thickness`: number - Border width
- `Transparency`: number (0-1)
- `Visible`: boolean

### Square
```lua
local square = Drawing.new("Square")
square.Position = Vector2.new(100, 100)
square.Size = Vector2.new(150, 100)
square.Color = Color3.new(0, 0, 1)  -- Blue
square.Filled = true
square.Rounding = 5  -- Rounded corners
```

**Properties**:
- `Position`: Vector2 - Top-left position
- `Size`: Vector2 - Width and height
- `Color`: Color3 - Fill/border color
- `Filled`: boolean - Fill or outline
- `Thickness`: number - Border width
- `Rounding`: number - Corner radius
- `Transparency`: number (0-1)
- `Visible`: boolean

### Text
```lua
local text = Drawing.new("Text")
text.Position = Vector2.new(200, 200)
text.Text = "Hello, World!"
text.Color = Color3.new(1, 1, 1)  -- White
text.TextSize = 24
text.Center = true
text.Outline = true
text.OutlineColor = Color3.new(0, 0, 0)  -- Black outline
```

**Properties**:
- `Position`: Vector2 - Text position
- `Text`: string - Text content
- `Color`: Color3 - Text color
- `TextSize`: number - Font size
- `Center`: boolean - Center text at position
- `Outline`: boolean - Add outline
- `OutlineColor`: Color3 - Outline color
- `Font`: string - Font name (platform-specific)
- `Transparency`: number (0-1)
- `Visible`: boolean

### Triangle
```lua
local triangle = Drawing.new("Triangle")
triangle.PointA = Vector2.new(100, 100)
triangle.PointB = Vector2.new(150, 200)
triangle.PointC = Vector2.new(50, 200)
triangle.Color = Color3.new(1, 1, 0)  -- Yellow
triangle.Filled = true
triangle.Thickness = 2
```

**Properties**:
- `PointA`, `PointB`, `PointC`: Vector2 - Triangle vertices
- `Color`: Color3 - Fill/border color
- `Filled`: boolean - Fill or outline
- `Thickness`: number - Border width
- `Transparency`: number (0-1)
- `Visible`: boolean

### Quad
```lua
local quad = Drawing.new("Quad")
quad.PointA = Vector2.new(100, 100)
quad.PointB = Vector2.new(200, 100)
quad.PointC = Vector2.new(200, 200)
quad.PointD = Vector2.new(100, 200)
quad.Color = Color3.new(1, 0, 1)  -- Magenta
quad.Filled = true
```

**Properties**:
- `PointA`, `PointB`, `PointC`, `PointD`: Vector2 - Quad vertices
- `Color`: Color3 - Fill/border color
- `Filled`: boolean - Fill or outline
- `Thickness`: number - Border width
- `Transparency`: number (0-1)
- `Visible`: boolean

### Image
```lua
local image = Drawing.new("Image")
image.Position = Vector2.new(100, 100)
image.Size = Vector2.new(200, 200)
image.Image = "base64_encoded_data"  -- or file path
image.Color = Color3.new(1, 1, 1)  -- Tint color
```

**Properties**:
- `Position`: Vector2 - Top-left position
- `Size`: Vector2 - Width and height
- `Image`: string - Base64 data or file path
- `Color`: Color3 - Tint color
- `Transparency`: number (0-1)
- `Visible`: boolean

## Lua API

### Drawing.new
```lua
Drawing.new(type: string) -> DrawingObject
```
Creates a new drawing object.

**Parameters**:
- `type`: "Line", "Circle", "Square", "Text", "Triangle", "Quad", "Image"

**Returns**: Drawing object

**Example**:
```lua
local line = Drawing.new("Line")
```

### Drawing.clear
```lua
Drawing.clear() -> void
```
Removes all drawing objects.

### Drawing.remove
```lua
Drawing.remove(id: number) -> void
```
Removes drawing object by ID.

**Parameters**:
- `id`: Object ID

### Drawing.get
```lua
Drawing.get(id: number) -> DrawingObject?
```
Gets drawing object by ID.

**Parameters**:
- `id`: Object ID

**Returns**: Object or nil

### Drawing.list
```lua
Drawing.list() -> table
```
Lists all drawing objects.

**Returns**: Array of objects with IDs

### Drawing.setScreenSize
```lua
Drawing.setScreenSize(width: number, height: number) -> void
```
Sets screen dimensions for rendering.

**Parameters**:
- `width`: Screen width
- `height`: Screen height

### Drawing.enable
```lua
Drawing.enable() -> void
```
Enables drawing system.

### Drawing.disable
```lua
Drawing.disable() -> void
```
Disables drawing system.

### Drawing.isEnabled
```lua
Drawing.isEnabled() -> boolean
```
Checks if drawing is enabled.

**Returns**: true if enabled

### Drawing.redraw
```lua
Drawing.redraw() -> void
```
Forces redraw of all objects.

### Drawing.setZIndex
```lua
Drawing.setZIndex(id: number, zindex: number) -> void
```
Sets z-index for object.

**Parameters**:
- `id`: Object ID
- `zindex`: Z-index value

### Drawing.setVisible
```lua
Drawing.setVisible(id: number, visible: boolean) -> void
```
Sets visibility of object.

**Parameters**:
- `id`: Object ID
- `visible`: Visibility

### Drawing.setTransparency
```lua
Drawing.setTransparency(id: number, transparency: number) -> void
```
Sets transparency of object.

**Parameters**:
- `id`: Object ID
- `transparency`: 0.0 to 1.0

### Drawing.setColor
```lua
Drawing.setColor(id: number, color: Color3) -> void
```
Sets color of object.

**Parameters**:
- `id`: Object ID
- `color`: Color3

### Drawing.bringToFront
```lua
Drawing.bringToFront(id: number) -> void
```
Brings object to front (highest z-index).

### Drawing.sendToBack
```lua
Drawing.sendToBack(id: number) -> void
```
Sends object to back (lowest z-index).

### Drawing.clone
```lua
Drawing.clone(id: number) -> number
```
Clones drawing object.

**Parameters**:
- `id`: Object ID to clone

**Returns**: New object ID

### Drawing.setAllVisible
```lua
Drawing.setAllVisible(visible: boolean) -> void
```
Sets visibility of all objects.

## Property Access

All drawing objects support property access:

```lua
local obj = Drawing.new("Line")

-- Set properties
obj.From = Vector2.new(100, 100)
obj.To = Vector2.new(200, 200)
obj.Color = Color3.new(1, 0, 0)
obj.Thickness = 3
obj.Visible = true
obj.ZIndex = 10

-- Get properties
local from = obj.From
local color = obj.Color
local is_visible = obj.Visible
```

### Common Properties

All objects support:
- `Visible`: boolean
- `Transparency`: number (0-1)
- `Color`: Color3
- `ZIndex`: number

## Vector2 and Color3

### Vector2.new
```lua
Vector2.new(x: number, y: number) -> Vector2
```
Creates a 2D vector.

### Color3.new
```lua
Color3.new(r: number, g: number, b: number) -> Color3
```
Creates an RGB color (0-1 range).

### Color3.fromRGB
```lua
Color3.fromRGB(r: number, g: number, b: number) -> Color3
```
Creates color from RGB (0-255 range).

### Color3.fromHex
```lua
Color3.fromHex(hex: string) -> Color3
```
Creates color from hex string.

**Example**:
```lua
Color3.fromHex("#FF0000")  -- Red
Color3.fromHex("00FF00")   -- Green
```

## Rendering Pipeline

### Update Loop
```
1. Check if redraw needed
2. Sort objects by ZIndex
3. For each object:
   - Skip if not visible
   - Apply transparency
   - Render based on type
   - Apply platform-specific drawing
4. Present to screen
```

### Platform-Specific Rendering

#### iOS (CoreGraphics)
- Uses `CGContextRef` for drawing
- CoreText for font rendering
- Quartz 2D for shapes
- Hardware accelerated

#### Android (Native Canvas)
- Uses Android Canvas API via JNI
- Native rendering performance
- Supports hardware acceleration

## Usage Examples

### Basic Drawing
```lua
-- Create a line
local line = Drawing.new("Line")
line.From = Vector2.new(100, 100)
line.To = Vector2.new(500, 100)
line.Color = Color3.new(1, 0, 0)
line.Thickness = 5

-- Create a circle
local circle = Drawing.new("Circle")
circle.Position = Vector2.new(300, 200)
circle.Radius = 50
circle.Color = Color3.new(0, 1, 0)
circle.Filled = true
```

### Text with Outline
```lua
local text = Drawing.new("Text")
text.Position = Vector2.new(400, 300)
text.Text = "Xoron Executor"
text.TextSize = 32
text.Color = Color3.new(1, 1, 1)
text.Outline = true
text.OutlineColor = Color3.new(0, 0, 0)
text.Center = true
```

### Dynamic Shapes
```lua
-- Animated circle
local circle = Drawing.new("Circle")
circle.Filled = true
circle.Color = Color3.new(0.5, 0.5, 1)

while true do
    local time = tick()
    circle.Position = Vector2.new(
        400 + math.sin(time) * 100,
        300 + math.cos(time) * 100
    )
    circle.Radius = 30 + math.sin(time * 2) * 20
    wait(0.016)  -- ~60 FPS
end
```

### HUD Elements
```lua
-- Background
local bg = Drawing.new("Square")
bg.Position = Vector2.new(10, 10)
bg.Size = Vector2.new(200, 150)
bg.Color = Color3.new(0, 0, 0)
bg.Filled = true
bg.Transparency = 0.5

-- Title
local title = Drawing.new("Text")
title.Position = Vector2.new(20, 20)
title.Text = "Player Stats"
title.TextSize = 18
title.Color = Color3.new(1, 1, 1)

-- Health bar
local health_bg = Drawing.new("Square")
health_bg.Position = Vector2.new(20, 50)
health_bg.Size = Vector2.new(180, 20)
health_bg.Color = Color3.new(0.3, 0.3, 0.3)
health_bg.Filled = true

local health = Drawing.new("Square")
health.Position = Vector2.new(20, 50)
health.Size = Vector2.new(180 * 0.75, 20)  -- 75% health
health.Color = Color3.new(0, 1, 0)
health.Filled = true
```

### Triangle and Quad
```lua
-- Triangle (arrow)
local arrow = Drawing.new("Triangle")
arrow.PointA = Vector2.new(400, 100)
arrow.PointB = Vector2.new(380, 140)
arrow.PointC = Vector2.new(420, 140)
arrow.Color = Color3.new(1, 0.5, 0)
arrow.Filled = true

-- Quad (diamond)
local diamond = Drawing.new("Quad")
diamond.PointA = Vector2.new(400, 200)
diamond.PointB = Vector2.new(450, 250)
diamond.PointC = Vector2.new(400, 300)
diamond.PointD = Vector2.new(350, 250)
diamond.Color = Color3.new(1, 0, 1)
diamond.Filled = true
diamond.Thickness = 2
```

### Image Drawing
```lua
-- Load image from file
local image_data = fs.readfile("workspace/avatar.png")
local image = Drawing.new("Image")
image.Position = Vector2.new(100, 100)
image.Size = Vector2.new(100, 100)
image.Image = image_data
image.Color = Color3.new(1, 1, 1)  -- No tint
```

### Layering with ZIndex
```lua
-- Background (z=0)
local bg = Drawing.new("Square")
bg.ZIndex = 0
bg.Size = Vector2.new(800, 600)
bg.Filled = true

-- Middle layer (z=5)
local mid = Drawing.new("Circle")
mid.ZIndex = 5
mid.Radius = 100

-- Top layer (z=10)
local top = Drawing.new("Text")
top.ZIndex = 10
top.Text = "Foreground"

-- Bring to front
Drawing.bringToFront(top.id)
```

### Visibility Control
```lua
local objects = {}

-- Create multiple objects
for i = 1, 10 do
    local obj = Drawing.new("Circle")
    obj.Position = Vector2.new(i * 50, 200)
    obj.Radius = 20
    obj.Color = Color3.new(i/10, 0, 1 - i/10)
    table.insert(objects, obj)
end

-- Toggle visibility
Drawing.setAllVisible(false)

-- Show only odd ones
for i, obj in ipairs(objects) do
    if i % 2 == 1 then
        obj.Visible = true
    end
end
```

### Cloning
```lua
-- Create template
local template = Drawing.new("Square")
template.Size = Vector2.new(50, 50)
template.Color = Color3.new(1, 0, 0)
template.Filled = true

-- Clone and modify
for i = 1, 5 do
    local clone_id = Drawing.clone(template.id)
    local clone = Drawing.get(clone_id)
    clone.Position = Vector2.new(i * 60, 100)
    clone.Color = Color3.new(i/5, 0, 0)
end
```

### Transparency Effects
```lua
local obj = Drawing.new("Circle")
obj.Filled = true
obj.Color = Color3.new(1, 0, 0)

-- Fade in
for alpha = 0, 1, 0.05 do
    obj.Transparency = 1 - alpha
    wait(0.05)
end

-- Pulse
while true do
    for alpha = 0, 1, 0.1 do
        obj.Transparency = 0.5 + 0.5 * math.sin(alpha * math.pi)
        wait(0.016)
    end
end
```

### Dynamic Updates
```lua
-- Create line
local line = Drawing.new("Line")
line.Thickness = 2
line.Color = Color3.new(0, 1, 1)

-- Update based on mouse
while true do
    local mouse = get_mouse_position()
    line.To = mouse
    wait(0.016)
end
```

### Performance Optimization
```lua
-- Batch creation
Drawing.disable()  -- Pause rendering

for i = 1, 100 do
    local obj = Drawing.new("Circle")
    -- Setup object
end

Drawing.enable()  -- Resume and render once
Drawing.redraw()  -- Force update
```

### Color Manipulation
```lua
-- Color from RGB
local red = Color3.fromRGB(255, 0, 0)

-- Color from hex
local blue = Color3.fromHex("#0000FF")

-- Color from decimal
local green = Color3.new(0, 1, 0)

-- Interpolate
local function lerp_color(c1, c2, t)
    return Color3.new(
        c1.r + (c2.r - c1.r) * t,
        c1.g + (c2.g - c1.g) * t,
        c1.b + (c2.b - c1.b) * t
    )
end

-- Rainbow effect
local rainbow = Drawing.new("Square")
rainbow.Filled = true

while true do
    local time = tick()
    rainbow.Color = Color3.new(
        math.sin(time) * 0.5 + 0.5,
        math.sin(time + 2) * 0.5 + 0.5,
        math.sin(time + 4) * 0.5 + 0.5
    )
    wait(0.016)
end
```

### Screen Size Management
```lua
-- Set screen size for proper rendering
local width, height = get_screen_size()
Drawing.setScreenSize(width, height)

-- Responsive positioning
local center = Vector2.new(width / 2, height / 2)
local obj = Drawing.new("Circle")
obj.Position = center
obj.Radius = 50
```

### Object Management
```lua
-- Track objects
local hud = {
    bg = Drawing.new("Square"),
    text = Drawing.new("Text"),
    health = Drawing.new("Square")
}

-- Update all
hud.bg.Size = Vector2.new(200, 100)
hud.text.Text = "HP: 100"
hud.health.Size = Vector2.new(200, 20)

-- Remove all
for _, obj in pairs(hud) do
    Drawing.remove(obj.id)
end
```

### Advanced Shapes
```lua
-- Hexagon using triangles
local function create_hexagon(center, radius, color)
    local points = {}
    for i = 0, 5 do
        local angle = (i * 60) * math.pi / 180
        points[i+1] = Vector2.new(
            center.x + radius * math.cos(angle),
            center.y + radius * math.sin(angle)
        )
    end
    
    -- Create triangles
    for i = 1, 6 do
        local tri = Drawing.new("Triangle")
        tri.PointA = center
        tri.PointB = points[i]
        tri.PointC = points[i % 6 + 1]
        tri.Color = color
        tri.Filled = true
    end
end

create_hexagon(Vector2.new(400, 300), 50, Color3.new(1, 0.5, 0))
```

### Text Effects
```lua
-- Shadowed text
local shadow = Drawing.new("Text")
shadow.Position = Vector2.new(201, 201)
shadow.Text = "Xoron"
shadow.Color = Color3.new(0, 0, 0)
shadow.TextSize = 32

local main = Drawing.new("Text")
main.Position = Vector2.new(200, 200)
main.Text = "Xoron"
main.Color = Color3.new(1, 1, 1)
main.TextSize = 32

-- Multi-colored text (using multiple objects)
local colors = {
    Color3.new(1, 0, 0),
    Color3.new(0, 1, 0),
    Color3.new(0, 0, 1)
}

local text = "RGB"
for i = 1, #text do
    local char = Drawing.new("Text")
    char.Position = Vector2.new(200 + i * 20, 200)
    char.Text = text:sub(i, i)
    char.Color = colors[i]
    char.TextSize = 32
end
```

### Performance Tips

1. **Batch Operations**:
```lua
Drawing.disable()
-- Create/update many objects
Drawing.enable()
Drawing.redraw()
```

2. **Reuse Objects**:
```lua
-- Instead of creating/destroying
local obj = Drawing.new("Circle")
while true do
    obj.Position = get_position()
    wait(0.016)
end
```

3. **Limit Visible Objects**:
```lua
-- Only show what's needed
for _, obj in pairs(objects) do
    obj.Visible = is_on_screen(obj)
end
```

4. **Use Transparency Sparingly**:
```lua
-- Transparency can be slower
obj.Transparency = 0.5  -- Use only when necessary
```

5. **Optimize Z-Index**:
```lua
-- Keep z-index range small
obj.ZIndex = 1  -- Instead of 1000
```

## Platform-Specific Notes

### iOS
- Uses CoreGraphics for all rendering
- Hardware accelerated via GPU
- Supports Retina displays
- Font rendering via CoreText
- Image support: PNG, JPEG (via UIImage)

### Android
- Uses Android Canvas via JNI
- Hardware acceleration available
- Supports various screen densities
- Font rendering via Android Paint
- Image support: PNG, JPEG (via Bitmap)

### Performance
- iOS: ~60 FPS with 100+ objects
- Android: ~60 FPS with 50+ objects
- Depends on device and complexity

## Error Handling

- Invalid object IDs return nil
- Invalid properties are ignored
- Drawing errors logged but don't crash
- Platform rendering failures handled gracefully

## Memory Management

- Objects are automatically cleaned up when removed
- No memory leaks in rendering loop
- Image data cached efficiently
- History limited to prevent overflow

## Related Files
- `xoron.h` (API declarations)
- `xoron_luau.cpp` (Lua bindings)
- `xoron_env.cpp` (environment integration)
- `xoron_android.cpp` (Android rendering)
- `xoron_ios.mm` (iOS rendering)
