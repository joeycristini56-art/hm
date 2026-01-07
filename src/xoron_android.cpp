/*
 * xoron_android.cpp - Native Android UI for Xoron Executor
 * Uses Android NDK for native rendering on Android
 * Purple & Black theme matching the executor design
 *
 * This replaces the Lua-based Drawing UI with native Android components
 * for better performance and native Android look/feel.
 */

#include "xoron.h"
#include "lua.h"
#include <android/native_window.h>
#include <android/native_activity.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <jni.h>
#include <mutex>
#include <vector>
#include <string>

#define LOG_TAG "XoronAndroid"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// ============================================================================
// Global State
// ============================================================================
static ANativeWindow* g_nativeWindow = nullptr;
static AInputQueue* g_inputQueue = nullptr;
static EGLDisplay g_eglDisplay = EGL_NO_DISPLAY;
static EGLSurface g_eglSurface = EGL_NO_SURFACE;
static EGLContext g_eglContext = EGL_NO_CONTEXT;
static lua_State* g_lua_state = nullptr;
static bool g_ui_visible = false;
static std::mutex g_ui_mutex;

// ============================================================================
// Theme Colors - Purple & Black (RGBA)
// ============================================================================
static const float XORON_COLOR_BACKGROUND[] = {12/255.0f, 12/255.0f, 15/255.0f, 1.0f};
static const float XORON_COLOR_BACKGROUND_DARK[] = {15/255.0f, 15/255.0f, 18/255.0f, 1.0f};
static const float XORON_COLOR_HEADER[] = {24/255.0f, 24/255.0f, 27/255.0f, 1.0f};
static const float XORON_COLOR_BORDER[] = {42/255.0f, 42/255.0f, 58/255.0f, 1.0f};
static const float XORON_COLOR_PURPLE[] = {147/255.0f, 51/255.0f, 234/255.0f, 1.0f};
static const float XORON_COLOR_PURPLE_LIGHT[] = {168/255.0f, 85/255.0f, 247/255.0f, 1.0f};
static const float XORON_COLOR_PURPLE_DARK[] = {109/255.0f, 40/255.0f, 217/255.0f, 1.0f};
static const float XORON_COLOR_TEXT[] = {228/255.0f, 228/255.0f, 231/255.0f, 1.0f};
static const float XORON_COLOR_TEXT_DIM[] = {113/255.0f, 113/255.0f, 122/255.0f, 1.0f};
static const float XORON_COLOR_TEXT_MUTED[] = {82/255.0f, 82/255.0f, 91/255.0f, 1.0f};
static const float XORON_COLOR_GREEN[] = {34/255.0f, 197/255.0f, 94/255.0f, 1.0f};
static const float XORON_COLOR_RED[] = {239/255.0f, 68/255.0f, 68/255.0f, 1.0f};
static const float XORON_COLOR_YELLOW[] = {251/255.0f, 191/255.0f, 36/255.0f, 1.0f};
static const float XORON_COLOR_BLUE[] = {96/255.0f, 165/255.0f, 250/255.0f, 1.0f};
static const float XORON_COLOR_BUTTON_BG[] = {39/255.0f, 39/255.0f, 42/255.0f, 1.0f};

// ============================================================================
// Console Message Structure
// ============================================================================
typedef enum {
    XORON_MESSAGE_INFO = 0,
    XORON_MESSAGE_SUCCESS = 1,
    XORON_MESSAGE_WARNING = 2,
    XORON_MESSAGE_ERROR = 3,
    XORON_MESSAGE_PRINT = 4
} XoronMessageType;

typedef struct {
    std::string message;
    XoronMessageType type;
    std::string timestamp;
} XoronConsoleMessage;

static std::vector<XoronConsoleMessage> g_console_messages;
static std::mutex g_console_mutex;

// ============================================================================
// UI State
// ============================================================================
typedef enum {
    XORON_TAB_EDITOR = 0,
    XORON_TAB_CONSOLE = 1,
    XORON_TAB_SCRIPTS = 2
} XoronTab;

