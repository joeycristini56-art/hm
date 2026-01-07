# 11. xoron_input.cpp - Input Handling Library

**File Path**: `src/src/xoron_input.cpp`  
**Size**: 15,844 bytes  
**Lines**: 561

**Platform**: Cross-platform (with platform-specific simulation)

## Overview
Provides input simulation and detection functions for keyboard, mouse, and touch input. Tracks input state locally and can be connected to actual input handlers when injected into game processes.

## Includes
- `xoron.h` (local)
- `cstdlib`, `cstring`, `cstdio` (system)
- `string`, `vector`, `unordered_map` (system)
- `mutex`, `thread`, `atomic` (system)
- Platform-specific:
  - iOS: `UIKit/UIKit.h`, `CoreGraphics/CoreGraphics.h`
  - Android: `android/input.h`, `android/keycodes.h`
  - Linux: `X11/Xlib.h`, `X11/keysym.h`
- `lua.h`, `lualib.h` (local - Luau)

## Core Structures

### InputState
```cpp
struct InputState {
    // Mouse state
    struct {
        float x, y;
        float delta_x, delta_y;
        bool left_down, right_down, middle_down;
        bool wheel_up, wheel_down;
    } mouse;
    
    // Keyboard state
    std::unordered_map<int, bool> keys;  // keycode -> pressed
    std::vector<int> pressed_keys;       // Keys pressed this frame
    std::vector<int> released_keys;      // Keys released this frame
    
    // Touch state (mobile)
    struct Touch {
        int id;
        float x, y;
        bool active;
    };
    std::vector<Touch> touches;
    
    // Gamepad state
    struct Gamepad {
        int id;
        std::unordered_map<int, float> axes;  // axis -> value
        std::unordered_map<int, bool> buttons; // button -> pressed
    };
    std::vector<Gamepad> gamepads;
    
    std::mutex mutex;
    bool enabled;
};
```

### Key Codes
Standard key codes matching platform conventions:
- `0x08`: Backspace
- `0x09`: Tab
- `0x0D`: Enter
- `0x1B`: Escape
- `0x20`: Space
- `0x30`-`0x39`: 0-9
- `0x41`-`0x5A`: A-Z
- `0x60`-`0x69`: Numpad 0-9
- `0x70`-`0x87`: F1-F24
- Arrow keys, modifiers, etc.

## C API Functions

### xoron_mouse_move
```c
void xoron_mouse_move(float x, float y)
```
Moves mouse to absolute position.

**Parameters**:
- `x`: X coordinate
- `y`: Y coordinate

### xoron_mouse_move_relative
```c
void xoron_mouse_move_relative(float dx, float dy)
```
Moves mouse relative to current position.

**Parameters**:
- `dx`: Delta X
- `dy`: Delta Y

### xoron_mouse_press
```c
void xoron_mouse_press(int button)
```
Presses mouse button.

**Parameters**:
- `button`: 0=left, 1=right, 2=middle

### xoron_mouse_release
```c
void xoron_mouse_release(int button)
```
Releases mouse button.

### xoron_mouse_click
```c
void xoron_mouse_click(int button)
```
Clicks mouse button (press + release).

### xoron_mouse_scroll
```c
void xoron_mouse_scroll(float delta)
```
Scrolls mouse wheel.

**Parameters**:
- `delta`: Scroll amount (positive = up, negative = down)

### xoron_key_press
```c
void xoron_key_press(int keycode)
```
Presses key.

**Parameters**:
- `keycode`: Key code

### xoron_key_release
```c
void xoron_key_release(int keycode)
```
Releases key.

### xoron_key_tap
```c
void xoron_key_tap(int keycode)
```
Taps key (press + release).

### xoron_is_key_down
```c
bool xoron_is_key_down(int keycode)
```
Checks if key is currently down.

**Parameters**:
- `keycode`: Key code

**Returns**: true if down

### xoron_is_mouse_button_down
```c
bool xoron_is_mouse_button_down(int button)
```
Checks if mouse button is down.

**Parameters**:
- `button`: 0=left, 1=right, 2=middle

**Returns**: true if down

### xoron_get_mouse_position
```c
void xoron_get_mouse_position(float* x, float* y)
```
Gets current mouse position.

