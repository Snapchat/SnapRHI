# SnapRHI Debugging Guide

This guide covers SnapRHI's built-in validation system, debug build configuration, sanitizers, graphics API debugging, and frame capture tools.

> **Related:** [Build Guide](build.md) | [Profiling Guide](profiling.md) | [Resource Management](resource-management.md) | [API Reference](about.md)

---

## Table of Contents

1. [Quick Start: Enable All Validation](#1-quick-start-enable-all-validation)
2. [SnapRHI Validation System](#2-snaprhi-validation-system)
3. [Debug Build Configuration](#3-debug-build-configuration)
4. [Debug Labels & Markers](#4-debug-labels--markers)
5. [Sanitizers](#5-sanitizers)
6. [Graphics API Validation](#6-graphics-api-validation)
7. [Frame Capture Tools](#7-frame-capture-tools)
8. [Creating Minimal Reproductions](#8-creating-minimal-reproductions)

---

## 1. Quick Start: Enable All Validation

The fastest way to enable comprehensive debugging is the **master validation switch**. This turns on every validation tag, Vulkan layers, slow safety checks, full logging, and report-level `all` in a single flag:

```bash
# Via build.sh
./build.sh --vulkan --with-demo --validation

# Via CMake presets (recommended)
cmake --preset macos-metal-demo-validation
cmake --build build/macos-metal-demo-validation

# Via raw CMake
cmake -B build -DSNAP_RHI_ENABLE_METAL=ON -DSNAP_RHI_ENABLE_ALL_VALIDATION=ON
```

When `SNAP_RHI_ENABLE_ALL_VALIDATION=ON` is set, the following options are force-enabled:

| Category | What gets enabled |
|----------|-------------------|
| Validation layers | `SNAP_RHI_VULKAN_ENABLE_LAYERS=ON` |
| Slow safety checks | `SNAP_RHI_ENABLE_SLOW_SAFETY_CHECKS=ON` |
| Debug labels | `SNAP_RHI_ENABLE_DEBUG_LABELS=ON` |
| Logging | `SNAP_RHI_ENABLE_LOGS=ON` |
| Report level | `SNAP_RHI_REPORT_LEVEL=all` |
| All 30 validation tags | Every `SNAP_RHI_VALIDATION_*` option set to `ON` |

> **⚠️ Performance:** Full validation has significant CPU overhead. Use it for development and debugging, not profiling or benchmarking.

---

## 2. SnapRHI Validation System

SnapRHI has a **two-layer** validation system:

1. **Compile-time validation tags** — CMake options that compile validation checks into the library
2. **Runtime validation config** — `DeviceCreateInfo` fields that control what gets reported at runtime

### 2.1 Compile-Time: CMake Debug Options

These options control what debug code is compiled into the library. Defined in `cmake/SnapRHIOptions.cmake`:

#### Core Debug Options

| CMake Option | Default | Description |
|-------------|---------|-------------|
| `SNAP_RHI_ENABLE_ALL_VALIDATION` | `OFF` | Master switch — enables everything below |
| `SNAP_RHI_ENABLE_DEBUG_LABELS` | `ON` | Debug labels for GPU resources (`setDebugLabel()`, encoder debug groups) |
| `SNAP_RHI_ENABLE_LOGS` | `ON` | Logging output |
| `SNAP_RHI_REPORT_LEVEL` | `warning` | Minimum severity: `all`, `debug`, `info`, `warning`, `performance_warning`, `error`, `critical_error` |
| `SNAP_RHI_ENABLE_SLOW_SAFETY_CHECKS` | `OFF` | Additional runtime safety checks (impacts performance) |
| `SNAP_RHI_ENABLE_API_DUMP` | `OFF` | Dumps the exact sequence of API calls for reproduction |
| `SNAP_RHI_ENABLE_SOURCE_LOCATION` | `OFF` | Source location tracking in API calls |
| `SNAP_RHI_THROW_ON_LIFETIME_VIOLATION` | `OFF` | Throw exceptions on object lifetime violations (instead of logging) |
| `SNAP_RHI_ENABLE_CUSTOM_PROFILING_LABELS` | `OFF` | Custom profiling callbacks (`ProfilingCreateInfo`) |

#### Backend-Specific Options

| CMake Option | Default | Description |
|-------------|---------|-------------|
| `SNAP_RHI_VULKAN_ENABLE_LAYERS` | `OFF` | Enable Vulkan validation layers (`VK_LAYER_KHRONOS_validation`) |

#### Validation Tags — Fine-Grained Control

Validation tags enable checks for specific resource types or operations. Each tag compiles to a `SNAP_RHI_ENABLE_VALIDATION_TAG_*` preprocessor definition.

**Resource creation & destruction:**

| Tag | Validates |
|-----|-----------|
| `SNAP_RHI_VALIDATION_CREATE_OP` | All resource creation parameters |
| `SNAP_RHI_VALIDATION_DESTROY_OP` | Resource destruction ordering and cleanup |
| `SNAP_RHI_VALIDATION_DEVICE_OP` | Device operations |

**Command recording:**

| Tag | Validates |
|-----|-----------|
| `SNAP_RHI_VALIDATION_COMMAND_BUFFER_OP` | Command buffer state transitions and lifecycle |
| `SNAP_RHI_VALIDATION_COMMAND_QUEUE_OP` | Queue submission parameters |
| `SNAP_RHI_VALIDATION_RENDER_COMMAND_ENCODER_OP` | Render encoder calls (pipeline bound, valid state, etc.) |
| `SNAP_RHI_VALIDATION_COMPUTE_COMMAND_ENCODER_OP` | Compute encoder calls |
| `SNAP_RHI_VALIDATION_BLIT_COMMAND_ENCODER_OP` | Blit encoder calls |

**Pipeline & shader:**

| Tag | Validates |
|-----|-----------|
| `SNAP_RHI_VALIDATION_RENDER_PIPELINE_OP` | Render pipeline creation parameters |
| `SNAP_RHI_VALIDATION_COMPUTE_PIPELINE_OP` | Compute pipeline creation parameters |
| `SNAP_RHI_VALIDATION_SHADER_MODULE_OP` | Shader module creation |
| `SNAP_RHI_VALIDATION_SHADER_LIBRARY_OP` | Shader library compilation |
| `SNAP_RHI_VALIDATION_PIPELINE_CACHE_OP` | Pipeline cache operations |

**Resources:**

| Tag | Validates |
|-----|-----------|
| `SNAP_RHI_VALIDATION_BUFFER_OP` | Buffer creation, mapping, alignment |
| `SNAP_RHI_VALIDATION_TEXTURE_OP` | Texture creation, format support |
| `SNAP_RHI_VALIDATION_SAMPLER_OP` | Sampler parameters |
| `SNAP_RHI_VALIDATION_RENDER_PASS_OP` | Render pass attachment compatibility |
| `SNAP_RHI_VALIDATION_FRAMEBUFFER_OP` | Framebuffer attachment validation |

**Descriptors:**

| Tag | Validates |
|-----|-----------|
| `SNAP_RHI_VALIDATION_DESCRIPTOR_SET_LAYOUT_OP` | Layout binding configuration |
| `SNAP_RHI_VALIDATION_PIPELINE_LAYOUT_OP` | Pipeline layout set slot completeness |
| `SNAP_RHI_VALIDATION_DESCRIPTOR_POOL_OP` | Pool allocation tracking |
| `SNAP_RHI_VALIDATION_DESCRIPTOR_SET_OP` | Descriptor set bindings |

**Synchronization:**

| Tag | Validates |
|-----|-----------|
| `SNAP_RHI_VALIDATION_FENCE_OP` | Fence lifecycle (reset-before-reuse, etc.) |
| `SNAP_RHI_VALIDATION_SEMAPHORE_OP` | Semaphore signal/wait pairing |
| `SNAP_RHI_VALIDATION_DEVICE_CONTEXT_OP` | Context binding (OpenGL) |
| `SNAP_RHI_VALIDATION_QUERY_POOL_OP` | Query pool operations |

**OpenGL-specific:**

| Tag | Validates |
|-----|-----------|
| `SNAP_RHI_VALIDATION_GL_COMMAND_QUEUE_EXTERNAL_ERROR` | `glGetError()` after each command |
| `SNAP_RHI_VALIDATION_GL_COMMAND_QUEUE_INTERNAL_ERROR` | Internal GL command queue errors |
| `SNAP_RHI_VALIDATION_GL_STATE_CACHE_OP` | GL state cache consistency |
| `SNAP_RHI_VALIDATION_GL_PROGRAM_VALIDATION_OP` | GL program linking/validation |
| `SNAP_RHI_VALIDATION_GL_PROFILE_OP` | GL profile capabilities |

#### Selective Validation Example

For targeted debugging without full validation overhead:

```bash
# Only validate render pipeline creation and command encoder usage
cmake -B build \
    -DSNAP_RHI_ENABLE_METAL=ON \
    -DSNAP_RHI_VALIDATION_RENDER_PIPELINE_OP=ON \
    -DSNAP_RHI_VALIDATION_RENDER_COMMAND_ENCODER_OP=ON \
    -DSNAP_RHI_REPORT_LEVEL=debug
```

### 2.2 Runtime: DeviceCreateInfo

At device creation time, you control validation behavior via `DeviceCreateInfo`:

```cpp
snap::rhi::DeviceCreateInfo info{};

// Enable debug callbacks (required for DebugMessenger)
info.deviceCreateFlags = snap::rhi::DeviceCreateFlags::EnableDebugCallback;

// Select which validation categories are active at runtime
info.enabledTags = snap::rhi::ValidationTag::CreateOp
                 | snap::rhi::ValidationTag::RenderPipelineOp
                 | snap::rhi::ValidationTag::BufferOp;

// Set minimum report severity
info.enabledReportLevel = snap::rhi::ReportLevel::Warning;
```

> **Note:** Runtime tags are ANDed with compile-time tags. A validation check runs only if **both** the CMake option and the `DeviceCreateInfo` tag are enabled.

### 2.3 DebugMessenger — Receiving Validation Messages

Register a callback to receive validation messages at runtime:

```cpp
auto messenger = device->createDebugMessenger({
    .debugMessengerCallback = [](const snap::rhi::DebugCallbackInfo& info) {
        std::cerr << "[SnapRHI] " << info.message << std::endl;
    }
});
```

Requirements:
- Device must be created with `DeviceCreateFlags::EnableDebugCallback`
- Callbacks may be invoked from **any thread** — the implementation must be thread-safe
- OpenGL: single global callback is fanned out to all `DebugMessenger` instances

---

## 3. Debug Build Configuration

### 3.1 CMake Build Types

| Type | Use Case |
|------|----------|
| `Debug` | Full debug symbols, no optimization (`-O0 -g`) |
| `RelWithDebInfo` | Optimized with debug symbols — recommended for GPU debugging |
| `Release` | Full optimization, no debug symbols |

### 3.2 CMake Presets

SnapRHI provides pre-configured presets in `CMakePresets.json` for each platform and backend:

**Standard presets:**

| Preset | Platform | Backend | Notes |
|--------|----------|---------|-------|
| `macos-metal` | macOS | Metal | Debug build |
| `macos-metal-release` | macOS | Metal | Optimized, no debug labels/logs |
| `macos-opengl` | macOS | OpenGL | Debug build |
| `macos-vulkan` | macOS | Vulkan/MoltenVK | Debug build |
| `linux-vulkan` | Linux | Vulkan | Debug build |
| `linux-opengl` | Linux | OpenGL | Debug build |
| `windows-vulkan` | Windows | Vulkan | Debug build |
| `windows-opengl` | Windows | OpenGL | Debug build |

**Demo app presets** (append `-demo`): Includes the triangle demo application.

**Full validation presets** (append `-demo-validation`): Demo app + `SNAP_RHI_ENABLE_ALL_VALIDATION=ON`:

```bash
# macOS Metal with full validation
cmake --preset macos-metal-demo-validation
cmake --build build/macos-metal-demo-validation

# Linux Vulkan with full validation
cmake --preset linux-vulkan-demo-validation
cmake --build build/linux-vulkan-demo-validation
```

### 3.3 build.sh Convenience Script

The `build.sh` script wraps CMake for macOS and Linux:

```bash
./build.sh --help

# Backend selection
./build.sh --metal               # Metal (macOS default)
./build.sh --vulkan              # Vulkan
./build.sh --opengl              # OpenGL

# Build options
./build.sh --with-demo           # Include demo application
./build.sh --release             # Release build
./build.sh --clean               # Clean rebuild
./build.sh --xcode --with-demo   # Generate Xcode project

# Validation
./build.sh --validation          # Enable ALL validation features
./build.sh --validation-layers   # Enable Vulkan validation layers only

# Combined
./build.sh --vulkan --with-demo --validation  # Full validation Vulkan demo
```

### 3.4 Platform-Specific Symbol Generation

| Platform | Method |
|----------|--------|
| macOS/iOS | `-g` flag (preserved by Xcode in Debug/RelWithDebInfo) |
| Windows | `/Zi` flag for PDB generation |
| Linux | `-g` flag with GCC/Clang |

---

## 4. Debug Labels & Markers

### 4.1 Object Labels

When `SNAP_RHI_ENABLE_DEBUG_LABELS=ON` (default), all `DeviceChild` objects support human-readable labels:

```cpp
auto buffer = device->createBuffer(info);
buffer->setDebugLabel("SceneUBO");

auto texture = device->createTexture(info);
texture->setDebugLabel("ShadowMap_Cascade0");

auto pipeline = device->createRenderPipeline(info);
pipeline->setDebugLabel("PBR_Forward_Pass");
```

Labels are forwarded to native APIs for GPU debugger visibility:
- **Metal:** Sets `MTLResource.label`
- **OpenGL:** Uses `GL_KHR_debug` object labels
- **Vulkan:** Uses `VK_EXT_debug_utils` object naming

When `SNAP_RHI_ENABLE_DEBUG_LABELS=OFF`, `setDebugLabel()` compiles to a no-op with zero overhead.

### 4.2 Encoder Debug Groups

Command encoders support hierarchical debug groups for GPU profiler nesting:

```cpp
auto encoder = commandBuffer->getRenderCommandEncoder();
encoder->beginEncoding(renderPassInfo);

encoder->beginDebugGroup("Shadow Pass");
// ... shadow render commands ...
encoder->endDebugGroup();

encoder->beginDebugGroup("Lighting Pass");
encoder->beginDebugGroup("Directional Lights");
// ... render commands ...
encoder->endDebugGroup();
encoder->beginDebugGroup("Point Lights");
// ... render commands ...
encoder->endDebugGroup();
encoder->endDebugGroup();

encoder->endEncoding();
```

Backend mapping:
- **Metal:** `-[MTLCommandEncoder pushDebugGroup:]` / `popDebugGroup`
- **OpenGL:** Recorded into command stream via KHR_debug
- **Vulkan:** Placeholder for `vkCmdBeginDebugUtilsLabelEXT` (not yet implemented)

> **Rule:** Every `beginDebugGroup()` must have a matching `endDebugGroup()`.

---

## 5. Sanitizers

Sanitizers detect memory and threading bugs at runtime. Use with debug or minimally-optimized builds.

### 5.1 CMake Integration

```cmake
# AddressSanitizer + UBSan
target_compile_options(<target> PRIVATE
    -fsanitize=address,undefined
    -fno-omit-frame-pointer)
target_link_options(<target> PRIVATE
    -fsanitize=address,undefined)
```

> **Tip:** Use `-O1` or `-O0` for best diagnostic quality.

### 5.2 Platform Configuration

| Platform | Configuration |
|----------|---------------|
| **macOS/iOS (Xcode)** | Scheme → Diagnostics → Enable Address/Thread/UB Sanitizer |
| **Android (NDK r18+)** | Add `-fsanitize=address` to CMake flags; requires ASan runtime on device |
| **Windows** | Use clang-cl with `-fsanitize=` flags, or VS AddressSanitizer for Clang toolchains |

### 5.3 Sanitizer Types

| Sanitizer | Detects | Compatibility |
|-----------|---------|---------------|
| AddressSanitizer (ASan) | Buffer overflows, use-after-free, memory leaks | ❌ Cannot combine with TSan |
| ThreadSanitizer (TSan) | Data races, deadlocks | ❌ Cannot combine with ASan |
| UndefinedBehaviorSanitizer (UBSan) | Undefined behavior (null deref, integer overflow) | ✅ Combinable with ASan |

---

## 6. Graphics API Validation

### 6.1 Metal (macOS/iOS)

**Enable in Xcode:** Scheme → Diagnostics → Metal API Validation

**Additional options:**
- Metal Shader Validation (Xcode 15+)
- Metal GPU-Side Validation
- GPU Frame Capture

**Capabilities:**
- API usage validation
- Shader validation (buffer bounds, null resources)
- Resource tracking and leak detection

### 6.2 Vulkan

**Enable via CMake:**
```bash
cmake -B build -DSNAP_RHI_ENABLE_VULKAN=ON -DSNAP_RHI_VULKAN_ENABLE_LAYERS=ON
```

**Or via build.sh:**
```bash
./build.sh --vulkan --validation-layers
```

When `SNAP_RHI_VULKAN_ENABLE_LAYERS=ON`, the backend requests `VK_LAYER_KHRONOS_validation` at device creation.

**Vulkan Configurator** (vkconfig) can be used to fine-tune layer settings without recompiling.

### 6.3 OpenGL / OpenGL ES

**SnapRHI integration:** Set `DeviceCreateFlags::EnableDebugCallback` on `DeviceCreateInfo` to activate `KHR_debug` callbacks. Then register a `DebugMessenger` to receive messages.

**OpenGL-specific validation tags** for deep driver debugging:
```bash
cmake -B build \
    -DSNAP_RHI_ENABLE_OPENGL=ON \
    -DSNAP_RHI_VALIDATION_GL_COMMAND_QUEUE_EXTERNAL_ERROR=ON \
    -DSNAP_RHI_VALIDATION_GL_STATE_CACHE_OP=ON \
    -DSNAP_RHI_VALIDATION_GL_PROGRAM_VALIDATION_OP=ON
```

**API dump:**
```bash
cmake -B build -DSNAP_RHI_ENABLE_OPENGL=ON -DSNAP_RHI_ENABLE_API_DUMP=ON
```

This captures the exact sequence of SnapRHI API calls for reproduction.

---

## 7. Frame Capture Tools

### 7.1 Tool Matrix

| Platform | Recommended Tool | API Support |
|----------|-----------------|-------------|
| macOS/iOS | Xcode GPU Frame Capture | Metal |
| All | RenderDoc | Vulkan, OpenGL |
| Android | Android GPU Inspector | Vulkan, OpenGL ES |
| Windows | PIX, RenderDoc | D3D12, Vulkan, OpenGL |
| NVIDIA | Nsight Graphics | Vulkan, OpenGL |

### 7.2 Capture Workflow

1. **Isolate the issue** — Reduce to minimal scene/frame
2. **Capture a frame** — Use tool's capture trigger
3. **Inspect draw calls** — Review state, resources, timings
4. **Check pixel history** — For rendering artifacts

### 7.3 Metal Frame Capture (Xcode)

1. Run app from Xcode
2. Click camera icon in Debug navigator
3. Inspect command buffers, encoders, resources
4. Use Metal System Trace for timing

> **Tip:** `SNAP_RHI_ENABLE_DEBUG_LABELS=ON` ensures all resources appear with readable names in the Xcode GPU debugger.

### 7.4 RenderDoc Usage

1. Launch application through RenderDoc
2. Press capture hotkey (F12 default)
3. Open capture for analysis
4. Inspect API calls, pipeline state, textures

> **Tip:** Encoder debug groups (`beginDebugGroup`/`endDebugGroup`) appear as collapsible sections in RenderDoc.

---

## 8. Creating Minimal Reproductions

### 8.1 Checklist for Bug Reports

- [ ] Exact git commit/tag
- [ ] Platform, OS version, GPU/driver versions
- [ ] CMake preset or build flags used
- [ ] Backend (Metal / Vulkan / OpenGL)
- [ ] Minimal reproduction steps
- [ ] SnapRHI validation output (build with `--validation`)
- [ ] Sanitizer output (if applicable)
- [ ] Frame capture or API dump

### 8.2 Isolation Strategy

```
1. Enable SNAP_RHI_ENABLE_ALL_VALIDATION
2. Enable SNAP_RHI_ENABLE_API_DUMP for call trace
3. Reduce scene complexity → single object
4. Reduce to single frame → disable animation
5. Isolate API calls → use validation output
6. Create standalone test case from triangle-demo-app
```

### 8.3 API Dump

Enable comprehensive API logging for exact call sequence capture:

```bash
cmake -B build -DSNAP_RHI_ENABLE_OPENGL=ON -DSNAP_RHI_ENABLE_API_DUMP=ON
```

### 8.4 Lifetime Violation Debugging

For tracking resource lifetime issues:

```bash
cmake -B build \
    -DSNAP_RHI_THROW_ON_LIFETIME_VIOLATION=ON \
    -DSNAP_RHI_VALIDATION_DESTROY_OP=ON
```

This combination will:
1. **Throw an exception** when a resource lifetime contract is violated
2. **Validate destruction order** (e.g., DescriptorPool outlives DescriptorSets)

---

## Further Reading

- [SnapRHI Resource Management Guide](resource-management.md) — Lifetime rules, synchronization contracts
- [SnapRHI API Reference](about.md) — Complete public API documentation
- [Clang Sanitizers](https://clang.llvm.org/docs/AddressSanitizer.html)
- [Vulkan Validation Layers](https://vulkan.lunarg.com/)
- [RenderDoc](https://renderdoc.org/)
- [Xcode Metal Debugging](https://developer.apple.com/documentation/metal/debugging_tools)

---

*Last updated: 2026-02-20*
