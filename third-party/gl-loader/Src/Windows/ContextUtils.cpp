#include "ContextUtils.hpp"

#include <GLAD/OpenGL.h>

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <wingdi.h>

#include <cassert>
#include <chrono>
#include <cstdio>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
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

namespace {

const wchar_t windowClassName[] = L"SnapRHIOffscreenWindow";
std::once_flag windowClassRegistered;
bool windowClassRegistrationFailed = false;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE: {
            PIXELFORMATDESCRIPTOR pfd = {
                sizeof(PIXELFORMATDESCRIPTOR),
                1,
                PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
                PFD_TYPE_RGBA,
                32,  // Color depth
                0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0,
                24,  // Depth buffer
                8,   // Stencil buffer
                0,
                PFD_MAIN_PLANE,
                0, 0, 0, 0
            };

            HDC hdc = GetDC(hWnd);
            int pixelFormat = ChoosePixelFormat(hdc, &pfd);
            SetPixelFormat(hdc, pixelFormat, &pfd);
            break;
        }
        default:
            return DefWindowProcW(hWnd, message, wParam, lParam);
    }
    return 0;
}

HWND createWindow() {
    HINSTANCE hInstance = static_cast<HINSTANCE>(GetModuleHandleW(nullptr));

    std::call_once(windowClassRegistered, [hInstance] {
        WNDCLASSW wc = {};
        wc.lpfnWndProc = WndProc;
        wc.hInstance = hInstance;
        wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
        wc.lpszClassName = windowClassName;
        wc.style = CS_OWNDC;

        if (!RegisterClassW(&wc)) {
            windowClassRegistrationFailed = true;
            GL_LOG_ERROR("Failed to register window class");
        }
    });

    if (windowClassRegistrationFailed) {
        return nullptr;
    }

    static int counter = 0;
    std::wstring windowName = L"SnapRHIOffscreen" + std::to_wstring(counter++);

    HWND hwnd = CreateWindowW(
        windowClassName,
        windowName.c_str(),
        0,  // Style (hidden)
        0, 0, 128, 128,
        nullptr, nullptr,
        hInstance,
        nullptr
    );

    return hwnd;
}

struct ContextInfo {
    std::unordered_map<HGLRC, HDC> toDCMap;
    std::unordered_map<HGLRC, HWND> toWindowMap;
    std::mutex mutex;
};

ContextInfo& getContextInfo() {
    static ContextInfo info;
    return info;
}

struct OpenGLModuleGuard {
    HMODULE module;

    OpenGLModuleGuard() : module(LoadLibraryA("opengl32.dll")) {}
    ~OpenGLModuleGuard() {
        if (module) {
            FreeLibrary(module);
        }
    }
};

void initGLAD() {
    static std::once_flag gladInitFlag;
    std::call_once(gladInitFlag, [] {
        int result = glad::loadOpenGLSafe(reinterpret_cast<GLADloadfunc>(gl::Windows::getGLProcAddr));
        if (result == 0) {
            throw std::runtime_error("[gl::Windows] Failed to load OpenGL via GLAD");
        }
        GL_LOG_INFO("GLAD initialized successfully");
    });
}

} // namespace

