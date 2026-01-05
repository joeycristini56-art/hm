--[[
    XORON EXECUTOR UI
    Purple & Black Theme
    Mobile-optimized for iPhone landscape
    
    Features:
    - Draggable window
    - Open/Close toggle button
    - Tabs: Editor, Console, Saved Scripts
    - Execute, Clear, Save, Copy buttons
    - FPS/Ping display
    - Connection status
]]

local XoronUI = {}

-- ==================== CONFIGURATION ====================
local Config = {
    -- Colors (Purple & Black theme)
    Colors = {
        Background = Color3.fromRGB(12, 12, 15),
        BackgroundDark = Color3.fromRGB(15, 15, 18),
        Header = Color3.fromRGB(24, 24, 27),
        Border = Color3.fromRGB(42, 42, 58),
        
        Purple = Color3.fromRGB(147, 51, 234),
        PurpleLight = Color3.fromRGB(168, 85, 247),
        PurpleDark = Color3.fromRGB(109, 40, 217),
        
        Text = Color3.fromRGB(228, 228, 231),
        TextDim = Color3.fromRGB(113, 113, 122),
        TextMuted = Color3.fromRGB(82, 82, 91),
        
        Green = Color3.fromRGB(34, 197, 94),
        Red = Color3.fromRGB(239, 68, 68),
        Yellow = Color3.fromRGB(251, 191, 36),
        Blue = Color3.fromRGB(96, 165, 250),
        Pink = Color3.fromRGB(244, 114, 182),
        
        ButtonBg = Color3.fromRGB(39, 39, 42),
        ButtonHover = Color3.fromRGB(52, 52, 56),
    },
    
    -- Sizing (will be calculated based on screen)
    Window = {
        WidthPercent = 0.65,
        HeightPercent = 0.85,
        MinWidth = 400,
        MinHeight = 280,
        MaxWidth = 700,
        MaxHeight = 400,
        CornerRadius = 12,
        Padding = 12,
    },
    
    -- Toggle button
    ToggleButton = {
        Size = 52,
        Margin = 20,
    },
    
    -- Fonts
    FontSize = {
        Title = 16,
        Tab = 12,
        Button = 13,
        Code = 11,
        Small = 10,
    },
}

-- ==================== STATE ====================
local State = {
    Visible = true,
    CurrentTab = "Editor",
    WindowPosition = nil,
    TogglePosition = nil,
    Dragging = false,
    DragOffset = Vector2.new(0, 0),
    
    EditorText = "-- Welcome to Xoron Executor!\n\nlocal player = game.Players.LocalPlayer\nlocal char = player.Character\n\nif char then\n    char.Humanoid.WalkSpeed = 100\nend\n\nprint(\"Speed boosted!\")",
    EditorScroll = 0,
    
    ConsoleLines = {},
    ConsoleScroll = 0,
    
    SavedScripts = {},
    
    FPS = 60,
    Ping = 45,
    Connected = true,
}

-- ==================== DRAWING ELEMENTS ====================
local Elements = {}

-- ==================== UTILITY FUNCTIONS ====================
local function GetScreenSize()
    local camera = workspace.CurrentCamera
    if camera then
        return camera.ViewportSize
    end
    return Vector2.new(844, 390)
end

local function CalculateWindowSize()
    local screen = GetScreenSize()
    local width = math.clamp(
        screen.X * Config.Window.WidthPercent,
        Config.Window.MinWidth,
        Config.Window.MaxWidth
    )
    local height = math.clamp(
        screen.Y * Config.Window.HeightPercent,
        Config.Window.MinHeight,
        Config.Window.MaxHeight
    )
    return Vector2.new(width, height)
end

local function CalculateWindowPosition()
    local screen = GetScreenSize()
    local size = CalculateWindowSize()
    return Vector2.new(
        (screen.X - size.X) / 2,
        (screen.Y - size.Y) / 2 - 10
    )
end

local function CalculateTogglePosition()
    local screen = GetScreenSize()
    return Vector2.new(
        screen.X - Config.ToggleButton.Size - Config.ToggleButton.Margin,
        Config.ToggleButton.Margin
    )
end

-- ==================== DRAWING FUNCTIONS ====================
local function CreateRect(props)
    local rect = Drawing.new("Square")
    rect.Visible = props.Visible ~= false
    rect.Position = props.Position or Vector2.new(0, 0)
    rect.Size = props.Size or Vector2.new(100, 100)
    rect.Color = props.Color or Config.Colors.Background
    rect.Filled = props.Filled ~= false
    rect.Thickness = props.Thickness or 1
    rect.Transparency = props.Transparency or 1
    return rect