static XoronTab g_current_tab = XORON_TAB_EDITOR;
static std::string g_editor_content = "-- Welcome to Xoron Executor!\n\nlocal player = game.Players.LocalPlayer\nlocal char = player.Character\n\nif char then\n    char.Humanoid.WalkSpeed = 100\nend\n\nprint(\"Speed boosted!\")";
static std::vector<std::string> g_saved_scripts = {
    "Speed Hack",
    "Jump Power", 
    "Infinite Jump"
};
static std::vector<std::string> g_script_code = {
    "game.Players.LocalPlayer.Character.Humanoid.WalkSpeed = 100",
    "game.Players.LocalPlayer.Character.Humanoid.JumpPower = 150",
    "-- Infinite Jump\nlocal uis = game:GetService(\"UserInputService\")\nuis.JumpRequest:Connect(function()\n    game.Players.LocalPlayer.Character.Humanoid:ChangeState(\"Jumping\")\nend)"
};

// ============================================================================
// OpenGL ES Utilities
// ============================================================================
static GLuint compile_shader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        GLchar log[512];
        glGetShaderInfoLog(shader, sizeof(log), NULL, log);
        LOGE("Shader compile error: %s", log);
        glDeleteShader(shader);
        return 0;
    }
    
    return shader;
}

static GLuint create_program(const char* vertex_source, const char* fragment_source) {
    GLuint vertex_shader = compile_shader(GL_VERTEX_SHADER, vertex_source);
    GLuint fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fragment_source);
    
    if (!vertex_shader || !fragment_shader) {
        return 0;
    }
    
    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    
    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
        GLchar log[512];
        glGetProgramInfoLog(program, sizeof(log), NULL, log);
        LOGE("Program link error: %s", log);
        glDeleteProgram(program);
        program = 0;
    }
    
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    
    return program;
}

// ============================================================================
// UI Rendering
// ============================================================================
static void draw_rounded_rect(float x, float y, float width, float height, float radius, const float* color) {
    // Simple implementation - would use more advanced rendering in production
    glUseProgram(0);
    glColor4fv(color);
    
    // Draw main rectangle
    GLfloat vertices[] = {
        x + radius, y,
        x + width - radius, y,
        x + width, y + radius,
        x + width, y + height - radius,
        x + width - radius, y + height,
        x + radius, y + height,
        x, y + height - radius,
        x, y + radius,
        x + radius, y
    };
    
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, vertices);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 10);
    glDisableClientState(GL_VERTEX_ARRAY);
}

static void draw_text(float x, float y, const char* text, const float* color) {
    // Text rendering would use a proper font atlas in production
    // For now, we'll just draw a placeholder
    glUseProgram(0);
    glColor4fv(color);
    
    // Draw text background
    float text_width = strlen(text) * 8;
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + text_width, y);
    glVertex2f(x + text_width, y + 16);
    glVertex2f(x, y + 16);
    glEnd();
}

