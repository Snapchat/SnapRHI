# gl-loader

Cross-platform minimal abstraction for creating, managing, and loading
OpenGL / OpenGL ES (and WebGL via Emscripten) contexts inside the SnapRHI
project. It unifies platform differences behind a small `gl::` namespace API
with typed enums for API versions, and delegates function-pointer resolution to
the sibling `glad` library where dynamic loading is required. On platforms
with static symbol availability (Apple, WebAssembly) the load step is a no-op.

---

## Key Features

- **Unified context lifecycle** — create / share / bind / destroy across all supported platforms.
- **Multiple Linux backends** selectable at CMake configure time (`glx`, `egl`, `osmesa`).
- **Offscreen / headless contexts** — Windows hidden window, GLX minimal X11 window, OSMesa pure-software rendering (thread-local backing buffer), EGL surfaceless binding, WebAssembly canvas binding, iOS/macOS offscreen contexts.
- **API version enumeration** — `gl::APIVersion` enum covering GL 2.0–4.6, GLES 2.0–3.2, and `Latest`.
- **Hook chain** around GL loading (`installLoadAPIHooks` / `loadAPIWithHooks`) for instrumentation or tracing.
- **Thread-safe lazy loading** via `glad::loadOpenGLSafe` on backends that require dynamic resolution.
- **Retain / release** for Apple platforms (`CFRetain` / `CFRelease`); no-ops elsewhere.
- **EGL debug contexts** — `useDebugFlags=true` activates `EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR` when the `EGL_KHR_create_context` extension is available.

---

## Supported Platforms & Backends

| Platform | Backend / Mechanism | GLAD Load | Notes |
|----------|---------------------|-----------|-------|
| Windows | WGL — hidden window + `wglCreateContext` | `glad::loadOpenGLSafe` (via `wglGetProcAddress` + `GetProcAddress` fallback) | Retry loop on `wglMakeCurrent` (up to 20 attempts × 200 ms) for hibernation/resume resilience. Shared contexts via `wglShareLists` with retry. |
| macOS | `NSOpenGLContext` — Legacy 2.1 or Core 4.1 | No-op (framework symbols linked directly) | ARC enabled for `.mm` files. `APIVersion::GL21` or `GL41` / `Latest`. |
| iOS | `EAGLContext` — prefers ES 3.0, falls back to ES 2.0 | No-op (framework symbols linked directly) | `APIVersion` parameter is ignored; highest available ES version is always selected. |
| Android | EGL (shared source with Linux EGL) | `glad::loadOpenGLSafe` (via `eglGetProcAddress`) | Links `EGL`, `GLESv2`. Binds `EGL_OPENGL_ES2_BIT`. |
| Linux | GLX (default) | `glad::loadOpenGLSafe` (via `glXGetProcAddress`) | Hidden 100×100 X11 window. Requests GL 2.0 via `glXCreateContextAttribsARB`; falls back to `glXCreateNewContext`. Context map guarded by `std::mutex`. |
| Linux | EGL | `glad::loadOpenGLSafe` (via `eglGetProcAddress`) | Surfaceless when `EGL_KHR_surfaceless_context` is available; otherwise creates a 64×64 PBuffer surface. Thread-local `eglReleaseThread()` cleanup via RAII guard. |
| Linux | OSMesa | `glad::loadOpenGLSafe` (via `OSMesaGetProcAddress`) | Pure software. Thread-local 128×128 RGBA backing buffer allocated on `bindContext`. |
| WebAssembly | `emscripten_webgl_create_context` — WebGL 2 → WebGL 1 fallback | No-op (statically linked) | High-performance power preference; `preserveDrawingBuffer=true`. |

### Selecting the Linux Backend

```cmake
# At configure time:
cmake -DGL_LOADER_LINUX_BACKEND=egl ...
```

Valid values: `glx` (default), `egl`, `osmesa`.

---

## CMake Target

| Target | Type | Public Dependency |
|--------|------|-------------------|
| `gl-loader::gl-loader` | STATIC | `glad::glad-gl` |

Consumed as a CMake sub-project from the SnapRHI root:

```cmake
# Root CMakeLists.txt (already wired up):
if(SNAP_RHI_ENABLE_OPENGL)
    add_subdirectory(third-party/gl-loader)
endif()
```

### Linking

```cmake
target_link_libraries(your_target PUBLIC gl-loader::gl-loader)
```

This transitively brings in `glad::glad-gl` and, through it, `glad::glad-platform`.

### CMake Option Reference

| Option | Type | Default | Effect |
|--------|------|---------|--------|
| `GL_LOADER_LINUX_BACKEND` | `STRING` | `glx` | Linux OpenGL backend: `glx`, `egl`, or `osmesa` |

---

## Platform-Specific Linking

| Platform | Private Libraries |
|----------|------------------|
| macOS | `-framework OpenGL`, `-framework AppKit` |
| iOS | `-framework OpenGLES` |
| Windows | `opengl32` |
| Android | `EGL`, `GLESv2` |
| Linux (GLX) | `X11`, `GL` |
| Linux (EGL) | `EGL`, `GL` |
| Linux (OSMesa) | `OSMesa` |
| WebAssembly | *(none — Emscripten provides symbols)* |