end

local function CreateText(props)
    local text = Drawing.new("Text")
    text.Visible = props.Visible ~= false
    text.Position = props.Position or Vector2.new(0, 0)
    text.Text = props.Text or ""
    text.Size = props.Size or Config.FontSize.Button
    text.Color = props.Color or Config.Colors.Text
    text.Center = props.Center or false
    text.Outline = props.Outline ~= false
    text.OutlineColor = Color3.fromRGB(0, 0, 0)
    text.Transparency = props.Transparency or 1
    return text
end

local function CreateLine(props)
    local line = Drawing.new("Line")
    line.Visible = props.Visible ~= false
    line.From = props.From or Vector2.new(0, 0)
    line.To = props.To or Vector2.new(100, 100)
    line.Color = props.Color or Config.Colors.Text
    line.Thickness = props.Thickness or 2
    line.Transparency = props.Transparency or 1
    return line
end

local function CreateCircle(props)
    local circle = Drawing.new("Circle")
    circle.Visible = props.Visible ~= false
    circle.Position = props.Position or Vector2.new(0, 0)
    circle.Radius = props.Radius or 10
    circle.Color = props.Color or Config.Colors.Purple
    circle.Filled = props.Filled ~= false
    circle.Thickness = props.Thickness or 1
    circle.Transparency = props.Transparency or 1
    return circle
end

local function CreateTriangle(props)
    local tri = Drawing.new("Triangle")
    tri.Visible = props.Visible ~= false
    tri.PointA = props.PointA or Vector2.new(0, 0)
    tri.PointB = props.PointB or Vector2.new(10, 5)
    tri.PointC = props.PointC or Vector2.new(0, 10)
    tri.Color = props.Color or Config.Colors.Text
    tri.Filled = props.Filled ~= false
    tri.Thickness = props.Thickness or 1
    tri.Transparency = props.Transparency or 1
    return tri
end

-- ==================== UI CREATION ====================
function XoronUI.CreateToggleButton()
    local pos = State.TogglePosition or CalculateTogglePosition()
    State.TogglePosition = pos
    local size = Config.ToggleButton.Size
    local center = pos + Vector2.new(size/2, size/2)
    
    Elements.ToggleOuter = CreateCircle({
        Position = center,
        Radius = size/2,
        Color = Config.Colors.Header,
        Filled = true,
    })
    
    Elements.ToggleBorder = CreateCircle({
        Position = center,
        Radius = size/2,
        Color = Config.Colors.Purple,
        Filled = false,
        Thickness = 2,
    })
    
    Elements.ToggleInner = CreateCircle({
        Position = center,
        Radius = size/2 - 6,
        Color = Config.Colors.Purple,
        Filled = true,
    })
    
    local xSize = 14
    Elements.ToggleX1 = CreateLine({
        From = center + Vector2.new(-xSize/2, -xSize/2),
        To = center + Vector2.new(xSize/2, xSize/2),
        Color = Config.Colors.Text,
        Thickness = 2.5,
    })
    Elements.ToggleX2 = CreateLine({
        From = center + Vector2.new(xSize/2, -xSize/2),
        To = center + Vector2.new(-xSize/2, xSize/2),
        Color = Config.Colors.Text,
        Thickness = 2.5,
    })
end

