/*
 * xoron_ui.cpp - In-game executor UI for Xoron
 * Renders a mobile-friendly executor menu using the Drawing library
 * Purple & Black theme, dynamically sized for iPhone landscape
 * Platforms: iOS 15+ (.dylib) and Android 10+ (.so)
 * 
 * iOS: Uses UIKit for native touch handling and haptic feedback
 * Android: Uses native touch events
 */

#include "xoron.h"
#include <string>
#include <vector>
#include <functional>
#include <cmath>
#include <mutex>
#include <sstream>
#include <ctime>

#include "lua.h"
#include "lualib.h"

// Platform-specific includes
#if defined(__APPLE__)
    #include <TargetConditionals.h>
    #if TARGET_OS_IPHONE
        #include <objc/objc.h>
        #include <objc/runtime.h>
        #include <objc/message.h>
        #define XORON_UI_IOS 1
    #endif
#elif defined(__ANDROID__)
    #include <android/log.h>
    #define XORON_UI_ANDROID 1
#endif

// Forward declarations for iOS native UI functions
#ifdef XORON_UI_IOS
extern "C" {
    void xoron_ios_ui_init(void);
    void xoron_ios_ui_show(void);
    void xoron_ios_ui_hide(void);
    void xoron_ios_ui_toggle(void);
    void xoron_ios_haptic_feedback(int style);
    void xoron_ios_console_print(const char* message, int type);
}
#endif

// Android haptic feedback and UI functions
#ifdef XORON_UI_ANDROID
#include <jni.h>

static JavaVM* g_ui_jvm = nullptr;
static jobject g_vibrator = nullptr;
static jobject g_ui_activity = nullptr;

static JNIEnv* get_ui_jni_env() {
    if (!g_ui_jvm) return nullptr;
    JNIEnv* env = nullptr;
    g_ui_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    return env;
}

// Android haptic feedback using Vibrator service
extern "C" void xoron_android_haptic_feedback(int style) {
    JNIEnv* env = get_ui_jni_env();
    if (!env || !g_vibrator) return;
    
    // Duration based on style: 0=light(10ms), 1=medium(20ms), 2=heavy(40ms)
    jlong duration = (style == 0) ? 10 : (style == 2) ? 40 : 20;
    
    jclass vibratorClass = env->GetObjectClass(g_vibrator);
    
    // Try VibrationEffect for API 26+ first
    jclass vibrationEffectClass = env->FindClass("android/os/VibrationEffect");
    if (vibrationEffectClass && !env->ExceptionCheck()) {
        jmethodID createOneShot = env->GetStaticMethodID(vibrationEffectClass, 
            "createOneShot", "(JI)Landroid/os/VibrationEffect;");
        if (createOneShot) {
            // VibrationEffect.DEFAULT_AMPLITUDE = -1
            jobject effect = env->CallStaticObjectMethod(vibrationEffectClass, createOneShot, duration, -1);
            if (effect) {
                jmethodID vibrate = env->GetMethodID(vibratorClass, "vibrate", "(Landroid/os/VibrationEffect;)V");
                if (vibrate) {
                    env->CallVoidMethod(g_vibrator, vibrate, effect);
                    env->DeleteLocalRef(effect);
                    return;
                }
                env->DeleteLocalRef(effect);
            }
        }
    }
    env->ExceptionClear();
    
    // Fallback for older APIs
    jmethodID vibrate = env->GetMethodID(vibratorClass, "vibrate", "(J)V");
    if (vibrate) {
        env->CallVoidMethod(g_vibrator, vibrate, duration);
    }
}

// Initialize Android UI (called from JNI)
extern "C" JNIEXPORT void JNICALL
Java_com_xoron_UI_init(JNIEnv* env, jobject obj, jobject activity, jobject vibrator) {
    env->GetJavaVM(&g_ui_jvm);
    g_ui_activity = env->NewGlobalRef(activity);
    if (vibrator) {
        g_vibrator = env->NewGlobalRef(vibrator);
    }
}

// Show/hide/toggle UI on Android (calls back to Java)
extern "C" void xoron_android_ui_show(void) {
    JNIEnv* env = get_ui_jni_env();
    if (!env || !g_ui_activity) return;
    
    jclass activityClass = env->GetObjectClass(g_ui_activity);
    jmethodID showUI = env->GetMethodID(activityClass, "showXoronUI", "()V");
    if (showUI) {
        env->CallVoidMethod(g_ui_activity, showUI);
    }
}