static void render_ui() {
    if (!g_ui_visible || !g_nativeWindow) return;
    
    std::lock_guard<std::mutex> lock(g_ui_mutex);
    
    // Get window dimensions
    int width = ANativeWindow_getWidth(g_nativeWindow);
    int height = ANativeWindow_getHeight(g_nativeWindow);
    
    // Clear screen
    glClearColor(XORON_COLOR_BACKGROUND[0], XORON_COLOR_BACKGROUND[1], 
                 XORON_COLOR_BACKGROUND[2], XORON_COLOR_BACKGROUND[3]);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Calculate UI dimensions
    float container_width = width * 0.85f;
    float container_height = height * 0.75f;
    float container_x = (width - container_width) / 2;
    float container_y = (height - container_height) / 2;
    
    // Draw main container
    draw_rounded_rect(container_x, container_y, container_width, container_height, 12, XORON_COLOR_BACKGROUND);
    
    // Draw header
    draw_rounded_rect(container_x, container_y, container_width, 44, 8, XORON_COLOR_HEADER);
    
    // Draw purple accent line
    draw_rounded_rect(container_x, container_y + 43, container_width, 2, 0, XORON_COLOR_PURPLE);
    
    // Draw X logo
    float logo_x = container_x + 16;
    float logo_y = container_y + 16;
    glLineWidth(2.5f);
    glBegin(GL_LINES);
    glColor4fv(XORON_COLOR_PURPLE);
    glVertex2f(logo_x, logo_y);
    glVertex2f(logo_x + 14, logo_y + 14);
    glVertex2f(logo_x, logo_y + 14);
    glVertex2f(logo_x + 14, logo_y);
    glEnd();
    
    // Draw title
    draw_text(container_x + 38, container_y + 12, "XORON", XORON_COLOR_TEXT);
    
    // Draw status dot
    float status_x = container_x + 105;
    float status_y = container_y + 18;
    draw_rounded_rect(status_x, status_y, 8, 8, 4, XORON_COLOR_GREEN);
    
    // Draw status text
    draw_text(container_x + 118, container_y + 12, "Connected", XORON_COLOR_GREEN);
    
    // Draw tabs
    float tab_width = container_width / 3;
    float tab_height = 32;
    float tab_y = container_y + 52;
    
    for (int i = 0; i < 3; i++) {
        float tab_x = container_x + 12 + (i * tab_width);
        const float* tab_color = (i == g_current_tab) ? XORON_COLOR_PURPLE : XORON_COLOR_HEADER;
        
        draw_rounded_rect(tab_x, tab_y, tab_width, tab_height, 6, tab_color);
        
        const char* tab_names[] = {"Editor", "Console", "Scripts"};
        draw_text(tab_x + 10, tab_y + 8, tab_names[i], 
                  (i == g_current_tab) ? XORON_COLOR_TEXT : XORON_COLOR_TEXT_DIM);
    }
    
    // Draw content based on current tab
    float content_y = container_y + 92;
    float content_height = container_height - 92 - 56; // Subtract header and button bar
    
    switch (g_current_tab) {
        case XORON_TAB_EDITOR: {
            // Draw editor container
            float editor_x = container_x + 12;
            float editor_width = container_width - 24;
            draw_rounded_rect(editor_x, content_y, editor_width, content_height, 8, XORON_COLOR_BACKGROUND_DARK);
            
            // Draw editor header
            draw_rounded_rect(editor_x, content_y, editor_width, 28, 6, XORON_COLOR_HEADER);
            
            // Draw file tab
            draw_rounded_rect(editor_x + 8, content_y + 4, 80, 20, 4, XORON_COLOR_BUTTON_BG);
            draw_text(editor_x + 16, content_y + 6, "script.lua", XORON_COLOR_TEXT_MUTED);
            
            // Draw editor content
            float editor_content_y = content_y + 28;
            float editor_content_height = content_height - 28;
            
            // Draw some sample text
            draw_text(editor_x + 10, editor_content_y + 10, g_editor_content.c_str(), XORON_COLOR_TEXT);
            
            break;
        }
        
        case XORON_TAB_CONSOLE: {
            // Draw console container
            float console_x = container_x + 12;
            float console_width = container_width - 24;
            draw_rounded_rect(console_x, content_y, console_width, content_height, 8, XORON_COLOR_BACKGROUND_DARK);
            
            // Draw console messages
            std::lock_guard<std::mutex> console_lock(g_console_mutex);
            float message_y = content_y + 10;
            
            for (const auto& msg : g_console_messages) {
                const float* msg_color;
                switch (msg.type) {
                    case XORON_MESSAGE_SUCCESS: msg_color = XORON_COLOR_GREEN; break;
                    case XORON_MESSAGE_WARNING: msg_color = XORON_COLOR_YELLOW; break;
                    case XORON_MESSAGE_ERROR: msg_color = XORON_COLOR_RED; break;
                    case XORON_MESSAGE_INFO: msg_color = XORON_COLOR_BLUE; break;
                    default: msg_color = XORON_COLOR_TEXT; break;
                }
                
                std::string full_msg = "[" + msg.timestamp + "] " + msg.message;
                draw_text(console_x + 10, message_y, full_msg.c_str(), msg_color);
                message_y += 20;
            }
            
            break;
        }
        
        case XORON_TAB_SCRIPTS: {
            // Draw scripts container
            float scripts_x = container_x + 12;
            float scripts_width = container_width - 24;
            draw_rounded_rect(scripts_x, content_y, scripts_width, content_height, 8, XORON_COLOR_BACKGROUND_DARK);
            
            // Draw scripts list
            float script_y = content_y + 10;
            for (size_t i = 0; i < g_saved_scripts.size(); i++) {
                draw_text(scripts_x + 10, script_y, g_saved_scripts[i].c_str(), XORON_COLOR_TEXT);
                script_y += 30;
            }
            
            break;
        }
    }
    
    // Draw button bar
    float button_bar_y = container_y + container_height - 48;
    float button_bar_x = container_x + 12;
    float button_bar_width = container_width - 24;
    float button_bar_height = 40;
    
    // Draw buttons
    float button_width = (button_bar_width - 30) / 4;
    float button_height = 36;
    
    const char* button_names[] = {"Execute", "Clear", "Save", "Copy"};
    const float* button_colors[] = {
        XORON_COLOR_PURPLE,
        XORON_COLOR_BUTTON_BG,
        XORON_COLOR_BUTTON_BG,
        XORON_COLOR_BUTTON_BG
    };
    
    for (int i = 0; i < 4; i++) {
        float button_x = button_bar_x + (i * (button_width + 10));
        draw_rounded_rect(button_x, button_bar_y + 2, button_width, button_height, 6, button_colors[i]);
        draw_text(button_x + 10, button_bar_y + 14, button_names[i], XORON_COLOR_TEXT);
    }
    
    // Swap buffers
    eglSwapBuffers(g_eglDisplay, g_eglSurface);
}