function XoronUI.CreateWindow()
    local pos = State.WindowPosition or CalculateWindowPosition()
    State.WindowPosition = pos
    local size = CalculateWindowSize()
    local padding = Config.Window.Padding
    
    Elements.WindowBg = CreateRect({
        Position = pos,
        Size = size,
        Color = Config.Colors.Background,
        Filled = true,
    })
    
    Elements.WindowBorder = CreateRect({
        Position = pos,
        Size = size,
        Color = Config.Colors.Border,
        Filled = false,
        Thickness = 1,
    })
    
    local headerHeight = 42
    Elements.HeaderBg = CreateRect({
        Position = pos,
        Size = Vector2.new(size.X, headerHeight),
        Color = Config.Colors.Header,
        Filled = true,
    })
    
    Elements.HeaderAccent = CreateRect({
        Position = pos + Vector2.new(0, headerHeight),
        Size = Vector2.new(size.X, 2),
        Color = Config.Colors.Purple,
        Filled = true,
    })
    
    local logoPos = pos + Vector2.new(20, 14)
    local logoSize = 12
    Elements.LogoX1 = CreateLine({
        From = logoPos,
        To = logoPos + Vector2.new(logoSize, logoSize),
        Color = Config.Colors.Purple,
        Thickness = 2.5,
    })
    Elements.LogoX2 = CreateLine({
        From = logoPos + Vector2.new(logoSize, 0),
        To = logoPos + Vector2.new(0, logoSize),
        Color = Config.Colors.Purple,
        Thickness = 2.5,
    })
    
    Elements.TitleText = CreateText({
        Position = pos + Vector2.new(42, 13),
        Text = "XORON",
        Size = Config.FontSize.Title,
        Color = Config.Colors.Text,
    })
    
    Elements.StatusDot = CreateCircle({
        Position = pos + Vector2.new(115, 21),
        Radius = 4,
        Color = State.Connected and Config.Colors.Green or Config.Colors.Red,
        Filled = true,
    })
    
    Elements.StatusText = CreateText({
        Position = pos + Vector2.new(125, 14),
        Text = State.Connected and "Connected" or "Disconnected",
        Size = Config.FontSize.Small,
        Color = State.Connected and Config.Colors.Green or Config.Colors.Red,
    })
    
    Elements.FPSLabel = CreateText({
        Position = pos + Vector2.new(size.X - 160, 14),
        Text = "FPS:",
        Size = Config.FontSize.Small,
        Color = Config.Colors.TextDim,
    })
    Elements.FPSValue = CreateText({
        Position = pos + Vector2.new(size.X - 135, 14),
        Text = tostring(State.FPS),
        Size = Config.FontSize.Small,
        Color = Config.Colors.TextMuted,
    })
    Elements.PingLabel = CreateText({
        Position = pos + Vector2.new(size.X - 105, 14),
        Text = "Ping:",
        Size = Config.FontSize.Small,
        Color = Config.Colors.TextDim,
    })
    Elements.PingValue = CreateText({
        Position = pos + Vector2.new(size.X - 75, 14),
        Text = State.Ping .. "ms",
        Size = Config.FontSize.Small,
        Color = Config.Colors.TextMuted,
    })
    
    local closeBtnPos = pos + Vector2.new(size.X - 36, 8)
    Elements.CloseBtn = CreateRect({
        Position = closeBtnPos,
        Size = Vector2.new(28, 28),
        Color = Config.Colors.ButtonBg,
        Filled = true,
    })
    local closeCenter = closeBtnPos + Vector2.new(14, 14)
    Elements.CloseX1 = CreateLine({
        From = closeCenter + Vector2.new(-5, -5),
        To = closeCenter + Vector2.new(5, 5),
        Color = Config.Colors.Red,
        Thickness = 2,
    })
    Elements.CloseX2 = CreateLine({
        From = closeCenter + Vector2.new(5, -5),
        To = closeCenter + Vector2.new(-5, 5),
        Color = Config.Colors.Red,
        Thickness = 2,
    })
end

function XoronUI.CreateTabBar()
    local pos = State.WindowPosition
    local size = CalculateWindowSize()
    local padding = Config.Window.Padding
    
    local tabY = pos.Y + 52
    local tabWidth = size.X - padding * 2
    local tabHeight = 36
    
    Elements.TabBg = CreateRect({
        Position = Vector2.new(pos.X + padding, tabY),
        Size = Vector2.new(tabWidth, tabHeight),
        Color = Config.Colors.Header,
        Filled = true,
    })
    
    local tabs = {"Editor", "Console", "Saved Scripts"}
    local tabBtnWidth = {100, 100, 120}
    local tabX = pos.X + padding + 4
    
    Elements.TabButtons = {}
    Elements.TabTexts = {}
    
    for i, tabName in ipairs(tabs) do
        local isActive = State.CurrentTab == tabName
        local btnWidth = tabBtnWidth[i]
        
        Elements.TabButtons[i] = CreateRect({
            Position = Vector2.new(tabX, tabY + 4),
            Size = Vector2.new(btnWidth, tabHeight - 8),
            Color = isActive and Config.Colors.Purple or Color3.fromRGB(0, 0, 0),
            Filled = true,
            Transparency = isActive and 1 or 0,
        })
        
        Elements.TabTexts[i] = CreateText({
            Position = Vector2.new(tabX + btnWidth/2 - 20, tabY + 10),
            Text = tabName,
            Size = Config.FontSize.Tab,
            Color = isActive and Config.Colors.Text or Config.Colors.TextDim,
        })
        
        tabX = tabX + btnWidth + 8
    end