**Parameters**:
- `x`: Output X
- `y`: Output Y

### xoron_get_mouse_delta
```c
void xoron_get_mouse_delta(float* dx, float* dy)
```
Gets mouse movement since last frame.

**Parameters**:
- `dx`: Delta X
- `dy`: Delta Y

### xoron_get_pressed_keys
```c
int* xoron_get_pressed_keys(int* count)
```
Gets keys pressed this frame.

**Parameters**:
- `count`: Output for array size

**Returns**: Array of keycodes (must be freed)

### xoron_get_released_keys
```c
int* xoron_get_released_keys(int* count)
```
Gets keys released this frame.

### xoron_get_touches
```c
int xoron_get_touches(touch_t** touches)
```
Gets active touch points.

**Parameters**:
- `touches`: Output for touch array

**Returns**: Number of touches

### xoron_get_gamepads
```c
int xoron_get_gamepads(gamepad_t** gamepads)
```
Gets connected gamepads.

### xoron_set_input_enabled
```c
void xoron_set_input_enabled(bool enabled)
```
Enables or disables input simulation.

## Lua API

All functions are registered in the `input` global table.

### Mouse Functions

#### input.mouse_move
```lua
input.mouse_move(x: number, y: number) -> void
```
Moves mouse to absolute position.

**Parameters**:
- `x`: X coordinate
- `y`: Y coordinate

**Example**:
```lua
input.mouse_move(400, 300)  -- Center of 800x600
```

#### input.mouse_moverel
```lua
input.mouse_moverel(dx: number, dy: number) -> void
```
Moves mouse relative to current position.

**Parameters**:
- `dx`: Delta X
- `dy`: Delta Y

**Example**:
```lua
input.mouse_moverel(10, 0)  -- Move right 10 pixels
```

#### input.mouse_press
```lua
input.mouse_press(button: number?) -> void
```
Presses mouse button.

**Parameters**:
- `button`: 0=left (default), 1=right, 2=middle

**Example**:
```lua
input.mouse_press(0)  -- Left click
```

#### input.mouse_release
```lua
input.mouse_release(button: number?) -> void
```
Releases mouse button.

#### input.mouse_click
```lua
input.mouse_click(button: number?) -> void
```
Clicks mouse button.

**Example**:
```lua
input.mouse_click(0)  -- Single left click
```

#### input.mouse_scroll
```lua
input.mouse_scroll(delta: number) -> void
```
Scrolls mouse wheel.

**Parameters**:
- `delta`: Scroll amount

**Example**:
```lua
input.mouse_scroll(1)   -- Scroll up
input.mouse_scroll(-1)  -- Scroll down
```

#### input.get_mouse_position
```lua
input.get_mouse_position() -> x: number, y: number
```
Gets current mouse position.

**Returns**: X and Y coordinates

**Example**:
```lua
local x, y = input.get_mouse_position()
print(string.format("Mouse at (%.0f, %.0f)", x, y))
```

#### input.get_mouse_delta
```lua
input.get_mouse_delta() -> dx: number, dy: number
```
Gets mouse movement since last frame.

**Returns**: Delta X and Y

#### input.is_mouse_button_down
```lua
input.is_mouse_button_down(button: number?) -> boolean
```
Checks if mouse button is down.

**Parameters**:
- `button`: 0=left (default), 1=right, 2=middle

**Returns**: true if down

**Example**:
```lua
if input.is_mouse_button_down(0) then
    print("Left button held")
end
```

### Keyboard Functions

#### input.key_press
```lua
input.key_press(keycode: number) -> void
```
Presses key.

**Parameters**:
- `keycode`: Key code

**Example**:
```lua
input.key_press(0x41)  -- Press 'A'
```

#### input.key_release
```lua
input.key_release(keycode: number) -> void
```
Releases key.

#### input.key_tap
```lua
input.key_tap(keycode: number) -> void
```
Taps key (press + release).

**Example**:
```lua
input.key_tap(0x1B)  -- Tap Escape
```

#### input.is_key_down
```lua
input.is_key_down(keycode: number) -> boolean
```
Checks if key is down.

