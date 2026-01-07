/*
 * xoron_drawing.cpp - Drawing library for executor
 * Provides: Drawing.new, Line, Circle, Square, Text, Triangle, Quad, Image
 * 
 * iOS: Uses CoreGraphics for rendering
 * Android: Uses native canvas rendering
 */

#include "xoron.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <cmath>

#include "lua.h"
#include "lualib.h"

// Platform-specific includes for rendering
#if defined(__APPLE__)
    #include <TargetConditionals.h>
    #if TARGET_OS_IPHONE
        #include <CoreGraphics/CoreGraphics.h>
        #include <CoreFoundation/CoreFoundation.h>
        #include <CoreText/CoreText.h>
        #include <objc/runtime.h>
        #include <objc/message.h>
        #include <algorithm>
        #define XORON_IOS_DRAWING 1
    #endif
#elif defined(__ANDROID__)
    #include <android/native_window.h>
    #include <jni.h>
    #include <algorithm>
    #define XORON_ANDROID_DRAWING 1
#endif

extern void xoron_set_error(const char* fmt, ...);

// Drawing object types
enum DrawingType {
    DRAWING_LINE = 0,
    DRAWING_CIRCLE,
    DRAWING_SQUARE,
    DRAWING_TEXT,
    DRAWING_TRIANGLE,
    DRAWING_QUAD,
    DRAWING_IMAGE
};

// Color structure
struct Color3 {
    float r, g, b;
    Color3() : r(1), g(1), b(1) {}
    Color3(float r_, float g_, float b_) : r(r_), g(g_), b(b_) {}
};

// Vector2 structure
struct Vector2 {
    float x, y;
    Vector2() : x(0), y(0) {}
    Vector2(float x_, float y_) : x(x_), y(y_) {}
};

// Base drawing object
struct DrawingObject {
    DrawingType type;
    bool visible;
    float transparency;
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
    
    DrawingObject() : type(DRAWING_LINE), visible(true), transparency(0), 
                      zindex(0), id(0), radius(0), textSize(16), center(false),
                      outline(false), filled(false), thickness(1), rounding(0) {}
};

// Drawing state
static std::mutex g_drawing_mutex;
static std::unordered_map<uint32_t, DrawingObject*> g_drawings;
static std::atomic<uint32_t> g_next_id{1};
static std::vector<std::string> g_fonts = {"UI", "System", "RobotoMono", "Legacy", "Plex"};

// Screen dimensions (updated by platform code)
static float g_screen_width = 844.0f;
static float g_screen_height = 390.0f;

#ifdef XORON_IOS_DRAWING
// iOS CoreGraphics rendering context
static CGContextRef g_cg_context = nullptr;
static void* g_render_layer = nullptr;  // CALayer for rendering

// iOS rendering functions using CoreGraphics
static void ios_set_render_context(CGContextRef ctx) {
    g_cg_context = ctx;
}

static void ios_draw_line(const DrawingObject* obj) {
    if (!g_cg_context || !obj->visible) return;
    
    CGContextSetRGBStrokeColor(g_cg_context, obj->color.r, obj->color.g, obj->color.b, 1.0 - obj->transparency);
    CGContextSetLineWidth(g_cg_context, obj->thickness);
    CGContextMoveToPoint(g_cg_context, obj->from.x, obj->from.y);
    CGContextAddLineToPoint(g_cg_context, obj->to.x, obj->to.y);
    CGContextStrokePath(g_cg_context);
}

static void ios_draw_circle(const DrawingObject* obj) {
    if (!g_cg_context || !obj->visible) return;
    
    CGRect rect = CGRectMake(obj->position.x - obj->radius, obj->position.y - obj->radius,
                             obj->radius * 2, obj->radius * 2);
    
    if (obj->filled) {
        CGContextSetRGBFillColor(g_cg_context, obj->color.r, obj->color.g, obj->color.b, 1.0 - obj->transparency);
        CGContextFillEllipseInRect(g_cg_context, rect);
    } else {
        CGContextSetRGBStrokeColor(g_cg_context, obj->color.r, obj->color.g, obj->color.b, 1.0 - obj->transparency);
        CGContextSetLineWidth(g_cg_context, obj->thickness);
        CGContextStrokeEllipseInRect(g_cg_context, rect);
    }
}

static void ios_draw_rect(const DrawingObject* obj) {
    if (!g_cg_context || !obj->visible) return;
    
    CGRect rect = CGRectMake(obj->position.x, obj->position.y, obj->size.x, obj->size.y);
    
    if (obj->rounding > 0) {
        // Rounded rectangle using bezier path
        CGFloat radius = obj->rounding;
        CGMutablePathRef path = CGPathCreateMutable();
        
        CGPathMoveToPoint(path, NULL, rect.origin.x + radius, rect.origin.y);
        CGPathAddLineToPoint(path, NULL, rect.origin.x + rect.size.width - radius, rect.origin.y);
        CGPathAddArcToPoint(path, NULL, rect.origin.x + rect.size.width, rect.origin.y,
                           rect.origin.x + rect.size.width, rect.origin.y + radius, radius);
        CGPathAddLineToPoint(path, NULL, rect.origin.x + rect.size.width, rect.origin.y + rect.size.height - radius);
        CGPathAddArcToPoint(path, NULL, rect.origin.x + rect.size.width, rect.origin.y + rect.size.height,
                           rect.origin.x + rect.size.width - radius, rect.origin.y + rect.size.height, radius);
        CGPathAddLineToPoint(path, NULL, rect.origin.x + radius, rect.origin.y + rect.size.height);
        CGPathAddArcToPoint(path, NULL, rect.origin.x, rect.origin.y + rect.size.height,
                           rect.origin.x, rect.origin.y + rect.size.height - radius, radius);
        CGPathAddLineToPoint(path, NULL, rect.origin.x, rect.origin.y + radius);
        CGPathAddArcToPoint(path, NULL, rect.origin.x, rect.origin.y,
                           rect.origin.x + radius, rect.origin.y, radius);
        CGPathCloseSubpath(path);
        
        CGContextAddPath(g_cg_context, path);
        
        if (obj->filled) {
            CGContextSetRGBFillColor(g_cg_context, obj->color.r, obj->color.g, obj->color.b, 1.0 - obj->transparency);
            CGContextFillPath(g_cg_context);
        } else {
            CGContextSetRGBStrokeColor(g_cg_context, obj->color.r, obj->color.g, obj->color.b, 1.0 - obj->transparency);
            CGContextSetLineWidth(g_cg_context, obj->thickness);
            CGContextStrokePath(g_cg_context);
        }
        
        CGPathRelease(path);
    } else {
        if (obj->filled) {
            CGContextSetRGBFillColor(g_cg_context, obj->color.r, obj->color.g, obj->color.b, 1.0 - obj->transparency);
            CGContextFillRect(g_cg_context, rect);
        } else {
            CGContextSetRGBStrokeColor(g_cg_context, obj->color.r, obj->color.g, obj->color.b, 1.0 - obj->transparency);
            CGContextSetLineWidth(g_cg_context, obj->thickness);
            CGContextStrokeRect(g_cg_context, rect);
        }
    }
}

