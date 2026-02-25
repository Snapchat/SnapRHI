#include <GLAD/OpenGL.h>
#include <OpenGL/Context.h>

#import <AppKit/NSOpenGL.h>
#import <Foundation/Foundation.h>

#include <stdexcept>
#include <string>

// Simple logging macros - can be disabled in release builds
#ifndef NDEBUG
    #define GL_LOG_INFO(fmt, ...) NSLog(@"[gl-loader] " fmt, ##__VA_ARGS__)
#else
    #define GL_LOG_INFO(fmt, ...) ((void)0)
#endif

namespace {

NSOpenGLPixelFormatAttribute* getGL21Attribs() {
    static NSOpenGLPixelFormatAttribute attributes[] = {
        NSOpenGLPFAOpenGLProfile,
        NSOpenGLProfileVersionLegacy,
        NSOpenGLPFAPixelBuffer,
        NSOpenGLPFANoRecovery,
        NSOpenGLPFAAccelerated,
        (NSOpenGLPixelFormatAttribute)0
    };
    return attributes;
}

NSOpenGLPixelFormatAttribute* getGL41Attribs() {
    static NSOpenGLPixelFormatAttribute attributes[] = {
        NSOpenGLPFAOpenGLProfile,
        NSOpenGLProfileVersion4_1Core,
        NSOpenGLPFANoRecovery,
        NSOpenGLPFAAccelerated,
        (NSOpenGLPixelFormatAttribute)0
    };
    return attributes;
}

} // namespace

namespace gl {

void setContextBehaviorFlagBits(ContextBehaviorFlagBits /*flags*/) {
    // No-op on macOS
}

Context createContext(APIVersion api, bool /*useDebugFlags*/) {
    NSOpenGLPixelFormatAttribute* attributes = nullptr;
    const char* apiName = "Unknown";

    if (api == APIVersion::GL21) {
        attributes = getGL21Attribs();
        apiName = "GL21";
    } else if (api == APIVersion::GL41 || api == APIVersion::Latest) {
        attributes = getGL41Attribs();
        apiName = (api == APIVersion::Latest) ? "Latest (GL41)" : "GL41";
    } else {
        throw std::runtime_error("[gl::createContext] Unsupported API version");
    }

    GL_LOG_INFO(@"Creating OpenGL context with API: %s", apiName);

    NSOpenGLPixelFormat* pixFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attributes];
    if (pixFormat == nil) {
        throw std::runtime_error("[gl::createContext] Failed to create pixel format");
    }

    NSOpenGLContext* openGLContext = [[NSOpenGLContext alloc] initWithFormat:pixFormat shareContext:nil];
    auto context = reinterpret_cast<Context>(const_cast<void*>(CFBridgingRetain(openGLContext)));

    if (context == nullptr) {
        throw std::runtime_error("[gl::createContext] Failed to create OpenGL context");
    }

    GL_LOG_INFO(@"Created OpenGL context: %p", context);
    return context;
}

Context createSharedContext(Context sharedContext, bool /*useDebugFlags*/) {
    GL_LOG_INFO(@"Creating shared OpenGL context (shared with: %p)", sharedContext);

    Context context = nullptr;

    if (sharedContext == nullptr) {
        context = createContext(APIVersion::GL21, false);
    } else {
        auto osxShared = (__bridge NSOpenGLContext*)sharedContext;
        NSOpenGLContext* openGLContext = [[NSOpenGLContext alloc] initWithFormat:osxShared.pixelFormat
                                                                    shareContext:osxShared];
        context = reinterpret_cast<Context>(const_cast<void*>(CFBridgingRetain(openGLContext)));
    }

    if (context == nullptr) {
        throw std::runtime_error("[gl::createSharedContext] Failed to create shared OpenGL context");
    }

    GL_LOG_INFO(@"Created shared OpenGL context: %p", context);
    return context;
}

void destroyContext(Context context) {
    GL_LOG_INFO(@"Destroying OpenGL context: %p", context);

    if (context == getActiveContext()) {
        bindContext(nullptr);
    }

    if (context != nullptr) {
        CFRelease(context);
    }

    GL_LOG_INFO(@"Destroyed OpenGL context: %p", context);
}

void bindContext(Context context) {
    if (context == getActiveContext()) {
        return;
    }

    GL_LOG_INFO(@"Binding OpenGL context: %p", context);

    if (context == nullptr) {
        [NSOpenGLContext clearCurrentContext];
    } else {
        auto osxContext = (__bridge NSOpenGLContext*)context;
        [osxContext makeCurrentContext];
    }
}

void bindContextWithSurfaces(Context context, const Surfaces& /*surfaces*/) {
    bindContext(context);
}

void retainContext(Context context) {
    if (context != nullptr) {
        GL_LOG_INFO(@"Retaining OpenGL context: %p", context);
        CFRetain(context);
    }
}

void releaseContext(Context context) {
    if (context != nullptr) {
        GL_LOG_INFO(@"Releasing OpenGL context: %p", context);
        CFRelease(context);
    }
}

Context getActiveContext() {
    NSOpenGLContext* actualContext = [NSOpenGLContext currentContext];
    if (actualContext == nil) {
        return nullptr;
    }

    auto currentContext = static_cast<Context>((__bridge_retained void*)actualContext);
    CFRelease(currentContext);
    return currentContext;
}

Display getDefaultDisplay() {
    return nullptr;
}

Surfaces getActiveSurfaces() {
    return Surfaces{};
}

APIProc getProcAddress(const char* /*procName*/) {
    return nullptr;
}

void loadAPI() {
    // No-op on macOS - OpenGL functions are linked directly via framework
}

} // namespace gl
