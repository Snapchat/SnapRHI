#include <GLAD/OpenGL.h>
#include <OpenGL/Context.h>

#include "EGLUtils.hpp"

#include <cassert>
#include <cstdio>
#include <functional>
#include <mutex>
#include <stdexcept>

// Logging macros - disabled in release builds
#ifndef NDEBUG
    #define GL_LOG_INFO(fmt, ...) std::fprintf(stderr, "[gl-loader] " fmt "\n", ##__VA_ARGS__)
    #define GL_LOG_WARN(fmt, ...) std::fprintf(stderr, "[gl-loader] WARNING: " fmt "\n", ##__VA_ARGS__)
    #define GL_LOG_ERROR(fmt, ...) std::fprintf(stderr, "[gl-loader] ERROR: " fmt "\n", ##__VA_ARGS__)
#else
    #define GL_LOG_INFO(fmt, ...) ((void)0)
    #define GL_LOG_WARN(fmt, ...) ((void)0)
    #define GL_LOG_ERROR(fmt, ...) ((void)0)
#endif

namespace {

// RAII scope guard for thread cleanup
class ThreadCleanupGuard {
public:
    ThreadCleanupGuard() {
#if !GLAD_PLATFORM_OPENGL_ES()
        eglBindAPI(EGL_OPENGL_API);
#endif
    }

    ~ThreadCleanupGuard() {
        if (eglReleaseThread() != EGL_TRUE) {
            GL_LOG_ERROR("Failed to release EGL thread");
        }
    }
};

void ensureContextReleaseForThread() {
#if !GLAD_PLATFORM_OPENGL_ES()
    eglBindAPI(EGL_OPENGL_API);
#endif
    thread_local ThreadCleanupGuard guard;
    (void)guard; // Ensure guard is instantiated
}

} // namespace

namespace gl {

void setContextBehaviorFlagBits(ContextBehaviorFlagBits flags) {
    setEglContextFlags(flags);
}

Context createContext(APIVersion /*api*/, bool useDebugFlags) {
    ensureContextReleaseForThread();
    return createSharedContext(nullptr, useDebugFlags);
}

Context createSharedContext(Context baseContext, bool useDebugFlags) {
    ensureContextReleaseForThread();
    GL_LOG_INFO("Creating shared context (base: %p)", baseContext);
    return gl::createContext(baseContext, useDebugFlags);
}

void destroyContext(Context context) {
    ensureContextReleaseForThread();
    GL_LOG_INFO("Destroying context: %p", context);
    if (context != nullptr) {
        deleteGLContext(context);
    }
}

void bindContext(Context context) {
    ensureContextReleaseForThread();
    GL_LOG_INFO("Binding context: %p", context);
    gl::bindContext(context, EGL_NO_SURFACE, EGL_NO_SURFACE);
}

void bindContextWithSurfaces(Context context, const Surfaces& surfaces) {
    ensureContextReleaseForThread();
    GL_LOG_INFO("Binding context with surfaces: %p", context);
    gl::bindContext(context, static_cast<EGLSurface>(surfaces.drawSurface),
                    static_cast<EGLSurface>(surfaces.readSurface));
}

void retainContext(Context /*context*/) {
    // No-op for EGL
}

void releaseContext(Context /*context*/) {
    // No-op for EGL
}

Context getActiveContext() {
    return static_cast<Context>(eglGetCurrentContext());
}

Display getDefaultDisplay() {
    return static_cast<Display>(gl::getDisplay());
}

Surfaces getActiveSurfaces() {
    Surfaces surfaces{};
    surfaces.drawSurface = static_cast<Surface>(eglGetCurrentSurface(EGL_DRAW));
    surfaces.readSurface = static_cast<Surface>(eglGetCurrentSurface(EGL_READ));
    return surfaces;
}

APIProc getProcAddress(const char* procName) {
    return reinterpret_cast<APIProc>(eglGetProcAddress(procName));
}

void loadAPI() {
    GL_LOG_INFO("Loading OpenGL API via GLAD");

    int gladInitResult = glad::loadOpenGLSafe(reinterpret_cast<GLADloadfunc>(gl::getProcAddress));
    if (gladInitResult == 0) {
        GL_LOG_ERROR("Failed to load OpenGL via GLAD");
        throw std::runtime_error("[gl::loadAPI] Failed to load OpenGL via GLAD");
    }

    GL_LOG_INFO("OpenGL API loaded successfully");
}

} // namespace gl