static void ios_draw_text(const DrawingObject* obj) {
    if (!g_cg_context || !obj->visible || obj->text.empty()) return;
    
    // Use CoreText for text rendering
    CFStringRef fontName = CFStringCreateWithCString(kCFAllocatorDefault, 
        obj->font.empty() ? "Helvetica" : obj->font.c_str(), kCFStringEncodingUTF8);
    CTFontRef font = CTFontCreateWithName(fontName, obj->textSize, NULL);
    CFRelease(fontName);
    
    // Create attributed string
    CFStringRef text = CFStringCreateWithCString(kCFAllocatorDefault, obj->text.c_str(), kCFStringEncodingUTF8);
    
    CGColorRef color = CGColorCreateSRGB(obj->color.r, obj->color.g, obj->color.b, 1.0 - obj->transparency);
    
    CFStringRef keys[] = { kCTFontAttributeName, kCTForegroundColorAttributeName };
    CFTypeRef values[] = { font, color };
    CFDictionaryRef attributes = CFDictionaryCreate(kCFAllocatorDefault, 
        (const void**)&keys, (const void**)&values, 2,
        &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    
    CFAttributedStringRef attrString = CFAttributedStringCreate(kCFAllocatorDefault, text, attributes);
    CTLineRef line = CTLineCreateWithAttributedString(attrString);
    
    // Calculate position
    CGFloat x = obj->position.x;
    CGFloat y = obj->position.y;
    
    if (obj->center) {
        CGRect bounds = CTLineGetBoundsWithOptions(line, 0);
        x -= bounds.size.width / 2;
    }
    
    // Draw outline if enabled
    if (obj->outline) {
        CGContextSetRGBFillColor(g_cg_context, obj->outlineColor.r, obj->outlineColor.g, obj->outlineColor.b, 1.0);
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                if (dx != 0 || dy != 0) {
                    CGContextSetTextPosition(g_cg_context, x + dx, y + dy);
                    CTLineDraw(line, g_cg_context);
                }
            }
        }
    }
    
    // Draw main text
    CGContextSetTextPosition(g_cg_context, x, y);
    CTLineDraw(line, g_cg_context);
    
    // Cleanup
    CFRelease(line);
    CFRelease(attrString);
    CFRelease(attributes);
    CGColorRelease(color);
    CFRelease(text);
    CFRelease(font);
}

static void ios_draw_triangle(const DrawingObject* obj) {
    if (!g_cg_context || !obj->visible) return;
    
    CGMutablePathRef path = CGPathCreateMutable();
    CGPathMoveToPoint(path, NULL, obj->pointA.x, obj->pointA.y);
    CGPathAddLineToPoint(path, NULL, obj->pointB.x, obj->pointB.y);
    CGPathAddLineToPoint(path, NULL, obj->pointC.x, obj->pointC.y);
    CGPathCloseSubpath(path);
    
    CGContextAddPath(g_cg_context, path);
    
    if (obj->filled) {
        CGContextSetRGBFillColor(g_cg_context, obj->color.r, obj->color.g, obj->color.b, 1.0 - obj->transparency);
        CGContextFillPath(g_cg_context);
    } else {
        CGContextSetRGBStrokeColor(g_cg_context, obj->color.r, obj->color.g, obj->color.b, 1.0 - obj->transparency);
        CGContextSetLineWidth(g_cg_context, obj->thickness);
        CGContextStrokePath(g_cg_context);
    }
    
    CGPathRelease(path);
}

static void ios_draw_quad(const DrawingObject* obj) {
    if (!g_cg_context || !obj->visible) return;
    
    CGMutablePathRef path = CGPathCreateMutable();
    CGPathMoveToPoint(path, NULL, obj->pointA.x, obj->pointA.y);
    CGPathAddLineToPoint(path, NULL, obj->pointB.x, obj->pointB.y);
    CGPathAddLineToPoint(path, NULL, obj->pointC.x, obj->pointC.y);
    CGPathAddLineToPoint(path, NULL, obj->pointD.x, obj->pointD.y);
    CGPathCloseSubpath(path);
    
    CGContextAddPath(g_cg_context, path);
    
    if (obj->filled) {
        CGContextSetRGBFillColor(g_cg_context, obj->color.r, obj->color.g, obj->color.b, 1.0 - obj->transparency);
        CGContextFillPath(g_cg_context);
    } else {
        CGContextSetRGBStrokeColor(g_cg_context, obj->color.r, obj->color.g, obj->color.b, 1.0 - obj->transparency);
        CGContextSetLineWidth(g_cg_context, obj->thickness);
        CGContextStrokePath(g_cg_context);
    }
    
    CGPathRelease(path);
}

