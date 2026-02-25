# Build Guide

## Prerequisites

| Tool | Minimum Version | Notes |
|------|----------------|-------|
| **CMake** | 3.25 | `brew install cmake` · `apt install cmake` · [cmake.org](https://cmake.org/download/) |
| **C++ compiler** | C++20 capable | Xcode 14+, VS 2022+, GCC 11+, Clang 14+ |

Additional per-platform requirements are listed under each platform section.

---

## Architecture at a Glance

```
┌──────────────────────────────────────────────────────────┐
│                    Your Application                      │
├──────────────────────────────────────────────────────────┤
│                 snap-rhi::public-api                     │
│           Unified C++20 API — types, interfaces          │
├──────────────────────────────────────────────────────────┤
│               snap-rhi::backend-common                   │
│        Shared utilities, validation, threading           │
├──────────┬──────────────┬──────────────┬─────────────────┤
│  Metal   │    Vulkan    │  OpenGL/ES   │   No-op (test)  │
│  .mm     │  GLAD loader │  gl-loader   │                 │
├──────────┴──────────────┴──────────────┴─────────────────┤
│               snap-rhi::interop (optional)               │
│         Cross-backend resource sharing & bridging        │
└──────────────────────────────────────────────────────────┘
```

All targets are **static libraries**. The Vulkan backend loads all functions at
runtime through a bundled GLAD loader — **no Vulkan SDK linkage at build time**.

---

## Quick Start

Every build follows the same two-step pattern — choose a **preset** and build:

```bash
# Library only (no demo app)
cmake --preset macos-metal && cmake --build build/macos-metal

# Library + demo app
cmake --preset macos-metal-demo && cmake --build build/macos-metal-demo
```

Or use the shell helper (macOS / Linux):

```bash
./build.sh                       # Library only — Metal on macOS
./build.sh --with-demo           # Library + demo app
./build.sh --vulkan --with-demo  # Vulkan demo
```

> Run `cmake --list-presets` to see every available preset.

---

## Two Build Targets: Library vs Demo App

SnapRHI can be built as a **standalone library** or with the **triangle demo application**.

### Library Only

Builds the static libraries without the demo app. Use this when integrating SnapRHI into your own project.

| Method | Command |
|--------|---------|
| **Preset** | `cmake --preset macos-metal` then `cmake --build build/macos-metal` |
| **Raw CMake** | `cmake -B build -DSNAP_RHI_ENABLE_METAL=ON` then `cmake --build build` |
| **build.sh** | `./build.sh` (macOS/Linux only) |

### Library + Demo App

Builds everything plus the triangle demo application with SDL3 windowing.

| Method | Command |
|--------|---------|
| **Preset** | `cmake --preset macos-metal-demo` then `cmake --build build/macos-metal-demo` |
| **Raw CMake** | `cmake -B build -DSNAP_RHI_ENABLE_METAL=ON -DSNAP_RHI_BUILD_EXAMPLES=ON -DSNAP_RHI_DEMO_APP_API=metal` then `cmake --build build` |
| **build.sh** | `./build.sh --with-demo` (macOS/Linux only) |

> **Naming convention:** presets ending in `-demo` include the demo app. Presets
> without `-demo` build the library only.

---

## Platform × Backend × IDE Coverage Matrix

| Platform | Metal | Vulkan | OpenGL/ES | Available IDEs | Build Method |
|----------|:-----:|:------:|:---------:|----------------|-------------|
| **macOS** | ✅ | ✅ (MoltenVK) | ✅ | VS Code, CLion, Xcode | CMake presets, build.sh |
| **iOS** | ✅ | ✅ (MoltenVK) | ✅ ES | Xcode | CMake → Xcode presets |
| **Windows** | — | ✅ | ✅ | VS Code, CLion, Visual Studio | CMake presets |
| **Linux** | — | ✅ | ✅ | VS Code, CLion | CMake presets, build.sh |
| **Android** | — | ✅ | ✅ ES | Android Studio | Gradle + CMake |
| **WebAssembly** | — | — | ⚠️ backend code only | — | Not yet wired |

> **WebAssembly:** The OpenGL ES backend contains Emscripten-aware code paths,
> but the build system and demo app do not yet support `emcmake`. This is a
> future capability — the backend is ready, the toolchain integration is not.

---

## Platform Build Instructions

### macOS

**Prerequisites:** Xcode 14+ or Xcode Command Line Tools (`xcode-select --install`).

#### Metal (recommended)

**Library Only**

| Method | Command |
|--------|---------|
| Preset | `cmake --preset macos-metal && cmake --build build/macos-metal` |
| Raw CMake | `cmake -B build -DSNAP_RHI_ENABLE_METAL=ON && cmake --build build` |
| build.sh | `./build.sh` |

**Library + Demo App**

| Method | Command |
|--------|---------|
| Preset | `cmake --preset macos-metal-demo && cmake --build build/macos-metal-demo` |
| Raw CMake | `cmake -B build -DSNAP_RHI_ENABLE_METAL=ON -DSNAP_RHI_BUILD_EXAMPLES=ON -DSNAP_RHI_DEMO_APP_API=metal && cmake --build build` |
| build.sh | `./build.sh --with-demo` |

Run the demo:
```bash
open build/macos-metal-demo/examples/triangle-demo-app/snap_rhi_demo_app.app
```

#### Vulkan (via MoltenVK)

Vulkan on macOS runs through **MoltenVK** (a Vulkan-to-Metal translation layer) and the
**Vulkan Loader** (`libvulkan.1.dylib`). Both must be installed and discoverable at runtime.

**1 — Install dependencies**

```bash
brew install vulkan-loader molten-vk
```

This installs:
- `vulkan-loader` — the Vulkan Loader (`libvulkan.1.dylib`) at `/opt/homebrew/lib/`
- `molten-vk` — the MoltenVK ICD driver with its manifest at `/opt/homebrew/etc/vulkan/icd.d/`

**2 — Set environment variables** (add to `~/.zshrc`):

```bash
# Required: Let dlopen find the Vulkan Loader
export DYLD_LIBRARY_PATH="/opt/homebrew/lib${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}"

# Required: Tell the Vulkan Loader where to find the MoltenVK ICD driver
export VK_ICD_FILENAMES="/opt/homebrew/etc/vulkan/icd.d/MoltenVK_icd.json"
export VK_DRIVER_FILES="/opt/homebrew/etc/vulkan/icd.d/MoltenVK_icd.json"
```

> **Intel Mac:** Replace `/opt/homebrew/` with `/usr/local/` in all paths above.

> **LunarG Vulkan SDK** (alternative to Homebrew):
> Download from [vulkan.lunarg.com](https://vulkan.lunarg.com/sdk/home), then:
> ```bash
> source ~/VulkanSDK/<version>/setup-env.sh
> ```
> This sets all required environment variables automatically.

**3 — Verify**

```bash
vulkaninfo --summary   # Should show MoltenVK and your GPU
```

**4 — Build**

**Library Only**

| Method | Command |
|--------|---------|
| Preset | `cmake --preset macos-vulkan && cmake --build build/macos-vulkan` |
| Raw CMake | `cmake -B build -DSNAP_RHI_ENABLE_VULKAN=ON && cmake --build build` |
| build.sh | `./build.sh --vulkan` |

**Library + Demo App**

| Method | Command |
|--------|---------|
| Preset | `cmake --preset macos-vulkan-demo && cmake --build build/macos-vulkan-demo` |
| Raw CMake | `cmake -B build -DSNAP_RHI_ENABLE_VULKAN=ON -DSNAP_RHI_BUILD_EXAMPLES=ON -DSNAP_RHI_DEMO_APP_API=vulkan && cmake --build build` |
| build.sh | `./build.sh --vulkan --with-demo` |

Run the demo:
```bash
./build/macos-vulkan-demo/examples/triangle-demo-app/snap_rhi_demo_app.app/Contents/MacOS/snap_rhi_demo_app
```

**5 — IDE Environment Variables for Vulkan on macOS**

IDEs launch apps without sourcing `~/.zshrc`, so you must add the environment variables
to your run configuration manually.

**CLion:**
1. **Run → Edit Configurations…**
2. Select `snap_rhi_demo_app`
3. In **Environment variables**, click **…** and add:

   | Name | Value |
   |------|-------|
   | `DYLD_LIBRARY_PATH` | `/opt/homebrew/lib` |
   | `VK_ICD_FILENAMES` | `/opt/homebrew/etc/vulkan/icd.d/MoltenVK_icd.json` |
   | `VK_DRIVER_FILES` | `/opt/homebrew/etc/vulkan/icd.d/MoltenVK_icd.json` |

**Xcode:**
1. **Product → Scheme → Edit Scheme…**
2. Select **Run** → **Arguments** tab
3. Under **Environment Variables**, add the same three variables above

**VS Code** (`.vscode/launch.json`):
```json
{
    "type": "lldb",
    "request": "launch",
    "name": "SnapRHI Vulkan Demo",
    "program": "${workspaceFolder}/build/macos-vulkan-demo/examples/triangle-demo-app/snap_rhi_demo_app.app/Contents/MacOS/snap_rhi_demo_app",
    "env": {
        "DYLD_LIBRARY_PATH": "/opt/homebrew/lib",
        "VK_ICD_FILENAMES": "/opt/homebrew/etc/vulkan/icd.d/MoltenVK_icd.json",
        "VK_DRIVER_FILES": "/opt/homebrew/etc/vulkan/icd.d/MoltenVK_icd.json"
    }
}
```

> **Why all three variables?**
> - `DYLD_LIBRARY_PATH` — so `dlopen("libvulkan.1.dylib")` finds the Homebrew-installed Vulkan Loader
> - `VK_ICD_FILENAMES` / `VK_DRIVER_FILES` — so the Loader discovers the MoltenVK ICD driver
>
> Without these, the demo app will crash with `"Failed to load Vulkan Portability library"` or
> `"Installed Vulkan Portability library doesn't implement the VK_KHR_surface extension"`.

#### OpenGL

> macOS caps OpenGL at 4.1 and Apple has deprecated it. Prefer Metal for
> production on Apple platforms.

**Library Only**

| Method | Command |
|--------|---------|
| Preset | `cmake --preset macos-opengl && cmake --build build/macos-opengl` |
| Raw CMake | `cmake -B build -DSNAP_RHI_ENABLE_OPENGL=ON && cmake --build build` |
| build.sh | `./build.sh --opengl` |

**Library + Demo App**

| Method | Command |
|--------|---------|
| Preset | `cmake --preset macos-opengl-demo && cmake --build build/macos-opengl-demo` |
| Raw CMake | `cmake -B build -DSNAP_RHI_ENABLE_OPENGL=ON -DSNAP_RHI_BUILD_EXAMPLES=ON -DSNAP_RHI_DEMO_APP_API=opengl && cmake --build build` |
| build.sh | `./build.sh --opengl --with-demo` |

Run the demo:
```bash
open build/macos-opengl-demo/examples/triangle-demo-app/snap_rhi_demo_app.app
```

#### Xcode Project (macOS)

```bash
cmake --preset macos-xcode-metal-demo       # Metal
cmake --preset macos-xcode-vulkan-demo      # Vulkan (MoltenVK)
cmake --preset macos-xcode-opengl-demo      # OpenGL
cmake --build --preset macos-xcode-metal-demo --config Debug
open build/macos-xcode-metal-demo/snap-rhi.xcodeproj
```

Or generate with raw CMake:
```bash
cmake -B build/macos-xcode-metal -G Xcode -DSNAP_RHI_ENABLE_METAL=ON -DSNAP_RHI_BUILD_EXAMPLES=ON -DSNAP_RHI_DEMO_APP_API=metal
open build/macos-xcode-metal/snap-rhi.xcodeproj
```

Select the `snap_rhi_demo_app` scheme → **⌘B** to build → **⌘R** to run.

> For Vulkan on macOS, you must add the environment variables to the Xcode scheme.
> See [IDE Environment Variables for Vulkan on macOS](#5--ide-environment-variables-for-vulkan-on-macos) above.

#### Release Builds (macOS)

| Method | Command |
|--------|---------|
| Preset | `cmake --preset macos-metal-release && cmake --build build/macos-metal-release` |
| Raw CMake | `cmake -B build -DCMAKE_BUILD_TYPE=Release -DSNAP_RHI_ENABLE_METAL=ON && cmake --build build` |
| build.sh | `./build.sh --release` (add `--vulkan` or `--opengl` for other backends) |

#### Validation Builds (macOS)

| Method | Command |
|--------|---------|
| Preset (Metal) | `cmake --preset macos-metal-demo-validation` |
| Preset (Vulkan) | `cmake --preset macos-vulkan-demo-validation` || Preset (OpenGL) | `cmake --preset macos-opengl-demo-validation` || Raw CMake | `cmake -B build -DSNAP_RHI_ENABLE_VULKAN=ON -DSNAP_RHI_ENABLE_ALL_VALIDATION=ON -DSNAP_RHI_BUILD_EXAMPLES=ON -DSNAP_RHI_DEMO_APP_API=vulkan && cmake --build build` |
| build.sh | `./build.sh --vulkan --with-demo --validation` |

---

### Windows

**Prerequisites:** Visual Studio 2022+ (or Build Tools for Visual Studio).
For Vulkan, install the [Vulkan SDK](https://vulkan.lunarg.com/).

#### Vulkan

**Library Only**

| Method | Command |
|--------|---------|
| Preset | `cmake --preset windows-vulkan` then `cmake --build build/windows-vulkan --config Debug` |
| Raw CMake | `cmake -B build -DSNAP_RHI_ENABLE_VULKAN=ON` then `cmake --build build --config Debug` |

**Library + Demo App**

| Method | Command |
|--------|---------|
| Preset | `cmake --preset windows-vulkan-demo` then `cmake --build build/windows-vulkan-demo --config Debug` |
| Raw CMake | `cmake -B build -DSNAP_RHI_ENABLE_VULKAN=ON -DSNAP_RHI_BUILD_EXAMPLES=ON -DSNAP_RHI_DEMO_APP_API=vulkan` then `cmake --build build --config Debug` |

Run the demo:
```powershell
.\build\windows-vulkan-demo\examples\triangle-demo-app\Debug\snap_rhi_demo_app.exe
```

#### OpenGL

**Library Only**

| Method | Command |
|--------|---------|
| Preset | `cmake --preset windows-opengl` then `cmake --build build/windows-opengl --config Debug` |
| Raw CMake | `cmake -B build -DSNAP_RHI_ENABLE_OPENGL=ON` then `cmake --build build --config Debug` |

**Library + Demo App**

| Method | Command |
|--------|---------|
| Preset | `cmake --preset windows-opengl-demo` then `cmake --build build/windows-opengl-demo --config Debug` |
| Raw CMake | `cmake -B build -DSNAP_RHI_ENABLE_OPENGL=ON -DSNAP_RHI_BUILD_EXAMPLES=ON -DSNAP_RHI_DEMO_APP_API=opengl` then `cmake --build build --config Debug` |

Run the demo:
```powershell
.\build\windows-opengl-demo\examples\triangle-demo-app\Debug\snap_rhi_demo_app.exe
```

#### Visual Studio Solution

```powershell
# Vulkan
cmake --preset windows-vs-vulkan-demo
cmake --build --preset windows-vs-vulkan-demo --config Debug
start build\windows-vs-vulkan-demo\snap-rhi.sln

# OpenGL
cmake --preset windows-vs-opengl-demo
cmake --build --preset windows-vs-opengl-demo --config Debug
start build\windows-vs-opengl-demo\snap-rhi.sln
```

Or generate with raw CMake:
```powershell
cmake -B build/windows-vs-vulkan -G "Visual Studio 17 2022" -DSNAP_RHI_ENABLE_VULKAN=ON -DSNAP_RHI_BUILD_EXAMPLES=ON -DSNAP_RHI_DEMO_APP_API=vulkan
start build\windows-vs-vulkan\snap-rhi.sln
```

Right-click `snap_rhi_demo_app` → **Set as Startup Project** → **F5**.

#### Validation Builds (Windows)

| Method | Command |
|--------|---------|
| Preset (Vulkan) | `cmake --preset windows-vulkan-demo-validation` |
| Preset (OpenGL) | `cmake --preset windows-opengl-demo-validation` |
| Raw CMake | `cmake -B build -DSNAP_RHI_ENABLE_VULKAN=ON -DSNAP_RHI_ENABLE_ALL_VALIDATION=ON -DSNAP_RHI_BUILD_EXAMPLES=ON -DSNAP_RHI_DEMO_APP_API=vulkan` |

> Vulkan validation layers work automatically on Windows when the
> [Vulkan SDK](https://vulkan.lunarg.com/) is installed.

---

### Linux

**Prerequisites:**

```bash
# Ubuntu / Debian
sudo apt install build-essential cmake

# Vulkan
sudo apt install libvulkan-dev vulkan-tools

# OpenGL
sudo apt install libgl1-mesa-dev libx11-dev
```

#### Vulkan

**Library Only**

| Method | Command |
|--------|---------|
| Preset | `cmake --preset linux-vulkan && cmake --build build/linux-vulkan` |
| Raw CMake | `cmake -B build -DSNAP_RHI_ENABLE_VULKAN=ON && cmake --build build` |
| build.sh | `./build.sh --vulkan` |

**Library + Demo App**

| Method | Command |
|--------|---------|
| Preset | `cmake --preset linux-vulkan-demo && cmake --build build/linux-vulkan-demo` |
| Raw CMake | `cmake -B build -DSNAP_RHI_ENABLE_VULKAN=ON -DSNAP_RHI_BUILD_EXAMPLES=ON -DSNAP_RHI_DEMO_APP_API=vulkan && cmake --build build` |
| build.sh | `./build.sh --vulkan --with-demo` |

Run the demo:
```bash
./build/linux-vulkan-demo/examples/triangle-demo-app/snap_rhi_demo_app
```

#### OpenGL

**Library Only**

| Method | Command |
|--------|---------|
| Preset | `cmake --preset linux-opengl && cmake --build build/linux-opengl` |
| Raw CMake | `cmake -B build -DSNAP_RHI_ENABLE_OPENGL=ON && cmake --build build` |
| build.sh | `./build.sh --opengl` |

**Library + Demo App**

| Method | Command |
|--------|---------|
| Preset | `cmake --preset linux-opengl-demo && cmake --build build/linux-opengl-demo` |
| Raw CMake | `cmake -B build -DSNAP_RHI_ENABLE_OPENGL=ON -DSNAP_RHI_BUILD_EXAMPLES=ON -DSNAP_RHI_DEMO_APP_API=opengl && cmake --build build` |
| build.sh | `./build.sh --opengl --with-demo` |

Run the demo:
```bash
./build/linux-opengl-demo/examples/triangle-demo-app/snap_rhi_demo_app
```

#### Validation Builds (Linux)

| Method | Command |
|--------|---------|
| Preset (Vulkan) | `cmake --preset linux-vulkan-demo-validation` |
| Preset (OpenGL) | `cmake --preset linux-opengl-demo-validation` |
| Raw CMake | `cmake -B build -DSNAP_RHI_ENABLE_VULKAN=ON -DSNAP_RHI_ENABLE_ALL_VALIDATION=ON -DSNAP_RHI_BUILD_EXAMPLES=ON -DSNAP_RHI_DEMO_APP_API=vulkan && cmake --build build` |
| build.sh | `./build.sh --vulkan --with-demo --validation` |

---

### iOS

**Prerequisites:** Xcode 14+, Apple Developer account (for device deployment).

iOS builds **require Xcode** — use the Xcode presets to generate `.xcodeproj` files,
then build and run within Xcode.

#### Metal (recommended)

| Method | Command |
|--------|---------|
| Preset | `cmake --preset ios-xcode-metal-demo` then `open build/ios-xcode-metal-demo/snap-rhi.xcodeproj` |
| Raw CMake | `cmake -B build/ios-metal -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DSNAP_RHI_ENABLE_METAL=ON -DSNAP_RHI_BUILD_EXAMPLES=ON -DSNAP_RHI_DEMO_APP_API=metal` then `open build/ios-metal/snap-rhi.xcodeproj` |

#### Vulkan (via MoltenVK)

| Method | Command |
|--------|---------|
| Preset | `cmake --preset ios-xcode-vulkan-demo` then `open build/ios-xcode-vulkan-demo/snap-rhi.xcodeproj` |
| Raw CMake | `cmake -B build/ios-vulkan -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DSNAP_RHI_ENABLE_VULKAN=ON -DSNAP_RHI_BUILD_EXAMPLES=ON -DSNAP_RHI_DEMO_APP_API=vulkan` then `open build/ios-vulkan/snap-rhi.xcodeproj` |

#### OpenGL ES

| Method | Command |
|--------|---------|
| Preset | `cmake --preset ios-xcode-opengl-demo` then `open build/ios-xcode-opengl-demo/snap-rhi.xcodeproj` |
| Raw CMake | `cmake -B build/ios-opengl -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DSNAP_RHI_ENABLE_OPENGL=ON -DSNAP_RHI_BUILD_EXAMPLES=ON -DSNAP_RHI_DEMO_APP_API=opengl` then `open build/ios-opengl/snap-rhi.xcodeproj` |

**In Xcode (all backends):**

1. Select the `snap_rhi_demo_app` scheme.
2. Configure signing under **Signing & Capabilities** (set your Team).
3. Choose a simulator or device → **⌘R** to run.

> **Library only on iOS:** remove `-DSNAP_RHI_BUILD_EXAMPLES=ON` and
> `-DSNAP_RHI_DEMO_APP_API=...` from the raw CMake command. The Xcode presets
> always include the demo app.

---

### Android

**Prerequisites:** Android Studio, NDK r28+.

Android builds use **Gradle**, which invokes CMake internally. The build system
does not use CMakePresets.json — the CMake arguments are set in `app/build.gradle`
via **product flavors**.

#### Android Studio (recommended)

1. **File → Open** → `examples/triangle-demo-app/platforms/android`
2. Wait for Gradle sync to complete.
3. **Build → Select Build Variant** → choose `vulkanDebug` or `openglDebug`.
4. **Run → Run 'app'** (`Shift+F10`).

| Build variant   | Backend     | Application ID              |
|-----------------|-------------|-----------------------------|
| `vulkanDebug`   | Vulkan      | `com.snap.rhi.demo.vulkan`  |
| `vulkanRelease` | Vulkan      | `com.snap.rhi.demo.vulkan`  |
| `openglDebug`   | OpenGL ES   | `com.snap.rhi.demo.opengl`  |
| `openglRelease` | OpenGL ES   | `com.snap.rhi.demo.opengl`  |

#### Command Line

```bash
cd examples/triangle-demo-app/platforms/android

# Vulkan backend
./gradlew assembleVulkanDebug
./gradlew installVulkanDebug

# OpenGL ES backend
./gradlew assembleOpenglDebug
./gradlew installOpenglDebug

adb shell am start -n com.snap.rhi.demo.vulkan/.MainActivity
```

> **NDK not found?** Open **Tools → SDK Manager → SDK Tools** and install
> **NDK (Side by side)**.

---

### WebAssembly

> **Status:** The OpenGL ES backend contains Emscripten-aware code paths, but
> the build system and demo app do not yet support `emcmake`. The backend code
> is ready; the toolchain integration is not yet wired up. This section will be
> updated when WebAssembly builds are supported.

---

## IDE Setup

Every IDE in this section integrates through `CMakePresets.json` — no manual
cache-variable juggling. Run `cmake --list-presets` to see all options.

> **Code style:** The repository includes an `.editorconfig` file that all major
> IDEs respect. No per-IDE formatting settings are needed.

---

### VS Code

The repository ships a ready-to-use `.vscode/` directory with settings, tasks,
launch configurations, and extension recommendations.

**Supported platforms:** macOS, Linux, Windows.

**Available presets per platform:**

| Platform | Presets |
|----------|---------|
| macOS | `macos-metal`, `macos-metal-demo`, `macos-vulkan`, `macos-vulkan-demo`, `macos-opengl`, `macos-opengl-demo`, + validation |
| Linux | `linux-vulkan`, `linux-vulkan-demo`, `linux-opengl`, `linux-opengl-demo`, + validation |
| Windows | `windows-vulkan`, `windows-vulkan-demo`, `windows-opengl`, `windows-opengl-demo`, + validation |

**1 — Install recommended extensions**

Open the workspace → VS Code will prompt to install recommended extensions.
Or manually: `Ctrl+Shift+P` → **Extensions: Show Recommended Extensions**.

Required:
- **CMake Tools** (`ms-vscode.cmake-tools`)
- **C/C++** (`ms-vscode.cpptools`)
- **CodeLLDB** (`vadimcn.vscode-lldb`) — macOS / Linux debugger

**2 — Select a preset**

`Ctrl+Shift+P` → **CMake: Select Configure Preset** → choose one
(e.g., `macos-metal-demo` for demo app, or `macos-metal` for library only).

**3 — Build**

Press **F7** or use `Ctrl+Shift+P` → **CMake: Build**.

**4 — Debug**

Press **F5** — the dropdown shows launch configs for every platform/backend
combination. Each config automatically builds its matching preset first.

**Vulkan on macOS:** the launch configs include `VK_ICD_FILENAMES` pre-set
for Homebrew on Apple Silicon. If your setup differs, edit
`.vscode/launch.json` → update the `env` block.

---

### CLion

CLion natively reads `CMakePresets.json` (2021.2+).

**Supported platforms:** macOS, Linux, Windows.

**Available presets per platform:**

| Platform | Presets |
|----------|---------|
| macOS | `macos-metal`, `macos-metal-demo`, `macos-vulkan`, `macos-vulkan-demo`, `macos-opengl`, `macos-opengl-demo`, + validation |
| Linux | `linux-vulkan`, `linux-vulkan-demo`, `linux-opengl`, `linux-opengl-demo`, + validation |
| Windows | `windows-vulkan`, `windows-vulkan-demo`, `windows-opengl`, `windows-opengl-demo`, + validation |

**1 — Open project**

**File → Open** → SnapRHI root directory.

**2 — Select a preset**

CLion detects presets automatically. Choose one from the **CMake Presets**
dropdown in the toolbar (e.g., `macos-metal-demo` for demo app, or `macos-metal`
for library only).

**3 — Build**

**Build → Build Project** (`⌘F9` / `Ctrl+F9`).

**4 — Run / Debug**

Select `snap_rhi_demo_app` in the run configuration dropdown → **Run** (`⌘R` /
`Shift+F10`) or **Debug** (`⌘D` / `Shift+F9`).

**Vulkan on macOS:** add environment variables to the run configuration:
**Run → Edit Configurations → Environment variables** →
```
VK_ICD_FILENAMES=/opt/homebrew/share/vulkan/icd.d/MoltenVK_icd.json
```

---

### Xcode

Dedicated Xcode presets generate `.xcodeproj` files with all CMake options
pre-configured.

**Supported platforms:** macOS, iOS.

**Available presets:**

| Preset | Platform | Backend |
|--------|----------|---------|
| `macos-xcode-metal-demo` | macOS | Metal |
| `macos-xcode-vulkan-demo` | macOS | Vulkan (MoltenVK) |
| `macos-xcode-opengl-demo` | macOS | OpenGL |
| `ios-xcode-metal-demo` | iOS | Metal |
| `ios-xcode-vulkan-demo` | iOS | Vulkan (MoltenVK) |
| `ios-xcode-opengl-demo` | iOS | OpenGL ES |

**1 — Generate the project**

```bash
cmake --preset macos-xcode-metal-demo   # or any preset above
```

**2 — Open in Xcode**

```bash
open build/macos-xcode-metal-demo/snap-rhi.xcodeproj
```

**3 — Build & Run**

Select the `snap_rhi_demo_app` scheme → **⌘B** to build → **⌘R** to run.

**Vulkan on macOS:** edit the scheme (**Product → Scheme → Edit Scheme →
Run → Arguments → Environment Variables**):

| Variable | Value |
|----------|-------|
| `VK_ICD_FILENAMES` | `/opt/homebrew/share/vulkan/icd.d/MoltenVK_icd.json` |

**iOS:** select a simulator or device in the scheme → configure **Signing &
Capabilities** → **⌘R**.

**GPU Debugging:** enable Metal validation in Xcode via **Product → Scheme →
Edit Scheme → Run → Diagnostics → Metal Validation**.

> **Library only with Xcode:** the Xcode presets always include the demo app. To
> build library only, use the raw CMake Xcode generator command without
> `-DSNAP_RHI_BUILD_EXAMPLES=ON`:
> ```bash
> cmake -B build/xcode-lib -G Xcode -DSNAP_RHI_ENABLE_METAL=ON
> open build/xcode-lib/snap-rhi.xcodeproj
> ```

---

### Visual Studio

Visual Studio 2022+ supports CMake presets natively.

**Supported platforms:** Windows.

**Available presets:**

| Preset | Backend | Notes |
|--------|---------|-------|
| `windows-vulkan` / `windows-vulkan-demo` | Vulkan | Library / Library + Demo |
| `windows-opengl` / `windows-opengl-demo` | OpenGL | Library / Library + Demo |
| `windows-vs-vulkan-demo` | Vulkan | Generates `.sln` with demo |
| `windows-vs-opengl-demo` | OpenGL | Generates `.sln` with demo |

**Option A — Open as CMake project (recommended)**

1. **File → Open → Folder** → SnapRHI root.
2. Visual Studio detects `CMakePresets.json` — select a preset from the
   toolbar dropdown (e.g., `windows-vulkan-demo`).
3. Right-click `snap_rhi_demo_app` in the **Solution Explorer** →
   **Set as Startup Item**.
4. **F5** to debug.

**Option B — Generate a solution**

```powershell
cmake --preset windows-vs-vulkan-demo    # or windows-vs-opengl-demo
start build\windows-vs-vulkan-demo\snap-rhi.sln
```

Right-click `snap_rhi_demo_app` → **Set as Startup Project** → **F5**.

**Vulkan validation layers:** ensure the [Vulkan SDK](https://vulkan.lunarg.com/)
is installed. Validation layers work automatically on Windows via the SDK's
implicit layer mechanism.

---

### Android Studio

**Supported platforms:** Android.

**1 — Open the project**

**File → Open** → `examples/triangle-demo-app/platforms/android`

**2 — Wait for Gradle sync**

Android Studio will download dependencies and configure the NDK.

**3 — Select the build variant**

**Build → Select Build Variant** → choose `vulkanDebug`, `vulkanRelease`,
`openglDebug`, or `openglRelease`. Each variant passes the correct CMake
arguments automatically via product flavors.

**4 — Run**

**Run → Run 'app'** (`Shift+F10`) or **Debug 'app'** (`Shift+F9`).

**NDK not found?** Open **Tools → SDK Manager → SDK Tools** and install
**NDK (Side by side)**.

---

### IDE Feature Matrix

| Feature | VS Code | CLion | Xcode | Visual Studio | Android Studio |
|---------|:-------:|:-----:|:-----:|:-------------:|:--------------:|
| CMake presets | ✓ | ✓ | ✓ (via preset) | ✓ | Gradle/CMake |
| IntelliSense / code completion | ✓ | ✓ | ✓ | ✓ | ✓ |
| One-click build | F7 | ⌘F9 | ⌘B | F7 | Shift+F10 |
| Integrated debugger | F5 | ⌘D | ⌘R | F5 | Shift+F9 |
| GPU frame capture | — | — | ✓ | PIX | AGI |
| Metal shader debugging | — | — | ✓ | — | — |
| Vulkan validation layers | env var | env var | env var | automatic | logcat |
| `.editorconfig` support | ✓ | ✓ | plugin | ✓ | ✓ |

---

## CMake Presets Reference

All presets live in `CMakePresets.json`. Build output goes to `build/<preset-name>/`.

### Desktop Presets (Makefile / Ninja)

| Preset | Platform | Backend | Demo | Validation | Build Type |
|--------|----------|---------|:----:|:----------:|:----------:|
| `macos-metal` | macOS | Metal | | | Debug |
| `macos-metal-release` | macOS | Metal | | | Release |
| `macos-metal-demo` | macOS | Metal | ✓ | | Debug |
| `macos-metal-demo-validation` | macOS | Metal | ✓ | ✓ | Debug |
| `macos-vulkan` | macOS | Vulkan | | | Debug |
| `macos-vulkan-demo` | macOS | Vulkan | ✓ | | Debug |
| `macos-vulkan-demo-validation` | macOS | Vulkan | ✓ | ✓ | Debug |
| `macos-opengl` | macOS | OpenGL | | | Debug |
| `macos-opengl-demo` | macOS | OpenGL | ✓ | | Debug |
| `macos-opengl-demo-validation` | macOS | OpenGL | ✓ | ✓ | Debug |
| `linux-vulkan` | Linux | Vulkan | | | Debug |
| `linux-vulkan-demo` | Linux | Vulkan | ✓ | | Debug |
| `linux-vulkan-demo-validation` | Linux | Vulkan | ✓ | ✓ | Debug |
| `linux-opengl` | Linux | OpenGL | | | Debug |
| `linux-opengl-demo` | Linux | OpenGL | ✓ | | Debug |
| `linux-opengl-demo-validation` | Linux | OpenGL | ✓ | ✓ | Debug |
| `windows-vulkan` | Windows | Vulkan | | | Debug |
| `windows-vulkan-demo` | Windows | Vulkan | ✓ | | Debug |
| `windows-vulkan-demo-validation` | Windows | Vulkan | ✓ | ✓ | Debug |
| `windows-opengl` | Windows | OpenGL | | | Debug |
| `windows-opengl-demo` | Windows | OpenGL | ✓ | | Debug |
| `windows-opengl-demo-validation` | Windows | OpenGL | ✓ | ✓ | Debug |

### Xcode Presets (generates `.xcodeproj`)

| Preset | Platform | Backend |
|--------|----------|---------|
| `macos-xcode-metal-demo` | macOS | Metal |
| `macos-xcode-vulkan-demo` | macOS | Vulkan (MoltenVK) |
| `macos-xcode-opengl-demo` | macOS | OpenGL |
| `ios-xcode-metal-demo` | iOS | Metal |
| `ios-xcode-vulkan-demo` | iOS | Vulkan (MoltenVK) |
| `ios-xcode-opengl-demo` | iOS | OpenGL ES |

> All Xcode presets include the demo app. Build with `cmake --build --preset <name> --config Debug` or build within Xcode.

### Visual Studio Presets (generates `.sln`)

| Preset | Backend |
|--------|---------|
| `windows-vs-vulkan-demo` | Vulkan |
| `windows-vs-opengl-demo` | OpenGL |

> Both include the demo app. Build with `cmake --build --preset <name> --config Debug` or build within Visual Studio.

---

## CMake Options Reference

### Backend & Build

| Option | Default | Description |
|--------|---------|-------------|
| `SNAP_RHI_ENABLE_METAL` | `OFF` | Metal backend (Apple only) |
| `SNAP_RHI_ENABLE_VULKAN` | `OFF` | Vulkan backend (all platforms) |
| `SNAP_RHI_ENABLE_OPENGL` | `OFF` | OpenGL / OpenGL ES backend |
| `SNAP_RHI_ENABLE_NOOP` | `OFF` | No-op stub backend (testing) |
| `SNAP_RHI_BUILD_EXAMPLES` | `OFF` | Build the demo application |
| `SNAP_RHI_DEMO_APP_API` | `metal` | Demo backend: `metal` · `vulkan` · `opengl` |
| `SNAP_RHI_WITH_INTEROP` | `OFF` | Build cross-backend interop module |
| `SNAP_RHI_WARNINGS_AS_ERRORS` | `OFF` | Promote compiler warnings to errors |

### Diagnostics & Validation

| Option | Default | Description |
|--------|---------|-------------|
| `SNAP_RHI_ENABLE_ALL_VALIDATION` | `OFF` | **Master switch** — enables every option below |
| `SNAP_RHI_ENABLE_DEBUG_LABELS` | `ON` | GPU debug markers and labels |
| `SNAP_RHI_ENABLE_LOGS` | `ON` | Runtime log output |
| `SNAP_RHI_REPORT_LEVEL` | `warning` | `all` · `debug` · `info` · `warning` · `performance_warning` · `error` · `critical_error` |
| `SNAP_RHI_VULKAN_ENABLE_LAYERS` | `OFF` | Vulkan validation layers (`VK_LAYER_KHRONOS_validation`) |
| `SNAP_RHI_ENABLE_SLOW_SAFETY_CHECKS` | `OFF` | Extra runtime assertions (perf impact) |
| `SNAP_RHI_ENABLE_SOURCE_LOCATION` | `OFF` | Track source locations in API calls |
| `SNAP_RHI_ENABLE_API_DUMP` | `OFF` | Log every API call |
| `SNAP_RHI_THROW_ON_LIFETIME_VIOLATION` | `OFF` | Throw on use-after-destroy |

> **Recommended for development:** use `SNAP_RHI_ENABLE_ALL_VALIDATION=ON` (or a
> `-validation` preset). It activates validation layers, all per-operation
> validation tags, slow checks, full logging, and debug labels in one switch.

### Per-Operation Validation Tags

Enabled automatically by `SNAP_RHI_ENABLE_ALL_VALIDATION`. For fine-grained
control, toggle individually (all default to `OFF`):

`SNAP_RHI_VALIDATION_CREATE_OP` · `DESTROY_OP` · `DEVICE_CONTEXT_OP` ·
`COMMAND_QUEUE_OP` · `COMMAND_BUFFER_OP` · `RENDER_COMMAND_ENCODER_OP` ·
`COMPUTE_COMMAND_ENCODER_OP` · `BLIT_COMMAND_ENCODER_OP` · `RENDER_PASS_OP` ·
`FRAMEBUFFER_OP` · `RENDER_PIPELINE_OP` · `COMPUTE_PIPELINE_OP` ·
`SHADER_MODULE_OP` · `SHADER_LIBRARY_OP` · `SAMPLER_OP` · `TEXTURE_OP` ·
`BUFFER_OP` · `DESCRIPTOR_SET_LAYOUT_OP` · `PIPELINE_LAYOUT_OP` ·
`DESCRIPTOR_POOL_OP` · `DESCRIPTOR_SET_OP` · `FENCE_OP` · `SEMAPHORE_OP` ·
`DEVICE_OP` · `PIPELINE_CACHE_OP` · `QUERY_POOL_OP`

OpenGL-specific:
`GL_COMMAND_QUEUE_EXTERNAL_ERROR` · `GL_COMMAND_QUEUE_INTERNAL_ERROR` ·
`GL_STATE_CACHE_OP` · `GL_PROGRAM_VALIDATION_OP` · `GL_PROFILE_OP`

---

## Build Script (`build.sh`)

A convenience wrapper for macOS and Linux:

```
Usage: ./build.sh [options]

Backend:
  --metal              Metal backend (macOS default)
  --vulkan             Vulkan backend
  --opengl             OpenGL backend

Build:
  --with-demo          Include demo application
  --release            Release build (default: Debug)
  --clean              Delete build directory first
  --xcode              Generate Xcode project (no build)

Validation:
  --validation         Enable ALL validation features
  --validation-layers  Enable Vulkan validation layers only

  --help               Show usage
```

**Examples:**

```bash
# Library only
./build.sh                                # Metal (macOS default)
./build.sh --vulkan                       # Vulkan library
./build.sh --opengl                       # OpenGL library

# Library + demo app
./build.sh --with-demo                    # Metal demo (macOS)
./build.sh --vulkan --with-demo           # Vulkan demo
./build.sh --opengl --with-demo           # OpenGL demo

# Release
./build.sh --release                      # Metal release library
./build.sh --vulkan --release --with-demo # Vulkan release demo

# Validation
./build.sh --vulkan --with-demo --validation       # Full validation
./build.sh --vulkan --with-demo --validation-layers # Layers only

# Xcode
./build.sh --xcode --with-demo           # Metal Xcode project
./build.sh --xcode --vulkan --with-demo  # Vulkan Xcode project
```

> `build.sh` is not available on Windows. Use CMake presets or Visual Studio.

---

## Code Formatting

SnapRHI uses [clang-format](https://clang.llvm.org/docs/ClangFormat.html) for
C/C++/Objective-C and [pre-commit](https://pre-commit.com) to enforce style
automatically. Configuration lives in `.clang-format` (code style) and
`.pre-commit-config.yaml` (hook definitions).

### Setup (one-time)

Install the pre-commit framework and activate the Git hooks:

```bash
pip install pre-commit   # or: brew install pre-commit
pre-commit install       # installs the Git pre-commit hook
```

After this, **every `git commit` will automatically format staged files** before
the commit is created. If a file is reformatted, the commit is aborted so you
can review and re-stage the changes.

### Running the formatter manually

```bash
# Format all project files (full repo scan)
pre-commit run --all-files

# Format only staged files (same as what the hook runs)
pre-commit run

# Run only clang-format (skip other hooks)
pre-commit run clang-format --all-files

# Format a specific file with clang-format directly
clang-format -i src/path/to/File.cpp

# Format all C++ sources under src/ and examples/
find src examples -type f \( -name '*.cpp' -o -name '*.h' -o -name '*.mm' -o -name '*.m' \) \
  | xargs clang-format -i
```

### What the hooks check

| Hook | Scope | What it does |
|------|-------|--------------|
| `clang-format` | `src/`, `examples/` — C/C++/ObjC files | Applies `.clang-format` style rules |
| `black` | Python files (build scripts, tools) | PEP 8-compliant Python formatting |
| `trailing-whitespace` | All text files | Strips trailing spaces |
| `end-of-file-fixer` | All text files | Ensures files end with a newline |
| `check-yaml` | YAML files | Validates YAML syntax |
| `check-added-large-files` | All files | Blocks files > 100 KB (excludes `.spv`, `.png`, etc.) |
| `mixed-line-ending` | All text files | Enforces LF line endings |
| `check-merge-conflict` | All text files | Catches leftover conflict markers |

### IDE integration

| IDE | Setup |
|-----|-------|
| **VS Code** | Install the **C/C++** or **clangd** extension — both respect `.clang-format`. Enable `"editor.formatOnSave": true` in settings for auto-format while editing. |
| **CLion** | Automatically detects `.clang-format`. Enable **Settings → Tools → Actions on Save → Reformat Code**. |
| **Xcode** | Use the **ClangFormat-Xcode** plugin, or format from the terminal before committing. |
| **Visual Studio** | Automatically detects `.clang-format` when **Tools → Options → Text Editor → C/C++ → Formatting → Use custom clang-format** is enabled. |

### Updating hooks

```bash
pre-commit autoupdate    # bumps hook versions in .pre-commit-config.yaml
pre-commit install       # re-install after updating
```

---

## Integration

Add SnapRHI to your CMake project:

```cmake
add_subdirectory(path/to/SnapRHI)

target_link_libraries(your_app PRIVATE
    snap-rhi::public-api
    snap-rhi::backend-vulkan   # or backend-metal, backend-opengl
)
```

### CMake Targets

| Target | Description |
|--------|-------------|
| `snap-rhi::public-api` | Core API — always required |
| `snap-rhi::backend-metal` | Metal backend (Apple only) |
| `snap-rhi::backend-vulkan` | Vulkan backend |
| `snap-rhi::backend-opengl` | OpenGL / OpenGL ES backend |
| `snap-rhi::backend-noop` | No-op stub (testing) |
| `snap-rhi::interop` | Cross-backend resource sharing |

---

## Troubleshooting

| Symptom | Cause | Fix |
|---------|-------|-----|
| `No CMAKE_C_COMPILER` | No compiler | macOS: `xcode-select --install` · Linux: `sudo apt install build-essential` |
| `Could NOT find Vulkan` | SDK missing | `brew install vulkan-loader molten-vk` (macOS) · `apt install libvulkan-dev` (Linux) · [vulkan.lunarg.com](https://vulkan.lunarg.com/) |
| `Failed to load Vulkan Portability library` | `libvulkan.1.dylib` not in library path | Set `DYLD_LIBRARY_PATH=/opt/homebrew/lib` — see [macOS Vulkan setup](#vulkan-via-moltenvk) |
| `doesn't implement the VK_KHR_surface extension` | MoltenVK ICD not installed or not found by Loader | `brew install molten-vk` and set `VK_ICD_FILENAMES` / `VK_DRIVER_FILES` — see [macOS Vulkan setup](#vulkan-via-moltenvk) |
| `Metal backend requires Apple` | `-DSNAP_RHI_ENABLE_METAL=ON` on non-Apple | Use Vulkan or OpenGL |
| `vkCreateInstance` fails on macOS | MoltenVK ICD not found | Set `VK_ICD_FILENAMES` — see [macOS Vulkan setup](#vulkan-via-moltenvk) |
| Vulkan demo crashes in CLion / Xcode / VS Code | IDE doesn't source `~/.zshrc` | Add `DYLD_LIBRARY_PATH`, `VK_ICD_FILENAMES`, `VK_DRIVER_FILES` to run configuration — see [IDE env vars](#5--ide-environment-variables-for-vulkan-on-macos) |
| OpenGL headers missing (Linux) | Mesa dev packages | `sudo apt install libgl1-mesa-dev libx11-dev` |
| Android NDK not found | NDK not installed | Android Studio → SDK Manager → SDK Tools → NDK |
| Shaders not found at runtime | Wrong working directory | Run from the build output dir, or set the IDE working directory |
| Black screen / no triangle | Shader format mismatch | Ensure correct shader variant is bundled (`.msl` / `.spv` / `.glsl`) |
| Preset not visible in IDE | Platform condition mismatch | Presets are filtered by host OS — macOS presets only show on macOS, etc. |
| `SNAP_RHI_DEMO_APP_API` mismatch | Backend enabled ≠ demo API | Ensure the enabled backend matches the demo API (e.g., `-DSNAP_RHI_ENABLE_VULKAN=ON -DSNAP_RHI_DEMO_APP_API=vulkan`) |
