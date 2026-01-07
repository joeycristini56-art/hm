# 13. xoron_ui.cpp - User Interface Components

**File Path**: `src/src/xoron_ui.cpp`  
**Size**: 27,976 bytes  
**Lines**: 868

**Platform**: Cross-platform (iOS: UIKit, Android: Native UI)

## Overview
Provides native UI components for the executor interface. Includes menus, buttons, text inputs, sliders, and other UI elements with platform-specific rendering.

## Includes
- `xoron.h` (local)
- `cstdlib`, `cstring`, `cstdio` (system)
- `string`, `vector`, `unordered_map` (system)
- `functional`, `memory` (system)
- `mutex`, `thread` (system)
- Platform-specific:
  - iOS: `UIKit/UIKit.h`, `Foundation/Foundation.h`, `CoreGraphics/CoreGraphics.h`
  - Android: `android/view/View.h`, `android/widget/TextView.h`, `jni.h`
- `lua.h`, `lualib.h` (local - Luau)

## Core Architecture

### UI Component Types
```cpp
enum UIComponentType {
    UI_WINDOW = 0,
    UI_BUTTON,
    UI_TEXT,
    UI_INPUT,
    UI_SLIDER,
    UI_CHECKBOX,
    UI_DROPDOWN,
    UI_LIST,
    UI_PROGRESS,
    UI_SEPARATOR,
    UI_CONTAINER
};
```

### UI State
```cpp
struct UIState {
    std::unordered_map<uint32_t, UIComponent*> components;
    std::mutex mutex;
    std::atomic<uint32_t> next_id{1};
    bool visible;
    bool initialized;
    
    // Platform-specific handles
    #if defined(XORON_PLATFORM_IOS)
        UIWindow* mainWindow;
        UIViewController* rootViewController;
    #elif defined(XORON_PLATFORM_ANDROID)
        jobject mainWindow;
        JNIEnv* env;
    #endif
};
```

### UI Component Base
```cpp
struct UIComponent {
    uint32_t id;
    UIComponentType type;
    std::string text;
    std::string tooltip;
    bool enabled;
    bool visible;
    float x, y;
    float width, height;
    std::vector<uint32_t> children;
    uint32_t parent;
    
    // Callbacks
    std::function<void()> on_click;
    std::function<void(const std::string&)> on_text_change;
    std::function<void(float)> on_value_change;
    
    // Platform-specific
    #if defined(XORON_PLATFORM_IOS)
        UIView* view;
    #elif defined(XORON_PLATFORM_ANDROID)
        jobject view;
    #endif
};
```

## C API Functions

### xoron_ui_create_window
```c
uint32_t xoron_ui_create_window(const char* title, float width, float height)
```
Creates a new window.

**Parameters**:
- `title`: Window title
- `width`: Window width
- `height`: Window height

**Returns**: Component ID

### xoron_ui_create_button
```c
uint32_t xoron_ui_create_button(uint32_t parent, const char* text, void (*callback)(void*), void* user_data)
```
Creates a button.

**Parameters**:
- `parent`: Parent component ID
- `text`: Button text
- `callback`: Click callback
- `user_data`: User data

**Returns**: Component ID

### xoron_ui_create_text
```c
uint32_t xoron_ui_create_text(uint32_t parent, const char* text)
```
Creates text label.

### xoron_ui_create_input
```c
uint32_t xoron_ui_create_input(uint32_t parent, const char* placeholder, void (*callback)(const char*, void*), void* user_data)
```
Creates text input.

### xoron_ui_create_slider
```c
uint32_t xoron_ui_create_slider(uint32_t parent, float min, float max, float value, void (*callback)(float, void*), void* user_data)
```
Creates slider.

### xoron_ui_create_checkbox
```c
uint32_t xoron_ui_create_checkbox(uint32_t parent, const char* text, bool checked, void (*callback)(bool, void*), void* user_data)
```
Creates checkbox.

### xoron_ui_set_text
```c
bool xoron_ui_set_text(uint32_t id, const char* text)
```
Sets component text.

### xoron_ui_set_visible
```c
bool xoron_ui_set_visible(uint32_t id, bool visible)
```
Sets visibility.