static void ios_draw_image(const DrawingObject* obj) {
    if (!g_cg_context || !obj->visible || obj->imageData.empty()) return;
    
    // Decode base64 image data
    CFDataRef base64Data = CFDataCreate(kCFAllocatorDefault, 
        (const UInt8*)obj->imageData.c_str(), obj->imageData.length());
    if (!base64Data) return;
    
    // Use SecTransform for base64 decoding (Security framework)
    // Or use NSData with Objective-C runtime
    Class NSDataClass = objc_getClass("NSData");
    if (NSDataClass) {
        // Create NSData from base64 string
        id nsString = ((id(*)(Class, SEL, const char*))objc_msgSend)(
            objc_getClass("NSString"), sel_registerName("stringWithUTF8String:"), obj->imageData.c_str());
        
        if (nsString) {
            // Decode base64
            id imageData = ((id(*)(id, SEL, id, unsigned long))objc_msgSend)(
                NSDataClass, sel_registerName("alloc"), nil, 0);
            imageData = ((id(*)(id, SEL, id, unsigned long))objc_msgSend)(
                imageData, sel_registerName("initWithBase64EncodedString:options:"), nsString, 0);
            
            if (imageData) {
                // Create UIImage from data
                Class UIImageClass = objc_getClass("UIImage");
                if (UIImageClass) {
                    id uiImage = ((id(*)(Class, SEL, id))objc_msgSend)(
                        UIImageClass, sel_registerName("imageWithData:"), imageData);
                    
                    if (uiImage) {
                        // Get CGImage from UIImage
                        CGImageRef cgImage = (CGImageRef)((id(*)(id, SEL))objc_msgSend)(
                            uiImage, sel_registerName("CGImage"));
                        
                        if (cgImage) {
                            // Draw the image
                            CGRect rect = CGRectMake(obj->position.x, obj->position.y, 
                                obj->size.x > 0 ? obj->size.x : CGImageGetWidth(cgImage),
                                obj->size.y > 0 ? obj->size.y : CGImageGetHeight(cgImage));
                            
                            // Flip context for proper image orientation
                            CGContextSaveGState(g_cg_context);
                            CGContextTranslateCTM(g_cg_context, 0, rect.origin.y + rect.size.height);
                            CGContextScaleCTM(g_cg_context, 1.0, -1.0);
                            rect.origin.y = 0;
                            
                            CGContextSetAlpha(g_cg_context, 1.0 - obj->transparency);
                            CGContextDrawImage(g_cg_context, rect, cgImage);
                            CGContextRestoreGState(g_cg_context);
                        }
                    }
                }
            }
        }
    }
    
    CFRelease(base64Data);
}

// Render all drawing objects (called from render loop)
extern "C" void xoron_drawing_render_ios(CGContextRef ctx) {
    if (!ctx) return;
    
    g_cg_context = ctx;
    
    std::lock_guard<std::mutex> lock(g_drawing_mutex);
    
    // Sort by zindex
    std::vector<DrawingObject*> sorted;
    for (auto& pair : g_drawings) {
        if (pair.second && pair.second->visible) {
            sorted.push_back(pair.second);
        }
    }
    std::sort(sorted.begin(), sorted.end(), [](DrawingObject* a, DrawingObject* b) {
        return a->zindex < b->zindex;
    });
    
    // Render each object
    for (DrawingObject* obj : sorted) {
        switch (obj->type) {
            case DRAWING_LINE: ios_draw_line(obj); break;
            case DRAWING_CIRCLE: ios_draw_circle(obj); break;
            case DRAWING_SQUARE: ios_draw_rect(obj); break;
            case DRAWING_TEXT: ios_draw_text(obj); break;
            case DRAWING_TRIANGLE: ios_draw_triangle(obj); break;
            case DRAWING_QUAD: ios_draw_quad(obj); break;
            case DRAWING_IMAGE: ios_draw_image(obj); break;
        }
    }
    
    g_cg_context = nullptr;
}
#endif // XORON_IOS_DRAWING

#ifdef XORON_ANDROID_DRAWING
// Android rendering using Canvas via JNI
static JavaVM* g_android_jvm = nullptr;
static jobject g_android_canvas = nullptr;
static jclass g_paint_class = nullptr;
static jclass g_canvas_class = nullptr;

// Initialize Android drawing (called from JNI)
extern "C" JNIEXPORT void JNICALL
Java_com_xoron_Drawing_init(JNIEnv* env, jobject obj, jobject canvas) {
    env->GetJavaVM(&g_android_jvm);
    g_android_canvas = env->NewGlobalRef(canvas);
    g_canvas_class = (jclass)env->NewGlobalRef(env->FindClass("android/graphics/Canvas"));
    g_paint_class = (jclass)env->NewGlobalRef(env->FindClass("android/graphics/Paint"));
}

static JNIEnv* get_jni_env() {
    if (!g_android_jvm) return nullptr;
    JNIEnv* env = nullptr;
    g_android_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    return env;
}

static void android_draw_line(JNIEnv* env, jobject canvas, jobject paint, const DrawingObject* obj) {
    if (!obj->visible) return;
    
    // Set paint color
    jmethodID setColor = env->GetMethodID(g_paint_class, "setColor", "(I)V");
    int color = (int)(obj->color.r * 255) << 16 | (int)(obj->color.g * 255) << 8 | (int)(obj->color.b * 255);
    color |= (int)((1.0f - obj->transparency) * 255) << 24;
    env->CallVoidMethod(paint, setColor, color);
    
    // Set stroke width
    jmethodID setStrokeWidth = env->GetMethodID(g_paint_class, "setStrokeWidth", "(F)V");
    env->CallVoidMethod(paint, setStrokeWidth, obj->thickness);
    
    // Draw line
    jmethodID drawLine = env->GetMethodID(g_canvas_class, "drawLine", "(FFFFP)V");
    env->CallVoidMethod(canvas, drawLine, obj->from.x, obj->from.y, obj->to.x, obj->to.y, paint);
}