end

function XoronUI.CreateEditorContent()
    local pos = State.WindowPosition
    local size = CalculateWindowSize()
    local padding = Config.Window.Padding
    
    local contentY = pos.Y + 96
    local contentWidth = size.X - padding * 2
    local contentHeight = size.Y - 96 - 56
    
    Elements.ContentBg = CreateRect({
        Position = Vector2.new(pos.X + padding, contentY),
        Size = Vector2.new(contentWidth, contentHeight),
        Color = Config.Colors.BackgroundDark,
        Filled = true,
    })
    
    Elements.ContentBorder = CreateRect({
        Position = Vector2.new(pos.X + padding, contentY),
        Size = Vector2.new(contentWidth, contentHeight),
        Color = Config.Colors.Border,
        Filled = false,
        Thickness = 1,
    })
    
    local editorHeaderHeight = 32
    Elements.EditorHeader = CreateRect({
        Position = Vector2.new(pos.X + padding, contentY),
        Size = Vector2.new(contentWidth, editorHeaderHeight),
        Color = Config.Colors.Header,
        Filled = true,
    })
    
    Elements.FileTab = CreateRect({
        Position = Vector2.new(pos.X + padding + 8, contentY + 6),
        Size = Vector2.new(80, 20),
        Color = Config.Colors.ButtonBg,
        Filled = true,
    })
    Elements.FileTabText = CreateText({
        Position = Vector2.new(pos.X + padding + 20, contentY + 9),
        Text = "script.lua",
        Size = Config.FontSize.Small,
        Color = Config.Colors.TextMuted,
    })
    
    Elements.AddTabBtn = CreateRect({
        Position = Vector2.new(pos.X + padding + 96, contentY + 6),
        Size = Vector2.new(24, 20),
        Color = Config.Colors.ButtonBg,
        Filled = true,
    })
    Elements.AddTabText = CreateText({
        Position = Vector2.new(pos.X + padding + 104, contentY + 7),
        Text = "+",
        Size = 14,
        Color = Config.Colors.TextDim,
    })
    
    local lineNumWidth = 30
    Elements.LineNumbersBg = CreateRect({
        Position = Vector2.new(pos.X + padding, contentY + editorHeaderHeight),
        Size = Vector2.new(lineNumWidth, contentHeight - editorHeaderHeight),
        Color = Color3.fromRGB(10, 10, 12),
        Filled = true,
    })
    
    local lines = {}
    for line in State.EditorText:gmatch("[^\n]*") do
        table.insert(lines, line)
    end
    
    local lineY = contentY + editorHeaderHeight + 8
    local lineHeight = 14
    
    Elements.LineNumbers = {}
    Elements.CodeLines = {}
    
    for i, line in ipairs(lines) do
        if i > 12 then break end
        
        Elements.LineNumbers[i] = CreateText({
            Position = Vector2.new(pos.X + padding + 8, lineY),
            Text = tostring(i),
            Size = Config.FontSize.Code,
            Color = Config.Colors.TextMuted,
        })
        
        Elements.CodeLines[i] = CreateText({
            Position = Vector2.new(pos.X + padding + lineNumWidth + 8, lineY),
            Text = line,
            Size = Config.FontSize.Code,
            Color = Config.Colors.Text,
        })
        
        lineY = lineY + lineHeight
    end
    
    Elements.Cursor = CreateRect({
        Position = Vector2.new(pos.X + padding + lineNumWidth + 200, contentY + editorHeaderHeight + 120),
        Size = Vector2.new(2, 14),
        Color = Config.Colors.Purple,
        Filled = true,
    })
end

