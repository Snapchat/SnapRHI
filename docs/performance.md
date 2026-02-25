# Performance Design

SnapRHI is designed for **low-latency, high-throughput** rendering workloads.
Every abstraction is evaluated against the cost of the underlying API call it
wraps. This document explains the architectural decisions that minimize overhead
and how to verify performance in your own workloads.

> **Related:** [Profiling Guide](profiling.md) | [Resource Management](resource-management.md) | [API Overview](about.md)

---

## Table of Contents

1. [Design Principles](#1-design-principles)
2. [Per-Backend Optimizations](#2-per-backend-optimizations)
3. [Overhead Budget](#3-overhead-budget)
4. [Validation Cost Model](#4-validation-cost-model)
5. [Build Configurations for Performance Work](#5-build-configurations-for-performance-work)
6. [Measuring Performance](#6-measuring-performance)

---

## 1. Design Principles

### 1.1 No Hidden Allocations on the Hot Path

Command recording, descriptor binding, and draw/dispatch calls **never allocate
heap memory**. All transient storage uses backend-specific strategies:

| Backend | Hot-Path Allocation Strategy |
|---------|----------------------------|
| **Metal** | Pooled `ResourceBatcher` instances (acquired/released per encoder) with pre-sized vectors (256 initial entries). Descriptor sub-allocation from a single `MTLBuffer`. |
| **Vulkan** | Pre-allocated `VkCommandPool` array (64 slots). `VkDescriptorPool` block allocation. |
| **OpenGL** | Linear command allocator (arena) inspired by Dawn. Commands are placement-new'd into a contiguous buffer — zero individual allocations during recording. |

The OpenGL command allocator enforces this at the type level — all command
structs must be **trivially destructible** (verified by `static_assert`),
so the entire recording buffer can be freed in one operation without walking
individual destructors.

### 1.2 Compile-Switchable Validation

Every validation check in SnapRHI is guarded by an `if constexpr` test against
a compile-time bitmask of enabled validation tags:

```cpp
#define SNAP_RHI_VALIDATE(layer, condition, reportLevel, tags, ...)                     \
    if constexpr ((static_cast<uint64_t>(tags) & snap::rhi::EnabledValidationTags) &&   \
                  (static_cast<uint64_t>(reportLevel) >= SNAP_RHI_ENABLED_REPORT_LEVEL)) { ...
```

When a tag is disabled (the default for Release builds), the compiler
**dead-code-eliminates** the entire check — not branched over at runtime, but
absent from the binary. Over 30 individual validation tags can be toggled
independently, or all at once via `SNAP_RHI_ENABLE_ALL_VALIDATION`.

```
Release build:   record → encode → submit        (validation code is not in the binary)
Debug + validation: record → validate → encode → submit
```

See [§4 — Validation Cost Model](#4-validation-cost-model) for details.

### 1.3 Aggressive Resource Pooling and Reuse

SnapRHI pools and recycles expensive objects rather than creating and destroying
them per-frame:

| Resource | Pooling Mechanism |
|----------|------------------|
| **Fences** | `FencePool` — acquires/releases reusable internal fences across all backends |
| **Command buffers (OpenGL)** | `CommandBufferPool` — maintains free/used lists, resets rather than re-allocates |
| **Command pools (Vulkan)** | Fixed-size pool of 64 `VkCommandPool` instances with availability queue |
| **Framebuffers (OpenGL)** | `FramebufferPool` — caches GL framebuffer objects by hashed description |
| **Descriptor memory (Metal)** | Sub-allocates from a single `MTLBuffer` with atomic block tracking |
| **Resource batchers (Metal)** | Pooled `ResourceBatcher` instances with acquire/release lifecycle |

### 1.4 Two Retention Modes

Applications can opt out of automatic reference retention for maximum
throughput. `CommandBufferCreateFlags::UnretainedResources` skips the
`ResourceResidencySet` bookkeeping entirely — the application guarantees
resource lifetimes manually, and SnapRHI adds zero overhead.

See [Resource Management — Retention Modes](resource-management.md#11-commandbuffer-retention-modes)
for the full contract.

---

## 2. Per-Backend Optimizations

### 2.1 Metal

| Technique | Details |
|-----------|---------|
| **Descriptor sub-allocation** | `DescriptorPool` sub-allocates from a single `MTLBuffer` with per-block atomic tracking — no per-set `MTLBuffer` creation. |
| **Resource usage batching** | `ResourceBatcher` aggregates `useResource:` calls with pre-sized buckets (256 initial), plus a `_lastBucket` cache for O(1) consecutive-add fast path. |
| **Pooled batchers** | Metal `Device` maintains a pool of `ResourceBatcher` instances — zero allocation during encoder setup. |

### 2.2 Vulkan

| Technique | Details |
|-----------|---------|
| **Pre-allocated command pools** | Fixed array of 64 `VkCommandPool` objects with a `std::queue` for O(1) acquire/release. |
| **Descriptor pool allocation** | Standard `VkDescriptorPool` block allocation — set creation is a fast pool sub-allocation. |

### 2.3 OpenGL

| Technique | Details |
|-----------|---------|
| **Linear command allocator** | Arena-style allocator that grows by doubling (starting at 2 KB). Commands are POD structs, placement-new'd sequentially. Iterated via `CommandIterator` without any virtual dispatch. |
| **Comprehensive state cache** | `StateCache` tracks all GL state (buffers, textures, samplers, programs, framebuffers, viewport, depth, stencil, blending, vertex attributes, feature toggles). Every `set*()` call checks the cache first via `inline` `should*()` methods — redundant `gl*` calls are eliminated before reaching the driver. |
| **Framebuffer cache** | `FramebufferPool` caches `GLuint` framebuffer objects keyed by a hashed attachment description — identical render pass configurations reuse existing FBOs. |
| **Command buffer pool** | `CommandBufferPool` recycles command buffers via free/used lists — `reset()` instead of delete + new. |
| **No `glGet*` calls** | The state cache explicitly avoids `glGet*` queries during rendering, which would force a GPU→CPU sync and stall the pipeline. All state is tracked CPU-side. |

---

## 3. Overhead Budget

This section characterizes where SnapRHI adds overhead versus a raw backend
call. The analysis is split by operation category and distinguishes between
backends where behavior differs materially.

### 3.1 Command Recording — Render & Compute Encoders

These operations are called many times per frame (inner-loop hot path).

| Operation | Metal | Vulkan | OpenGL |
|-----------|-------|--------|--------|
| **`draw` / `drawIndexed`** | Virtual call → `pipelineState.setAllStates()` (deferred descriptor bind via Obj-C messages) → native draw | Virtual call → `bindDescriptorSets()` (deferred `vkCmdBindDescriptorSets`) → native draw | Virtual call → placement-new `DrawCmd` into arena (no heap alloc). Actual GL calls happen later during replay. |
| **`bindRenderPipeline`** | Virtual call → `static_cast` → stores pipeline pointer + sets depth bias via Obj-C message → `resourceResidencySet.track()` | Virtual call → `static_cast` → `vkCmdBindPipeline` → `resourceResidencySet.track()` | Virtual call → placement-new `SetRenderPipelineCmd` into arena → `resourceResidencySet.track()` |
| **`bindDescriptorSet`** | Virtual call → `static_cast` → stores in fixed-size array (compared against previous, no native call until draw) | Virtual call → `static_cast` → stores `VkDescriptorSet` handle + copies dynamic offsets into `std::vector` (no native call until draw) | Virtual call → placement-new `BindDescriptorSetCmd` + copies dynamic offsets into fixed-size array → `collectReferences()` |
| **`bindVertexBuffer`** | Virtual call → `static_cast` → stores pointer (deferred) → `track()` | Virtual call → `static_cast` → calls `bindVertexBuffers` → **heap-allocates** `std::vector<VkBuffer>` + `std::vector<VkDeviceSize>` → `vkCmdBindVertexBuffers` → `track()` | Virtual call → placement-new `SetVertexBufferCmd` → `track()` |
| **`bindVertexBuffers`** _(multi)_ | Virtual call → loop of `static_cast` + store → `track()` per buffer | Same as single — **heap alloc** for temp vectors | Virtual call → placement-new `SetVertexBuffersCmd` → `track()` per buffer |
| **`bindIndexBuffer`** | Virtual call → `static_cast` → stores pointer + offset (no native call) → `track()` | Virtual call → `static_cast` → `vkCmdBindIndexBuffer` → `track()` | Virtual call → placement-new `SetIndexBufferCmd` → `track()` |
| **`setViewport`** | Virtual call → struct conversion → `[encoder setViewport:]` | Virtual call → `static_cast` → `vkCmdSetViewport` + `vkCmdSetScissor` | Virtual call → placement-new `SetViewportCmd`. During replay: `gl.viewport()` through state cache `shouldViewport()` |
| **`setDepthBias`** | Virtual call → stores values → conditional `[encoder setDepthBias:...]` | Virtual call → `static_cast` → `vkCmdSetDepthBias` | Virtual call → placement-new `SetDepthBiasCmd` |
| **`setStencilReference`** | Virtual call → stores values → `[encoder setStencilFrontReferenceValue:...]` | Virtual call → `static_cast` → `vkCmdSetStencilReference` | Virtual call → placement-new `SetStencilReferenceCmd` |
| **`setBlendConstants`** | Virtual call → `[encoder setBlendColorRed:...]` | Virtual call → `static_cast` → `vkCmdSetBlendConstants` | Virtual call → placement-new `SetBlendConstantsCmd` |

#### Key observations

- **Metal and Vulkan call native APIs inline** during encoding (no deferred
  command buffer). Metal defers _descriptor set binding_ until draw time via
  `setAllStates()`. Vulkan defers descriptor set binding until draw via
  `bindDescriptorSets()`.
- **OpenGL records all commands into a linear allocator** (arena). The actual
  `gl*` calls happen during `replayCommands()` at submit time, where the
  `RenderCmdPerformer` walks the arena via `CommandIterator` (no virtual
  dispatch during replay — just a `switch` on command IDs).
- **Vulkan `bindVertexBuffers` allocates temporary `std::vector`s** on each
  call to transform handle types. This is a known overhead for the multi-buffer
  path. The single-buffer path (`bindVertexBuffer`) still goes through
  `bindVertexBuffers` internally with a span of size 1.

### 3.2 Resource Retention (`track()`)

Every bind/record operation calls `resourceResidencySet.track()` per resource.
In **retained mode** (default), this:

1. Calls `device->resolveResource(ptr)` which does a **mutex-locked**
   `std::unordered_map::find` in `DeviceResourceRegistry` and returns a
   `shared_ptr` (weak → lock).
2. Pushes the `shared_ptr` into a `std::vector` (amortized O(1), but involves
   an atomic ref-count increment).

In **unretained mode** (`UnretainedResources` flag), `track()` is a no-op in
Release builds — zero overhead. In debug builds with slow validations, it
records a `weak_ptr` for lifetime checking instead.

| Retention Mode | Per-`track()` Cost |
|----------------|-------------------|
| **Retained** (default) | Mutex lock + hash lookup + `shared_ptr` copy (atomic increment) + `vector::push_back` |
| **Unretained** (Release) | **Zero** — early return, no work |
| **Unretained** (Debug + slow validations) | Mutex lock + hash lookup + `weak_ptr` copy |

> **Implication:** For maximum throughput, use `UnretainedResources` and manage
> lifetimes manually. This eliminates the per-operation mutex acquisition on the
> hot path.

### 3.3 Encoder Lifecycle (Begin / End)

| Phase | Metal | Vulkan | OpenGL |
|-------|-------|--------|--------|
| **`beginEncoding` (RenderPass)** | Creates `MTLRenderPassDescriptor` → `[commandBuffer renderCommandEncoderWithDescriptor:]` → tracks attachments | Tracks attachments → image layout transitions via `ImageLayoutManager` (may emit `vkCmdPipelineBarrier`) → **heap-allocates** `std::vector<VkClearValue>` → `vkCmdBeginRenderPass` | Placement-new `BeginRenderPassCmd` into arena → tracks attachments |
| **`beginEncoding` (Dynamic Rendering)** | Creates `MTLRenderPassDescriptor` → native encoder | Tracks attachments → image layout transitions → **heap-allocates** `std::vector<VkRenderingAttachmentInfo>` → `vkCmdBeginRendering` | Placement-new `BeginRenderPass1Cmd` into arena |
| **`endEncoding`** | `[encoder endEncoding]` → clears pipeline state | `vkCmdEndRenderPass` or `vkCmdEndRendering` → resets descriptor state | Placement-new `EndRenderPassCmd` |

> **Vulkan `beginEncoding` has the highest per-call overhead** due to
> image layout management (`ImageLayoutManager` maintains a per-image
> `std::unordered_map` of layout state and may emit pipeline barriers) and
> temporary vector allocations for clear values and attachment info.

### 3.4 Command Buffer Submission

| Step | Cost | Notes |
|------|------|-------|
| `submissionTracker.tryReclaim()` | Mutex lock + linear scan of in-flight submissions | Reclaims completed slots; bounded by max in-flight count |
| Texture interop processing | Per-interop-texture work | Only when interop textures are present |
| Fence acquisition (via `buildFence`) | `FencePool::acquireFence()` — mutex lock + linear scan for reusable fence | Pool avoids `createFence` on hot path |
| Semaphore wait/signal | Per-semaphore native wait/signal | Metal: Obj-C dispatch. Vulkan: native semaphore handles. OpenGL: `glFenceSync` |
| Native submit | `[mtlCommandBuffer commit]` / `vkQueueSubmit` / GL command replay | Core driver cost |
| `submissionTracker.track()` | Mutex lock + `resolveResource` (mutex + hash lookup) × 2 + push into slot | Only in retained mode |
| **OpenGL-specific: `replayCommands()`** | Mutex lock on queue → reset GL state cache → iterate arena via `CommandIterator` (`switch` dispatch) → `RenderCmdPerformer` calls `gl*` through state cache | The GL submit is where actual GPU commands are issued. State cache `should*()` checks (inline comparisons) filter redundant calls. |

### 3.5 Resource Creation

All creation methods go through `Device::createResource<>()`, which:

1. `new` — heap-allocates the backend implementation object.
2. Wraps in `std::shared_ptr` with a **custom deleter** that will call
   `DeviceResourceRegistry::erase()` on destruction.
3. `DeviceResourceRegistry::insert()` — **mutex lock** + `std::unordered_map::insert`.
4. Backend-specific initialization (native API object creation, struct
   translation).

| Operation | Additional Backend Cost |
|-----------|----------------------|
| **Create texture** | Metal: `[device newTextureWithDescriptor:]`. Vulkan: `vkCreateImage` + `vkAllocateMemory` + `vkBindImageMemory` + image view creation. OpenGL: `glGenTextures` + `glTexStorage*`. |
| **Create buffer** | Metal: `[device newBufferWithLength:]`. Vulkan: `vkCreateBuffer` + `vkAllocateMemory` + `vkBindBufferMemory`. OpenGL: `glGenBuffers` + `glBufferData`. |
| **Create render pipeline** | Metal: `[device newRenderPipelineStateWithDescriptor:]` (may compile shaders). Vulkan: `vkCreateGraphicsPipelines` (may compile shaders). OpenGL: `glLinkProgram` + state translation. |
| **Create sampler** | Metal: `[device newSamplerStateWithDescriptor:]`. Vulkan: `vkCreateSampler`. OpenGL: `glGenSamplers` + `glSamplerParameter*`. |
| **Create descriptor set** | Metal: argument buffer sub-allocation from `DescriptorPool`. Vulkan: `vkAllocateDescriptorSets` from pool. OpenGL: stores binding metadata. |
| **Create command buffer** | Metal: `[commandQueue commandBuffer]` + encoder allocation. Vulkan: `vkAllocateCommandBuffers` from pre-allocated `CommandPool`. OpenGL: allocates `CommandAllocator` arena (initial 2 KB). |

> **Pipeline creation is the most expensive operation** — it may trigger shader
> compilation. Use `PipelineCache` to amortize this cost.

### 3.6 Resource Destruction

Destruction happens when the last `shared_ptr` reference is released:

1. Custom deleter calls `DeviceResourceRegistry::erase()` — **mutex lock** +
   `std::unordered_map::erase`.
2. Backend destructor releases native API objects.
3. (Slow validations only) Validates resource lifetimes across in-flight
   submissions.

### 3.7 Descriptor Set Updates

Descriptor writes (`bindUniformBuffer`, `bindTexture`, `bindSampler`,
`updateDescriptorSet`) are virtual calls into backend-specific logic:

- **Metal:** Writes into argument buffer memory (sub-allocated from
  `DescriptorPool`). No native API call — direct memory writes.
- **Vulkan:** Calls `vkUpdateDescriptorSets` with translated write structs.
- **OpenGL:** Stores binding metadata into internal arrays (applied during
  replay).

### 3.8 Buffer Map / Unmap

- **Metal:** `[buffer contents]` + offset — returns pointer to shared memory.
- **Vulkan:** `vkMapMemory` / `vkUnmapMemory` — may involve driver
  synchronization.
- **OpenGL:** `glMapBufferRange` / `glUnmapBuffer`.

No additional SnapRHI overhead beyond the virtual call + `static_cast`.

### 3.9 Validation

| Build Configuration | Cost |
|--------------------|------|
| **Validation disabled** (Release default) | **Zero** — `if constexpr` eliminates all validation code from the binary |
| **Validation enabled** (selective tags) | Per-enabled-tag checks at each guarded call site |
| **Slow validations** (`SNAP_RHI_ENABLE_SLOW_VALIDATIONS`) | Additional per-`track()` `weak_ptr` recording + per-submit lifetime validation scan |

### 3.10 Summary Table

| Operation Category | Overhead vs. Raw API | Dominant Cost |
|-------------------|---------------------|---------------|
| Draw / dispatch | Virtual call + deferred descriptor flush | ~1–2 ns virtual call; descriptor flush is O(bound sets) |
| Dynamic state (viewport, depth bias, etc.) | Virtual call | Metal/Vulkan: inline native call. OpenGL: arena write (replay adds state cache check) |
| Bind vertex/index buffer | Virtual call + `track()` | Retained mode: mutex + hash lookup per resource. Vulkan multi-bind: temp vector alloc |
| Bind descriptor set | Virtual call + store | Deferred — no native call until draw |
| Begin/end render pass | Virtual call + attachment setup | Vulkan: image layout transitions + temp allocs. Metal: Obj-C descriptor creation. OpenGL: arena write |
| Submit command buffer | Fence pool + submission tracker | Mutex locks (fence pool, submission tracker, GL queue). OpenGL: full command replay |
| Resource creation | `shared_ptr` + registry insert | Mutex lock + native API object creation. Pipelines: may compile shaders |
| Resource destruction | `shared_ptr` custom deleter | Mutex lock + registry erase + native API release |
| Descriptor writes | Virtual call + backend write | Metal: memory write. Vulkan: `vkUpdateDescriptorSets`. OpenGL: store metadata |

> **Note on virtual dispatch:** SnapRHI uses virtual functions for backend
> polymorphism. On modern CPUs, a predicted virtual call costs ~1–2 ns. This is
> the cost of a **uniform, type-safe** multi-backend API. Applications that
> build against a single backend can expect the compiler to devirtualize `final`
> class methods in many cases (all backend classes are marked `final`).
>
> **Note on mutexes:** The `DeviceResourceRegistry` mutex appears on every
> `track()` call in retained mode and every resource create/destroy. In
> practice, these are uncontended single-thread locks (nanosecond cost). For
> maximum throughput on the recording hot path, use `UnretainedResources` to
> bypass this entirely.

---

## 4. Validation Cost Model

### 4.1 How It Works

Each validation check is tagged with one or more `ValidationTag` values (30+
tags covering `CreateOp`, `DestroyOp`, `RenderCommandEncoderOp`,
`CommandBufferOp`, `TextureOp`, `BufferOp`, etc.). Tags are aggregated at
build time into a `constexpr uint64_t EnabledValidationTags` bitmask.

The `SNAP_RHI_VALIDATE` macro uses `if constexpr` to test the bitmask.
When a tag is disabled, the compiler removes the entire validation block —
including the string literals, format arguments, and lambda captures used
by the check.

### 4.2 Cost Summary

| Build Configuration | Validation Cost | Binary Size Impact |
|--------------------|-----------------|-------------------|
| Release (all tags OFF) | **Zero** — code is not compiled in | None |
| Debug, tags OFF | **Zero** — same elimination | None |
| Debug, selective tags ON | Per-enabled-tag checks only | Small |
| `SNAP_RHI_ENABLE_ALL_VALIDATION` | All checks active + Vulkan layers + slow safety checks | Significant — debug only |

### 4.3 Fine-Grained Control

You don't have to choose between "all validation" and "none". Enable only the
tags you need:

```cmake
cmake -B build \
    -DSNAP_RHI_ENABLE_VULKAN=ON \
    -DSNAP_RHI_VALIDATION_RENDER_COMMAND_ENCODER_OP=ON \
    -DSNAP_RHI_VALIDATION_TEXTURE_OP=ON \
    -DCMAKE_BUILD_TYPE=Debug
```

This enables validation for render encoder and texture operations only —
everything else compiles to zero cost.

---

## 5. Build Configurations for Performance Work

| Goal | Preset / Flags | Notes |
|------|----------------|-------|
| **Profile** (real-world perf) | `macos-metal-release` / `--release` | No validation, compiler optimizations enabled |
| **Debug** (correctness) | `macos-metal-demo` | Debug labels + logs, no validation overhead |
| **Full validation** | `macos-metal-demo-validation` | All checks enabled — expect slower execution |
| **Selective validation** | Raw CMake with individual `SNAP_RHI_VALIDATION_*` flags | Surgical debugging with minimal perf impact |

> **Always profile in Release builds.** Debug builds include debug labels,
> logging, and potentially unoptimized code that does not represent production
> performance. See the [Profiling Guide](profiling.md) for tool-specific setup.

---

## 6. Measuring Performance

### 6.1 GPU Timestamp Queries

SnapRHI provides cross-backend GPU timestamp queries via `QueryPool`:

```cpp
auto queryPool = device->createQueryPool({ .queryCount = 2 });

commandBuffer->resetQueryPool(queryPool, 0, 2);
commandBuffer->writeTimestamp(queryPool, 0);  // start
// ... render work ...
commandBuffer->writeTimestamp(queryPool, 1);  // end

auto results = queryPool->getResults(0, 2);
auto gpuTime = results[1] - results[0];  // std::chrono::nanoseconds
```

### 6.2 Custom Profiling Labels

Enable `SNAP_RHI_ENABLE_CUSTOM_PROFILING_LABELS` to inject scoped markers:

```cpp
SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "Shadow Pass");
// ... rendering code — label is automatically popped at scope exit
```

These labels appear in platform GPU profilers (Xcode Instruments, RenderDoc,
Nsight, AGI).

### 6.3 Platform Profiling Tools

| Platform | Recommended Tool | What It Measures |
|----------|-----------------|-----------------|
| macOS / iOS | Xcode Instruments (Metal System Trace) | GPU timeline, shader execution, memory bandwidth |
| Windows | PIX, NVIDIA Nsight, RenderDoc | GPU timeline, draw call breakdown, resource usage |
| Linux | RenderDoc, NVIDIA Nsight | Frame capture, pipeline state, GPU counters |
| Android | AGI (Android GPU Inspector), Perfetto | GPU counters, frame pacing, memory |

> **Full details:** [Profiling Guide](profiling.md)

---

## Further Reading

- [Profiling Guide](profiling.md) — Platform-specific profiling tools and workflows
- [Resource Management](resource-management.md) — Lifetime rules, retention modes, memory patterns
- [API Overview](about.md) — Full API reference and design philosophy
- [Debugging Guide](debugging.md) — Validation layers, sanitizers, diagnostics