static void android_draw_circle(JNIEnv* env, jobject canvas, jobject paint, const DrawingObject* obj) {
    if (!obj->visible) return;
    
    jmethodID setColor = env->GetMethodID(g_paint_class, "setColor", "(I)V");
    int color = (int)(obj->color.r * 255) << 16 | (int)(obj->color.g * 255) << 8 | (int)(obj->color.b * 255);
    color |= (int)((1.0f - obj->transparency) * 255) << 24;
    env->CallVoidMethod(paint, setColor, color);
    
    jmethodID setStyle = env->GetMethodID(g_paint_class, "setStyle", "(Landroid/graphics/Paint$Style;)V");
    jclass styleClass = env->FindClass("android/graphics/Paint$Style");
    jfieldID styleField = env->GetStaticFieldID(styleClass, obj->filled ? "FILL" : "STROKE", "Landroid/graphics/Paint$Style;");
    jobject style = env->GetStaticObjectField(styleClass, styleField);
    env->CallVoidMethod(paint, setStyle, style);
    
    if (!obj->filled) {
        jmethodID setStrokeWidth = env->GetMethodID(g_paint_class, "setStrokeWidth", "(F)V");
        env->CallVoidMethod(paint, setStrokeWidth, obj->thickness);
    }
    
    jmethodID drawCircle = env->GetMethodID(g_canvas_class, "drawCircle", "(FFFP)V");
    env->CallVoidMethod(canvas, drawCircle, obj->position.x, obj->position.y, obj->radius, paint);
}

static void android_draw_rect(JNIEnv* env, jobject canvas, jobject paint, const DrawingObject* obj) {
    if (!obj->visible) return;
    
    jmethodID setColor = env->GetMethodID(g_paint_class, "setColor", "(I)V");
    int color = (int)(obj->color.r * 255) << 16 | (int)(obj->color.g * 255) << 8 | (int)(obj->color.b * 255);
    color |= (int)((1.0f - obj->transparency) * 255) << 24;
    env->CallVoidMethod(paint, setColor, color);
    
    jmethodID setStyle = env->GetMethodID(g_paint_class, "setStyle", "(Landroid/graphics/Paint$Style;)V");
    jclass styleClass = env->FindClass("android/graphics/Paint$Style");
    jfieldID styleField = env->GetStaticFieldID(styleClass, obj->filled ? "FILL" : "STROKE", "Landroid/graphics/Paint$Style;");
    jobject style = env->GetStaticObjectField(styleClass, styleField);
    env->CallVoidMethod(paint, setStyle, style);
    
    if (obj->rounding > 0) {
        jmethodID drawRoundRect = env->GetMethodID(g_canvas_class, "drawRoundRect", "(FFFFFFP)V");
        env->CallVoidMethod(canvas, drawRoundRect, 
            obj->position.x, obj->position.y,
            obj->position.x + obj->size.x, obj->position.y + obj->size.y,
            obj->rounding, obj->rounding, paint);
    } else {
        jmethodID drawRect = env->GetMethodID(g_canvas_class, "drawRect", "(FFFFP)V");
        env->CallVoidMethod(canvas, drawRect,
            obj->position.x, obj->position.y,
            obj->position.x + obj->size.x, obj->position.y + obj->size.y, paint);
    }
}

static void android_draw_text(JNIEnv* env, jobject canvas, jobject paint, const DrawingObject* obj) {
    if (!obj->visible || obj->text.empty()) return;
    
    jmethodID setColor = env->GetMethodID(g_paint_class, "setColor", "(I)V");
    int color = (int)(obj->color.r * 255) << 16 | (int)(obj->color.g * 255) << 8 | (int)(obj->color.b * 255);
    color |= (int)((1.0f - obj->transparency) * 255) << 24;
    env->CallVoidMethod(paint, setColor, color);
    
    jmethodID setTextSize = env->GetMethodID(g_paint_class, "setTextSize", "(F)V");
    env->CallVoidMethod(paint, setTextSize, obj->textSize);
    
    if (obj->center) {
        jmethodID setTextAlign = env->GetMethodID(g_paint_class, "setTextAlign", "(Landroid/graphics/Paint$Align;)V");
        jclass alignClass = env->FindClass("android/graphics/Paint$Align");
        jfieldID centerField = env->GetStaticFieldID(alignClass, "CENTER", "Landroid/graphics/Paint$Align;");
        jobject centerAlign = env->GetStaticObjectField(alignClass, centerField);
        env->CallVoidMethod(paint, setTextAlign, centerAlign);
    }
    
    jstring text = env->NewStringUTF(obj->text.c_str());
    jmethodID drawText = env->GetMethodID(g_canvas_class, "drawText", "(Ljava/lang/String;FFP)V");
    env->CallVoidMethod(canvas, drawText, text, obj->position.x, obj->position.y, paint);
    env->DeleteLocalRef(text);
}

static void android_draw_triangle(JNIEnv* env, jobject canvas, jobject paint, const DrawingObject* obj) {
    if (!obj->visible) return;
    
    jmethodID setColor = env->GetMethodID(g_paint_class, "setColor", "(I)V");
    int color = (int)(obj->color.r * 255) << 16 | (int)(obj->color.g * 255) << 8 | (int)(obj->color.b * 255);
    color |= (int)((1.0f - obj->transparency) * 255) << 24;
    env->CallVoidMethod(paint, setColor, color);
    
    jmethodID setStyle = env->GetMethodID(g_paint_class, "setStyle", "(Landroid/graphics/Paint$Style;)V");
    jclass styleClass = env->FindClass("android/graphics/Paint$Style");
    jfieldID styleField = env->GetStaticFieldID(styleClass, obj->filled ? "FILL" : "STROKE", "Landroid/graphics/Paint$Style;");
    jobject style = env->GetStaticObjectField(styleClass, styleField);
    env->CallVoidMethod(paint, setStyle, style);
    
    // Create Path for triangle
    jclass pathClass = env->FindClass("android/graphics/Path");
    jmethodID pathInit = env->GetMethodID(pathClass, "<init>", "()V");
    jobject path = env->NewObject(pathClass, pathInit);
    
    jmethodID moveTo = env->GetMethodID(pathClass, "moveTo", "(FF)V");
    jmethodID lineTo = env->GetMethodID(pathClass, "lineTo", "(FF)V");
    jmethodID close = env->GetMethodID(pathClass, "close", "()V");
    
    env->CallVoidMethod(path, moveTo, obj->pointA.x, obj->pointA.y);
    env->CallVoidMethod(path, lineTo, obj->pointB.x, obj->pointB.y);
    env->CallVoidMethod(path, lineTo, obj->pointC.x, obj->pointC.y);
    env->CallVoidMethod(path, close);
    
    jmethodID drawPath = env->GetMethodID(g_canvas_class, "drawPath", "(Landroid/graphics/Path;Landroid/graphics/Paint;)V");
    env->CallVoidMethod(canvas, drawPath, path, paint);
    
    env->DeleteLocalRef(path);
}

