#include "EGLUtils.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

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
namespace {

struct ContextSurfaceData {
    EGLSurface drawSurface = EGL_NO_SURFACE;
    EGLSurface readSurface = EGL_NO_SURFACE;
};

std::unordered_map<Context, ContextSurfaceData> contextToSurfaceMap;
std::mutex contextToSurfaceMapMutex;

bool GL_KHR_no_config_context_enabled = false;
bool GL_KHR_surfaceless_context_enabled = false;
bool GL_KHR_create_context_enabled = false;

const EGLint* getEglConfigAttribs() {
    static EGLint configAttribs[] = {
#if defined(GLAD_PLATFORM_OPENGL_ES) && GLAD_PLATFORM_OPENGL_ES
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
#else
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
#endif
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_NONE, EGL_NONE,
    };
    return configAttribs;
}

const char* eglErr2Str(EGLint errCode) {
    switch (errCode) {
        case EGL_SUCCESS: return "EGL_SUCCESS";
        case EGL_NOT_INITIALIZED: return "EGL_NOT_INITIALIZED";
        case EGL_BAD_ACCESS: return "EGL_BAD_ACCESS";
        case EGL_BAD_ALLOC: return "EGL_BAD_ALLOC";
        case EGL_BAD_ATTRIBUTE: return "EGL_BAD_ATTRIBUTE";
        case EGL_BAD_CONTEXT: return "EGL_BAD_CONTEXT";
        case EGL_BAD_CONFIG: return "EGL_BAD_CONFIG";
        case EGL_BAD_CURRENT_SURFACE: return "EGL_BAD_CURRENT_SURFACE";
        case EGL_BAD_DISPLAY: return "EGL_BAD_DISPLAY";
        case EGL_BAD_SURFACE: return "EGL_BAD_SURFACE";
        case EGL_BAD_MATCH: return "EGL_BAD_MATCH";
        case EGL_BAD_PARAMETER: return "EGL_BAD_PARAMETER";
        case EGL_BAD_NATIVE_PIXMAP: return "EGL_BAD_NATIVE_PIXMAP";
        case EGL_BAD_NATIVE_WINDOW: return "EGL_BAD_NATIVE_WINDOW";
        case EGL_CONTEXT_LOST: return "EGL_CONTEXT_LOST";
        default: {
            thread_local char msg[32];
            std::snprintf(msg, sizeof(msg), "<unknown error: 0x%04X>", errCode);
            return msg;
        }
    }
}

std::string formatAllEglErrors() {
    EGLint err;
    std::stringstream ss;
    while ((err = eglGetError()) != EGL_SUCCESS) {
        ss << eglErr2Str(err) << ",";
    }
    auto res = ss.str();
    if (!res.empty()) {
        res.pop_back();
    }
    return res;
}

std::string extensionsList;
std::once_flag extensionsInitFlag;

bool checkEglExtension(std::string_view extensionName) {
    std::call_once(extensionsInitFlag, [] {
        EGLDisplay display = getDisplay();
        eglInitialize(display, nullptr, nullptr);

        const char* extensions = eglQueryString(display, EGL_EXTENSIONS);
        if (extensions != nullptr) {
            extensionsList = extensions;
            std::replace(extensionsList.begin(), extensionsList.end(), ' ', '\n');
            GL_LOG_INFO("EGL extensions:\n%s", extensionsList.c_str());
        }
    });

    auto pos = extensionsList.find(extensionName);
    return pos != std::string::npos &&
           (pos + extensionName.size() == extensionsList.size() ||
            extensionsList.at(pos + extensionName.size()) == '\n');
}

EGLConfig chooseEglConfig() {
    EGLDisplay display = getDisplay();
    eglInitialize(display, nullptr, nullptr);

    const EGLint* configAttribs = getEglConfigAttribs();
    constexpr EGLint configsMaxSize = 1;
    EGLConfig configs[configsMaxSize];
    EGLint configsSize = 0;
    EGLBoolean didChoose = eglChooseConfig(display, configAttribs, configs, configsMaxSize, &configsSize);

    if (!didChoose || configsSize == 0) {
        auto errorMessage = formatAllEglErrors();
        GL_LOG_ERROR("Could not choose EGL config: %s", errorMessage.c_str());
        throw std::runtime_error("[gl::EGL] Could not choose EGL config: " + errorMessage);
    }
    return configs[0];
}

EGLSurface createEglSurface(EGLDisplay display) {
    EGLint surfaceParams[] = {
        EGL_WIDTH, 64,
        EGL_HEIGHT, 64,
        EGL_NONE,
    };

    EGLConfig config = chooseEglConfig();
    EGLSurface surface = eglCreatePbufferSurface(display, config, surfaceParams);
    if (surface == EGL_NO_SURFACE) {
        auto errorMessage = formatAllEglErrors();
        throw std::runtime_error("[gl::EGL] Could not create EGL surface: " + errorMessage);
    }
    GL_LOG_INFO("Created EGL PBuffer surface: %p", surface);
    return surface;
}

EGLContext createEGLContext(EGLDisplay display,
                            EGLConfig config,
                            bool useDebugFlags,
                            EGLContext sharedContext = EGL_NO_CONTEXT,
                            EGLint clientVersion = 2) {
    std::vector<EGLint> attribs = {EGL_CONTEXT_CLIENT_VERSION, clientVersion};

    if (useDebugFlags && GL_KHR_create_context_enabled) {
        attribs.push_back(EGL_CONTEXT_FLAGS_KHR);
        attribs.push_back(EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR);
    }
    attribs.push_back(EGL_NONE);

    EGLContext context = eglCreateContext(display, config, sharedContext, attribs.data());

    GL_LOG_INFO("Created EGL context: %p (version: %d)", context, clientVersion);
    return context;
}

void initContextFlags() {
    setEglContextFlags(ContextBehaviorFlagBits::None);
}

} // namespace

