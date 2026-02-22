#include <GLAD/OpenGL.h>
#include <OpenGL/Context.h>

#include <GL/glx.h>
#include <GL/glxext.h>
#include <X11/Xlib.h>

#include <cassert>
#include <cstdio>
#include <mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>

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

#define GLX_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB 0x2092
#define GLX_CONTEXT_PROFILE_MASK_ARB 0x9126
#define GLX_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001

using GLXCREATECONTEXTATTRIBSARBPROC = GLXContext (*)(::Display*, GLXFBConfig, GLXContext, Bool, const int*);

namespace {

/// Context properties with proper resource management
struct XOpenGLProps {
    ::Display* display = nullptr;
    Window window = 0;
    Colormap colormap = 0;
    GLXContext context = nullptr;

    XOpenGLProps() = default;

    // Non-copyable
    XOpenGLProps(const XOpenGLProps&) = delete;
    XOpenGLProps& operator=(const XOpenGLProps&) = delete;

    // Movable
    XOpenGLProps(XOpenGLProps&& other) noexcept
        : display(other.display), window(other.window), colormap(other.colormap), context(other.context) {
        other.display = nullptr;
        other.window = 0;
        other.colormap = 0;
        other.context = nullptr;
    }

    XOpenGLProps& operator=(XOpenGLProps&& other) noexcept {
        if (this != &other) {
            cleanup();
            display = other.display;
            window = other.window;
            colormap = other.colormap;
            context = other.context;
            other.display = nullptr;
            other.window = 0;
            other.colormap = 0;
            other.context = nullptr;
        }
        return *this;
    }

    ~XOpenGLProps() {
        cleanup();
    }

    void cleanup() {
        if (display != nullptr) {
            // Unbind if this is the current context
            if (context != nullptr && glXGetCurrentContext() == context) {
                glXMakeCurrent(display, None, nullptr);
            }

            // Destroy GLX context
            if (context != nullptr) {
                glXDestroyContext(display, context);
                context = nullptr;
            }

            // Destroy window
            if (window != 0) {
                XDestroyWindow(display, window);
                window = 0;
            }

            // Free colormap
            if (colormap != 0) {
                XFreeColormap(display, colormap);
                colormap = 0;
            }

            // Close display
            XCloseDisplay(display);
            display = nullptr;
        }
    }

    bool isValid() const {
        return context != nullptr && display != nullptr;
    }
};

XOpenGLProps createXWindowAndContext(GLXContext sharedContext) {
    ::Display* dpy = XOpenDisplay(nullptr);
    if (dpy == nullptr) {
        throw std::runtime_error("[gl::GLX] Failed to open X display");
    }

    const int screen = DefaultScreen(dpy);

    // Choose framebuffer config
    int nelements = 0;
    GLXFBConfig* fbc = glXChooseFBConfig(dpy, screen, nullptr, &nelements);
    if (fbc == nullptr || nelements == 0) {
        XCloseDisplay(dpy);
        throw std::runtime_error("[gl::GLX] Failed to find suitable GLXFBConfig");
    }

    // Get visual info
    static int attributeList[] = {
        GLX_RGBA, GLX_RED_SIZE, 1, GLX_GREEN_SIZE, 1, GLX_BLUE_SIZE, 1, GLX_DOUBLEBUFFER, None};
    XVisualInfo* vi = glXChooseVisual(dpy, screen, attributeList);
    if (vi == nullptr) {
        XFree(fbc);
        XCloseDisplay(dpy);
        throw std::runtime_error("[gl::GLX] Failed to get XVisualInfo");
    }

    // Create colormap
    Colormap cmap = XCreateColormap(dpy, RootWindow(dpy, vi->screen), vi->visual, AllocNone);
    if (cmap == 0) {
        XFree(vi);
        XFree(fbc);
        XCloseDisplay(dpy);
        throw std::runtime_error("[gl::GLX] Failed to create colormap");
    }

    // Create window
    XSetWindowAttributes swa{};
    swa.colormap = cmap;
    swa.border_pixel = 0;
    swa.event_mask = StructureNotifyMask;

    Window win = XCreateWindow(dpy,
                               RootWindow(dpy, vi->screen),
                               0, 0, 100, 100, 0,
                               vi->depth,
                               InputOutput,
                               vi->visual,
                               CWBorderPixel | CWColormap | CWEventMask,
                               &swa);

    if (win == 0) {
        XFreeColormap(dpy, cmap);
        XFree(vi);
        XFree(fbc);
        XCloseDisplay(dpy);
        throw std::runtime_error("[gl::GLX] Failed to create window");
    }

    // Get glXCreateContextAttribsARB function
    auto glXCreateContextAttribsARB = reinterpret_cast<GLXCREATECONTEXTATTRIBSARBPROC>(
        glXGetProcAddress(reinterpret_cast<const GLubyte*>("glXCreateContextAttribsARB")));

    GLXContext ctx = nullptr;

    if (glXCreateContextAttribsARB != nullptr) {
        int attribs[] = {GLX_CONTEXT_MAJOR_VERSION_ARB, 2, GLX_CONTEXT_MINOR_VERSION_ARB, 0, None};
        ctx = glXCreateContextAttribsARB(dpy, fbc[0], sharedContext, True, attribs);
    } else {
        // Fallback to legacy context creation
        ctx = glXCreateNewContext(dpy, fbc[0], GLX_RGBA_TYPE, sharedContext, True);
    }

    // Clean up temporaries
    XFree(vi);
    XFree(fbc);

    if (ctx == nullptr) {
        XDestroyWindow(dpy, win);
        XFreeColormap(dpy, cmap);
        XCloseDisplay(dpy);
        throw std::runtime_error("[gl::GLX] Failed to create GLX context");
    }

    // Success - return props with ownership
    XOpenGLProps props;
    props.display = dpy;
    props.window = win;
    props.colormap = cmap;
    props.context = ctx;

    return props;
}

std::unordered_map<GLXContext, XOpenGLProps> contextToPropsMap;
std::mutex contextMutex;

} // namespace