function XoronUI.CreateActionButtons()
    local pos = State.WindowPosition
    local size = CalculateWindowSize()
    local padding = Config.Window.Padding
    
    local btnY = pos.Y + size.Y - 48
    local btnHeight = 38
    local btnSpacing = 10
    local startX = pos.X + padding
    
    local execWidth = 120
    Elements.ExecuteBtn = CreateRect({
        Position = Vector2.new(startX, btnY),
        Size = Vector2.new(execWidth, btnHeight),
        Color = Config.Colors.Purple,
        Filled = true,
    })
    local triCenter = Vector2.new(startX + 35, btnY + btnHeight/2)
    Elements.ExecuteIcon = CreateTriangle({
        PointA = triCenter + Vector2.new(-5, -6),
        PointB = triCenter + Vector2.new(7, 0),
        PointC = triCenter + Vector2.new(-5, 6),
        Color = Config.Colors.Text,
        Filled = true,
    })
    Elements.ExecuteText = CreateText({
        Position = Vector2.new(startX + 48, btnY + 11),
        Text = "EXECUTE",
        Size = Config.FontSize.Button,
        Color = Config.Colors.Text,
    })
    
    local clearX = startX + execWidth + btnSpacing
    local clearWidth = 90
    Elements.ClearBtn = CreateRect({
        Position = Vector2.new(clearX, btnY),
        Size = Vector2.new(clearWidth, btnHeight),
        Color = Config.Colors.ButtonBg,
        Filled = true,
    })
    Elements.ClearText = CreateText({
        Position = Vector2.new(clearX + 25, btnY + 11),
        Text = "CLEAR",
        Size = Config.FontSize.Button,
        Color = Config.Colors.TextMuted,
    })
    
    local saveX = clearX + clearWidth + btnSpacing
    local saveWidth = 90
    Elements.SaveBtn = CreateRect({
        Position = Vector2.new(saveX, btnY),
        Size = Vector2.new(saveWidth, btnHeight),
        Color = Config.Colors.ButtonBg,
        Filled = true,
    })
    Elements.SaveText = CreateText({
        Position = Vector2.new(saveX + 28, btnY + 11),
        Text = "SAVE",
        Size = Config.FontSize.Button,
        Color = Config.Colors.TextMuted,
    })
    
    local copyX = saveX + saveWidth + btnSpacing
    local copyWidth = 90
    Elements.CopyBtn = CreateRect({
        Position = Vector2.new(copyX, btnY),
        Size = Vector2.new(copyWidth, btnHeight),
        Color = Config.Colors.ButtonBg,
        Filled = true,
    })
    Elements.CopyText = CreateText({
        Position = Vector2.new(copyX + 28, btnY + 11),
        Text = "COPY",
        Size = Config.FontSize.Button,
        Color = Config.Colors.TextMuted,
    })
end

-- ==================== VISIBILITY CONTROL ====================
function XoronUI.SetWindowVisible(visible)
    State.Visible = visible
    
    for name, element in pairs(Elements) do
        if element and type(element) == "userdata" and element.Remove then
            if name:find("Toggle") then
                element.Visible = true
            else
                element.Visible = visible
            end
        elseif type(element) == "table" then
            for _, subElement in pairs(element) do
                if subElement and type(subElement) == "userdata" and subElement.Remove then
                    subElement.Visible = visible
                end
            end
        end
    end
end

function XoronUI.Toggle()
    XoronUI.SetWindowVisible(not State.Visible)
end

-- ==================== TAB SWITCHING ====================
function XoronUI.SwitchTab(tabName)
    State.CurrentTab = tabName
    
    local tabs = {"Editor", "Console", "Saved Scripts"}
    for i, tab in ipairs(tabs) do
        local isActive = tab == tabName
        if Elements.TabButtons and Elements.TabButtons[i] then
            Elements.TabButtons[i].Color = isActive and Config.Colors.Purple or Color3.fromRGB(0, 0, 0)
            Elements.TabButtons[i].Transparency = isActive and 1 or 0
        end
        if Elements.TabTexts and Elements.TabTexts[i] then
            Elements.TabTexts[i].Color = isActive and Config.Colors.Text or Config.Colors.TextDim
        end
    end
end

-- ==================== BUTTON ACTIONS ====================
function XoronUI.Execute()
    local success, err = pcall(function()
        loadstring(State.EditorText)()
    end)
    
    if not success then
        warn("[Xoron] Error: " .. tostring(err))
    end
end

function XoronUI.Clear()
    State.EditorText = ""
    for i, elem in pairs(Elements.CodeLines or {}) do
        if elem then elem.Text = "" end
    end
end

function XoronUI.Save()
    local name = "script_" .. os.time() .. ".lua"
    table.insert(State.SavedScripts, {name = name, code = State.EditorText})
    print("[Xoron] Script saved as " .. name)
end

function XoronUI.Copy()
    if setclipboard then
        setclipboard(State.EditorText)
        print("[Xoron] Script copied to clipboard")
    end
end

-- ==================== CLEANUP ====================
function XoronUI.Destroy()
    for name, element in pairs(Elements) do
        if element and type(element) == "userdata" and element.Remove then
            element:Remove()
        elseif type(element) == "table" then
            for _, subElement in pairs(element) do
                if subElement and type(subElement) == "userdata" and subElement.Remove then
                    subElement:Remove()
                end
            end
        end
    end
    Elements = {}