static void android_draw_quad(JNIEnv* env, jobject canvas, jobject paint, const DrawingObject* obj) {
    if (!obj->visible) return;
    
    jmethodID setColor = env->GetMethodID(g_paint_class, "setColor", "(I)V");
    int color = (int)(obj->color.r * 255) << 16 | (int)(obj->color.g * 255) << 8 | (int)(obj->color.b * 255);
    color |= (int)((1.0f - obj->transparency) * 255) << 24;
    env->CallVoidMethod(paint, setColor, color);
    
    jmethodID setStyle = env->GetMethodID(g_paint_class, "setStyle", "(Landroid/graphics/Paint$Style;)V");
    jclass styleClass = env->FindClass("android/graphics/Paint$Style");
    jfieldID styleField = env->GetStaticFieldID(styleClass, obj->filled ? "FILL" : "STROKE", "Landroid/graphics/Paint$Style;");
    jobject style = env->GetStaticObjectField(styleClass, styleField);
    env->CallVoidMethod(paint, setStyle, style);
    
    // Create Path for quad
    jclass pathClass = env->FindClass("android/graphics/Path");
    jmethodID pathInit = env->GetMethodID(pathClass, "<init>", "()V");
    jobject path = env->NewObject(pathClass, pathInit);
    
    jmethodID moveTo = env->GetMethodID(pathClass, "moveTo", "(FF)V");
    jmethodID lineTo = env->GetMethodID(pathClass, "lineTo", "(FF)V");
    jmethodID close = env->GetMethodID(pathClass, "close", "()V");
    
    env->CallVoidMethod(path, moveTo, obj->pointA.x, obj->pointA.y);
    env->CallVoidMethod(path, lineTo, obj->pointB.x, obj->pointB.y);
    env->CallVoidMethod(path, lineTo, obj->pointC.x, obj->pointC.y);
    env->CallVoidMethod(path, lineTo, obj->pointD.x, obj->pointD.y);
    env->CallVoidMethod(path, close);
    
    jmethodID drawPath = env->GetMethodID(g_canvas_class, "drawPath", "(Landroid/graphics/Path;Landroid/graphics/Paint;)V");
    env->CallVoidMethod(canvas, drawPath, path, paint);
    
    env->DeleteLocalRef(path);
}

static void android_draw_image(JNIEnv* env, jobject canvas, jobject paint, const DrawingObject* obj) {
    if (!obj->visible || obj->imageData.empty()) return;
    
    // Decode base64 image data using BitmapFactory
    jclass base64Class = env->FindClass("android/util/Base64");
    if (!base64Class) return;
    
    jmethodID decode = env->GetStaticMethodID(base64Class, "decode", "(Ljava/lang/String;I)[B");
    if (!decode) return;
    
    jstring imageStr = env->NewStringUTF(obj->imageData.c_str());
    jbyteArray bytes = (jbyteArray)env->CallStaticObjectMethod(base64Class, decode, imageStr, 0);
    env->DeleteLocalRef(imageStr);
    
    if (!bytes) return;
    
    // Create bitmap from bytes
    jclass bitmapFactoryClass = env->FindClass("android/graphics/BitmapFactory");
    jmethodID decodeByteArray = env->GetStaticMethodID(bitmapFactoryClass, "decodeByteArray", 
        "([BII)Landroid/graphics/Bitmap;");
    
    jsize len = env->GetArrayLength(bytes);
    jobject bitmap = env->CallStaticObjectMethod(bitmapFactoryClass, decodeByteArray, bytes, 0, len);
    env->DeleteLocalRef(bytes);
    
    if (!bitmap) return;
    
    // Draw bitmap
    jmethodID drawBitmap = env->GetMethodID(g_canvas_class, "drawBitmap", 
        "(Landroid/graphics/Bitmap;FFLandroid/graphics/Paint;)V");
    env->CallVoidMethod(canvas, drawBitmap, bitmap, obj->position.x, obj->position.y, paint);
    
    env->DeleteLocalRef(bitmap);
}

// Render all drawing objects for Android (called from render loop)
extern "C" JNIEXPORT void JNICALL
Java_com_xoron_Drawing_render(JNIEnv* env, jobject obj, jobject canvas) {
    if (!canvas) return;
    
    // Create paint object
    jmethodID paintInit = env->GetMethodID(g_paint_class, "<init>", "()V");
    jobject paint = env->NewObject(g_paint_class, paintInit);
    
    // Set anti-aliasing
    jmethodID setAntiAlias = env->GetMethodID(g_paint_class, "setAntiAlias", "(Z)V");
    env->CallVoidMethod(paint, setAntiAlias, JNI_TRUE);
    
    std::lock_guard<std::mutex> lock(g_drawing_mutex);
    
    // Sort by zindex
    std::vector<DrawingObject*> sorted;
    for (auto& pair : g_drawings) {
        if (pair.second && pair.second->visible) {
            sorted.push_back(pair.second);
        }
    }
    std::sort(sorted.begin(), sorted.end(), [](DrawingObject* a, DrawingObject* b) {
        return a->zindex < b->zindex;
    });
    
    // Render each object
    for (DrawingObject* drawObj : sorted) {
        switch (drawObj->type) {
            case DRAWING_LINE: android_draw_line(env, canvas, paint, drawObj); break;
            case DRAWING_CIRCLE: android_draw_circle(env, canvas, paint, drawObj); break;
            case DRAWING_SQUARE: android_draw_rect(env, canvas, paint, drawObj); break;
            case DRAWING_TEXT: android_draw_text(env, canvas, paint, drawObj); break;
            case DRAWING_TRIANGLE: android_draw_triangle(env, canvas, paint, drawObj); break;
            case DRAWING_QUAD: android_draw_quad(env, canvas, paint, drawObj); break;
            case DRAWING_IMAGE: android_draw_image(env, canvas, paint, drawObj); break;
        }
    }
    
    env->DeleteLocalRef(paint);
}