**Parameters**:
- `keycode`: Key code

**Returns**: true if down

**Example**:
```lua
if input.is_key_down(0x41) then  -- 'A' key
    print("A is held down")
end
```

#### input.get_pressed_keys
```lua
input.get_pressed_keys() -> table
```
Gets keys pressed this frame.

**Returns**: Array of keycodes

**Example**:
```lua
local keys = input.get_pressed_keys()
for _, key in ipairs(keys) do
    print("Pressed:", string.format("0x%02X", key))
end
```

#### input.get_released_keys
```lua
input.get_released_keys() -> table
```
Gets keys released this frame.

**Returns**: Array of keycodes

### Touch Functions (Mobile)

#### input.touch_tap
```lua
input.touch_tap(x: number, y: number) -> void
```
Simulates tap at position.

**Parameters**:
- `x`: X coordinate
- `y`: Y coordinate

**Example**:
```lua
input.touch_tap(400, 300)  -- Tap center
```

#### input.touch_press
```lua
input.touch_press(x: number, y: number, duration: number?) -> void
```
Presses and holds touch.

**Parameters**:
- `x`: X coordinate
- `y`: Y coordinate
- `duration`: Hold time in seconds (default: 0.1)

#### input.touch_swipe
```lua
input.touch_swipe(x1: number, y1: number, x2: number, y2: number, duration: number?) -> void
```
Swipes from one point to another.

**Parameters**:
- `x1`, `y1`: Start position
- `x2`, `y2`: End position
- `duration`: Swipe time (default: 0.2)

**Example**:
```lua
-- Swipe right
input.touch_swipe(200, 300, 600, 300, 0.3)
```

#### input.get_touches
```lua
input.get_touches() -> table
```
Gets active touch points.

**Returns**: Array of touches

**Each touch**:
```lua
{
    id = 1,
    x = 400,
    y = 300,
    active = true
}
```

#### input.is_touching
```lua
input.is_touching() -> boolean
```
Checks if any touch is active.

**Returns**: true if touching

### Gamepad Functions

#### input.gamepad_press
```lua
input.gamepad_press(gamepad_id: number, button: number) -> void
```
Presses gamepad button.

**Parameters**:
- `gamepad_id`: Gamepad index
- `button`: Button code

#### input.gamepad_release
```lua
input.gamepad_release(gamepad_id: number, button: number) -> void
```
Releases gamepad button.

#### input.gamepad_set_axis
```lua
input.gamepad_set_axis(gamepad_id: number, axis: number, value: number) -> void
```
Sets gamepad axis value.

**Parameters**:
- `gamepad_id`: Gamepad index
- `axis`: Axis code
- `value`: Axis value (-1.0 to 1.0)

#### input.get_gamepads
```lua
input.get_gamepads() -> table
```
Gets connected gamepads.

**Returns**: Array of gamepad info

**Each gamepad**:
```lua
{
    id = 0,
    axes = {left_x = 0.5, left_y = 0.0},
    buttons = {a = true, b = false}
}
```

#### input.is_gamepad_connected
```lua
input.is_gamepad_connected(gamepad_id: number?) -> boolean
```
Checks if gamepad is connected.

**Parameters**:
- `gamepad_id`: Gamepad index (default: 0)

**Returns**: true if connected

### Utility Functions

#### input.get_state
```lua
input.get_state() -> table
```
Gets complete input state.

**Returns**: Table with all input data

**Example**:
```lua
local state = input.get_state()
print("Mouse:", state.mouse.x, state.mouse.y)
print("Keys down:", #state.keyboard.down)
```

#### input.set_enabled
```lua
input.set_enabled(enabled: boolean) -> void
```
Enables or disables input simulation.

**Parameters**:
- `enabled`: Enable flag

#### input.is_enabled
```lua
input.is_enabled() -> boolean
```
Checks if input is enabled.

**Returns**: true if enabled

#### input.clear
```lua
input.clear() -> void
```
Clears all input state.

#### input.update
```lua
input.update() -> void
```
Updates input state (called automatically).

### Key Code Constants

Common key codes are available as constants:

```lua
-- Letters
input.KEY_A = 0x41
input.KEY_B = 0x42
-- ... KEY_B through KEY_Z

-- Numbers
input.KEY_0 = 0x30
input.KEY_1 = 0x31
-- ... KEY_0 through KEY_9

-- Function keys
input.KEY_F1 = 0x70
input.KEY_F2 = 0x71
-- ... KEY_F1 through KEY_F24

-- Special keys
input.KEY_BACKSPACE = 0x08
input.KEY_TAB = 0x09
input.KEY_ENTER = 0x0D
input.KEY_ESCAPE = 0x1B
input.KEY_SPACE = 0x20

-- Arrow keys
input.KEY_LEFT = 0x25
input.KEY_UP = 0x26
input.KEY_RIGHT = 0x27
input.KEY_DOWN = 0x28

-- Modifiers
input.KEY_SHIFT = 0x10
input.KEY_CONTROL = 0x11
input.KEY_ALT = 0x12
input.KEY_CAPSLOCK = 0x14

-- Numpad
input.KEY_NUMPAD0 = 0x60
input.KEY_NUMPAD1 = 0x61
-- ... through KEY_NUMPAD9

-- Mouse buttons
input.MOUSE_LEFT = 0
input.MOUSE_RIGHT = 1
input.MOUSE_MIDDLE = 2
```

## Usage Examples

### Basic Mouse Movement
```lua
-- Move to center
input.mouse_move(400, 300)

-- Move relative
for i = 1, 10 do
    input.mouse_moverel(5, 0)
    wait(0.1)
end
```

### Click Sequence
```lua
-- Double click
input.mouse_click(0)
wait(0.1)
input.mouse_click(0)
```

### Keyboard Typing
```lua
-- Type "Hello"
local text = "Hello"
for char in text:gmatch(".") do
    local keycode = string.byte(char:upper())
    input.key_tap(keycode)
    wait(0.05)
end
```

### Drag and Drop
```lua
-- Click and drag
input.mouse_press(0)
wait(0.1)
input.mouse_move(400, 300)
wait(0.2)
input.mouse_release(0)
```

### Key Combination
```lua
-- Ctrl+C
input.key_press(input.KEY_CONTROL)
input.key_tap(input.KEY_C)
input.key_release(input.KEY_CONTROL)
```

### Mobile Swipe
```lua
-- Swipe up to refresh
input.touch_swipe(400, 500, 400, 200, 0.3)
```

### Gamepad Simulation
```lua
-- Press A button
input.gamepad_press(0, 0)  -- Button 0 = A

-- Move left stick
input.gamepad_set_axis(0, 0, -0.5)  -- Left X axis, left
input.gamepad_set_axis(0, 1, 0.0)   -- Left Y axis, center
```

### Input Recording
```lua
local recording = {}

-- Record inputs
function start_recording()
    recording = {}
    local start_time = os.clock()
    
    hookfunction(input.mouse_move, function(x, y)
        table.insert(recording, {
            time = os.clock() - start_time,
            type = "mouse_move",
            x = x, y = y
        })
        return original_mouse_move(x, y)
    end)
end

-- Playback
function playback()
    for _, event in ipairs(recording) do
        wait(event.time)
        if event.type == "mouse_move" then
            input.mouse_move(event.x, event.y)
        end
    end
end
```

### Macro System
```lua
local macros = {
    ["combo1"] = function()
        input.key_tap(input.KEY_SPACE)
        wait(0.1)
        input.mouse_click(0)
        wait(0.1)
        input.key_tap(input.KEY_R)
    end,
    
    ["combo2"] = function()
        input.key_press(input.KEY_SHIFT)
        input.key_tap(input.KEY_W)
        input.key_release(input.KEY_SHIFT)
    end
}

-- Execute macro
function execute_macro(name)
    if macros[name] then
        macros[name]()
    end
end

-- Usage
execute_macro("combo1")
```

### Input State Check
```lua
-- Continuous movement while key held
while true do
    if input.is_key_down(input.KEY_W) then
        input.mouse_moverel(0, -5)
    end
    if input.is_key_down(input.KEY_S) then
        input.mouse_moverel(0, 5)
    end
    wait(0.016)
end
```