Apple `.mm` sources are compiled with `-fobjc-arc`.

---

## Public API

### Headers

| Header | Purpose |
|--------|---------|
| `<OpenGL/Context.h>` | Context lifecycle, binding, hooks |
| `<OpenGL/runtime.h>` | Types: `Context`, `Surface`, `Display`, `APIProc`, `Surfaces`, `APIVersion`, `ContextBehaviorFlagBits` |
| `<OpenGL/API.h>` | Convenience re-export of `<GLAD/OpenGL.h>` |

### Types (`runtime.h`)

```cpp
namespace gl {

enum class APIVersion : uint32_t {
    None = 0,
    GL20 = 20, GL21 = 21,
    GL30 = 30, GL31 = 31, GL32 = 32, GL33 = 33,
    GL40 = 40, GL41 = 41, GL42 = 42, GL43 = 43, GL44 = 44, GL45 = 45, GL46 = 46,
    GLES20 = 1020, GLES30 = 1030, GLES31 = 1031, GLES32 = 1032,
    Latest = UINT32_MAX,
};

using Context = void*;
using Surface = void*;
using Display = void*;
using APIProc = void (*)(void);

struct Surfaces {
    Surface readSurface = nullptr;
    Surface drawSurface = nullptr;
};

enum class ContextBehaviorFlagBits : uint32_t {
    None       = 0,
    NoConfig   = 1 << 0,   // EGL_KHR_no_config_context
    SurfaceLess = 1 << 1,  // EGL_KHR_surfaceless_context
};

} // namespace gl
```

### Functions (`Context.h`)

```cpp
namespace gl {

// EGL behavior flags (no-op on non-EGL backends)
void setContextBehaviorFlagBits(ContextBehaviorFlagBits flags);

// Context lifecycle
Context createContext(APIVersion api, bool useDebugFlags = false);
Context createSharedContext(Context share, bool useDebugFlags = false);
void    destroyContext(Context);

// WebAssembly only
#if GLAD_PLATFORM_EMSCRIPTEN
Context createContext(std::string_view canvasTarget);
#endif

// Binding
void bindContext(Context);
void bindContextWithSurfaces(Context, const Surfaces&);

// Reference counting (Apple: CFRetain/CFRelease; others: no-op)
void retainContext(Context);
void releaseContext(Context);

// Queries
Context  getActiveContext();
Display  getDefaultDisplay();   // Non-null only on EGL
Surfaces getActiveSurfaces();   // Meaningful on EGL and Windows (HDC)

// GL function loading
APIProc getProcAddress(const char* name);
void    loadAPI();              // Calls glad::loadOpenGLSafe where needed; no-op on Apple/WebAssembly

// Hook support
using LoadAPIHook = void (*)(void);
struct LoadAPIHooks { LoadAPIHook preHook, inHook, postHook; };
void installLoadAPIHooks(const LoadAPIHooks&);  // Throws if already installed
void loadAPIWithHooks();                         // Thread-safe (std::call_once)

} // namespace gl
```

---

## Quick-Start Examples

### Basic Usage

```cpp
#include <OpenGL/Context.h>

int main() {
    gl::Context ctx = gl::createContext(gl::APIVersion::Latest);
    gl::bindContext(ctx);
    gl::loadAPI();  // No-op on Apple/WebAssembly; loads via GLAD elsewhere

    // ... render ...

    gl::destroyContext(ctx);
    return 0;
}
```

### Shared Contexts

```cpp
gl::Context primary = gl::createContext(gl::APIVersion::Latest);
gl::Context shared  = gl::createSharedContext(primary);
```

### WebAssembly Canvas

```cpp
#ifdef __EMSCRIPTEN__
auto ctx = gl::createContext("#canvas");
gl::bindContext(ctx);
#endif
```

### Hooks Around Loading

```cpp
gl::installLoadAPIHooks({
    []() { /* pre-load */ },
    []() { /* in-load (after GLAD, before post) */ },
    []() { /* post-load */ },
});
gl::loadAPIWithHooks();  // Thread-safe; calls loadAPI() internally
```

---

## API Version Selection

Not every `APIVersion` value is honoured on every platform:

| Platform | Behaviour |
|----------|-----------|
| macOS | `GL21` → Legacy profile; `GL41` or `Latest` → Core 4.1 profile. All others throw. |
| iOS | Ignored — always tries ES 3.0, falls back to ES 2.0. |
| EGL (Linux / Android) | Tries ES 3 then ES 2 (or matches shared context version). |
| GLX | Requests GL 2.0 via `glXCreateContextAttribsARB`; falls back to `glXCreateNewContext`. |
| OSMesa | Ignored — creates `OSMESA_RGBA` context. |
| Windows | Ignored — creates legacy WGL context. |
| WebAssembly | Ignored — WebGL 2 → 1 fallback. |

> **Tip:** validate the actual version at runtime with `glGetString(GL_VERSION)` after binding.

---

## Debug Context Support