end

-- ==================== INITIALIZATION ====================
function XoronUI.Init()
    State.WindowPosition = CalculateWindowPosition()
    State.TogglePosition = CalculateTogglePosition()
    
    XoronUI.CreateToggleButton()
    XoronUI.CreateWindow()
    XoronUI.CreateTabBar()
    XoronUI.CreateEditorContent()
    XoronUI.CreateActionButtons()
    
    XoronUI.SwitchTab("Editor")
    
    print("[Xoron] UI initialized")
    return XoronUI
end

-- ==================== UPDATE LOOP ====================
function XoronUI.Update()
    if Elements.FPSValue then
        Elements.FPSValue.Text = tostring(State.FPS)
    end
    if Elements.PingValue then
        Elements.PingValue.Text = State.Ping .. "ms"
    end
    
    if Elements.Cursor and State.CurrentTab == "Editor" then
        local time = tick()
        Elements.Cursor.Visible = (math.floor(time * 2) % 2) == 0
    end
end

-- ==================== INPUT HANDLING ====================
local function IsPointInRect(x, y, rectX, rectY, rectW, rectH)
    return x >= rectX and x <= rectX + rectW and y >= rectY and y <= rectY + rectH
end

local function IsPointInCircle(x, y, cx, cy, radius)
    local dx = x - cx
    local dy = y - cy
    return (dx * dx + dy * dy) <= (radius * radius)
end

function XoronUI.HandleTouchBegan(x, y)
    local pos = State.WindowPosition
    local size = CalculateWindowSize()
    local togglePos = State.TogglePosition
    local toggleSize = Config.ToggleButton.Size
    
    -- Check toggle button click
    if IsPointInCircle(x, y, togglePos.X + toggleSize/2, togglePos.Y + toggleSize/2, toggleSize/2) then
        XoronUI.Toggle()
        return true
    end
    
    if not State.Visible then
        return false
    end
    
    -- Check if in window bounds
    if not IsPointInRect(x, y, pos.X, pos.Y, size.X, size.Y) then
        return false
    end
    
    -- Check header for dragging (top 40px)
    if IsPointInRect(x, y, pos.X, pos.Y, size.X, 40) then
        State.Dragging = true
        State.DragOffset = Vector2.new(x - pos.X, y - pos.Y)
        return true
    end
    
    -- Check close button
    local closeX = pos.X + size.X - 40
    local closeY = pos.Y + 8
    if IsPointInRect(x, y, closeX, closeY, 24, 24) then
        XoronUI.SetWindowVisible(false)
        return true
    end
    
    -- Check tab buttons
    local tabY = pos.Y + 48
    local tabWidth = 100
    local tabs = {"Editor", "Console", "Saved Scripts"}
    for i, tab in ipairs(tabs) do
        local tabX = pos.X + Config.Window.Padding + (i-1) * (tabWidth + 8)
        if IsPointInRect(x, y, tabX, tabY, tabWidth, 32) then
            XoronUI.SwitchTab(tab)
            return true
        end
    end
    
    -- Check action buttons
    local btnY = pos.Y + size.Y - 48
    local btnHeight = 38
    local startX = pos.X + Config.Window.Padding
    
    -- Execute button
    if IsPointInRect(x, y, startX, btnY, 120, btnHeight) then
        XoronUI.Execute()
        return true
    end
    
    -- Clear button
    local clearX = startX + 130
    if IsPointInRect(x, y, clearX, btnY, 90, btnHeight) then
        XoronUI.Clear()
        return true
    end
    
    -- Save button
    local saveX = clearX + 100
    if IsPointInRect(x, y, saveX, btnY, 90, btnHeight) then
        XoronUI.Save()
        return true
    end
    
    -- Copy button
    local copyX = saveX + 100
    if IsPointInRect(x, y, copyX, btnY, 90, btnHeight) then
        XoronUI.Copy()
        return true
    end
    
    return true
end

function XoronUI.HandleTouchMoved(x, y)
    if State.Dragging then
        local newX = x - State.DragOffset.X
        local newY = y - State.DragOffset.Y
        
        -- Clamp to screen bounds
        local screenSize = GetScreenSize()
        local windowSize = CalculateWindowSize()
        newX = math.max(0, math.min(newX, screenSize.X - windowSize.X))
        newY = math.max(0, math.min(newY, screenSize.Y - windowSize.Y))
        
        State.WindowPosition = Vector2.new(newX, newY)
        XoronUI.UpdatePositions()
        return true
    end
    return false