// Get screen size on Android
extern "C" JNIEXPORT void JNICALL
Java_com_xoron_Drawing_setScreenSize(JNIEnv* env, jobject obj, jfloat width, jfloat height) {
    g_screen_width = width;
    g_screen_height = height;
}
#endif // XORON_ANDROID_DRAWING

// Update screen size (called from platform code)
extern "C" void xoron_drawing_set_screen_size(float width, float height) {
    g_screen_width = width;
    g_screen_height = height;
}

// Metatable name
static const char* DRAWING_MT = "XoronDrawing";

// Get drawing object from userdata
static DrawingObject* get_drawing(lua_State* L, int idx) {
    DrawingObject** ud = (DrawingObject**)luaL_checkudata(L, idx, DRAWING_MT);
    if (!ud || !*ud) {
        luaL_error(L, "Invalid drawing object");
        return nullptr;
    }
    return *ud;
}

// Push Color3 to Lua
static void push_color3(lua_State* L, const Color3& c) {
    lua_newtable(L);
    lua_pushnumber(L, c.r);
    lua_setfield(L, -2, "R");
    lua_pushnumber(L, c.g);
    lua_setfield(L, -2, "G");
    lua_pushnumber(L, c.b);
    lua_setfield(L, -2, "B");
}

// Get Color3 from Lua
static Color3 get_color3(lua_State* L, int idx) {
    Color3 c;
    if (lua_istable(L, idx)) {
        lua_getfield(L, idx, "R");
        if (lua_isnumber(L, -1)) c.r = lua_tonumber(L, -1);
        lua_pop(L, 1);
        
        lua_getfield(L, idx, "G");
        if (lua_isnumber(L, -1)) c.g = lua_tonumber(L, -1);
        lua_pop(L, 1);
        
        lua_getfield(L, idx, "B");
        if (lua_isnumber(L, -1)) c.b = lua_tonumber(L, -1);
        lua_pop(L, 1);
    }
    return c;
}

// Push Vector2 to Lua
static void push_vector2(lua_State* L, const Vector2& v) {
    lua_newtable(L);
    lua_pushnumber(L, v.x);
    lua_setfield(L, -2, "X");
    lua_pushnumber(L, v.y);
    lua_setfield(L, -2, "Y");
}

// Get Vector2 from Lua
static Vector2 get_vector2(lua_State* L, int idx) {
    Vector2 v;
    if (lua_istable(L, idx)) {
        lua_getfield(L, idx, "X");
        if (lua_isnumber(L, -1)) v.x = lua_tonumber(L, -1);
        lua_pop(L, 1);
        
        lua_getfield(L, idx, "Y");
        if (lua_isnumber(L, -1)) v.y = lua_tonumber(L, -1);
        lua_pop(L, 1);
    }
    return v;
}

// Drawing object __index
static int drawing_index(lua_State* L) {
    DrawingObject* obj = get_drawing(L, 1);
    const char* key = luaL_checkstring(L, 2);
    
    if (strcmp(key, "Visible") == 0) {
        lua_pushboolean(L, obj->visible);
    } else if (strcmp(key, "Color") == 0) {
        push_color3(L, obj->color);
    } else if (strcmp(key, "Transparency") == 0) {
        lua_pushnumber(L, obj->transparency);
    } else if (strcmp(key, "ZIndex") == 0) {
        lua_pushinteger(L, obj->zindex);
    } else if (strcmp(key, "From") == 0) {
        push_vector2(L, obj->from);
    } else if (strcmp(key, "To") == 0) {
        push_vector2(L, obj->to);
    } else if (strcmp(key, "Position") == 0) {
        push_vector2(L, obj->position);
    } else if (strcmp(key, "Radius") == 0) {
        lua_pushnumber(L, obj->radius);
    } else if (strcmp(key, "Size") == 0) {
        push_vector2(L, obj->size);
    } else if (strcmp(key, "Text") == 0) {
        lua_pushstring(L, obj->text.c_str());
    } else if (strcmp(key, "TextBounds") == 0) {
        // Calculate text bounds
        Vector2 bounds;
        bounds.x = obj->text.length() * obj->textSize * 0.6f;
        bounds.y = obj->textSize;
        push_vector2(L, bounds);
    } else if (strcmp(key, "TextSize") == 0 || strcmp(key, "Size") == 0) {
        lua_pushnumber(L, obj->textSize);
    } else if (strcmp(key, "Center") == 0) {
        lua_pushboolean(L, obj->center);
    } else if (strcmp(key, "Outline") == 0) {
        lua_pushboolean(L, obj->outline);
    } else if (strcmp(key, "OutlineColor") == 0) {
        push_color3(L, obj->outlineColor);
    } else if (strcmp(key, "Filled") == 0) {
        lua_pushboolean(L, obj->filled);
    } else if (strcmp(key, "Thickness") == 0) {
        lua_pushnumber(L, obj->thickness);
    } else if (strcmp(key, "PointA") == 0) {
        push_vector2(L, obj->pointA);
    } else if (strcmp(key, "PointB") == 0) {
        push_vector2(L, obj->pointB);
    } else if (strcmp(key, "PointC") == 0) {
        push_vector2(L, obj->pointC);
    } else if (strcmp(key, "PointD") == 0) {
        push_vector2(L, obj->pointD);
    } else if (strcmp(key, "Data") == 0) {
        lua_pushstring(L, obj->imageData.c_str());
    } else if (strcmp(key, "Rounding") == 0) {
        lua_pushnumber(L, obj->rounding);
    } else if (strcmp(key, "Font") == 0) {
        lua_pushinteger(L, 0); // Font enum
    } else if (strcmp(key, "Remove") == 0 || strcmp(key, "Destroy") == 0) {
        // Return the remove function
        lua_pushvalue(L, 1);
        lua_pushcclosure(L, [](lua_State* L) -> int {
            DrawingObject* obj = get_drawing(L, lua_upvalueindex(1));
            if (obj) {
                std::lock_guard<std::mutex> lock(g_drawing_mutex);
                g_drawings.erase(obj->id);
                delete obj;
            }
            return 0;
        }, "Remove", 1);
    } else {
        lua_pushnil(L);
    }
    
    return 1;
}