### xoron_ui_set_enabled
```c
bool xoron_ui_set_enabled(uint32_t id, bool enabled)
```
Sets enabled state.

### xoron_ui_set_position
```c
bool xoron_ui_set_position(uint32_t id, float x, float y)
```
Sets position.

### xoron_ui_set_size
```c
bool xoron_ui_set_size(uint32_t id, float width, float height)
```
Sets size.

### xoron_ui_destroy
```c
bool xoron_ui_destroy(uint32_t id)
```
Destroys component.

### xoron_ui_show
```c
void xoron_ui_show()
```
Shows the UI.

### xoron_ui_hide
```c
void xoron_ui_hide()
```
Hides the UI.

### xoron_ui_toggle
```c
void xoron_ui_toggle()
```
Toggles UI visibility.

### xoron_ui_clear
```c
void xoron_ui_clear()
```
Clears all UI components.

## Lua API

All functions are registered in the `ui` global table.

### Window Management

#### ui.create_window
```lua
ui.create_window(title: string, width: number, height: number) -> number
```
Creates a window.

**Parameters**:
- `title`: Window title
- `width`: Width in pixels
- `height`: Height in pixels

**Returns**: Window ID

**Example**:
```lua
local window = ui.create_window("Xoron Executor", 800, 600)
```

#### ui.show
```lua
ui.show() -> void
```
Shows the UI.

#### ui.hide
```lua
ui.hide() -> void
```
Hides the UI.

#### ui.toggle
```lua
ui.toggle() -> void
```
Toggles UI visibility.

#### ui.clear
```lua
ui.clear() -> void
```
Clears all UI components.

### Button

#### ui.create_button
```lua
ui.create_button(parent: number, text: string, callback: function?) -> number
```
Creates a button.

**Parameters**:
- `parent`: Parent component ID
- `text`: Button text
- `callback`: Click handler (optional)

**Returns**: Button ID

**Example**:
```lua
local btn = ui.create_button(window, "Click Me", function()
    print("Button clicked!")
end)
```

#### ui.set_button_text
```lua
ui.set_button_text(id: number, text: string) -> void
```
Sets button text.

### Text Label

#### ui.create_text
```lua
ui.create_text(parent: number, text: string) -> number
```
Creates text label.

**Parameters**:
- `parent`: Parent component ID
- `text`: Text content

**Returns**: Text ID

**Example**:
```lua
local label = ui.create_text(window, "Hello, World!")
```

#### ui.set_text
```lua
ui.set_text(id: number, text: string) -> void
```
Sets text content.

### Input Field

#### ui.create_input
```lua
ui.create_input(parent: number, placeholder: string, callback: function?) -> number
```
Creates text input.

**Parameters**:
- `parent`: Parent component ID
- `placeholder`: Placeholder text
- `callback`: Text change handler (optional)

**Returns**: Input ID

**Example**:
```lua
local input = ui.create_input(window, "Enter text...", function(text)
    print("Input changed:", text)
end)
```

#### ui.get_input_text
```lua
ui.get_input_text(id: number) -> string
```
Gets input text.

#### ui.set_input_text
```lua
ui.set_input_text(id: number, text: string) -> void
```
Sets input text.

### Slider

#### ui.create_slider
```lua
ui.create_slider(parent: number, min: number, max: number, value: number, callback: function?) -> number
```
Creates slider.

**Parameters**:
- `parent`: Parent component ID
- `min`: Minimum value
- `max`: Maximum value
- `value`: Initial value
- `callback`: Value change handler (optional)

**Returns**: Slider ID

**Example**:
```lua
local slider = ui.create_slider(window, 0, 100, 50, function(value)
    print("Value:", value)
end)
```

#### ui.get_slider_value
```lua
ui.get_slider_value(id: number) -> number
```
Gets slider value.

#### ui.set_slider_value
```lua
ui.set_slider_value(id: number, value: number) -> void
```
Sets slider value.

### Checkbox

#### ui.create_checkbox
```lua
ui.create_checkbox(parent: number, text: string, checked: boolean, callback: function?) -> number
```
Creates checkbox.

**Parameters**:
- `parent`: Parent component ID
- `text`: Label text
- `checked`: Initial state
- `callback`: State change handler (optional)