void setEglContextFlags(ContextBehaviorFlagBits flags) {
    static std::once_flag chooseConfigFlag;
    std::call_once(chooseConfigFlag, [&] {
        auto flagsVal = static_cast<uint32_t>(flags);
        if (flagsVal & static_cast<uint32_t>(ContextBehaviorFlagBits::NoConfig)) {
            GL_KHR_no_config_context_enabled = checkEglExtension("EGL_KHR_no_config_context");
        }
        if (flagsVal & static_cast<uint32_t>(ContextBehaviorFlagBits::SurfaceLess)) {
            GL_KHR_surfaceless_context_enabled = checkEglExtension("EGL_KHR_surfaceless_context");
        }
        GL_KHR_create_context_enabled = checkEglExtension("EGL_KHR_create_context");
        GL_LOG_INFO("EGL flags: no-config=%d, surfaceless=%d, debug=%d",
                    GL_KHR_no_config_context_enabled,
                    GL_KHR_surfaceless_context_enabled,
                    GL_KHR_create_context_enabled);
    });
}

EGLDisplay getDisplay() {
    static EGLDisplay display = EGL_NO_DISPLAY;
    static std::once_flag initDisplayFlag;
    std::call_once(initDisplayFlag, [] {
        display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (display == EGL_NO_DISPLAY) {
            std::string errorMessage = formatAllEglErrors();
            throw std::runtime_error("[gl::EGL] Failed to get default display: " + errorMessage);
        }
        GL_LOG_INFO("Got EGL display: %p", display);
    });
    return display;
}

