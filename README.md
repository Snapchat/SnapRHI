# SnapRHI

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](./LICENSE.md)
[![FOSSA Status](https://app.fossa.com/api/projects/custom%2B22766%2Fsnap%2FSnapRHI.svg?type=shield&issueType=license)](https://app.fossa.com/projects/custom%2B22766%2Fsnap%2FSnapRHI?ref=badge_shield&issueType=license)
[![Platforms](https://img.shields.io/badge/platforms-macOS%20%7C%20iOS%20%7C%20Windows%20%7C%20Linux%20%7C%20Android-lightgrey)](.)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue?logo=cplusplus)](https://isocpp.org/)
[![CMake ≥ 3.25](https://img.shields.io/badge/CMake-%E2%89%A5%203.25-blue?logo=cmake)](https://cmake.org/)
[![Documentation](https://img.shields.io/badge/docs-available-brightgreen)](./docs/about.md)
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg)](./CONTRIBUTING.md)

**SnapRHI** is a cross-platform, low-level graphics abstraction layer — one API, multiple backends, many targets.

It provides a unified interface for modern rendering APIs (Metal, Vulkan, OpenGL/ES), enabling portable, high-performance graphics code. The API design mirrors Vulkan/Metal semantics, giving engine authors explicit control over synchronization, resource lifetimes, and memory management.

## Backend Support

| Backend | Platforms |
|---------|-----------|
| **Metal** | macOS, iOS |
| **Vulkan** | Windows, Linux, Android, macOS/iOS (MoltenVK) |
| **OpenGL/ES** | Windows, Linux, macOS, Android, iOS |

> **WebAssembly:** The OpenGL ES backend contains Emscripten code paths but the
> build system does not yet wire them up. See the [Build Guide](docs/build.md).

## Quick Start

```bash
git clone <repo-url>
cd SnapRHI

# Library only — pick a backend
cmake --preset macos-metal            # macOS Metal
cmake --preset macos-vulkan           # macOS Vulkan (MoltenVK)
cmake --preset linux-vulkan           # Linux Vulkan
cmake --preset windows-vulkan         # Windows Vulkan
cmake --build build/<preset-name>

# Library + demo app — add "-demo" suffix
cmake --preset macos-metal-demo
cmake --build build/macos-metal-demo

# Or build manually with raw CMake
cmake -B build -DSNAP_RHI_ENABLE_METAL=ON                          # Library only
cmake -B build -DSNAP_RHI_ENABLE_METAL=ON -DSNAP_RHI_BUILD_EXAMPLES=ON -DSNAP_RHI_DEMO_APP_API=metal  # + demo
cmake --build build

# Or use the shell helper (macOS / Linux)
./build.sh                       # Library only
./build.sh --vulkan --with-demo  # Demo app
```

> Run `cmake --list-presets` to see all available configurations.

**→ [Build Guide](docs/build.md)** — all platforms (macOS, iOS, Windows, Linux, Android), all IDEs (VS Code, CLion, Xcode, Visual Studio, Android Studio), preset-based and raw CMake instructions, validation options, and integration guide.

## Documentation

| Document | Description |
|----------|-------------|
| **[Build Guide](docs/build.md)** | Complete build instructions, IDE setup, CMake options |
| **[Demo App](examples/triangle-demo-app/README.md)** | Triangle demo application |
| **[API Overview](docs/about.md)** | Public API documentation |
| **[Resource Management](docs/resource-management.md)** | Lifetime rules, threading, memory |
| **[Performance](docs/performance.md)** | Architecture for low overhead, profiling guidance |
| **[Debugging](docs/debugging.md)** | Validation layers, sanitizers, frame capture |
| **[Profiling](docs/profiling.md)** | Performance tools per platform |

## Performance

SnapRHI is designed for **low-latency, high-throughput** rendering. The cost of
using the library versus calling Metal, Vulkan, or OpenGL directly is minimal
in Release builds.

| Design Decision | Impact |
|-----------------|--------|
| **No hot-path heap allocations** | Command recording, descriptor binding, and draw calls use pre-sized arenas, pooled objects, and sub-allocators |
| **Compile-switchable validation** | All 30+ validation checks use `if constexpr` and are dead-code-eliminated in Release — not branched over, but absent from the binary |
| **Aggressive resource pooling** | Fences, command buffers, command pools, framebuffers, and descriptor memory are recycled — not created/destroyed per frame |
| **OpenGL state cache** | Comprehensive CPU-side state tracking eliminates redundant `gl*` calls without costly `glGet*` queries |
| **`final` backend classes** | All backend implementations are marked `final`, enabling the compiler to devirtualize in single-backend builds |
| **Two retention modes** | Opt out of automatic `shared_ptr` bookkeeping with `UnretainedResources` for zero-overhead submission |

> **Deep dive:** [docs/performance.md](docs/performance.md) — overhead budget per
> operation, per-backend optimization details, validation cost model, and
> profiling guidance.

## Integration

```cmake
add_subdirectory(SnapRHI)
target_link_libraries(your_app PRIVATE
    snap-rhi::public-api
    snap-rhi::backend-metal  # or backend-vulkan, backend-opengl
)
```

## License

MIT — see [LICENSE.md](LICENSE.md)