**Returns**: Checkbox ID

**Example**:
```lua
local checkbox = ui.create_checkbox(window, "Enable Debug", false, function(checked)
    print("Debug:", checked)
end)
```

#### ui.get_checkbox_state
```lua
ui.get_checkbox_state(id: number) -> boolean
```
Gets checkbox state.

#### ui.set_checkbox_state
```lua
ui.set_checkbox_state(id: number, checked: boolean) -> void
```
Sets checkbox state.

### Dropdown

#### ui.create_dropdown
```lua
ui.create_dropdown(parent: number, items: table, callback: function?) -> number
```
Creates dropdown menu.

**Parameters**:
- `parent`: Parent component ID
- `items`: Array of strings
- `callback`: Selection handler (optional)

**Returns**: Dropdown ID

**Example**:
```lua
local dropdown = ui.create_dropdown(window, {"Option 1", "Option 2", "Option 3"}, function(item)
    print("Selected:", item)
end)
```

#### ui.get_dropdown_selected
```lua
ui.get_dropdown_selected(id: number) -> string
```
Gets selected item.

#### ui.set_dropdown_selected
```lua
ui.set_dropdown_selected(id: number, item: string) -> void
```
Sets selected item.

### List

#### ui.create_list
```lua
ui.create_list(parent: number, items: table, callback: function?) -> number
```
Creates list view.

**Parameters**:
- `parent`: Parent component ID
- `items`: Array of strings
- `callback`: Selection handler (optional)

**Returns**: List ID

**Example**:
```lua
local list = ui.create_list(window, {"Item 1", "Item 2", "Item 3"}, function(item)
    print("Clicked:", item)
end)
```

#### ui.get_list_selected
```lua
ui.get_list_selected(id: number) -> string
```
Gets selected item.

#### ui.set_list_items
```lua
ui.set_list_items(id: number, items: table) -> void
```
Sets list items.

### Progress Bar

#### ui.create_progress
```lua
ui.create_progress(parent: number, value: number, max: number?) -> number
```
Creates progress bar.

**Parameters**:
- `parent`: Parent component ID
- `value`: Current value
- `max`: Maximum value (default: 100)

**Returns**: Progress ID

**Example**:
```lua
local progress = ui.create_progress(window, 0, 100)
```

#### ui.set_progress_value
```lua
ui.set_progress_value(id: number, value: number) -> void
```
Sets progress value.

### Separator

#### ui.create_separator
```lua
ui.create_separator(parent: number, horizontal: boolean?) -> number
```
Creates separator line.

**Parameters**:
- `parent`: Parent component ID
- `horizontal`: true for horizontal (default)

**Returns**: Separator ID

### Container

#### ui.create_container
```lua
ui.create_container(parent: number) -> number
```
Creates container for grouping.

**Parameters**:
- `parent`: Parent component ID

**Returns**: Container ID

### Layout

#### ui.set_position
```lua
ui.set_position(id: number, x: number, y: number) -> void
```
Sets component position.

**Parameters**:
- `id`: Component ID
- `x`: X coordinate
- `y`: Y coordinate

#### ui.set_size
```lua
ui.set_size(id: number, width: number, height: number) -> void
```
Sets component size.

#### ui.set_enabled
```lua
ui.set_enabled(id: number, enabled: boolean) -> void
```
Enables or disables component.

#### ui.set_visible
```lua
ui.set_visible(id: number, visible: boolean) -> void
```
Sets visibility.

#### ui.set_tooltip
```lua
ui.set_tooltip(id: number, tooltip: string) -> void
```
Sets tooltip text.

### State Management

#### ui.get_text
```lua
ui.get_text(id: number) -> string
```
Gets text content of component.

#### ui.get_value
```lua
ui.get_value(id: number) -> any
```
Gets value of component.

#### ui.set_value
```lua
ui.set_value(id: number, value: any) -> void
```
Sets value of component.

### Destruction

#### ui.destroy
```lua
ui.destroy(id: number) -> void
```
Destroys component.

#### ui.destroy_children
```lua
ui.destroy_children(parent: number) -> void
```
Destroys all children of component.

### Event Handling

#### ui.on_click
```lua
ui.on_click(id: number, callback: function) -> void
```
Sets click handler.