namespace gl::Windows {

void* getGLProcAddr(const char* name) {
    static OpenGLModuleGuard gl;

    void* p = reinterpret_cast<void*>(wglGetProcAddress(name));

    // wglGetProcAddress returns NULL, 1, 2, 3, or -1 for invalid/unavailable functions
    if (p == nullptr || p == reinterpret_cast<void*>(1) ||
        p == reinterpret_cast<void*>(2) || p == reinterpret_cast<void*>(3) ||
        p == reinterpret_cast<void*>(-1)) {
        p = reinterpret_cast<void*>(GetProcAddress(gl.module, name));
    }

    return p;
}

Context getCurrentContext() {
    return static_cast<Context>(wglGetCurrentContext());
}

void* getCurrentHDC() {
    return static_cast<void*>(wglGetCurrentDC());
}

Context createContext(Context sharedContext) {
    GL_LOG_INFO("Creating Windows OpenGL context (shared: %p)", sharedContext);

    HWND hwnd = createWindow();
    if (hwnd == nullptr) {
        throw std::runtime_error("[gl::Windows] Failed to create window");
    }

    HDC hdc = GetDC(hwnd);
    if (hdc == nullptr) {
        DestroyWindow(hwnd);
        throw std::runtime_error("[gl::Windows] Failed to get device context");
    }

    HGLRC context = wglCreateContext(hdc);
    if (context == nullptr) {
        ReleaseDC(hwnd, hdc);
        DestroyWindow(hwnd);
        throw std::runtime_error("[gl::Windows] Failed to create OpenGL context");
    }

    {
        auto& info = getContextInfo();
        std::lock_guard<std::mutex> lock(info.mutex);
        info.toDCMap[context] = hdc;
        info.toWindowMap[context] = hwnd;
    }

    if (sharedContext != nullptr) {
        constexpr int maxRetries = 5;
        bool shareSuccess = false;

        for (int retry = 0; retry < maxRetries; ++retry) {
            if (wglShareLists(static_cast<HGLRC>(sharedContext), context)) {
                GL_LOG_INFO("Shared contexts %p and %p successfully", sharedContext, static_cast<void*>(context));
                shareSuccess = true;
                break;
            }

            DWORD error = GetLastError();
            GL_LOG_WARN("Failed to share contexts (attempt %d/%d, error: %lu)", retry + 1, maxRetries, error);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        if (!shareSuccess) {
            deleteContext(static_cast<Context>(context));
            throw std::runtime_error("[gl::Windows] Failed to share OpenGL contexts");
        }
    }

    GL_LOG_INFO("Created Windows OpenGL context: %p", static_cast<void*>(context));
    return static_cast<Context>(context);
}

void bindContext(Context context, void* hdc) {
    HDC dc = static_cast<HDC>(hdc);
    HGLRC glContext = static_cast<HGLRC>(context);

    if (glContext == wglGetCurrentContext()) {
        return;
    }

    GL_LOG_INFO("Binding Windows OpenGL context: %p", context);

    auto& info = getContextInfo();
    std::unique_lock<std::mutex> lock(info.mutex);

    if (context != nullptr && dc == nullptr) {
        auto it = info.toDCMap.find(glContext);
        if (it != info.toDCMap.end()) {
            dc = it->second;
        } else {
            lock.unlock();
            throw std::runtime_error("[gl::Windows] Context not tracked - no valid HDC");
        }
    }
    lock.unlock();

    constexpr int maxRetries = 20;
    for (int retry = 0; retry < maxRetries; ++retry) {
        if (wglMakeCurrent(dc, glContext)) {
            if (context != nullptr) {
                initGLAD();
            }
            return;
        }

        DWORD error = GetLastError();
        GL_LOG_WARN("Failed to make context current (attempt %d/%d, error: %lu)", retry + 1, maxRetries, error);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    throw std::runtime_error("[gl::Windows] Failed to make context current");
}

void deleteContext(Context context) {
    if (context == nullptr) {
        return;
    }

    GL_LOG_INFO("Deleting Windows OpenGL context: %p", context);

    HGLRC glContext = static_cast<HGLRC>(context);
    auto& info = getContextInfo();

    std::unique_lock<std::mutex> lock(info.mutex);

    HDC hdc = nullptr;
    HWND hwnd = nullptr;

    auto dcIt = info.toDCMap.find(glContext);
    if (dcIt != info.toDCMap.end()) {
        hdc = dcIt->second;
        info.toDCMap.erase(dcIt);
    }

    auto windowIt = info.toWindowMap.find(glContext);
    if (windowIt != info.toWindowMap.end()) {
        hwnd = windowIt->second;
        info.toWindowMap.erase(windowIt);
    }

    lock.unlock();

    // Unbind if currently bound
    if (wglGetCurrentContext() == glContext) {
        wglMakeCurrent(nullptr, nullptr);
    }

    if (!wglDeleteContext(glContext)) {
        GL_LOG_ERROR("Failed to delete OpenGL context");
    }

    if (hdc != nullptr && hwnd != nullptr) {
        ReleaseDC(hwnd, hdc);
    }

    if (hwnd != nullptr) {
        DestroyWindow(hwnd);
    }

    GL_LOG_INFO("Deleted Windows OpenGL context: %p", context);
}

} // namespace gl::Windows
