#include <GLAD/OpenGL.h>
#include <OpenGL/Context.h>

#include "ContextUtils.hpp"

#include <cstdio>
#include <stdexcept>

// Logging macros - disabled in release builds
#ifndef NDEBUG
    #define GL_LOG_INFO(fmt, ...) std::fprintf(stderr, "[gl-loader] " fmt "\n", ##__VA_ARGS__)
    #define GL_LOG_ERROR(fmt, ...) std::fprintf(stderr, "[gl-loader] ERROR: " fmt "\n", ##__VA_ARGS__)
#else
    #define GL_LOG_INFO(fmt, ...) ((void)0)
    #define GL_LOG_ERROR(fmt, ...) ((void)0)
#endif

namespace gl {

void setContextBehaviorFlagBits(ContextBehaviorFlagBits /*flags*/) {
    // No-op on Windows
}

Context createContext(APIVersion /*api*/, bool /*useDebugFlags*/) {
    return Windows::createContext(nullptr);
}

Context createSharedContext(Context context, bool /*useDebugFlags*/) {
    return Windows::createContext(context);
}

void destroyContext(Context context) {
    Windows::deleteContext(context);
}

void bindContext(Context context) {
    Windows::bindContext(context, nullptr);
}

void bindContextWithSurfaces(Context context, const Surfaces& surfaces) {
    Windows::bindContext(context, surfaces.readSurface);
}

void retainContext(Context /*context*/) {
    // No-op on Windows
}

void releaseContext(Context /*context*/) {
    // No-op on Windows
}

Context getActiveContext() {
    return Windows::getCurrentContext();
}

Display getDefaultDisplay() {
    return nullptr;
}

Surfaces getActiveSurfaces() {
    return Surfaces{Windows::getCurrentHDC(), nullptr};
}

APIProc getProcAddress(const char* procName) {
    return reinterpret_cast<APIProc>(Windows::getGLProcAddr(procName));
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