#### ui.on_text_change
```lua
ui.on_text_change(id: number, callback: function) -> void
```
Sets text change handler.

#### ui.on_value_change
```lua
ui.on_value_change(id: number, callback: function) -> void
```
Sets value change handler.

## Usage Examples

### Basic Window with Controls
```lua
-- Create main window
local window = ui.create_window("Xoron Control Panel", 600, 400)

-- Add title
local title = ui.create_text(window, "Xoron Executor v2.0")
ui.set_position(title, 250, 20)

-- Add button
local execute_btn = ui.create_button(window, "Execute Script", function()
    print("Executing...")
    -- Execute logic here
end)
ui.set_position(execute_btn, 20, 50)
ui.set_size(execute_btn, 150, 40)

-- Add input field
local input = ui.create_input(window, "Enter script path...", function(text)
    print("Path:", text)
end)
ui.set_position(input, 20, 100)
ui.set_size(input, 300, 30)

-- Add checkbox
local debug_cb = ui.create_checkbox(window, "Debug Mode", false, function(checked)
    print("Debug:", checked)
end)
ui.set_position(debug_cb, 20, 140)

-- Add slider
local slider = ui.create_slider(window, 0, 100, 50, function(value)
    print("Intensity:", value)
end)
ui.set_position(slider, 20, 170)
ui.set_size(slider, 200, 30)

-- Show UI
ui.show()
```

### Settings Panel
```lua
local settings_window = ui.create_window("Settings", 400, 500)

-- Theme selection
ui.create_text(settings_window, "Theme:")
local theme_dropdown = ui.create_dropdown(settings_window, 
    {"Dark", "Light", "Blue"}, 
    function(theme)
        print("Theme changed to:", theme)
    end
)
ui.set_position(theme_dropdown, 100, 20)

-- Font size
ui.create_text(settings_window, "Font Size:")
local font_slider = ui.create_slider(settings_window, 8, 24, 14, function(size)
    print("Font size:", size)
end)
ui.set_position(font_slider, 100, 60)

-- Auto-save
ui.create_checkbox(settings_window, "Auto-save scripts", true, function(checked)
    print("Auto-save:", checked)
end)
ui.set_position(100, 100)

-- Buttons
local save_btn = ui.create_button(settings_window, "Save", function()
    print("Settings saved")
    ui.hide()
end)
ui.set_position(save_btn, 100, 150)

local cancel_btn = ui.create_button(settings_window, "Cancel", function()
    ui.hide()
end)
ui.set_position(cancel_btn, 250, 150)

ui.show()
```

### Script Manager
```lua
local script_window = ui.create_window("Script Manager", 500, 400)

-- List of scripts
local scripts = {"main.lua", "helper.lua", "config.lua"}
local list = ui.create_list(script_window, scripts, function(item)
    print("Selected:", item)
    -- Load script content
    local content = fs.readfile("workspace/" .. item)
    ui.set_text(display, content)
end)
ui.set_position(list, 20, 20)
ui.set_size(list, 150, 300)

-- Display area
local display = ui.create_text(script_window, "Select a script to view")
ui.set_position(display, 190, 20)
ui.set_size(display, 280, 300)

-- Action buttons
local run_btn = ui.create_button(script_window, "Run", function()
    local selected = ui.get_list_selected(list)
    if selected then
        print("Running:", selected)
        -- Execute script
    end
end)
ui.set_position(run_btn, 20, 340)

local refresh_btn = ui.create_button(script_window, "Refresh", function()
    -- Reload script list
    local files = fs.listfiles("workspace")
    ui.set_list_items(list, files)
end)
ui.set_position(refresh_btn, 120, 340)

ui.show()
```

### Progress Dialog
```lua
local progress_window = ui.create_window("Processing", 300, 150)
local progress = ui.create_progress(progress_window, 0, 100)
ui.set_position(progress, 20, 40)
ui.set_size(progress, 250, 30)

local status = ui.create_text(progress_window, "Starting...")
ui.set_position(status, 20, 80)

ui.show()

-- Simulate progress
spawn(function()
    for i = 1, 100 do
        ui.set_progress_value(progress, i)
        ui.set_text(status, "Processing: " .. i .. "%")
        wait(0.05)
    end
    
    ui.set_text(status, "Complete!")
    wait(1)
    ui.hide()
end)
```