// ============================================================================
// Input Handling
// ============================================================================
static int32_t handle_input(AInputEvent* event) {
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        int32_t action = AMotionEvent_getAction(event);
        int32_t action_code = action & AMOTION_EVENT_ACTION_MASK;
        
        if (action_code == AMOTION_EVENT_ACTION_DOWN) {
            // Touch down - toggle UI visibility
            g_ui_visible = !g_ui_visible;
            
            if (g_ui_visible) {
                // Initialize EGL when UI becomes visible
                if (g_eglDisplay == EGL_NO_DISPLAY) {
                    g_eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
                    if (g_eglDisplay == EGL_NO_DISPLAY) {
                        LOGE("Failed to get EGL display");
                        return 0;
                    }
                    
                    EGLint major, minor;
                    if (!eglInitialize(g_eglDisplay, &major, &minor)) {
                        LOGE("Failed to initialize EGL");
                        return 0;
                    }
                    
                    EGLint attribList[] = {
                        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
                        EGL_BLUE_SIZE, 8,
                        EGL_GREEN_SIZE, 8,
                        EGL_RED_SIZE, 8,
                        EGL_ALPHA_SIZE, 8,
                        EGL_DEPTH_SIZE, 16,
                        EGL_NONE
                    };
                    
                    EGLConfig config;
                    EGLint numConfigs;
                    if (!eglChooseConfig(g_eglDisplay, attribList, &config, 1, &numConfigs)) {
                        LOGE("Failed to choose EGL config");
                        return 0;
                    }
                    
                    EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
                    g_eglContext = eglCreateContext(g_eglDisplay, config, EGL_NO_CONTEXT, contextAttribs);
                    if (g_eglContext == EGL_NO_CONTEXT) {
                        LOGE("Failed to create EGL context");
                        return 0;
                    }
                    
                    g_eglSurface = eglCreateWindowSurface(g_eglDisplay, config, g_nativeWindow, NULL);
                    if (g_eglSurface == EGL_NO_SURFACE) {
                        LOGE("Failed to create EGL surface");
                        return 0;
                    }
                    
                    if (!eglMakeCurrent(g_eglDisplay, g_eglSurface, g_eglSurface, g_eglContext)) {
                        LOGE("Failed to make EGL context current");
                        return 0;
                    }
                    
                    // Set viewport
                    int width = ANativeWindow_getWidth(g_nativeWindow);
                    int height = ANativeWindow_getHeight(g_nativeWindow);
                    glViewport(0, 0, width, height);
                    
                    // Basic OpenGL setup
                    glDisable(GL_DEPTH_TEST);
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    
                    // Orthographic projection
                    glMatrixMode(GL_PROJECTION);
                    glLoadIdentity();
                    glOrthof(0, width, height, 0, -1, 1);
                    glMatrixMode(GL_MODELVIEW);
                    glLoadIdentity();
                }
            }
            
            return 1; // Event handled
        }
    }
    
    return 0; // Event not handled
}

