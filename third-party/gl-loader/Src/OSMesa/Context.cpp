#include <GLAD/OpenGL.h>
#include <OpenGL/Context.h>

#include <GL/osmesa.h>

#include <cstdio>
#include <stdexcept>
#include <string>

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

namespace gl {

void setContextBehaviorFlagBits(ContextBehaviorFlagBits /*flags*/) {
    // No-op for OSMesa
}

Context createContext(APIVersion /*api*/, bool /*useDebugFlags*/) {
    return createSharedContext(nullptr, false);
}

Context createSharedContext(Context sharedContext, bool /*useDebugFlags*/) {
    GL_LOG_INFO("Creating OSMesa context (shared with: %p)", sharedContext);

    OSMesaContext context = OSMesaCreateContext(OSMESA_RGBA, static_cast<OSMesaContext>(sharedContext));

    if (context == nullptr) {
        GL_LOG_ERROR("Failed to create OSMesa context");
        throw std::runtime_error("[gl::createSharedContext] Failed to create OSMesa context");
    }

    GL_LOG_INFO("Created OSMesa context: %p", static_cast<void*>(context));
    return static_cast<Context>(context);
}

void destroyContext(Context context) {
    GL_LOG_INFO("Destroying OSMesa context: %p", context);

    if (context != nullptr) {
        OSMesaDestroyContext(static_cast<OSMesaContext>(context));
    }

    GL_LOG_INFO("Destroyed OSMesa context: %p", context);
}

void bindContext(Context context) {
    GL_LOG_INFO("Binding OSMesa context: %p", context);

    // OSMesa requires a buffer to render into
    constexpr size_t imageWidth = 128;
    constexpr size_t imageHeight = 128;
    constexpr size_t bytesPerPixel = 4;
    thread_local uint8_t imageMemoryBuf[imageWidth * imageHeight * bytesPerPixel];

    GLboolean result = GL_FALSE;
    if (context == nullptr) {
        result = OSMesaMakeCurrent(nullptr, nullptr, GL_UNSIGNED_BYTE, 0, 0);
    } else {
        result = OSMesaMakeCurrent(static_cast<OSMesaContext>(context),
                                   imageMemoryBuf,
                                   GL_UNSIGNED_BYTE,
                                   imageWidth,
                                   imageHeight);
    }

    if (!result) {
        GL_LOG_ERROR("Failed to make OSMesa context current");
        throw std::runtime_error("[gl::bindContext] Failed to make OSMesa context current");
    }
}

void bindContextWithSurfaces(Context context, const Surfaces& /*surfaces*/) {
    bindContext(context);
}

void retainContext(Context /*context*/) {
    // No-op for OSMesa - no reference counting
}

void releaseContext(Context /*context*/) {
    // No-op for OSMesa - no reference counting
}

Context getActiveContext() {
    return static_cast<Context>(OSMesaGetCurrentContext());
}

Display getDefaultDisplay() {
    return nullptr;
}

Surfaces getActiveSurfaces() {
    return Surfaces{};
}

APIProc getProcAddress(const char* procName) {
    return reinterpret_cast<APIProc>(OSMesaGetProcAddress(procName));
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