### Form Validation
```lua
local form_window = ui.create_window("User Registration", 400, 300)

local name_input = ui.create_input(form_window, "Name", function(text)
    -- Real-time validation
    if #text < 3 then
        ui.set_tooltip(name_input, "Name must be at least 3 characters")
    else
        ui.set_tooltip(name_input, "")
    end
end)
ui.set_position(name_input, 120, 20)
ui.set_size(name_input, 250, 30)

local email_input = ui.create_input(form_window, "Email", function(text)
    -- Email validation
    if not text:find("@") then
        ui.set_tooltip(email_input, "Invalid email format")
    else
        ui.set_tooltip(email_input, "")
    end
end)
ui.set_position(email_input, 120, 60)
ui.set_size(email_input, 250, 30)

local submit_btn = ui.create_button(form_window, "Submit", function()
    local name = ui.get_input_text(name_input)
    local email = ui.get_input_text(email_input)
    
    if #name >= 3 and email:find("@") then
        print("Valid submission:", name, email)
        ui.hide()
    else
        print("Please fix errors")
    end
end)
ui.set_position(submit_btn, 150, 110)

ui.show()
```

### Multi-tab Interface
```lua
local main_window = ui.create_window("Multi-tab UI", 600, 400)

-- Tab buttons
local tab1_btn = ui.create_button(main_window, "Scripts", function()
    show_tab(1)
end)
ui.set_position(tab1_btn, 20, 20)
ui.set_size(tab1_btn, 80, 30)

local tab2_btn = ui.create_button(main_window, "Settings", function()
    show_tab(2)
end)
ui.set_position(tab2_btn, 110, 20)
ui.set_size(tab2_btn, 80, 30)

-- Tab containers
local tab1 = ui.create_container(main_window)
local tab2 = ui.create_container(main_window)

-- Tab 1 content
ui.create_text(tab1, "Scripts Tab")
ui.create_button(tab1, "Load", function() end)

-- Tab 2 content
ui.create_text(tab2, "Settings Tab")
ui.create_checkbox(tab2, "Option", false)

-- Tab switching
function show_tab(tab_num)
    ui.set_visible(tab1, tab_num == 1)
    ui.set_visible(tab2, tab_num == 2)
end

show_tab(1)
ui.show()
```

### Context Menu
```lua
local window = ui.create_window("Context Menu Demo", 400, 300)

local label = ui.create_text(window, "Right-click for menu")
ui.set_position(label, 150, 100)

-- Simulate context menu with buttons
local menu = ui.create_container(window)
ui.set_visible(menu, false)

ui.create_button(menu, "Copy", function()
    print("Copy clicked")
    ui.set_visible(menu, false)
end)
ui.set_position(150, 120)

ui.create_button(menu, "Paste", function()
    print("Paste clicked")
    ui.set_visible(menu, false)
end)
ui.set_position(150, 150)

-- Show menu on button click
ui.create_button(window, "Show Menu", function()
    ui.set_visible(menu, true)
end)
ui.set_position(20, 20)

ui.show()
```

### Dynamic UI
```lua
local window = ui.create_window("Dynamic UI", 500, 400)

local container = ui.create_container(window)
ui.set_position(container, 20, 50)

local add_btn = ui.create_button(window, "Add Item", function()
    local count = #ui.get_list_items(container) + 1
    ui.create_text(container, "Item " .. count)
    -- Layout updates
end)
ui.set_position(add_btn, 20, 20)

local clear_btn = ui.create_button(window, "Clear", function()
    ui.destroy_children(container)
end)
ui.set_position(clear_btn, 120, 20)

ui.show()
```