void bindContext(Context context, EGLSurface drawSurface, EGLSurface readSurface) {
    assert(drawSurface == readSurface && "[gl::EGL] drawSurface and readSurface must be equal");
    initContextFlags();

    EGLDisplay currentDisplay = getDisplay();
    EGLContext eglContext = static_cast<EGLContext>(context);

    if (context == nullptr) {
        eglContext = EGL_NO_CONTEXT;
        drawSurface = EGL_NO_SURFACE;
        readSurface = EGL_NO_SURFACE;
    } else if (drawSurface == EGL_NO_SURFACE) {
        if (GL_KHR_surfaceless_context_enabled) {
            drawSurface = readSurface = EGL_NO_SURFACE;
        } else {
            std::unique_lock<std::mutex> lk(contextToSurfaceMapMutex);
            auto& surfacePair = contextToSurfaceMap[context];
            lk.unlock();

            if (surfacePair.drawSurface == EGL_NO_SURFACE) {
                surfacePair.drawSurface = surfacePair.readSurface = createEglSurface(currentDisplay);
            }
            drawSurface = surfacePair.drawSurface;
            readSurface = surfacePair.readSurface;
        }
    }

    GL_LOG_INFO("Binding EGL context: %p (draw: %p, read: %p)", context, drawSurface, readSurface);

    EGLBoolean didMakeCurrent = eglMakeCurrent(currentDisplay, drawSurface, readSurface, eglContext);
    if (!didMakeCurrent) {
        auto errorMessage = formatAllEglErrors();
        throw std::runtime_error("[gl::EGL] eglMakeCurrent failed: " + errorMessage);
    }
}

void deleteGLContext(Context context) {
    GL_LOG_INFO("Deleting EGL context: %p", context);

    EGLContext eglContext = eglGetCurrentContext();
    EGLContext currentGLContext = static_cast<EGLContext>(context);
    if (currentGLContext == eglContext) {
        GL_LOG_WARN("Deleting currently bound context - unbinding first");
        bindContext(nullptr, EGL_NO_SURFACE, EGL_NO_SURFACE);
    }

    std::unique_lock<std::mutex> lk(contextToSurfaceMapMutex);
    auto it = contextToSurfaceMap.find(context);
    EGLDisplay display = getDisplay();
    if (it != contextToSurfaceMap.end()) {
        auto surface = it->second;
        contextToSurfaceMap.erase(it);
        lk.unlock();

        if (surface.drawSurface != EGL_NO_SURFACE) {
            GL_LOG_INFO("Destroying EGL surface: %p", surface.drawSurface);
            eglDestroySurface(display, surface.drawSurface);

            if (surface.drawSurface != surface.readSurface) {
                GL_LOG_INFO("Destroying EGL read surface: %p", surface.readSurface);
                eglDestroySurface(display, surface.readSurface);
            }
        }
    } else {
        lk.unlock();
    }

    GL_LOG_INFO("Destroying EGL context: %p", context);
    eglDestroyContext(display, currentGLContext);
}

Context createContext(Context sharedContext, bool useDebugFlags) {
    GL_LOG_INFO("Creating EGL context (shared with: %p)", sharedContext);
    initContextFlags();

    EGLDisplay display = getDisplay();
    EGLConfig config = GL_KHR_no_config_context_enabled ? EGL_NO_CONFIG_KHR : chooseEglConfig();

    EGLContext context = EGL_NO_CONTEXT;
    if (sharedContext != nullptr) {
        EGLContext sharedContextEGL = static_cast<EGLContext>(sharedContext);

        EGLint sharedContextVersion = 2;
        EGLBoolean contextVersionQuerySucceed =
            eglQueryContext(display, sharedContextEGL, EGL_CONTEXT_CLIENT_VERSION, &sharedContextVersion);
        if (!contextVersionQuerySucceed) {
            EGLint error = eglGetError();
            GL_LOG_ERROR("Could not query shared context version: 0x%X", error);
        } else {
            context = createEGLContext(display, config, useDebugFlags, sharedContextEGL, sharedContextVersion);
        }
    }

    // Try ES3 then ES2
    if (context == EGL_NO_CONTEXT) {
        context = createEGLContext(display, config, useDebugFlags, static_cast<EGLContext>(sharedContext), 3);
        if (context == EGL_NO_CONTEXT) {
            context = createEGLContext(display, config, useDebugFlags, static_cast<EGLContext>(sharedContext), 2);
        }
    }

    if (context == EGL_NO_CONTEXT) {
        auto errorMessage = formatAllEglErrors();
        throw std::runtime_error("[gl::EGL] Could not create context: " + errorMessage);
    }

    GL_LOG_INFO("Created EGL context: %p", context);
    return static_cast<Context>(context);
}

} // namespace gl