extern "C" void xoron_android_ui_hide(void) {
    JNIEnv* env = get_ui_jni_env();
    if (!env || !g_ui_activity) return;
    
    jclass activityClass = env->GetObjectClass(g_ui_activity);
    jmethodID hideUI = env->GetMethodID(activityClass, "hideXoronUI", "()V");
    if (hideUI) {
        env->CallVoidMethod(g_ui_activity, hideUI);
    }
}

extern "C" void xoron_android_ui_toggle(void) {
    JNIEnv* env = get_ui_jni_env();
    if (!env || !g_ui_activity) return;
    
    jclass activityClass = env->GetObjectClass(g_ui_activity);
    jmethodID toggleUI = env->GetMethodID(activityClass, "toggleXoronUI", "()V");
    if (toggleUI) {
        env->CallVoidMethod(g_ui_activity, toggleUI);
    }
}

// Send console message to Android UI
extern "C" void xoron_android_console_print(const char* message, int type) {
    JNIEnv* env = get_ui_jni_env();
    if (!env || !g_ui_activity || !message) return;
    
    jclass activityClass = env->GetObjectClass(g_ui_activity);
    jmethodID addConsole = env->GetMethodID(activityClass, "addConsoleMessage", "(Ljava/lang/String;I)V");
    if (addConsole) {
        jstring jmsg = env->NewStringUTF(message);
        env->CallVoidMethod(g_ui_activity, addConsole, jmsg, type);
        env->DeleteLocalRef(jmsg);
    }
}
#endif // XORON_UI_ANDROID

// UI State
namespace XoronUI {

// Colors (RGBA format for Drawing library)
struct Color {
    int r, g, b, a;
    Color(int r = 255, int g = 255, int b = 255, int a = 255) : r(r), g(g), b(b), a(a) {}
};

// Theme colors
namespace Theme {
    const Color Background(12, 12, 15, 245);      // #0c0c0f with alpha
    const Color HeaderBg(24, 24, 27, 255);        // #18181b
    const Color PurplePrimary(147, 51, 234, 255); // #9333ea
    const Color PurpleSecondary(124, 58, 237, 255); // #7c3aed
    const Color ButtonBg(39, 39, 42, 255);        // #27272a
    const Color TextPrimary(255, 255, 255, 255);  // White
    const Color TextSecondary(161, 161, 170, 255); // #a1a1aa
    const Color TextMuted(113, 113, 122, 255);    // #71717a
    const Color Green(34, 197, 94, 255);          // #22c55e
    const Color Red(239, 68, 68, 255);            // #ef4444
    const Color EditorBg(15, 15, 18, 255);        // #0f0f12
    const Color LineNumberBg(10, 10, 12, 255);    // #0a0a0c
    const Color Border(42, 42, 58, 255);          // #2a2a3a
    
    // Syntax highlighting
    const Color SyntaxKeyword(192, 132, 252, 255);  // #c084fc - purple
    const Color SyntaxString(74, 222, 128, 255);    // #4ade80 - green
    const Color SyntaxNumber(244, 114, 182, 255);   // #f472b6 - pink
    const Color SyntaxComment(107, 114, 128, 255);  // #6b7280 - gray
    const Color SyntaxGlobal(251, 191, 36, 255);    // #fbbf24 - yellow
    const Color SyntaxProperty(96, 165, 250, 255);  // #60a5fa - blue
}

// UI Element types
enum class ElementType {
    Rectangle,
    Text,
    Line,
    Circle
};

// Drawing element
struct DrawElement {
    ElementType type;
    float x, y, w, h;
    Color color;
    Color outlineColor;
    float outlineThickness;
    std::string text;
    int fontSize;
    bool filled;
    float radius;
    bool visible;
    int zIndex;
    
    DrawElement() : x(0), y(0), w(0), h(0), fontSize(14), filled(true), 
                    radius(0), outlineThickness(0), visible(true), zIndex(0) {}
};

// Tab enum
enum class Tab {
    Editor,
    Console,
    SavedScripts
};

// Console message type
enum class ConsoleMessageType {
    Info,
    Success,
    Warning,
    Error,
    Print
};

struct ConsoleMessage {
    std::string text;
    ConsoleMessageType type;
    std::string timestamp;
};

// Saved script
struct SavedScript {
    std::string name;
    std::string content;
};

// UI State
class UIState {
public:
    bool isOpen = false;
    bool isDragging = false;
    float windowX = 60;
    float windowY = 25;
    float windowWidth = 560;
    float windowHeight = 340;
    