// Drawing object __newindex
static int drawing_newindex(lua_State* L) {
    DrawingObject* obj = get_drawing(L, 1);
    const char* key = luaL_checkstring(L, 2);
    
    if (strcmp(key, "Visible") == 0) {
        obj->visible = lua_toboolean(L, 3);
    } else if (strcmp(key, "Color") == 0) {
        obj->color = get_color3(L, 3);
    } else if (strcmp(key, "Transparency") == 0) {
        obj->transparency = lua_tonumber(L, 3);
    } else if (strcmp(key, "ZIndex") == 0) {
        obj->zindex = lua_tointeger(L, 3);
    } else if (strcmp(key, "From") == 0) {
        obj->from = get_vector2(L, 3);
    } else if (strcmp(key, "To") == 0) {
        obj->to = get_vector2(L, 3);
    } else if (strcmp(key, "Position") == 0) {
        obj->position = get_vector2(L, 3);
    } else if (strcmp(key, "Radius") == 0) {
        obj->radius = lua_tonumber(L, 3);
    } else if (strcmp(key, "Size") == 0) {
        if (lua_istable(L, 3)) {
            obj->size = get_vector2(L, 3);
        } else {
            obj->textSize = lua_tonumber(L, 3);
        }
    } else if (strcmp(key, "Text") == 0) {
        obj->text = luaL_checkstring(L, 3);
    } else if (strcmp(key, "TextSize") == 0) {
        obj->textSize = lua_tonumber(L, 3);
    } else if (strcmp(key, "Center") == 0) {
        obj->center = lua_toboolean(L, 3);
    } else if (strcmp(key, "Outline") == 0) {
        obj->outline = lua_toboolean(L, 3);
    } else if (strcmp(key, "OutlineColor") == 0) {
        obj->outlineColor = get_color3(L, 3);
    } else if (strcmp(key, "Filled") == 0) {
        obj->filled = lua_toboolean(L, 3);
    } else if (strcmp(key, "Thickness") == 0) {
        obj->thickness = lua_tonumber(L, 3);
    } else if (strcmp(key, "PointA") == 0) {
        obj->pointA = get_vector2(L, 3);
    } else if (strcmp(key, "PointB") == 0) {
        obj->pointB = get_vector2(L, 3);
    } else if (strcmp(key, "PointC") == 0) {
        obj->pointC = get_vector2(L, 3);
    } else if (strcmp(key, "PointD") == 0) {
        obj->pointD = get_vector2(L, 3);
    } else if (strcmp(key, "Data") == 0) {
        obj->imageData = luaL_checkstring(L, 3);
    } else if (strcmp(key, "Rounding") == 0) {
        obj->rounding = lua_tonumber(L, 3);
    } else if (strcmp(key, "Font") == 0) {
        // Font enum or index
        if (lua_isnumber(L, 3)) {
            int idx = lua_tointeger(L, 3);
            if (idx >= 0 && idx < (int)g_fonts.size()) {
                obj->font = g_fonts[idx];
            }
        }
    }
    
    return 0;
}

// Drawing object __gc
static int drawing_gc(lua_State* L) {
    DrawingObject** ud = (DrawingObject**)lua_touserdata(L, 1);
    if (ud && *ud) {
        std::lock_guard<std::mutex> lock(g_drawing_mutex);
        g_drawings.erase((*ud)->id);
        delete *ud;
        *ud = nullptr;
    }
    return 0;
}

// Create drawing object
static DrawingObject* create_drawing(lua_State* L, DrawingType type) {
    DrawingObject* obj = new DrawingObject();
    obj->type = type;
    obj->id = g_next_id++;
    
    {
        std::lock_guard<std::mutex> lock(g_drawing_mutex);
        g_drawings[obj->id] = obj;
    }
    
    // Create userdata
    DrawingObject** ud = (DrawingObject**)lua_newuserdata(L, sizeof(DrawingObject*));
    *ud = obj;
    
    // Set metatable
    luaL_getmetatable(L, DRAWING_MT);
    lua_setmetatable(L, -2);
    
    return obj;
}

// Drawing.new(type) - Creates a new drawing object
static int lua_drawing_new(lua_State* L) {
    const char* type_str = luaL_checkstring(L, 1);
    
    DrawingType type;
    if (strcmp(type_str, "Line") == 0) {
        type = DRAWING_LINE;
    } else if (strcmp(type_str, "Circle") == 0) {
        type = DRAWING_CIRCLE;
    } else if (strcmp(type_str, "Square") == 0) {
        type = DRAWING_SQUARE;
    } else if (strcmp(type_str, "Text") == 0) {
        type = DRAWING_TEXT;
    } else if (strcmp(type_str, "Triangle") == 0) {
        type = DRAWING_TRIANGLE;
    } else if (strcmp(type_str, "Quad") == 0) {
        type = DRAWING_QUAD;
    } else if (strcmp(type_str, "Image") == 0) {
        type = DRAWING_IMAGE;
    } else {
        luaL_error(L, "Invalid drawing type: %s", type_str);
        return 0;
    }
    
    create_drawing(L, type);
    return 1;
}

// Drawing.Fonts - Table of available fonts
static int lua_drawing_fonts(lua_State* L) {
    lua_newtable(L);
    for (size_t i = 0; i < g_fonts.size(); i++) {
        lua_pushinteger(L, (int)i);
        lua_setfield(L, -2, g_fonts[i].c_str());
    }
    return 1;
}