### Color Picker (Custom)
```lua
local window = ui.create_window("Color Picker", 400, 300)

local r_slider = ui.create_slider(window, 0, 255, 128, update_color)
local g_slider = ui.create_slider(window, 0, 255, 128, update_color)
local b_slider = ui.create_slider(window, 0, 255, 128, update_color)

ui.set_position(r_slider, 20, 20)
ui.set_position(g_slider, 20, 60)
ui.set_position(b_slider, 20, 100)

local preview = ui.create_text(window, "Color Preview")
ui.set_position(preview, 200, 60)
ui.set_size(preview, 100, 50)

function update_color()
    local r = ui.get_slider_value(r_slider)
    local g = ui.get_slider_value(g_slider)
    local b = ui.get_slider_value(b_slider)
    
    ui.set_text(preview, string.format("RGB(%d, %d, %d)", r, g, b))
    -- In real implementation, would change background color
end

update_color()
ui.show()
```

### File Browser
```lua
local window = ui.create_window("File Browser", 600, 400)

local path_input = ui.create_input(window, "Path", function(path)
    refresh_list(path)
end)
ui.set_position(path_input, 20, 20)
ui.set_size(path_input, 400, 30)

local list = ui.create_list(window, {}, function(item)
    local current = ui.get_input_text(path_input)
    local new_path = fs.join(current, item)
    
    if fs.isfolder(new_path) then
        ui.set_input_text(path_input, new_path)
        refresh_list(new_path)
    else
        print("Selected file:", new_path)
    end
end)
ui.set_position(list, 20, 60)
ui.set_size(list, 550, 280)

function refresh_list(path)
    if not path or path == "" then
        path = fs.getworkspace()
    end
    
    local items = fs.listfiles(path)
    ui.set_list_items(list, items)
end

refresh_list(fs.getworkspace())
ui.show()
```

### Console Output
```lua
local window = ui.create_window("Console", 700, 500)

local output = ui.create_text(window, "Console Output:\n")
ui.set_position(output, 20, 20)
ui.set_size(output, 650, 400)

local input = ui.create_input(window, "Enter command...", function(cmd)
    if cmd ~= "" then
        local current = ui.get_text(output)
        ui.set_text(output, current .. "\n> " .. cmd)
        
        -- Simulate command execution
        local result = "Executed: " .. cmd
        ui.set_text(output, ui.get_text(output) .. "\n" .. result)
    end
end)
ui.set_position(input, 20, 440)
ui.set_size(input, 500, 30)

local clear_btn = ui.create_button(window, "Clear", function()
    ui.set_text(output, "Console Output:\n")
end)
ui.set_position(clear_btn, 540, 440)

ui.show()
```

### Login Dialog
```lua
local login_window = ui.create_window("Login", 350, 250)

ui.create_text(login_window, "Username:")
local user_input = ui.create_input(login_window, "Username", nil)
ui.set_position(user_input, 100, 20)
ui.set_size(user_input, 200, 30)

ui.create_text(login_window, "Password:")
local pass_input = ui.create_input(window, "Password", nil)
ui.set_position(pass_input, 100, 60)
ui.set_size(pass_input, 200, 30)

local login_btn = ui.create_button(login_window, "Login", function()
    local user = ui.get_input_text(user_input)
    local pass = ui.get_input_text(pass_input)
    
    if user ~= "" and pass ~= "" then
        print("Login attempt:", user)
        ui.hide()
    else
        print("Please fill all fields")
    end
end)
ui.set_position(login_btn, 100, 110)

local cancel_btn = ui.create_button(login_window, "Cancel", function()
    ui.hide()
end)
ui.set_position(cancel_btn, 200, 110)

ui.show()
```

### Notification System
```lua
function show_notification(message, duration)
    local notif = ui.create_window("Notification", 300, 100)
    ui.set_position(notif, 500, 20)  -- Top right
    
    local msg = ui.create_text(notif, message)
    ui.set_position(msg, 20, 30)
    
    ui.show()
    
    spawn(function()
        wait(duration or 3)
        ui.destroy(notif)
    end)
end

-- Usage
show_notification("Script executed successfully!", 2)
```

### Loading Spinner
```lua
function show_loading(text)
    local load_window = ui.create_window("Loading", 250, 100)
    local msg = ui.create_text(load_window, text or "Loading...")
    ui.set_position(msg, 80, 30)
    
    local progress = ui.create_progress(load_window, 0, 100)
    ui.set_position(progress, 20, 50)
    ui.set_size(progress, 200, 20)
    
    ui.show()
    
    spawn(function()
        for i = 1, 100, 2 do
            ui.set_progress_value(progress, i)
            wait(0.02)
        end
        ui.destroy(load_window)
    end)
end

-- Usage
show_loading("Processing data...")
```

