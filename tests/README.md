# SnapRHI Unit Tests

Comprehensive test suite for the SnapRHI rendering hardware interface.
Tests run against every compile-time-enabled backend (Metal, Vulkan, OpenGL)
from a single executable, using **Catch2 v2** as the test framework.

---

## Table of Contents

1. [Quick Start](#quick-start)
2. [Prerequisites](#prerequisites)
3. [Build Configuration](#build-configuration)
4. [Running Tests](#running-tests)
5. [Filtering Tests](#filtering-tests)
6. [Metal Debug Layer](#metal-debug-layer)
7. [Vulkan Validation Layers](#vulkan-validation-layers)
8. [Test Architecture](#test-architecture)
9. [Directory Structure](#directory-structure)
10. [Writing New Tests](#writing-new-tests)
11. [Vulkan SPIR-V Shaders](#vulkan-spir-v-shaders)
12. [CI Integration](#ci-integration)

---

## Quick Start

```bash
# Metal (macOS)
cmake -B build -S . -DSNAP_RHI_BUILD_TESTS=ON -DSNAP_RHI_ENABLE_METAL=ON
cmake --build build --target snap-rhi-tests --parallel
ctest --test-dir build --output-on-failure

# Vulkan (any platform — requires Vulkan SDK)
cmake -B build -S . -DSNAP_RHI_BUILD_TESTS=ON -DSNAP_RHI_ENABLE_VULKAN=ON
cmake --build build --target snap-rhi-tests --parallel
ctest --test-dir build --output-on-failure

# OpenGL (any desktop platform)
cmake -B build -S . -DSNAP_RHI_BUILD_TESTS=ON -DSNAP_RHI_ENABLE_OPENGL=ON
cmake --build build --target snap-rhi-tests --parallel
ctest --test-dir build --output-on-failure

# Multiple backends at once
cmake -B build -S . \
  -DSNAP_RHI_BUILD_TESTS=ON \
  -DSNAP_RHI_ENABLE_METAL=ON \
  -DSNAP_RHI_ENABLE_VULKAN=ON
cmake --build build --target snap-rhi-tests --parallel
ctest --test-dir build --output-on-failure
```

---

## Prerequisites

| Platform | Backend | Requirements |
|----------|---------|-------------|
| macOS    | Metal   | Xcode ≥ 15, macOS ≥ 14 (Apple Silicon or Intel with Metal support) |
| macOS    | Vulkan  | Vulkan SDK ≥ 1.3 (provides MoltenVK) |
| macOS    | OpenGL  | Built-in (deprecated on macOS, but functional for GL 4.1) |
| Linux    | Vulkan  | `libvulkan-dev`, GPU driver with Vulkan ICD |
| Linux    | OpenGL  | `libgl1-mesa-dev`, `libx11-dev`, plus Wayland/X11 dev packages |
| Windows  | Vulkan  | Vulkan SDK ≥ 1.3, GPU driver with Vulkan ICD |
| Windows  | OpenGL  | GPU driver with OpenGL ≥ 4.1 support |

**Common requirements:** CMake ≥ 3.25, C++20-capable compiler, Ninja or Make.

Catch2 v2.13.10 is fetched automatically via `FetchContent` if not already
installed on the system.

---

## Build Configuration

### Core CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `SNAP_RHI_BUILD_TESTS` | `OFF` | **Must be `ON`** to build the test executable |
| `SNAP_RHI_ENABLE_METAL` | `OFF` | Enable Metal backend (Apple only) |
| `SNAP_RHI_ENABLE_VULKAN` | `OFF` | Enable Vulkan backend |
| `SNAP_RHI_ENABLE_OPENGL` | `OFF` | Enable OpenGL backend |
| `SNAP_RHI_ENABLE_ALL_VALIDATION` | `OFF` | Turn on every validation tag + Vulkan layers + slow checks |

### Validation Options (selected)

| Option | Default | Description |
|--------|---------|-------------|
| `SNAP_RHI_VULKAN_ENABLE_LAYERS` | `OFF` | Enable Vulkan validation layers |
| `SNAP_RHI_ENABLE_SLOW_SAFETY_CHECKS` | `OFF` | Extra runtime checks (reduces performance) |
| `SNAP_RHI_REPORT_LEVEL` | `warning` | Minimum log level: `all`, `debug`, `info`, `warning`, `error`, `critical_error` |

See `cmake/SnapRHIOptions.cmake` for the full list of per-operation validation
tags (e.g., `SNAP_RHI_VALIDATION_BUFFER_OP`, `SNAP_RHI_VALIDATION_RENDER_PIPELINE_OP`).

---

## Running Tests

### Via CTest (recommended)

```bash
ctest --test-dir build --output-on-failure
```

Useful CTest flags:

| Flag | Purpose |
|------|---------|
| `--output-on-failure` | Print stdout/stderr only for failing tests |
| `--verbose` | Print all test output |
| `--timeout 120` | Kill tests that hang beyond 120 seconds |
| `-R <regex>` | Only run tests whose CTest name matches the regex |

### Direct executable

```bash
./build/tests/snap-rhi-tests                    # run all
./build/tests/snap-rhi-tests --list-tests       # list every test case
./build/tests/snap-rhi-tests --list-tags        # list all tags
```

---

## Filtering Tests

The test executable accepts all standard **Catch2 v2** CLI options.

### By test name (substring match)

```bash
./build/tests/snap-rhi-tests "Buffer"           # all test cases containing "Buffer"
./build/tests/snap-rhi-tests "RenderCommandEncoder — Basic draw call"
```

### By tag

Every test case is tagged. Tags can be combined:

```bash
./build/tests/snap-rhi-tests [buffer]           # only buffer tests
./build/tests/snap-rhi-tests [rendering]        # all rendering tests
./build/tests/snap-rhi-tests [api][texture]     # tests tagged with BOTH api AND texture
./build/tests/snap-rhi-tests [compute],[blit]   # tests tagged with compute OR blit
./build/tests/snap-rhi-tests ~[rendering]       # everything EXCEPT rendering tests
```

### Available Tags

| Tag | Category | Description |
|-----|----------|-------------|
| `[api]` | All | Present on every test (core API surface) |
| `[buffer]` | API | Buffer creation, mapping, read-back |
| `[texture]` | API | Texture 2D, cubemap, texture views |
| `[sampler]` | API | Sampler state creation |
| `[device]` | API | Device creation, queries, debug messenger |
| `[descriptor]` | API | Descriptor set layout, pool, binding |
| `[pipeline-cache]` | API | Pipeline cache creation |
| `[command-buffer]` | API | Command buffer creation, encoder access |
| `[command-queue]` | API | Command queue waitIdle / waitUntilScheduled |
| `[sync]` | API | Synchronization primitives |
| `[fence]` | API | Fence creation, wait, signal |
| `[semaphore]` | API | Semaphore creation |
| `[query]` | API | Query pool (timestamps, occlusion) |
| `[blit]` | Blit | Buffer ↔ texture transfers, mipmap generation |
| `[readback]` | Blit | Texture → buffer read-back |
| `[mipmap]` | Blit | Mipmap generation |
| `[compute]` | Compute | Compute pipeline, dispatch, SSBO |
| `[dispatch]` | Compute | Compute dispatch with data verification |
| `[pipeline]` | Compute/Rendering | Pipeline creation |
| `[rendering]` | Rendering | All render tests |
| `[encoder]` | Rendering | Basic draw calls |
| `[dynamic]` | Rendering | Dynamic rendering (VK_KHR_dynamic_rendering) |
| `[indexed]` | Rendering | Indexed draw calls |
| `[instancing]` | Rendering | Instanced draw calls |
| `[blending]` | Rendering | Alpha blending |
| `[depth]` | Rendering | Depth testing |
| `[culling]` | Rendering | Back-face culling |
| `[viewport]` | Rendering | Viewport clipping |
| `[writemask]` | Rendering | Color write mask |
| `[msaa]` | Rendering | Multi-sample anti-aliasing |
| `[shader]` | Rendering | Shader library/module creation |

---

## Metal Debug Layer

Apple's Metal debug layer provides GPU-side validation similar to Vulkan
validation layers.  Enable it via environment variables:

```bash
MTL_DEBUG_LAYER=1 \
MTL_DEBUG_LAYER_ERROR_MODE=assert \
MTL_DEBUG_LAYER_WARNING_MODE=nslog \
  ./build/tests/snap-rhi-tests
```

| Variable | Values | Recommendation |
|----------|--------|----------------|
| `MTL_DEBUG_LAYER` | `0` / `1` | Set to `1` during development |
| `MTL_DEBUG_LAYER_ERROR_MODE` | `assert` / `nslog` / `ignore` | Use `assert` — fails immediately on Metal errors |
| `MTL_DEBUG_LAYER_WARNING_MODE` | `assert` / `nslog` / `ignore` | Use `nslog` (see note below) |

---

## Vulkan Validation Layers

Enable Vulkan validation layers for detailed GPU-side error reporting:

```bash
cmake -B build -S . \
  -DSNAP_RHI_BUILD_TESTS=ON \
  -DSNAP_RHI_ENABLE_VULKAN=ON \
  -DSNAP_RHI_VULKAN_ENABLE_LAYERS=ON
cmake --build build --target snap-rhi-tests --parallel
./build/tests/snap-rhi-tests
```

Or enable everything at once:

```bash
cmake -B build -S . \
  -DSNAP_RHI_BUILD_TESTS=ON \
  -DSNAP_RHI_ENABLE_VULKAN=ON \
  -DSNAP_RHI_ENABLE_ALL_VALIDATION=ON
```

---

## Test Architecture

### Multi-Backend Execution Model

Each `TEST_CASE` iterates over all **compile-time-enabled backends**.  The
pattern looks like this:

```cpp
TEST_CASE("Buffer — Creation and lifecycle", "[api][buffer]") {
    for (auto api : test_harness::getAvailableAPIs()) {
        auto ctx = test_harness::createTestContext(api);
        if (!ctx) continue;

        SNAP_RHI_TEST_BACKEND_SECTION(ctx->apiName) {
            // Test body — ctx->device, ctx->commandQueue are ready
        }
    }
}
```

- `getAvailableAPIs()` returns the APIs compiled in (e.g., `{Metal}` or `{Metal, Vulkan}`).
- `createTestContext(api)` creates a GPU device for the backend, returns
  `std::nullopt` if the backend is unavailable at runtime (e.g., no Vulkan ICD).
- `SNAP_RHI_TEST_BACKEND_SECTION(name)` opens a Catch2 `DYNAMIC_SECTION` and
  logs the test + backend name once.

This means a single test case may produce multiple sub-sections (one per
backend), and the same assertion is verified against every available GPU API.

### Test Harness Components

| File | Purpose |
|------|---------|
| `TestHarness/DeviceTestFixture.h` | `createTestContext()`, `getAvailableAPIs()`, `SNAP_RHI_TEST_BACKEND_SECTION` macro |
| `TestHarness/ShaderHelper.h` | `createPassthroughShaders()`, `createFillShaders()`, `createComputeShaders()` — returns compiled shader modules per-backend |
| `TestHarness/ShaderSources.h` | Embedded GLSL/MSL shader source strings |
| `TestHarness/ReadbackHelper.h` | GPU → CPU pixel read-back utilities for verifying rendered output |

### Shader System

| Backend | Shader Format | Source |
|---------|---------------|--------|
| Metal | MSL source code | Compiled at runtime via shader library API; sources in `ShaderSources.h` |
| Vulkan | Pre-compiled SPIR-V | Compiled from GLSL at build time (or pre-generated fallback `generated/VulkanSPIRV.h`) |
| OpenGL | GLSL source code | Compiled at runtime; sources in `ShaderSources.h` |

---

## Directory Structure

```
tests/
├── CMakeLists.txt                    # Test build configuration
├── README.md                         # This file
├── cmake/
│   └── CompileVulkanShaders.cmake    # CMake module: GLSL → SPIR-V at build time
├── generated/
│   └── VulkanSPIRV.h                 # Pre-generated SPIR-V fallback header
├── scripts/
│   └── compile_vulkan_shaders.sh     # Manual SPIR-V regeneration script
├── shaders/
│   ├── metal/                        # Metal shader sources (if any external)
│   ├── opengl/                       # OpenGL shader sources (if any external)
│   └── vulkan/                       # Vulkan GLSL sources
│       ├── passthrough.vert
│       ├── passthrough.frag
│       ├── fill.vert
│       ├── fill.frag
│       └── compute_fill.comp
└── src/
    ├── main.cpp                      # Catch2 entry point (non-Apple platforms)
    ├── main_apple.mm                 # Catch2 entry point (Apple — @autoreleasepool)
    ├── API/                          # Core API tests
    │   ├── BufferTest.cpp
    │   ├── CommandBufferTest.cpp
    │   ├── DescriptorSetTest.cpp
    │   ├── DeviceTest.cpp
    │   ├── PipelineCacheTest.cpp
    │   ├── QueryPoolTest.cpp
    │   ├── SamplerTest.cpp
    │   ├── SynchronizationTest.cpp
    │   └── TextureTest.cpp
    ├── Blit/                         # Blit encoder tests
    │   └── BlitCommandEncoderTest.cpp
    ├── Compute/                      # Compute encoder tests
    │   └── ComputeCommandEncoderTest.cpp
    ├── Rendering/                    # Render pipeline and encoder tests
    │   ├── RenderCommandEncoderTest.cpp
    │   └── RenderPipelineTest.cpp
    └── TestHarness/                  # Shared test infrastructure
        ├── DeviceTestFixture.h
        ├── ReadbackHelper.h
        ├── ShaderHelper.h
        └── ShaderSources.h
```

---

## Writing New Tests

### 1. Create a test file

Place it in the appropriate subdirectory under `src/`:

- `API/` — resource creation, lifecycle, queries
- `Blit/` — buffer/texture transfer operations
- `Compute/` — compute pipeline and dispatch
- `Rendering/` — render pipeline, draw calls, framebuffer operations

The `CMakeLists.txt` uses `file(GLOB_RECURSE)`, so new `.cpp` files are picked
up automatically on the next configure.

### 2. Use the standard test pattern

```cpp
#include <catch2/catch.hpp>
#include "TestHarness/DeviceTestFixture.h"

TEST_CASE("MyResource — Description of behavior", "[api][my-tag]") {
    for (auto api : test_harness::getAvailableAPIs()) {
        auto ctx = test_harness::createTestContext(api);
        if (!ctx) continue;

        SNAP_RHI_TEST_BACKEND_SECTION(ctx->apiName) {
            auto& device = ctx->device;
            auto* queue  = ctx->commandQueue;

            // --- Test logic ---
            REQUIRE(device != nullptr);

            // Use SECTION for sub-cases:
            SECTION("Sub-behavior A") {
                // ...
                CHECK(result == expected);
            }

            SECTION("Sub-behavior B") {
                // ...
                REQUIRE(other == expected);
            }
        }
    }
}
```

### 3. Add shaders (if needed)

- **Metal / OpenGL:** Add source strings to `TestHarness/ShaderSources.h` and
  create a helper in `TestHarness/ShaderHelper.h`.
- **Vulkan:** Add `.vert`/`.frag`/`.comp` GLSL files to `shaders/vulkan/`,
  then regenerate the fallback header:

  ```bash
  tests/scripts/compile_vulkan_shaders.sh
  ```

  The header at `generated/VulkanSPIRV.h` should be committed so builds
  without `glslc` still work.

### 4. Use assertions correctly

| Macro | When to use |
|-------|-------------|
| `REQUIRE(expr)` | Critical check — test aborts on failure |
| `CHECK(expr)` | Non-critical check — test continues on failure, reports at end |
| `REQUIRE_FALSE(expr)` | Assert that expression is false (critical) |
| `REQUIRE_NOTHROW(expr)` | Assert no exception is thrown |

---

## Vulkan SPIR-V Shaders

Vulkan tests require pre-compiled SPIR-V bytecode. Two mechanisms ensure
shaders are always available:

### Build-time compilation (preferred)

If `glslc` (from the Vulkan SDK) is on `PATH`, CMake compiles the GLSL shaders
in `shaders/vulkan/` at build time and generates a header at
`build/generated/VulkanSPIRV.h`.

### Pre-generated fallback

When `glslc` is not available, CMake falls back to the checked-in header at
`generated/VulkanSPIRV.h`.

**To regenerate the fallback header after modifying shaders:**

```bash
# Requires glslc on PATH
tests/scripts/compile_vulkan_shaders.sh
git add tests/generated/VulkanSPIRV.h
```

---

## CI Integration

Tests run automatically on every push and PR via GitHub Actions (see
`.github/workflows/ci.yml`).

### CI Matrix

| Platform | Backend | Runner | Tests Run | GPU Guaranteed |
|----------|---------|--------|-----------|----------------|
| macOS | Metal | `macos-14` (Apple Silicon) | Yes | **Yes** — `test_required: true` |
| macOS | Vulkan | `macos-14` | Yes | No |
| macOS | OpenGL | `macos-14` | Yes | No |
| Linux | Vulkan | `ubuntu-24.04` | Yes | No |
| Linux | OpenGL | `ubuntu-24.04` | Yes | No |
| Windows | Vulkan | `windows-latest` | Yes | No |
| Windows | OpenGL | `windows-latest` | Yes | No |
| iOS | Metal/Vulkan/OpenGL ES | `macos-14` | Build only | N/A (no device) |
| Android | Vulkan/OpenGL ES | `ubuntu-24.04` | Build only | N/A (no device) |

When `test_required` is not set, the test step uses `continue-on-error: true`
so that runners without a GPU do not block CI.

---