| Backend | Status |
|---------|--------|
| EGL | ✅ Functional — `useDebugFlags=true` sets `EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR` when `EGL_KHR_create_context` is available |
| Windows | ⬜ Scaffolded but not wired (legacy `wglCreateContext` used) |
| macOS / iOS / GLX / OSMesa / WebAssembly | ⬜ `useDebugFlags` accepted but ignored |

---

## Surfaces

`gl::Surfaces` wraps draw/read surface handles. Only EGL and Windows populate
them meaningfully:

| Backend | `getActiveSurfaces()` |
|---------|-----------------------|
| EGL | Real `EGLSurface` handles (draw + read) |
| Windows | `{ currentHDC, nullptr }` |
| macOS / iOS / GLX / OSMesa / WebAssembly | `{ nullptr, nullptr }` |

`bindContextWithSurfaces` delegates to `bindContext` on all backends except
EGL (which passes surfaces through to `eglMakeCurrent`) and Windows (which
uses the draw surface as HDC).

---

## Error Handling

All errors throw `std::runtime_error` with descriptive `[gl::…]` prefixed
messages. There are no error-code return paths. Wrap calls in `try`/`catch` if
you need alternative propagation.

---

## Threading Considerations

| Backend | Notes |
|---------|-------|
| Windows | Context/HDC maps guarded by `std::mutex`. `wglMakeCurrent` retried up to 20 × 200 ms; `wglShareLists` retried up to 5 × 100 ms. |
| GLX | `std::mutex` guards the context-to-`XOpenGLProps` map. |
| EGL | Thread-local RAII `ThreadCleanupGuard` ensures `eglReleaseThread()` on thread exit. Surface map guarded by `std::mutex`. |
| macOS / iOS | Thread safety delegated to `NSOpenGLContext` / `EAGLContext` (single-writer expected). |
| OSMesa | Thread-local 128×128 backing buffer; no explicit locking. |
| WebAssembly | Single-threaded (main thread). |

`loadAPIWithHooks()` is thread-safe (`std::call_once`).

---

## Hooks Lifecycle

`installLoadAPIHooks` stores three function pointers. It throws if hooks were
already installed (one-shot). `loadAPIWithHooks()` executes via `std::call_once`:

1. `preHook()`
2. `loadAPI()` (GLAD or no-op)
3. `inHook()`
4. `postHook()` (via RAII scope guard — runs even if `inHook` throws)

Use hooks to inject logging, custom symbol resolution, or performance metrics.

---

## Troubleshooting

| Symptom | Likely Cause | Resolution |
|---------|-------------|------------|
| `Failed to load OpenGL via GLAD` | No current GL context when `loadAPI()` is called | Call `bindContext()` **before** `loadAPI()` |
| `Failed to open X display` (GLX) | No `$DISPLAY` environment variable or X server unavailable | Run inside an X session, or switch to EGL/OSMesa backend |
| `Failed to create window` (Windows) | Window class registration failed | Check system resources; ensure `opengl32.dll` is present |
| `setCurrentContext failed` (iOS) | Invalid `EAGLContext` | Ensure context was created successfully and not already destroyed |
| `Could not choose EGL config` | No matching config for the requested attributes | Verify GPU drivers support the EGL renderable type; try `osmesa` for software fallback |
| `eglMakeCurrent failed` | Mismatched surface/config or context already current on another thread | Unbind from the other thread first; check `eglGetError()` output in debug builds |
| `OpenGL initialization hooks were already installed` | `installLoadAPIHooks` called twice | Only install hooks once per process lifetime |

---

## Source Layout

```
gl-loader/
├── CMakeLists.txt                  # Root project — requires CMake ≥ 3.25
├── LICENSE.md
├── README.md                       # This file
└── Src/
    ├── CMakeLists.txt              # Library target, platform selection, linking
    ├── Context.cpp                 # Shared: hook storage, installLoadAPIHooks, loadAPIWithHooks
    ├── include/
    │   └── OpenGL/
    │       ├── API.h               # Re-exports <GLAD/OpenGL.h>
    │       ├── Context.h           # Public API declarations
    │       └── runtime.h           # Types: Context, Surface, APIVersion, etc.
    ├── macOS/
    │   └── Context.mm              # NSOpenGLContext (GL 2.1 / 4.1 Core)
    ├── iOS/
    │   └── Context.mm              # EAGLContext (ES 3.0 → 2.0 fallback)
    ├── Windows/
    │   ├── Context.cpp             # WGL front-end
    │   ├── ContextUtils.hpp        # Windows-specific internal API
    │   └── ContextUtils.cpp        # Hidden window, wglCreateContext, retry logic
    ├── EGL/
    │   ├── Context.cpp             # EGL front-end (Android + Linux EGL)
    │   ├── EGLUtils.hpp            # EGL internal API
    │   └── EGLUtils.cpp            # Display/config/surface/context management
    ├── GLX/
    │   └── Context.cpp             # GLX + X11 window + context map
    ├── OSMesa/
    │   └── Context.cpp             # OSMesa software rendering
    └── WebAssembly/
        └── Context.cpp             # Emscripten WebGL 2/1
```