    // Toggle button position
    float toggleX = 780;
    float toggleY = 20;
    float toggleRadius = 26;
    
    // Screen dimensions (will be set dynamically)
    float screenWidth = 844;
    float screenHeight = 390;
    
    // Current tab
    Tab currentTab = Tab::Editor;
    
    // Editor state
    std::string editorContent = "-- Welcome to Xoron Executor!\n\nlocal player = game.Players.LocalPlayer\nlocal char = player.Character\n\nif char then\n    char.Humanoid.WalkSpeed = 100\nend\n\nprint(\"Speed boosted!\")";
    int cursorPosition = 0;
    int scrollOffset = 0;
    std::string currentFileName = "script.lua";
    
    // Console state
    std::vector<ConsoleMessage> consoleMessages;
    int consoleScrollOffset = 0;
    
    // Saved scripts
    std::vector<SavedScript> savedScripts;
    
    // Performance stats
    int fps = 60;
    int ping = 45;
    bool connected = true;
    
    // Touch state
    float touchStartX = 0;
    float touchStartY = 0;
    float windowStartX = 0;
    float windowStartY = 0;
    
    std::mutex stateMutex;
    
    UIState() {
        // Add some default saved scripts
        savedScripts.push_back({"Speed Hack", "game.Players.LocalPlayer.Character.Humanoid.WalkSpeed = 100"});
        savedScripts.push_back({"Jump Power", "game.Players.LocalPlayer.Character.Humanoid.JumpPower = 100"});
        savedScripts.push_back({"Infinite Jump", "-- Infinite Jump Script\nlocal uis = game:GetService(\"UserInputService\")\nuis.JumpRequest:Connect(function()\n    game.Players.LocalPlayer.Character.Humanoid:ChangeState(\"Jumping\")\nend)"});
    }
    
    void addConsoleMessage(const std::string& text, ConsoleMessageType type) {
        std::lock_guard<std::mutex> lock(stateMutex);
        ConsoleMessage msg;
        msg.text = text;
        msg.type = type;
        
        // Get current time
        time_t now = time(nullptr);
        struct tm* t = localtime(&now);
        char buf[16];
        snprintf(buf, sizeof(buf), "%02d:%02d:%02d", t->tm_hour, t->tm_min, t->tm_sec);
        msg.timestamp = buf;
        
        consoleMessages.push_back(msg);
        
        // Keep only last 100 messages
        if (consoleMessages.size() > 100) {
            consoleMessages.erase(consoleMessages.begin());
        }
        
#ifdef XORON_UI_IOS
        // Also send to native iOS console
        xoron_ios_console_print(text.c_str(), static_cast<int>(type));
#elif defined(XORON_UI_ANDROID)
        // Also send to native Android console
        xoron_android_console_print(text.c_str(), static_cast<int>(type));
#endif
    }
    
    void clearConsole() {
        std::lock_guard<std::mutex> lock(stateMutex);
        consoleMessages.clear();
    }
    
    void clearEditor() {
        std::lock_guard<std::mutex> lock(stateMutex);
        editorContent = "";
        cursorPosition = 0;
    }
    
    void saveScript(const std::string& name) {
        std::lock_guard<std::mutex> lock(stateMutex);
        // Check if script with same name exists
        for (auto& script : savedScripts) {
            if (script.name == name) {
                script.content = editorContent;
                return;
            }
        }
        savedScripts.push_back({name, editorContent});
    }
    
    void loadScript(const std::string& name) {
        std::lock_guard<std::mutex> lock(stateMutex);
        for (const auto& script : savedScripts) {
            if (script.name == name) {
                editorContent = script.content;
                currentFileName = name + ".lua";
                cursorPosition = 0;
                scrollOffset = 0;
                currentTab = Tab::Editor;
                return;
            }
        }
    }
    
    void deleteScript(const std::string& name) {
        std::lock_guard<std::mutex> lock(stateMutex);
        for (auto it = savedScripts.begin(); it != savedScripts.end(); ++it) {
            if (it->name == name) {
                savedScripts.erase(it);
                return;
            }
        }
    }
    