end

function XoronUI.HandleTouchEnded(x, y)
    State.Dragging = false
    return false
end

function XoronUI.UpdatePositions()
    local pos = State.WindowPosition
    local size = CalculateWindowSize()
    
    -- Update main window elements
    if Elements.WindowBg then
        Elements.WindowBg.Position = pos
    end
    if Elements.WindowBorder then
        Elements.WindowBorder.Position = pos
    end
    if Elements.Header then
        Elements.Header.Position = pos
    end
    if Elements.Title then
        Elements.Title.Position = Vector2.new(pos.X + Config.Window.Padding, pos.Y + 12)
    end
    
    -- Update all elements relative to window position
    local offsetX = pos.X - (Elements.WindowBg and Elements.WindowBg.Position.X or 0)
    local offsetY = pos.Y - (Elements.WindowBg and Elements.WindowBg.Position.Y or 0)
    
    -- Update tab bar
    if Elements.TabBar then
        Elements.TabBar.Position = Vector2.new(pos.X, pos.Y + 42)
    end
    
    -- Update tab buttons
    for _, tabBtn in pairs(Elements.TabButtons or {}) do
        if tabBtn and tabBtn.Position then
            tabBtn.Position = Vector2.new(tabBtn.Position.X + offsetX, tabBtn.Position.Y + offsetY)
        end
    end
    
    -- Update content area
    if Elements.ContentArea then
        Elements.ContentArea.Position = Vector2.new(pos.X, pos.Y + 88)
    end
    
    -- Update editor elements
    if Elements.EditorBg then
        Elements.EditorBg.Position = Vector2.new(pos.X + 12, pos.Y + 96)
    end
    if Elements.LineNumberBg then
        Elements.LineNumberBg.Position = Vector2.new(pos.X + 12, pos.Y + 96)
    end
    
    -- Update code lines
    for i, line in pairs(Elements.CodeLines or {}) do
        if line and line.Position then
            line.Position = Vector2.new(pos.X + 60, pos.Y + 96 + (i-1) * 18)
        end
    end
    
    -- Update line numbers
    for i, num in pairs(Elements.LineNumbers or {}) do
        if num and num.Position then
            num.Position = Vector2.new(pos.X + 20, pos.Y + 96 + (i-1) * 18)
        end
    end
    
    -- Update action buttons
    for _, btn in pairs(Elements.ActionButtons or {}) do
        if btn and btn.Position then
            btn.Position = Vector2.new(btn.Position.X + offsetX, btn.Position.Y + offsetY)
        end
    end
    
    -- Update stats bar
    if Elements.StatsBar then
        Elements.StatsBar.Position = Vector2.new(pos.X, pos.Y + Config.Window.Height - 28)
    end
    
    -- Update close button
    if Elements.CloseButton then
        Elements.CloseButton.Position = Vector2.new(pos.X + Config.Window.Width - 40, pos.Y + 8)
    end
end

-- ==================== INPUT SERVICE CONNECTIONS ====================
function XoronUI.ConnectInputs()
    local UserInputService = game:GetService("UserInputService")
    
    -- Touch/Mouse input began
    UserInputService.InputBegan:Connect(function(input, gameProcessed)
        if gameProcessed then return end
        
        if input.UserInputType == Enum.UserInputType.Touch or 
           input.UserInputType == Enum.UserInputType.MouseButton1 then
            local pos = input.Position
            XoronUI.HandleTouchBegan(pos.X, pos.Y)
        end
    end)
    
    -- Touch/Mouse input moved
    UserInputService.InputChanged:Connect(function(input, gameProcessed)
        if input.UserInputType == Enum.UserInputType.Touch or 
           input.UserInputType == Enum.UserInputType.MouseMovement then
            local pos = input.Position
            XoronUI.HandleTouchMoved(pos.X, pos.Y)
        end
    end)
    
    -- Touch/Mouse input ended
    UserInputService.InputEnded:Connect(function(input, gameProcessed)
        if input.UserInputType == Enum.UserInputType.Touch or 
           input.UserInputType == Enum.UserInputType.MouseButton1 then
            local pos = input.Position
            XoronUI.HandleTouchEnded(pos.X, pos.Y)
        end
    end)
    
    -- Keyboard input for editor
    UserInputService.InputBegan:Connect(function(input, gameProcessed)
        if gameProcessed then return end
        if State.CurrentTab ~= "Editor" then return end
        
        if input.UserInputType == Enum.UserInputType.Keyboard then
            local key = input.KeyCode
            
            -- Handle special keys
            if key == Enum.KeyCode.Backspace then
                if #State.EditorText > 0 then
                    State.EditorText = State.EditorText:sub(1, -2)
                    XoronUI.UpdateEditorDisplay()
                end
            elseif key == Enum.KeyCode.Return then
                State.EditorText = State.EditorText .. "\n"
                XoronUI.UpdateEditorDisplay()
            elseif key == Enum.KeyCode.Tab then
                State.EditorText = State.EditorText .. "    "
                XoronUI.UpdateEditorDisplay()
            end
        end
    end)
    
    -- Text input for editor
    UserInputService.TextInput:Connect(function(text, gameProcessed)
        if gameProcessed then return end
        if State.CurrentTab ~= "Editor" then return end
        
        State.EditorText = State.EditorText .. text
        XoronUI.UpdateEditorDisplay()
    end)