### Touch Gestures
```lua
-- Pinch to zoom
local center_x, center_y = 400, 300
local distance = 100

-- Two-finger pinch
input.touch_tap(center_x - distance, center_y)
input.touch_tap(center_x + distance, center_y)

-- Zoom out
wait(0.2)
input.touch_swipe(center_x - distance, center_y, center_x - distance * 0.5, center_y)
input.touch_swipe(center_x + distance, center_y, center_x + distance * 0.5, center_y)
```

### Input Filtering
```lua
-- Only allow certain keys
local allowed_keys = {
    [input.KEY_W] = true,
    [input.KEY_A] = true,
    [input.KEY_S] = true,
    [input.KEY_D] = true
}

function safe_key_press(keycode)
    if allowed_keys[keycode] then
        input.key_press(keycode)
    end
end
```

### Mouse Smoothing
```lua
-- Smooth mouse movement
local target_x, target_y = 400, 300
local current_x, current_y = input.get_mouse_position()

function update_mouse()
    -- Lerp towards target
    current_x = current_x + (target_x - current_x) * 0.1
    current_y = current_y + (target_y - current_y) * 0.1
    
    input.mouse_move(current_x, current_y)
end
```

### Input Buffer
```lua
-- Buffer inputs for combos
local input_buffer = {}
local buffer_time = 0.2

function add_to_buffer(keycode)
    table.insert(input_buffer, {
        key = keycode,
        time = os.clock()
    })
end

function check_buffer(combo)
    -- Check if buffer matches combo
    for i, key in ipairs(combo) do
        if not input_buffer[i] or input_buffer[i].key ~= key then
            return false
        end
    end
    return true
end

-- Usage
add_to_buffer(input.KEY_Q)
add_to_buffer(input.KEY_W)
add_to_buffer(input.KEY_E)

if check_buffer({input.KEY_Q, input.KEY_W, input.KEY_E}) then
    print("Combo executed!")
end
```

### Input Debugging
```lua
-- Debug input state
function debug_input()
    local x, y = input.get_mouse_position()
    print(string.format("Mouse: (%.0f, %.0f)", x, y))
    
    local pressed = input.get_pressed_keys()
    if #pressed > 0 then
        print("Pressed:", table.concat(pressed, ", "))
    end
    
    if input.is_mouse_button_down(0) then
        print("Left button down")
    end
end

-- Show input every second
while true do
    debug_input()
    wait(1)
end
```

### Input Locking
```lua
-- Lock input to specific area
local lock_x, lock_y = 400, 300
local lock_radius = 100

function lock_input()
    local x, y = input.get_mouse_position()
    local dx = x - lock_x
    local dy = y - lock_y
    local dist = math.sqrt(dx*dx + dy*dy)
    
    if dist > lock_radius then
        -- Clamp to circle
        local angle = math.atan2(dy, dx)
        input.mouse_move(
            lock_x + math.cos(angle) * lock_radius,
            lock_y + math.sin(angle) * lock_radius
        )
    end
end
```

## Platform-Specific Notes

### iOS
- Uses UIKit for touch simulation
- Requires integration with game's input system
- Supports game controllers (MFi)
- Touch events go through `UIWindow`

### Android
- Uses Android NDK input APIs
- Requires `android.permission.INJECT_EVENTS`
- Works with both touch and keyboard
- Gamepad via `android/input.h`

### Desktop (Linux)
- Uses X11/Xlib for input
- Requires X server access
- Works with virtual displays
- Supports XTest extension

## Performance

- **Latency**: ~1-2ms for local simulation
- **Throughput**: 1000+ events per second
- **Memory**: Minimal (state only)
- **CPU**: Negligible when idle

## Security Considerations

- **Permissions**: May require special permissions on mobile
- **Detection**: Some games detect synthetic input
- **Rate Limiting**: Avoid flooding with events
- **Ethics**: Use responsibly, respect ToS

## Error Handling

- Invalid keycodes are ignored
- Out-of-bounds coordinates are clamped
- Disabled input silently fails
- Platform errors logged but don't crash

## Related Files
- `xoron.h` (API declarations)
- `xoron_luau.cpp` (Lua bindings)
- `xoron_env.cpp` (environment integration)