    // Calculate dynamic window size based on screen
    void updateWindowSize() {
        // Window takes about 65% of screen width, centered vertically
        windowWidth = screenWidth * 0.65f;
        windowHeight = screenHeight * 0.87f;
        
        // Position window with some margin
        windowX = 60;
        windowY = (screenHeight - windowHeight) / 2;
        
        // Toggle button in top right
        toggleX = screenWidth - 70;
        toggleY = 20;
    }
};

// Global UI state
static UIState g_uiState;

// Helper to check if point is inside rectangle
bool isPointInRect(float px, float py, float rx, float ry, float rw, float rh) {
    return px >= rx && px <= rx + rw && py >= ry && py <= ry + rh;
}

// Helper to check if point is inside circle
bool isPointInCircle(float px, float py, float cx, float cy, float r) {
    float dx = px - cx;
    float dy = py - cy;
    return (dx * dx + dy * dy) <= (r * r);
}

// Split string by newlines
std::vector<std::string> splitLines(const std::string& str) {
    std::vector<std::string> lines;
    std::istringstream stream(str);
    std::string line;
    while (std::getline(stream, line)) {
        lines.push_back(line);
    }
    if (lines.empty()) lines.push_back("");
    return lines;
}

// Lua function to render the UI
static int lua_render_ui(lua_State* L) {
    // This function should be called every frame to render the UI
    // It uses the Drawing library functions
    
    if (!g_uiState.isOpen) {
        // Only render toggle button when closed
        // Draw toggle button background
        lua_getglobal(L, "Drawing");
        lua_getfield(L, -1, "new");
        lua_pushstring(L, "Circle");
        lua_call(L, 1, 1);
        
        // Set circle properties using Vector2 format
        lua_newtable(L);
        lua_pushnumber(L, g_uiState.toggleX + g_uiState.toggleRadius);
        lua_setfield(L, -2, "X");
        lua_pushnumber(L, g_uiState.toggleY + g_uiState.toggleRadius);
        lua_setfield(L, -2, "Y");
        lua_setfield(L, -2, "Position");
        
        lua_pushnumber(L, g_uiState.toggleRadius);
        lua_setfield(L, -2, "Radius");
        
        lua_pushboolean(L, true);
        lua_setfield(L, -2, "Filled");
        
        // Set purple color
        lua_newtable(L);
        lua_pushnumber(L, 147.0/255.0);
        lua_setfield(L, -2, "R");
        lua_pushnumber(L, 51.0/255.0);
        lua_setfield(L, -2, "G");
        lua_pushnumber(L, 234.0/255.0);
        lua_setfield(L, -2, "B");
        lua_setfield(L, -2, "Color");
        
        lua_pushboolean(L, true);
        lua_setfield(L, -2, "Visible");
        
        lua_pop(L, 1); // Pop the circle object
        
        return 0;
    }
    
    // Render full UI when open
    // This would use the Drawing library to render all elements
    
    return 0;
}

// Lua function to handle touch/click
static int lua_handle_touch(lua_State* L) {
    float x = (float)luaL_checknumber(L, 1);
    float y = (float)luaL_checknumber(L, 2);
    bool isDown = lua_toboolean(L, 3);
    
    if (isDown) {
        // Check toggle button
        if (isPointInCircle(x, y, g_uiState.toggleX + g_uiState.toggleRadius, 
                           g_uiState.toggleY + g_uiState.toggleRadius, g_uiState.toggleRadius)) {
            g_uiState.isOpen = !g_uiState.isOpen;
#ifdef XORON_UI_IOS
            xoron_ios_haptic_feedback(1); // Medium haptic on toggle
#elif defined(XORON_UI_ANDROID)
            xoron_android_haptic_feedback(1); // Medium haptic on toggle
#endif
            lua_pushboolean(L, true);
            return 1;
        }
        
        if (!g_uiState.isOpen) {
            lua_pushboolean(L, false);
            return 1;
        }
        
        // Check if touch is in header (for dragging)
        if (isPointInRect(x, y, g_uiState.windowX, g_uiState.windowY, 
                         g_uiState.windowWidth, 42)) {
            // Check close button
            float closeX = g_uiState.windowX + g_uiState.windowWidth - 40;
            float closeY = g_uiState.windowY + 8;
            if (isPointInRect(x, y, closeX, closeY, 28, 28)) {
                g_uiState.isOpen = false;
                lua_pushboolean(L, true);
                return 1;
            }
            
            // Start dragging
            g_uiState.isDragging = true;
            g_uiState.touchStartX = x;
            g_uiState.touchStartY = y;
            g_uiState.windowStartX = g_uiState.windowX;
            g_uiState.windowStartY = g_uiState.windowY;
            lua_pushboolean(L, true);
            return 1;
        }
        
        // Check tabs
        float tabY = g_uiState.windowY + 52;
        float tabHeight = 36;
        
        // Editor tab
        if (isPointInRect(x, y, g_uiState.windowX + 16, tabY + 4, 100, 28)) {
            g_uiState.currentTab = Tab::Editor;
            lua_pushboolean(L, true);
            return 1;
        }
        
        // Console tab
        if (isPointInRect(x, y, g_uiState.windowX + 124, tabY + 4, 100, 28)) {
            g_uiState.currentTab = Tab::Console;
            lua_pushboolean(L, true);
            return 1;
        }
        
        // Saved Scripts tab
        if (isPointInRect(x, y, g_uiState.windowX + 232, tabY + 4, 120, 28)) {
            g_uiState.currentTab = Tab::SavedScripts;
            lua_pushboolean(L, true);
            return 1;
        }
        
        // Check action buttons (only in Editor tab)
        if (g_uiState.currentTab == Tab::Editor) {
            float btnY = g_uiState.windowY + g_uiState.windowHeight - 48;
            float btnX = g_uiState.windowX + 12;
            
            // Execute button
            if (isPointInRect(x, y, btnX, btnY, 120, 38)) {
#ifdef XORON_UI_IOS
                xoron_ios_haptic_feedback(1); // Medium haptic on execute
#elif defined(XORON_UI_ANDROID)
                xoron_android_haptic_feedback(1); // Medium haptic on execute
#endif
                // Execute the script
                lua_getglobal(L, "xoron_execute");
                if (lua_isfunction(L, -1)) {
                    lua_pushstring(L, g_uiState.editorContent.c_str());
                    lua_call(L, 1, 0);
                } else {
                    lua_pop(L, 1);
                }
                g_uiState.addConsoleMessage("Script executed", ConsoleMessageType::Success);
                lua_pushboolean(L, true);
                return 1;
            }
            
            // Clear button
            if (isPointInRect(x, y, btnX + 130, btnY, 100, 38)) {
#ifdef XORON_UI_IOS
                xoron_ios_haptic_feedback(0); // Light haptic on clear
#elif defined(XORON_UI_ANDROID)
                xoron_android_haptic_feedback(0); // Light haptic on clear
#endif
                g_uiState.clearEditor();
                lua_pushboolean(L, true);
                return 1;
            }
            
            // Save button
            if (isPointInRect(x, y, btnX + 240, btnY, 100, 38)) {
#ifdef XORON_UI_IOS
                xoron_ios_haptic_feedback(0); // Light haptic on save
#elif defined(XORON_UI_ANDROID)
                xoron_android_haptic_feedback(0); // Light haptic on save
#endif
                g_uiState.saveScript(g_uiState.currentFileName);
                g_uiState.addConsoleMessage("Script saved: " + g_uiState.currentFileName, ConsoleMessageType::Success);
                lua_pushboolean(L, true);
                return 1;
            }
            
            // Copy button
            if (isPointInRect(x, y, btnX + 350, btnY, 100, 38)) {
#ifdef XORON_UI_IOS
                xoron_ios_haptic_feedback(0); // Light haptic on copy
#elif defined(XORON_UI_ANDROID)
                xoron_android_haptic_feedback(0); // Light haptic on copy
#endif
                // Copy to clipboard
                lua_getglobal(L, "setclipboard");
                if (lua_isfunction(L, -1)) {
                    lua_pushstring(L, g_uiState.editorContent.c_str());
                    lua_call(L, 1, 0);
                    g_uiState.addConsoleMessage("Copied to clipboard", ConsoleMessageType::Info);
                } else {
                    lua_pop(L, 1);
                }
                lua_pushboolean(L, true);
                return 1;
            }
        }
    } else {
        // Touch up
        g_uiState.isDragging = false;
    }
    
    lua_pushboolean(L, false);
    return 1;
}

// Lua function to handle touch move (for dragging)
static int lua_handle_touch_move(lua_State* L) {
    float x = (float)luaL_checknumber(L, 1);
    float y = (float)luaL_checknumber(L, 2);
    
    if (g_uiState.isDragging) {
        float dx = x - g_uiState.touchStartX;
        float dy = y - g_uiState.touchStartY;
        
        g_uiState.windowX = g_uiState.windowStartX + dx;
        g_uiState.windowY = g_uiState.windowStartY + dy;
        
        // Clamp to screen bounds
        g_uiState.windowX = std::max(0.0f, std::min(g_uiState.windowX, 
                                     g_uiState.screenWidth - g_uiState.windowWidth));
        g_uiState.windowY = std::max(0.0f, std::min(g_uiState.windowY, 
                                     g_uiState.screenHeight - g_uiState.windowHeight));
        
        lua_pushboolean(L, true);
        return 1;
    }
    
    lua_pushboolean(L, false);
    return 1;
}

// Lua function to set editor content
static int lua_set_editor_content(lua_State* L) {
    const char* content = luaL_checkstring(L, 1);
    g_uiState.editorContent = content;
    return 0;
}

// Lua function to get editor content
static int lua_get_editor_content(lua_State* L) {
    lua_pushstring(L, g_uiState.editorContent.c_str());
    return 1;
}

// Lua function to add console message
static int lua_add_console_message(lua_State* L) {
    const char* text = luaL_checkstring(L, 1);
    int type = luaL_optinteger(L, 2, 0);
    g_uiState.addConsoleMessage(text, static_cast<ConsoleMessageType>(type));
    return 0;
}

// Lua function to toggle UI
static int lua_toggle_ui(lua_State* L) {
    g_uiState.isOpen = !g_uiState.isOpen;
    
#ifdef XORON_UI_IOS
    // Trigger haptic feedback on iOS
    xoron_ios_haptic_feedback(1); // Medium impact
    // Also toggle native UI if available
    xoron_ios_ui_toggle();
#elif defined(XORON_UI_ANDROID)
    // Trigger haptic feedback on Android
    xoron_android_haptic_feedback(1); // Medium impact
    // Also toggle native UI if available
    xoron_android_ui_toggle();
#endif
    
    lua_pushboolean(L, g_uiState.isOpen);
    return 1;
}

// Lua function to set screen size
static int lua_set_screen_size(lua_State* L) {
    g_uiState.screenWidth = (float)luaL_checknumber(L, 1);
    g_uiState.screenHeight = (float)luaL_checknumber(L, 2);
    g_uiState.updateWindowSize();
    return 0;
}

// Lua function to update stats
static int lua_update_stats(lua_State* L) {
    g_uiState.fps = luaL_optinteger(L, 1, 60);
    g_uiState.ping = luaL_optinteger(L, 2, 0);
    g_uiState.connected = lua_toboolean(L, 3);
    return 0;
}

// Lua function to get UI state for rendering
static int lua_get_ui_state(lua_State* L) {
    lua_newtable(L);
    
    lua_pushboolean(L, g_uiState.isOpen);
    lua_setfield(L, -2, "isOpen");
    
    lua_pushnumber(L, g_uiState.windowX);
    lua_setfield(L, -2, "windowX");
    
    lua_pushnumber(L, g_uiState.windowY);
    lua_setfield(L, -2, "windowY");
    
    lua_pushnumber(L, g_uiState.windowWidth);
    lua_setfield(L, -2, "windowWidth");
    
    lua_pushnumber(L, g_uiState.windowHeight);
    lua_setfield(L, -2, "windowHeight");
    
    lua_pushnumber(L, g_uiState.toggleX);
    lua_setfield(L, -2, "toggleX");
    
    lua_pushnumber(L, g_uiState.toggleY);
    lua_setfield(L, -2, "toggleY");
    
    lua_pushnumber(L, g_uiState.toggleRadius);
    lua_setfield(L, -2, "toggleRadius");
    
    lua_pushinteger(L, static_cast<int>(g_uiState.currentTab));
    lua_setfield(L, -2, "currentTab");
    
    lua_pushstring(L, g_uiState.editorContent.c_str());
    lua_setfield(L, -2, "editorContent");
    
    lua_pushstring(L, g_uiState.currentFileName.c_str());
    lua_setfield(L, -2, "currentFileName");
    
    lua_pushinteger(L, g_uiState.fps);
    lua_setfield(L, -2, "fps");
    
    lua_pushinteger(L, g_uiState.ping);
    lua_setfield(L, -2, "ping");
    
    lua_pushboolean(L, g_uiState.connected);
    lua_setfield(L, -2, "connected");
    
    // Console messages
    lua_newtable(L);
    int i = 1;
    for (const auto& msg : g_uiState.consoleMessages) {
        lua_newtable(L);
        lua_pushstring(L, msg.text.c_str());
        lua_setfield(L, -2, "text");
        lua_pushstring(L, msg.timestamp.c_str());
        lua_setfield(L, -2, "timestamp");
        lua_pushinteger(L, static_cast<int>(msg.type));
        lua_setfield(L, -2, "type");
        lua_rawseti(L, -2, i++);
    }
    lua_setfield(L, -2, "consoleMessages");
    
    // Saved scripts
    lua_newtable(L);
    i = 1;
    for (const auto& script : g_uiState.savedScripts) {
        lua_newtable(L);
        lua_pushstring(L, script.name.c_str());
        lua_setfield(L, -2, "name");
        lua_pushstring(L, script.content.c_str());
        lua_setfield(L, -2, "content");
        lua_rawseti(L, -2, i++);
    }
    lua_setfield(L, -2, "savedScripts");
    
    return 1;
}

// Lua function to load a saved script
static int lua_load_saved_script(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    g_uiState.loadScript(name);
    return 0;
}

// Lua function to delete a saved script
static int lua_delete_saved_script(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    g_uiState.deleteScript(name);
    return 0;
}

// Lua function to clear console
static int lua_clear_console(lua_State* L) {
    g_uiState.clearConsole();
    return 0;
}

} // namespace XoronUI