// cleardrawcache() - Clears all drawings
static int lua_cleardrawcache(lua_State* L) {
    (void)L;
    
    std::lock_guard<std::mutex> lock(g_drawing_mutex);
    for (auto& pair : g_drawings) {
        delete pair.second;
    }
    g_drawings.clear();
    
    return 0;
}

// Drawing.clear() - Alias for cleardrawcache
static int lua_drawing_clear(lua_State* L) {
    return lua_cleardrawcache(L);
}

// getrenderproperty(obj, property) - Gets a render property
static int lua_getrenderproperty_impl(lua_State* L) {
    DrawingObject* obj = get_drawing(L, 1);
    const char* prop = luaL_checkstring(L, 2);
    
    // Reuse index logic
    lua_pushvalue(L, 1);
    lua_pushstring(L, prop);
    return drawing_index(L);
}

// setrenderproperty(obj, property, value) - Sets a render property
static int lua_setrenderproperty_impl(lua_State* L) {
    // Reuse __newindex
    return drawing_newindex(L);
}

// isrenderobj(obj) - Checks if object is a drawing
static int lua_isrenderobj(lua_State* L) {
    void* ud = lua_touserdata(L, 1);
    if (ud) {
        lua_getmetatable(L, 1);
        luaL_getmetatable(L, DRAWING_MT);
        bool is_drawing = lua_rawequal(L, -1, -2);
        lua_pop(L, 2);
        lua_pushboolean(L, is_drawing);
    } else {
        lua_pushboolean(L, false);
    }
    return 1;
}

// getrenderproperty(obj, property) - Gets a render property
static int lua_getrenderproperty(lua_State* L) {
    DrawingObject* obj = get_drawing(L, 1);
    const char* prop = luaL_checkstring(L, 2);
    (void)obj;
    
    lua_pushvalue(L, 1);
    lua_pushstring(L, prop);
    return drawing_index(L);
}

// setrenderproperty(obj, property, value) - Sets a render property
static int lua_setrenderproperty(lua_State* L) {
    // Reuse __newindex
    return drawing_newindex(L);
}

// getscreensize() - Gets the screen size
static int lua_getscreensize(lua_State* L) {
#ifdef XORON_IOS_DRAWING
    // Get actual screen size from iOS
    Class UIScreenClass = objc_getClass("UIScreen");
    if (UIScreenClass) {
        id mainScreen = ((id(*)(Class, SEL))objc_msgSend)(UIScreenClass, sel_registerName("mainScreen"));
        if (mainScreen) {
            // Get bounds - returns CGRect
            CGRect bounds;
            ((void(*)(CGRect*, id, SEL))objc_msgSend_stret)(&bounds, mainScreen, sel_registerName("bounds"));
            g_screen_width = bounds.size.width;
            g_screen_height = bounds.size.height;
        }
    }
#elif defined(XORON_ANDROID_DRAWING)
    // Android screen size is set via JNI (Java_com_xoron_Drawing_setScreenSize)
    // If not set yet, try to get it via JNI
    if (g_screen_width <= 0 || g_screen_height <= 0) {
        JNIEnv* env = get_jni_env();
        if (env) {
            jclass displayMetricsClass = env->FindClass("android/util/DisplayMetrics");
            if (displayMetricsClass) {
                jmethodID init = env->GetMethodID(displayMetricsClass, "<init>", "()V");
                jobject metrics = env->NewObject(displayMetricsClass, init);
                if (metrics) {
                    jfieldID widthField = env->GetFieldID(displayMetricsClass, "widthPixels", "I");
                    jfieldID heightField = env->GetFieldID(displayMetricsClass, "heightPixels", "I");
                    g_screen_width = (float)env->GetIntField(metrics, widthField);
                    g_screen_height = (float)env->GetIntField(metrics, heightField);
                    env->DeleteLocalRef(metrics);
                }
            }
        }
    }
#endif
    
    lua_newtable(L);
    lua_pushnumber(L, g_screen_width);
    lua_setfield(L, -2, "X");
    lua_pushnumber(L, g_screen_height);
    lua_setfield(L, -2, "Y");
    return 1;
}

// Register drawing library
void xoron_register_drawing(lua_State* L) {
    // Create metatable for drawing objects
    luaL_newmetatable(L, DRAWING_MT);
    
    lua_pushcfunction(L, drawing_index, "__index");
    lua_setfield(L, -2, "__index");
    
    lua_pushcfunction(L, drawing_newindex, "__newindex");
    lua_setfield(L, -2, "__newindex");
    
    lua_pushcfunction(L, drawing_gc, "__gc");
    lua_setfield(L, -2, "__gc");
    
    lua_pop(L, 1);
    
    // Create Drawing table
    lua_newtable(L);
    
    lua_pushcfunction(L, lua_drawing_new, "new");
    lua_setfield(L, -2, "new");
    
    lua_pushcfunction(L, lua_drawing_clear, "clear");
    lua_setfield(L, -2, "clear");
    
    // Add Fonts table
    lua_newtable(L);
    for (size_t i = 0; i < g_fonts.size(); i++) {
        lua_pushinteger(L, (int)i);
        lua_setfield(L, -2, g_fonts[i].c_str());
    }
    lua_setfield(L, -2, "Fonts");
    
    lua_setglobal(L, "Drawing");
    
    // Global functions
    lua_pushcfunction(L, lua_cleardrawcache, "cleardrawcache");
    lua_setglobal(L, "cleardrawcache");
    
    lua_pushcfunction(L, lua_isrenderobj, "isrenderobj");
    lua_setglobal(L, "isrenderobj");
    
    lua_pushcfunction(L, lua_getrenderproperty, "getrenderproperty");
    lua_setglobal(L, "getrenderproperty");
    
    lua_pushcfunction(L, lua_setrenderproperty, "setrenderproperty");
    lua_setglobal(L, "setrenderproperty");
    
    lua_pushcfunction(L, lua_getscreensize, "getscreensize");
    lua_setglobal(L, "getscreensize");
}