### Data Table
```lua
local window = ui.create_window("Data Table", 600, 400)

-- Headers
local headers = {"Name", "Value", "Status"}
for i, header in ipairs(headers) do
    local h = ui.create_text(window, header)
    ui.set_position(h, 20 + (i-1) * 180, 20)
end

-- Data rows
local data = {
    {"Item 1", "100", "OK"},
    {"Item 2", "250", "Warning"},
    {"Item 3", "50", "Error"}
}

for row, items in ipairs(data) do
    for col, item in ipairs(items) do
        local cell = ui.create_text(window, item)
        ui.set_position(cell, 20 + (col-1) * 180, 50 + (row-1) * 30)
    end
end

ui.show()
```

### Wizard/Step-by-step
```lua
local window = ui.create_window("Setup Wizard", 500, 300)

local step1 = ui.create_container(window)
local step2 = ui.create_container(window)
local step3 = ui.create_container(window)

-- Step 1
ui.create_text(step1, "Step 1: Basic Configuration")
ui.create_input(step1, "Enter name...")
local next1 = ui.create_button(step1, "Next", function()
    ui.set_visible(step1, false)
    ui.set_visible(step2, true)
end)

-- Step 2
ui.create_text(step2, "Step 2: Advanced Settings")
ui.create_checkbox(step2, "Enable feature", false)
local next2 = ui.create_button(step2, "Next", function()
    ui.set_visible(step2, false)
    ui.set_visible(step3, true)
end)
local back2 = ui.create_button(step2, "Back", function()
    ui.set_visible(step2, false)
    ui.set_visible(step1, true)
end)

-- Step 3
ui.create_text(step3, "Step 3: Confirmation")
local finish = ui.create_button(step3, "Finish", function()
    print("Wizard complete!")
    ui.hide()
end)
local back3 = ui.create_button(step3, "Back", function()
    ui.set_visible(step3, false)
    ui.set_visible(step2, true)
end)

-- Layout
ui.set_position(step1, 20, 50)
ui.set_position(step2, 20, 50)
ui.set_position(step3, 20, 50)

ui.set_visible(step2, false)
ui.set_visible(step3, false)

ui.show()
```

## Platform-Specific Implementation

### iOS (UIKit)
- Uses `UIWindow` as root container
- `UIViewController` for view management
- `UIView` subclasses for components
- Auto-layout constraints
- Supports both iPhone and iPad

### Android (Native)
- Uses `android.view.View` hierarchy
- `Activity` for window management
- `LinearLayout` and `RelativeLayout` for layout
- Material Design components
- Supports multiple screen densities

### Layout System
- Absolute positioning (x, y)
- Manual size management
- No automatic layout (for simplicity)
- Parent-child relationships
- Z-ordering via container

## Event System

### Callback Architecture
```cpp
// C++ callback wrapper
void handle_click(void* user_data) {
    // Call Lua function
    lua_State* L = (lua_State*)user_data;
    lua_pcall(L, 0, 0, 0);
}
```

### Event Flow
```
User Action (click)
    ↓
Platform Event
    ↓
C++ Handler
    ↓
Lua Callback
    ↓
User Code
```

## Performance

- **Creation**: O(1) per component
- **Rendering**: Platform-native (fast)
- **Event Handling**: O(1) per event
- **Memory**: ~1KB per component

## Error Handling

- Invalid IDs return false/nil
- Platform failures logged
- Graceful degradation
- No crashes on missing parents

## Thread Safety

- UI operations must be on main thread
- Mutex protects component map
- Callbacks scheduled on main thread
- Safe concurrent access

## Best Practices

1. **Keep UI responsive**: Use spawn for long operations
2. **Destroy unused components**: Prevent memory leaks
3. **Validate inputs**: Check user data
4. **Use tooltips**: Help users understand controls
5. **Group logically**: Use containers
6. **Test on both platforms**: iOS and Android differ

## Related Files
- `xoron.h` (API declarations)
- `xoron_luau.cpp` (Lua bindings)
- `xoron_android.cpp` (Android UI)
- `xoron_ios.mm` (iOS UI)