// ============================================================================
// Android Native Functions
// ============================================================================
extern "C" void xoron_android_ui_init(ANativeActivity* activity) {
    if (!activity) {
        LOGE("Activity is null");
        return;
    }
    
    g_nativeWindow = activity->window;
    g_inputQueue = activity->inputQueue;
    
    LOGI("Android UI initialized");
}

extern "C" void xoron_android_ui_show(void) {
    std::lock_guard<std::mutex> lock(g_ui_mutex);
    g_ui_visible = true;
    LOGI("Android UI shown");
}

extern "C" void xoron_android_ui_hide(void) {
    std::lock_guard<std::mutex> lock(g_ui_mutex);
    g_ui_visible = false;
    LOGI("Android UI hidden");
}

extern "C" void xoron_android_ui_toggle(void) {
    std::lock_guard<std::mutex> lock(g_ui_mutex);
    g_ui_visible = !g_ui_visible;
    LOGI("Android UI toggled: %d", g_ui_visible);
}

extern "C" void xoron_android_haptic_feedback(int style) {
    // Android haptic feedback would be implemented here
    // This would use the Android Vibrator API via JNI
    LOGI("Haptic feedback: %d", style);
}

extern "C" void xoron_android_console_print(const char* message, int type) {
    if (!message) return;
    
    std::lock_guard<std::mutex> lock(g_console_mutex);
    
    // Get current time for timestamp
    time_t now = time(nullptr);
    tm* local_time = localtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%H:%M:%S", local_time);
    
    g_console_messages.push_back({message, static_cast<XoronMessageType>(type), timestamp});
    
    // Keep only last 100 messages
    if (g_console_messages.size() > 100) {
        g_console_messages.erase(g_console_messages.begin());
    }
    
    LOGI("Console: [%s] %s", timestamp, message);
}

extern "C" void xoron_android_set_lua_state(lua_State* L) {
    g_lua_state = L;
}

extern "C" lua_State* xoron_android_get_lua_state(void) {
    return g_lua_state;
}

// ============================================================================
// Lua Registration
// ============================================================================
void xoron_register_android(lua_State* L) {
    g_lua_state = L;
    
    // Register Android-specific Lua functions
    lua_newtable(L);
    
    lua_pushcfunction(L, [](lua_State* L) -> int {
        xoron_android_ui_show();
        return 0;
    }, "show");
    lua_setfield(L, -2, "show");
    
    lua_pushcfunction(L, [](lua_State* L) -> int {
        xoron_android_ui_hide();
        return 0;
    }, "hide");
    lua_setfield(L, -2, "hide");
    
    lua_pushcfunction(L, [](lua_State* L) -> int {
        xoron_android_ui_toggle();
        return 0;
    }, "toggle");
    lua_setfield(L, -2, "toggle");
    
    lua_pushcfunction(L, [](lua_State* L) -> int {
        const char* msg = luaL_checkstring(L, 1);
        int type = luaL_optinteger(L, 2, 0);
        xoron_android_console_print(msg, type);
        return 0;
    }, "print");
    lua_setfield(L, -2, "print");
    
    lua_setglobal(L, "XoronNative");
    
    LOGI("Android Lua functions registered");
}

// ============================================================================
// JNI Entry Point
// ============================================================================
extern "C" {

JNIEXPORT void JNICALL 
Java_com_xoron_Executor_nativeInit(JNIEnv* env, jobject obj, jobject activity) {
    (void)obj;
    
    // Get native activity
    ANativeActivity* native_activity = ANativeActivity_fromJava(env, activity);
    if (native_activity) {
        xoron_android_ui_init(native_activity);
        LOGI("Native activity initialized");
    } else {
        LOGE("Failed to get native activity");
    }
}

JNIEXPORT void JNICALL 
Java_com_xoron_Executor_nativeRender(JNIEnv* env, jobject obj) {
    (void)env; (void)obj;
    render_ui();
}

JNIEXPORT jint JNICALL 
Java_com_xoron_Executor_nativeHandleInput(JNIEnv* env, jobject obj, jobject input_event) {
    (void)env; (void)obj;
    
    AInputEvent* event = AInputEvent_fromJava(env, input_event);
    if (event) {
        return handle_input(event);
    }
    
    return 0;
}

} // extern "C"