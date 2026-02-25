#include <GLAD/OpenGL.h>
#include <OpenGL/Context.h>

#include <cstdio>
#include <stdexcept>
#include <string>

#include <emscripten.h>
#include <emscripten/html5.h>

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

std::string defaultContextTarget;
bool instantiatedAsWebGL1 = false;

EmscriptenWebGLContextAttributes getDefaultWebGLContextAttributes() {
    EmscriptenWebGLContextAttributes attrs{};
    attrs.explicitSwapControl = false;
    attrs.alpha = true;
    attrs.depth = false;
    attrs.stencil = false;
    attrs.antialias = false;
    attrs.majorVersion = 2;
    attrs.minorVersion = 0;
    attrs.powerPreference = EM_WEBGL_POWER_PREFERENCE_HIGH_PERFORMANCE;
    attrs.failIfMajorPerformanceCaveat = false;
    attrs.renderViaOffscreenBackBuffer = false;
    attrs.premultipliedAlpha = true;
    attrs.proxyContextToMainThread = false;
    attrs.preserveDrawingBuffer = true;
    attrs.enableExtensionsByDefault = true;
    return attrs;
}

gl::Context createWebGLContext(std::string_view target) {
    defaultContextTarget = target;
    auto attrs = getDefaultWebGLContextAttributes();

    // If WebGL1 was previously instantiated, use WebGL1 params
    if (instantiatedAsWebGL1) {
        emscripten_webgl_init_context_attributes(&attrs);
    }

    GL_LOG_INFO("Creating WebGL context for target: %s", target.empty() ? "(default)" : target.data());

    auto context = emscripten_webgl_create_context(target.data(), &attrs);
    if (context <= 0) {
        // Try WebGL1 as fallback
        GL_LOG_WARN("WebGL2 failed, trying WebGL1");
        instantiatedAsWebGL1 = true;
        emscripten_webgl_init_context_attributes(&attrs);
        context = emscripten_webgl_create_context(target.data(), &attrs);

        if (context <= 0) {
            GL_LOG_ERROR("Failed to create WebGL context: %d", context);
            throw std::runtime_error("[gl::WebAssembly] Failed to create WebGL context");
        }
    }

    GL_LOG_INFO("Created WebGL context: %d", context);
    return reinterpret_cast<gl::Context>(context);
}

} // namespace

namespace gl {

void setContextBehaviorFlagBits(ContextBehaviorFlagBits /*flags*/) {
    // No-op for WebAssembly
}

Context createContext(APIVersion /*api*/, bool /*useDebugFlags*/) {
    return createSharedContext(nullptr, false);
}

Context createContext(std::string_view canvasTarget) {
    return createWebGLContext(canvasTarget);
}

Context createSharedContext(Context /*baseContext*/, bool /*useDebugFlags*/) {
    std::string_view target = defaultContextTarget.empty() ? std::string_view() : std::string_view(defaultContextTarget);
    return createWebGLContext(target);
}

void destroyContext(Context context) {
    GL_LOG_INFO("Destroying WebGL context: %p", context);
    if (context != nullptr) {
        emscripten_webgl_destroy_context(reinterpret_cast<EMSCRIPTEN_WEBGL_CONTEXT_HANDLE>(context));
    }
}

void bindContext(Context context) {
    GL_LOG_INFO("Binding WebGL context: %p", context);
    EMSCRIPTEN_RESULT result = emscripten_webgl_make_context_current(reinterpret_cast<EMSCRIPTEN_WEBGL_CONTEXT_HANDLE>(context));
    if (result != EMSCRIPTEN_RESULT_SUCCESS) {
        GL_LOG_ERROR("Failed to make WebGL context current: %d", result);
        throw std::runtime_error("[gl::WebAssembly] Failed to bind WebGL context");
    }
}

void bindContextWithSurfaces(Context context, const Surfaces& /*surfaces*/) {
    bindContext(context);
}

void retainContext(Context /*context*/) {
    // No-op for WebAssembly
}

void releaseContext(Context /*context*/) {
    // No-op for WebAssembly
}

Context getActiveContext() {
    return reinterpret_cast<Context>(emscripten_webgl_get_current_context());
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
    // No-op for WebAssembly - GL functions are statically linked
    GL_LOG_INFO("WebAssembly: OpenGL functions are statically linked");
}

} // namespace gl
