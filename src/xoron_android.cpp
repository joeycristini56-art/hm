/*
 * xoron_android.cpp - Native Android Integration for Xoron Executor
 * Uses Android NDK and JNI for native Android functionality
 * Purple & Black theme matching the executor design
 *
 * This provides native Android integration without OpenGL ES
 * Uses Android Canvas via JNI for rendering when needed
 */

#include "xoron.h"
#include "lua.h"
#include <jni.h>
#include <mutex>
#include <vector>
#include <string>
#include <ctime>
#include <android/log.h>
#include <android/native_activity.h>

#define LOG_TAG "XoronAndroid"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// ============================================================================
// Global State
// ============================================================================
static JavaVM* g_jvm = nullptr;
static jobject g_activity = nullptr;
static jobject g_vibrator = nullptr;
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
// JNI Helper
// ============================================================================
static JNIEnv* get_jni_env() {
    if (!g_jvm) return nullptr;
    JNIEnv* env = nullptr;
    g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    return env;
}

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
// UI State Management (No rendering - handled by Java/Kotlin UI)
// ============================================================================

static void update_ui_state() {
    // UI state is managed here, but rendering is done by Java/Kotlin
    // This function updates internal state without rendering
    std::lock_guard<std::mutex> lock(g_ui_mutex);
    // State updates only - no OpenGL rendering
}

// Input handling is managed by Java/Kotlin UI layer
// This function is no longer needed with native Android UI approach

// ============================================================================
// Android Native Functions
// ============================================================================
void xoron_android_ui_init(ANativeActivity* activity) {
    if (!activity) {
        LOGE("Activity is null");
        return;
    }
    
    // Store activity reference for JNI operations
    // Note: UI rendering is handled by Java/Kotlin, not native OpenGL
    
    LOGI("Android UI initialized (native state management only)");
}

// Show/hide/toggle UI on Android (calls back to Java)
void xoron_android_ui_show(void) {
    JNIEnv* env = get_jni_env();
    if (!env || !g_activity) return;

    jclass activityClass = env->GetObjectClass(g_activity);
    jmethodID showUI = env->GetMethodID(activityClass, "showXoronUI", "()V");
    if (showUI) {
        env->CallVoidMethod(g_activity, showUI);
    }
}

void xoron_android_ui_hide(void) {
    JNIEnv* env = get_jni_env();
    if (!env || !g_activity) return;

    jclass activityClass = env->GetObjectClass(g_activity);
    jmethodID hideUI = env->GetMethodID(activityClass, "hideXoronUI", "()V");
    if (hideUI) {
        env->CallVoidMethod(g_activity, hideUI);
    }
}

void xoron_android_ui_toggle(void) {
    JNIEnv* env = get_jni_env();
    if (!env || !g_activity) return;

    jclass activityClass = env->GetObjectClass(g_activity);
    jmethodID toggleUI = env->GetMethodID(activityClass, "toggleXoronUI", "()V");
    if (toggleUI) {
        env->CallVoidMethod(g_activity, toggleUI);
    }
}

// Android haptic feedback using Vibrator service
void xoron_android_haptic_feedback(int style) {
    JNIEnv* env = get_jni_env();
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

// Send console message to Android UI
void xoron_android_console_print(const char* message, int type) {
    JNIEnv* env = get_jni_env();
    if (!env || !g_activity || !message) return;

    jclass activityClass = env->GetObjectClass(g_activity);
    jmethodID addConsole = env->GetMethodID(activityClass, "addConsoleMessage", "(Ljava/lang/String;I)V");
    if (addConsole) {
        jstring jmsg = env->NewStringUTF(message);
        env->CallVoidMethod(g_activity, addConsole, jmsg, type);
        env->DeleteLocalRef(jmsg);
    }
}

void xoron_android_set_lua_state(lua_State* L) {
    g_lua_state = L;
}

lua_State* xoron_android_get_lua_state(void) {
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
        const char* msg = lua_tostring(L, 1);
        int type = lua_tointeger(L, 2);
        if (msg) {
            xoron_android_console_print(msg, type);
        }
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
    
    // Store Java activity reference for later use
    // UI rendering is handled by Java/Kotlin, so we just store the reference
    if (activity) {
        g_activity = env->NewGlobalRef(activity);
        LOGI("Native activity reference stored");
    } else {
        LOGE("Failed to store native activity reference");
    }
}

JNIEXPORT void JNICALL 
Java_com_xoron_Executor_nativeRender(JNIEnv* env, jobject obj) {
    (void)env; (void)obj;
    // UI rendering is handled by Java/Kotlin UI components
    // This function is kept for compatibility but does nothing
    update_ui_state();
}

JNIEXPORT jint JNICALL 
Java_com_xoron_Executor_nativeHandleInput(JNIEnv* env, jobject obj, jobject input_event) {
    (void)env; (void)obj; (void)input_event;
    
    // Input handling is done at Java level
    // This is a placeholder for future native input handling if needed
    LOGI("Native input handler called");
    
    return 0;
}

// UI initialization (called from Java)
JNIEXPORT void JNICALL
Java_com_xoron_UI_init(JNIEnv* env, jobject obj, jobject activity, jobject vibrator) {
    env->GetJavaVM(&g_jvm);
    g_activity = env->NewGlobalRef(activity);
    if (vibrator) {
        g_vibrator = env->NewGlobalRef(vibrator);
    }
    LOGI("UI initialized from Java");
}

} // extern "C"