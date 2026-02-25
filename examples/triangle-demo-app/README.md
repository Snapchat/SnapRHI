# Triangle Demo Application

A minimal cross-platform demo showcasing SnapRHI rendering with Metal, Vulkan,
and OpenGL backends.

## Platform × Backend Matrix

| Platform | Metal | Vulkan | OpenGL | IDE |
|----------|:-----:|:------:|:------:|-----|
| macOS    | ✅    | ✅ (MoltenVK) | ✅     | VS Code, CLion, Xcode |
| iOS      | ✅    | ✅ (MoltenVK) | ✅ ES  | Xcode |
| Windows  | —     | ✅     | ✅     | VS Code, CLion, Visual Studio |
| Linux    | —     | ✅     | ✅     | VS Code, CLion |
| Android  | —     | ✅     | ✅ ES  | Android Studio |

## Quick Start

Build from the **SnapRHI root** directory:

```bash
cmake --preset macos-metal-demo       # macOS Metal
cmake --preset macos-vulkan-demo      # macOS Vulkan
cmake --preset macos-opengl-demo      # macOS OpenGL
cmake --preset linux-vulkan-demo      # Linux Vulkan
cmake --preset linux-opengl-demo      # Linux OpenGL
cmake --preset windows-vulkan-demo    # Windows Vulkan
cmake --preset windows-opengl-demo    # Windows OpenGL
cmake --build build/<preset-name>
```

Or with raw CMake (any platform):

```bash
cmake -B build \
    -DSNAP_RHI_ENABLE_VULKAN=ON \
    -DSNAP_RHI_BUILD_EXAMPLES=ON \
    -DSNAP_RHI_DEMO_APP_API=vulkan
cmake --build build
```

Or with the shell helper (macOS / Linux):

```bash
./build.sh --with-demo                # Metal demo (macOS default)
./build.sh --vulkan --with-demo       # Vulkan demo
./build.sh --opengl --with-demo       # OpenGL demo
```

**→ [Build Guide](../../docs/build.md)** for full platform instructions, IDE setup, and troubleshooting.

## Demo App Options

| Option | Values | Default | Description |
|--------|--------|---------|-------------|
| `SNAP_RHI_BUILD_EXAMPLES` | `ON` / `OFF` | `OFF` | Must be `ON` to build the demo |
| `SNAP_RHI_DEMO_APP_API` | `metal` · `vulkan` · `opengl` | `metal` | Rendering backend |

The preset selects these automatically. For manual builds, both options are required.

## Running the Demo

| Platform | Command |
|----------|---------|
| macOS (Metal/OpenGL) | `open build/<preset>/examples/triangle-demo-app/snap_rhi_demo_app.app` |
| macOS (Vulkan) | `./build/<preset>/examples/triangle-demo-app/snap_rhi_demo_app.app/Contents/MacOS/snap_rhi_demo_app` |
| Linux | `./build/<preset>/examples/triangle-demo-app/snap_rhi_demo_app` |
| Windows | `.\build\<preset>\examples\triangle-demo-app\Debug\snap_rhi_demo_app.exe` |
| iOS | Open Xcode project → select scheme → ⌘R |
| Android | Android Studio → Run 'app' |

## iOS

Use Xcode presets:

```bash
cmake --preset ios-xcode-metal-demo             # Metal
cmake --preset ios-xcode-vulkan-demo      # Vulkan (MoltenVK)
cmake --preset ios-xcode-opengl-demo      # OpenGL ES
open build/ios-xcode-metal-demo/snap-rhi.xcodeproj
```

Or raw CMake:

```bash
cmake -B build/ios -G Xcode -DCMAKE_SYSTEM_NAME=iOS \
    -DSNAP_RHI_ENABLE_METAL=ON -DSNAP_RHI_BUILD_EXAMPLES=ON -DSNAP_RHI_DEMO_APP_API=metal
open build/ios/snap-rhi.xcodeproj
```

In Xcode: select `snap_rhi_demo_app` scheme → configure signing → choose device → **⌘R**.

## macOS via Xcode

```bash
cmake --preset macos-xcode-metal-demo           # Metal
cmake --preset macos-xcode-vulkan-demo           # Vulkan
cmake --preset macos-xcode-opengl-demo           # OpenGL
open build/macos-xcode-metal-demo/snap-rhi.xcodeproj
```

## Android

Open `platforms/android` in Android Studio, or build from the command line:

```bash
cd platforms/android

# Vulkan backend
./gradlew assembleVulkanDebug
./gradlew installVulkanDebug

# OpenGL ES backend
./gradlew assembleOpenglDebug
./gradlew installOpenglDebug

adb shell am start -n com.snap.rhi.demo.vulkan/.MainActivity
```

The backend is selected via **product flavors** — no manual editing required.
In Android Studio: **Build → Select Build Variant** → choose `vulkanDebug` or
`openglDebug`.

## Project Structure

```
triangle-demo-app/
├── src/
│   ├── main.cpp              # Entry point and backend selection
│   ├── Window.*              # SDL3 window management (base class)
│   ├── TriangleRenderer.*    # Backend-agnostic rendering logic
│   ├── FrameGuard.h          # RAII frame submission guard
│   ├── Metal/                # Metal window + swapchain
│   ├── Vulkan/               # Vulkan window + swapchain
│   └── OpenGL/               # OpenGL window + context
├── assets/                   # Shaders and resources
└── platforms/
    ├── android/              # Android Studio project
    ├── ios/                  # iOS bundle resources
    └── macos/                # macOS bundle resources
```

## Shaders

| File | Backend | Description |
|------|---------|-------------|
| `draw_color.msl` | Metal | Metal Shading Language source |
| `draw_color.spv` | Vulkan | SPIR-V shader |
| `draw_color.glsl` | OpenGL | GLSL source |
| `draw_color.frontend.glsl` | — | GLSL source used for SPIR-V compilation |

### Recompiling SPIR-V shaders

```bash
cd assets
glslang -V -S vert -Dsnap_rhi_demo_triangle_vs draw_color.frontend.glsl --entry-point snap_rhi_demo_triangle_vs --source-entrypoint main -o draw_color_vs.spv
glslang -V -S frag -Dsnap_rhi_demo_triangle_fs draw_color.frontend.glsl --entry-point snap_rhi_demo_triangle_fs --source-entrypoint main -o draw_color_fs.spv

spirv-link draw_color_vs.spv draw_color_fs.spv -o draw_color.spv
```

## Troubleshooting

| Symptom | Fix |
|---------|-----|
| Shaders not found | Run from the build output directory, or set the IDE working directory |
| Black screen | Verify shader files match the selected backend |
| Vulkan device lost | Update GPU drivers; enable validation layers for diagnostics |
| `SNAP_RHI_DEMO_APP_API` mismatch | Ensure backend option matches demo API (e.g., `-DSNAP_RHI_ENABLE_VULKAN=ON -DSNAP_RHI_DEMO_APP_API=vulkan`) |
