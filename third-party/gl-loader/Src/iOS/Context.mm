#include <GLAD/OpenGL.h>
#include <OpenGL/Context.h>

#include <OpenGLES/EAGL.h>

#include <stdexcept>
#include <string>

// Logging macros - disabled in release builds
#ifndef NDEBUG
    #include <Foundation/Foundation.h>
    #define GL_LOG_INFO(fmt, ...) NSLog(@"[gl-loader] " fmt, ##__VA_ARGS__)
    #define GL_LOG_WARN(fmt, ...) NSLog(@"[gl-loader] WARNING: " fmt, ##__VA_ARGS__)
    #define GL_LOG_ERROR(fmt, ...) NSLog(@"[gl-loader] ERROR: " fmt, ##__VA_ARGS__)
#else
    #define GL_LOG_INFO(fmt, ...) ((void)0)
    #define GL_LOG_WARN(fmt, ...) ((void)0)
    #define GL_LOG_ERROR(fmt, ...) ((void)0)
#endif

namespace gl {

void setContextBehaviorFlagBits(ContextBehaviorFlagBits /*flags*/) {
    // No-op on iOS
}

Context createContext(APIVersion /*api*/, bool /*useDebugFlags*/) {
    return createSharedContext(nullptr, false);
}

Context createSharedContext(Context sharedContext, bool /*useDebugFlags*/) {
    GL_LOG_INFO(@"Creating shared context (shared with: %p)", sharedContext);

    EAGLContext* sharedContextEAGL = (__bridge EAGLContext*)sharedContext;
    Context context = nullptr;

    if (sharedContextEAGL != nullptr) {
        context = reinterpret_cast<Context>(const_cast<void*>(CFBridgingRetain(
            [[EAGLContext alloc] initWithAPI:sharedContextEAGL.API
                                 sharegroup:[sharedContextEAGL sharegroup]])));
    } else {
        // Try OpenGL ES 3.0 first, then fall back to 2.0
        EAGLContext* eaglContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];
        if (eaglContext == nil) {
            eaglContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
        }
        if (eaglContext != nil) {
            context = reinterpret_cast<Context>(const_cast<void*>(CFBridgingRetain(eaglContext)));
        }
    }

    if (context == nullptr) {
        GL_LOG_ERROR(@"Failed to create EAGLContext shared with %p", sharedContext);
        throw std::runtime_error("[gl::createSharedContext] Failed to create EAGLContext");
    }

    GL_LOG_INFO(@"Created OpenGL ES context: %p", context);
    return context;
}

void destroyContext(Context context) {
    GL_LOG_INFO(@"Destroying context: %p", context);

    if (context == getActiveContext()) {
        GL_LOG_WARN(@"Destroying currently bound context - unbinding first");
        bindContext(nullptr);
    }

    if (context != nullptr) {
        CFRelease(context);
    }

    GL_LOG_INFO(@"Destroyed context: %p", context);
}

void bindContext(Context context) {
    if (context == getActiveContext()) {
        return;
    }

    GL_LOG_INFO(@"Binding context: %p", context);

    BOOL didSetContext = NO;
    if (context != nullptr) {
        didSetContext = [EAGLContext setCurrentContext:(__bridge EAGLContext*)context];
    } else {
        didSetContext = [EAGLContext setCurrentContext:nil];
    }

    if (!didSetContext) {
        throw std::runtime_error("[gl::bindContext] setCurrentContext failed");
    }
}

void bindContextWithSurfaces(Context context, const Surfaces& /*surfaces*/) {
    bindContext(context);
}

void retainContext(Context context) {
    if (context != nullptr) {
        GL_LOG_INFO(@"Retaining context: %p", context);
        CFRetain(context);
    }
}

void releaseContext(Context context) {
    if (context != nullptr) {
        GL_LOG_INFO(@"Releasing context: %p", context);
        CFRelease(context);
    }
}

Context getActiveContext() {
    EAGLContext* actualContext = [EAGLContext currentContext];
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
    // No-op on iOS - OpenGL ES functions are linked directly via framework
}

} // namespace gl