namespace gl {

void setContextBehaviorFlagBits(ContextBehaviorFlagBits /*flags*/) {
    // No-op for GLX
}

Context createContext(APIVersion /*api*/, bool /*useDebugFlags*/) {
    return createSharedContext(nullptr, false);
}

Context createSharedContext(Context sharedContext, bool /*useDebugFlags*/) {
    GL_LOG_INFO("Creating GLX context (shared with: %p)", sharedContext);

    XOpenGLProps openGLProps = createXWindowAndContext(static_cast<GLXContext>(sharedContext));

    if (!openGLProps.isValid()) {
        throw std::runtime_error("[gl::createSharedContext] Created invalid context");
    }

    GLXContext context = openGLProps.context;

    std::lock_guard<std::mutex> lock(contextMutex);
    contextToPropsMap.emplace(context, std::move(openGLProps));

    GL_LOG_INFO("Created GLX context: %p", static_cast<void*>(context));
    return static_cast<Context>(context);
}

void destroyContext(Context vContext) {
    GL_LOG_INFO("Destroying GLX context: %p", vContext);

    if (vContext == nullptr) {
        return;
    }

    auto context = static_cast<GLXContext>(vContext);

    std::lock_guard<std::mutex> lock(contextMutex);
    auto it = contextToPropsMap.find(context);

    if (it == contextToPropsMap.end()) {
        GL_LOG_WARN("Attempting to destroy untracked context: %p", vContext);
        return;
    }

    // Cleanup done by XOpenGLProps destructor when erasing
    contextToPropsMap.erase(it);

    GL_LOG_INFO("Destroyed GLX context: %p", vContext);
}

void bindContext(Context vContext) {
    if (vContext == nullptr) {
        GL_LOG_INFO("Unbinding GLX context");
        auto currentDisplay = glXGetCurrentDisplay();
        if (currentDisplay != nullptr) {
            glXMakeCurrent(currentDisplay, None, nullptr);
        }
        return;
    }

    GL_LOG_INFO("Binding GLX context: %p", vContext);

    auto context = static_cast<GLXContext>(vContext);

    std::lock_guard<std::mutex> lock(contextMutex);
    auto it = contextToPropsMap.find(context);

    if (it == contextToPropsMap.end()) {
        throw std::runtime_error("[gl::bindContext] Attempting to bind untracked context");
    }

    const XOpenGLProps& props = it->second;

    if (!glXMakeCurrent(props.display, props.window, context)) {
        throw std::runtime_error("[gl::bindContext] Failed to make context current");
    }
}

void bindContextWithSurfaces(Context context, const Surfaces& /*surfaces*/) {
    bindContext(context);
}

void retainContext(Context /*context*/) {
    // No-op for GLX
}

void releaseContext(Context /*context*/) {
    // No-op for GLX
}

Context getActiveContext() {
    return static_cast<Context>(glXGetCurrentContext());
}

Display getDefaultDisplay() {
    return nullptr;
}

Surfaces getActiveSurfaces() {
    return Surfaces{};
}

APIProc getProcAddress(const char* procName) {
    if (procName == nullptr) {
        return nullptr;
    }
    return reinterpret_cast<APIProc>(glXGetProcAddress(reinterpret_cast<const GLubyte*>(procName)));
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