end

function XoronUI.UpdateEditorDisplay()
    local lines = {}
    for line in (State.EditorText .. "\n"):gmatch("([^\n]*)\n") do
        table.insert(lines, line)
    end
    
    for i, elem in pairs(Elements.CodeLines or {}) do
        if elem then
            elem.Text = lines[i] or ""
        end
    end
end

-- ==================== STATS UPDATE ====================
function XoronUI.UpdateStats(fps, ping, connected)
    State.FPS = fps or State.FPS
    State.Ping = ping or State.Ping
    State.Connected = connected ~= nil and connected or State.Connected
    
    if Elements.FPSValue then
        Elements.FPSValue.Text = tostring(State.FPS)
    end
    if Elements.PingValue then
        Elements.PingValue.Text = State.Ping .. "ms"
    end
    if Elements.StatusDot then
        Elements.StatusDot.Color = State.Connected and Config.Colors.Green or Config.Colors.Red
    end
end

-- ==================== CONSOLE FUNCTIONS ====================
function XoronUI.AddConsoleLine(text, messageType)
    local color = Config.Colors.Text
    if messageType == "error" then
        color = Config.Colors.Red
    elseif messageType == "warn" then
        color = Config.Colors.Yellow
    elseif messageType == "success" then
        color = Config.Colors.Green
    elseif messageType == "info" then
        color = Config.Colors.Blue
    end
    
    table.insert(State.ConsoleLines, {
        text = text,
        color = color,
        time = os.time()
    })
    
    -- Keep only last 100 lines
    while #State.ConsoleLines > 100 do
        table.remove(State.ConsoleLines, 1)
    end
end

function XoronUI.ClearConsole()
    State.ConsoleLines = {}
end

-- ==================== SAVED SCRIPTS FUNCTIONS ====================
function XoronUI.LoadScript(name)
    for _, script in ipairs(State.SavedScripts) do
        if script.name == name then
            State.EditorText = script.code
            XoronUI.UpdateEditorDisplay()
            XoronUI.SwitchTab("Editor")
            return true
        end
    end
    return false
end

function XoronUI.DeleteScript(name)
    for i, script in ipairs(State.SavedScripts) do
        if script.name == name then
            table.remove(State.SavedScripts, i)
            return true
        end
    end
    return false
end

function XoronUI.GetSavedScripts()
    return State.SavedScripts
end

-- ==================== FILE INTEGRATION ====================
function XoronUI.SaveToFile()
    if writefile then
        local name = "script_" .. os.time() .. ".lua"
        local path = "scripts/" .. name
        writefile(path, State.EditorText)
        table.insert(State.SavedScripts, {name = name, code = State.EditorText, path = path})
        XoronUI.AddConsoleLine("Script saved to " .. path, "success")
        return true
    end
    return false
end

function XoronUI.LoadFromFile(path)
    if readfile and isfile and isfile(path) then
        local content = readfile(path)
        State.EditorText = content
        XoronUI.UpdateEditorDisplay()
        XoronUI.AddConsoleLine("Loaded " .. path, "success")
        return true
    end
    return false
end

function XoronUI.RefreshScriptList()
    if listfiles then
        local files = listfiles("scripts")
        State.SavedScripts = {}
        for _, path in ipairs(files) do
            if path:match("%.lua$") then
                local name = path:match("([^/\\]+)$")
                table.insert(State.SavedScripts, {
                    name = name,
                    path = path,
                    code = nil -- Load on demand
                })
            end
        end
    end
end

return XoronUI
