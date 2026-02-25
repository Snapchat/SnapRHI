# GLAD — OpenGL / Vulkan Loader (SnapRHI Fork)

Generated (and lightly customised) function loaders for the SnapRHI project,
produced with **[GLAD 2.0.8](https://glad.dav1d.de/)**.

Supported APIs:

| API | Spec | Extensions |
|-----|------|------------|
| OpenGL (desktop) | `gl:compatibility=4.6` | 12 selected extensions |
| OpenGL ES (mobile) | `gles2=3.2` | 47 selected extensions |
| Vulkan | `vulkan=1.4` | 405 extensions (all) |

---

## CMake Targets

| Target | Type | Purpose |
|--------|------|---------|
| `glad::glad-platform` | STATIC | Shared platform-detection header (`<GLAD/gladPlatform.h>`) and utilities used by every loader |
| `glad::glad-gl` | STATIC | Unified OpenGL entry point — selects desktop GL **or** GLES at build time and re-exports the correct sub-target |
| `glad::glad-platform-gl` | STATIC | Desktop OpenGL loader (`gl:compatibility=4.6`). Built when `SNAP_RHI_ENABLE_OPENGL=ON` on Windows, Linux, or macOS |
| `glad::glad-platform-gl-es` | STATIC | OpenGL ES loader (`gles2=3.2`). Built when `SNAP_RHI_ENABLE_OPENGL=ON` on Android or iOS |
| `glad::glad-vulkan` | STATIC | Vulkan 1.4 function loader with optional iOS framework / validation-layer support. Built when `SNAP_RHI_ENABLE_VULKAN=ON` |

### Backend-Selection Logic (OpenGL)

```
SNAP_RHI_ENABLE_OPENGL=ON
 ├─ Android / iOS  →  glad-platform-gl-es  (GLAD_OPENGL_ES=1, GLAD_OPENGL=0)
 └─ Windows / Linux / macOS  →  glad-platform-gl  (GLAD_OPENGL=1, GLAD_OPENGL_ES=0)
```

On **Linux**, a cache variable selects the windowing backend:

```cmake
set(GLAD_OPENGL_BACKEND_LINUX "glx" CACHE STRING "Linux OpenGL backend (glx, egl, osmesa)")
```

---

## Include Directives

**OpenGL** (unified header — resolves to GL or GLES at compile time):

```cpp
#include <GLAD/OpenGL.h>
```

**Vulkan**:

```cpp
#include <GLAD/vulkan.h>
```

> Internal generated headers such as `GLAD/gl.h`, `GLAD/gles.h`, and the
> `KHR/`, `EGL/`, `Android/` sub-headers are pulled in transitively — prefer
> the two public includes above.

---

## Public Loader Entry Points

### OpenGL / OpenGL ES

Declared in the unified wrapper `<GLAD/OpenGL.h>` (delegates to generated
headers):

```cpp
// In namespace glad (from OpenGL.cpp):
int glad::loadOpenGL(GLADloadfunc load);      // One-shot load (GL or GLES per platform)
int glad::loadOpenGLSafe(GLADloadfunc load);  // Thread-safe (std::call_once) variant

// Raw generated functions (desktop — from gl.h):
int gladLoadGL(GLADloadfunc load);
int gladLoadGLSafe(GLADloadfunc load);

// Raw generated functions (mobile — from gles.h):
int gladLoadGLES(GLADloadfunc load);
int gladLoadGLESSafe(GLADloadfunc load);
```

### Vulkan

Declared in `<GLAD/vulkan.h>`:

```cpp
int  gladLoadVulkan(VkPhysicalDevice physDev, GLADloadfunc load);
int  gladLoadVulkanUserPtr(VkPhysicalDevice physDev, GLADuserptrloadfunc load, void* userptr);

// Built-in loader (GLAD_OPTION_VULKAN_LOADER) — dlopen's the Vulkan library:
int  gladLoaderLoadVulkan(VkInstance instance, VkPhysicalDevice physDev, VkDevice device);
int  gladLoaderInitSafe();          // Thread-safe global init (no instance/device)
void gladLoaderUnloadVulkan(void);  // Tear-down
```

Use the **`*Safe`** variants when multiple threads may race to initialise the
loader.

---

## Build & Integration

GLAD is consumed as a CMake sub-project from the SnapRHI root `CMakeLists.txt`.
It is added automatically when either OpenGL or Vulkan is enabled:

```cmake
# In the root CMakeLists.txt (already wired up):
if(SNAP_RHI_ENABLE_OPENGL OR SNAP_RHI_ENABLE_VULKAN)
    add_subdirectory(third-party/GLAD)
endif()
```

### Linking Against GLAD

```cmake
# OpenGL
target_link_libraries(your_target PUBLIC glad::glad-gl)

# Vulkan
target_link_libraries(your_target PUBLIC glad::glad-vulkan)
```

### CMake Option Reference

| Option | Type | Default | Effect |
|--------|------|---------|--------|
| `SNAP_RHI_ENABLE_OPENGL` | `BOOL` | `OFF` | Build the OpenGL loader targets (`glad-gl`, `glad-platform-gl` or `glad-platform-gl-es`) |
| `SNAP_RHI_ENABLE_VULKAN` | `BOOL` | `OFF` | Build the Vulkan loader target (`glad-vulkan`) |
| `SNAP_RHI_VULKAN_ENABLE_LAYERS` | `BOOL` | `OFF` | Enable Vulkan validation-layer integration (iOS framework mode; sets `VULKAN_NATIVE_VALIDATION_LAYERS=1`) |
| `GLAD_OPENGL_BACKEND_LINUX` | `STRING` | `glx` | Linux OpenGL windowing backend: `glx`, `egl`, or `osmesa` |

---

## Platform-Specific Linking Details

### Desktop OpenGL (`glad-platform-gl`)

| Platform | Linked Libraries |
|----------|-----------------|
| macOS | `-framework OpenGL` |
| Windows | `OpenGL::GL` (via `FindOpenGL`). MSVC warnings `/wd5030`, `/wd4068` suppressed |
| Linux (GLX) | `X11`, `GL`, `${CMAKE_DL_LIBS}` |
| Linux (EGL) | `EGL`, `GL`, `${CMAKE_DL_LIBS}` |
| Linux (OSMesa) | `${CMAKE_DL_LIBS}` (OSMesa linking handled externally) |

### OpenGL ES (`glad-platform-gl-es`)

| Platform | Linked Libraries |
|----------|-----------------|
| Android | `GLESv2`, `EGL` |
| iOS | `-framework GLKit`, `-framework OpenGLES` |

### Vulkan (`glad-vulkan`)

All platforms link `glad::glad-platform`.

| Platform | Additional Libraries / Notes |
|----------|------------------------------|
| Linux | `${CMAKE_DL_LIBS}` |
| Android | `VK_USE_PLATFORM_ANDROID_KHR` defined |
| Apple | `VK_ENABLE_BETA_EXTENSIONS`, `VK_USE_PLATFORM_METAL_EXT` defined |
| **iOS — static mode** (default) | Links `libMoltenVK.a` from the Vulkan SDK `MoltenVK.xcframework`. Also links `-framework Metal`, `-framework IOSurface`, `-framework QuartzCore`, `-framework Foundation`. GLAD resolves symbols via `RTLD_DEFAULT` |
| **iOS — framework mode** (`SNAP_RHI_VULKAN_ENABLE_LAYERS=ON`) | Links `vulkan.framework` (Vulkan Loader) + `MoltenVK.framework` (ICD). Frameworks must be embedded in the app bundle's `Frameworks/` directory. **Not permitted for App Store submissions.** Sets `SNAP_RHI_IOS_VULKAN_USE_FRAMEWORKS=1` |

> iOS framework mode exports several cache variables for the consuming
> application to embed the correct frameworks:
> `SNAP_RHI_IOS_VULKAN_FRAMEWORK`, `SNAP_RHI_IOS_MOLTENVK_FRAMEWORK`,
> `SNAP_RHI_IOS_VULKAN_SDK_LIB`, `SNAP_RHI_IOS_VULKAN_SDK_SHARE`.

---

## Quick-Start Examples

### OpenGL (Desktop)

```cpp
#include <GLAD/OpenGL.h>
#include <cstdio>

// Platform-specific proc-address resolver (e.g. SDL_GL_GetProcAddress).
extern "C" void* myGetProcAddress(const char* name);

bool initOpenGL() {
    if (!glad::loadOpenGL(reinterpret_cast<GLADloadfunc>(myGetProcAddress))) {
        std::fprintf(stderr, "Failed to load OpenGL functions\n");
        return false;
    }
    return true;
}
```

Thread-safe (idempotent) variant:

```cpp
glad::loadOpenGLSafe(reinterpret_cast<GLADloadfunc>(myGetProcAddress));
```

### Vulkan

```cpp
#include <GLAD/vulkan.h>

bool initVulkan(VkInstance instance, VkPhysicalDevice physDev, VkDevice device) {
    // Built-in loader: resolves core + instance + device level commands.
    int version = gladLoaderLoadVulkan(instance, physDev, device);
    if (!version) return false;

    // Use Vulkan normally — all function pointers are now populated.
    return true;
}
```

Minimal thread-safe global init (before `VkInstance` creation):

```cpp
if (!gladLoaderInitSafe()) { /* handle failure */ }
```

Clean-up:

```cpp
gladLoaderUnloadVulkan();
```

### Enabling Validation Layers

Build with `-DSNAP_RHI_VULKAN_ENABLE_LAYERS=ON`. This defines
`VULKAN_NATIVE_VALIDATION_LAYERS=1` and (on iOS) switches to the framework
linking mode so the Vulkan Loader and Khronos Validation Layers are available at
runtime. Ensure the Vulkan SDK layer libraries are findable in your environment.

---

## Thread Safety

| Function | Thread-Safe? | Mechanism |
|----------|-------------|-----------|
| `glad::loadOpenGLSafe()` | ✅ | `std::call_once` |
| `gladLoadGLSafe()` | ✅ | `std::call_once` |
| `gladLoadGLESSafe()` | ✅ | `std::call_once` |
| `gladLoaderInitSafe()` | ✅ | `std::call_once` |
| All other loaders | ❌ | Caller must synchronise |

---

## Troubleshooting

| Symptom | Likely Cause | Resolution |
|---------|-------------|------------|
| `Failed to load OpenGL functions` | No current GL context, or the proc-address function returns `NULL` | Ensure a valid GL context is current before calling the loader; verify the function pointer resolver |
| Missing Vulkan commands at runtime | Instance or device not passed to `gladLoaderLoadVulkan` | Call `gladLoaderLoadVulkan(instance, physDev, device)` **after** creating each object |
| Validation layers not active | Built without `SNAP_RHI_VULKAN_ENABLE_LAYERS=ON`, or layer libraries not in the search path | Rebuild with `-DSNAP_RHI_VULKAN_ENABLE_LAYERS=ON`; ensure Vulkan SDK layers are findable at runtime |
| Wrong Linux GL backend | `GLAD_OPENGL_BACKEND_LINUX` mismatch | Reconfigure with `-DGLAD_OPENGL_BACKEND_LINUX=<glx\|egl\|osmesa>` |
| iOS Vulkan link errors (MoltenVK) | Vulkan SDK not installed or `VULKAN_SDK` env var not set | Install the [LunarG Vulkan SDK](https://vulkan.lunarg.com/sdk/home) and set `VULKAN_SDK` |

---

## Source Layout

```
GLAD/
├── CMakeLists.txt                          # Root project — requires CMake ≥ 3.25
├── README.md                               # This file
└── Src/
    ├── CMakeLists.txt                      # glad-platform; conditionally adds OpenGL/ and Vulkan/
    ├── gladPlatfrom.cpp                    # Platform stub (includes gladPlatform.h)
    ├── include/
    │   └── GLAD/
    │       └── gladPlatform.h              # Platform detection, calling-convention macros,
    │                                       #   GLAD_GENERATOR_VERSION "2.0.8"
    ├── OpenGL/
    │   ├── CMakeLists.txt                  # glad-gl — selects GL or GLES sub-target
    │   ├── OpenGL.cpp                      # glad::loadOpenGL / glad::loadOpenGLSafe
    │   ├── include/GLAD/
    │   │   └── OpenGL.h                    # Unified public header
    │   ├── GL/
    │   │   ├── CMakeLists.txt              # glad-platform-gl (desktop)
    │   │   └── GLAD/
    │   │       ├── gl.h                    # Generated — GL 4.6 compatibility
    │   │       ├── gl.cpp
    │   │       └── KHR/khrplatform.h
    │   └── GLES/
    │       ├── CMakeLists.txt              # glad-platform-gl-es (mobile)
    │       └── GLAD/
    │           ├── gles.h                  # Generated — GLES 3.2
    │           ├── gles.cpp
    │           ├── Android/
    │           │   ├── hardware_buffer.h
    │           │   └── hardware_buffer.cpp
    │           └── EGL/
    │               ├── egl.h
    │               └── egl.cpp
    └── Vulkan/
        ├── CMakeLists.txt                  # glad-vulkan + iOS MoltenVK / framework logic
        └── GLAD/
            ├── vulkan.h                    # Generated — Vulkan 1.4 + 405 extensions
            ├── vulkan.cpp
            ├── vk_platform.h
            └── vk_video/                   # Video codec headers (H.264, H.265, AV1)
```