// Register UI functions
void xoron_register_ui(lua_State* L) {
    // Create XoronUI table
    lua_newtable(L);
    
    lua_pushcfunction(L, XoronUI::lua_render_ui, "render");
    lua_setfield(L, -2, "render");
    
    lua_pushcfunction(L, XoronUI::lua_handle_touch, "handleTouch");
    lua_setfield(L, -2, "handleTouch");
    
    lua_pushcfunction(L, XoronUI::lua_handle_touch_move, "handleTouchMove");
    lua_setfield(L, -2, "handleTouchMove");
    
    lua_pushcfunction(L, XoronUI::lua_set_editor_content, "setEditorContent");
    lua_setfield(L, -2, "setEditorContent");
    
    lua_pushcfunction(L, XoronUI::lua_get_editor_content, "getEditorContent");
    lua_setfield(L, -2, "getEditorContent");
    
    lua_pushcfunction(L, XoronUI::lua_add_console_message, "addConsoleMessage");
    lua_setfield(L, -2, "addConsoleMessage");
    
    lua_pushcfunction(L, XoronUI::lua_toggle_ui, "toggle");
    lua_setfield(L, -2, "toggle");
    
    lua_pushcfunction(L, XoronUI::lua_set_screen_size, "setScreenSize");
    lua_setfield(L, -2, "setScreenSize");
    
    lua_pushcfunction(L, XoronUI::lua_update_stats, "updateStats");
    lua_setfield(L, -2, "updateStats");
    
    lua_pushcfunction(L, XoronUI::lua_get_ui_state, "getState");
    lua_setfield(L, -2, "getState");
    
    lua_pushcfunction(L, XoronUI::lua_load_saved_script, "loadScript");
    lua_setfield(L, -2, "loadScript");
    
    lua_pushcfunction(L, XoronUI::lua_delete_saved_script, "deleteScript");
    lua_setfield(L, -2, "deleteScript");
    
    lua_pushcfunction(L, XoronUI::lua_clear_console, "clearConsole");
    lua_setfield(L, -2, "clearConsole");
    
    // Constants for message types
    lua_newtable(L);
    lua_pushinteger(L, 0); lua_setfield(L, -2, "Info");
    lua_pushinteger(L, 1); lua_setfield(L, -2, "Success");
    lua_pushinteger(L, 2); lua_setfield(L, -2, "Warning");
    lua_pushinteger(L, 3); lua_setfield(L, -2, "Error");
    lua_pushinteger(L, 4); lua_setfield(L, -2, "Print");
    lua_setfield(L, -2, "MessageType");
    
    // Constants for tabs
    lua_newtable(L);
    lua_pushinteger(L, 0); lua_setfield(L, -2, "Editor");
    lua_pushinteger(L, 1); lua_setfield(L, -2, "Console");
    lua_pushinteger(L, 2); lua_setfield(L, -2, "SavedScripts");
    lua_setfield(L, -2, "Tab");
    
    lua_setglobal(L, "XoronUI");
}
