# SnapRHI Resource Management Guide

This document defines the authoritative contracts for resource lifetime, reference retention, multithreading, synchronization, and memory management in SnapRHI. Understanding these rules is essential for building correct, high-performance graphics applications.

> **Target Audience:** Engine developers integrating SnapRHI. This guide assumes familiarity with modern graphics APIs (Vulkan, Metal, OpenGL).

---

## Table of Contents

1. [Resource Lifetime & Reference Retention](#1-resource-lifetime--reference-retention)
2. [Multithreading Rules](#2-multithreading-rules)
3. [Memory Hazard Tracking & Synchronization](#3-memory-hazard-tracking--synchronization)
4. [Memory Management](#4-memory-management)
5. [Resource Creation Contracts](#5-resource-creation-contracts)
6. [Best Practices & Performance Patterns](#6-best-practices--performance-patterns)

---

## 1. Resource Lifetime & Reference Retention

SnapRHI provides two resource retention modes controlled by `CommandBufferCreateFlags`. Understanding when SnapRHI retains references—and when it does not—is critical for avoiding use-after-free bugs.

### 1.1 CommandBuffer Retention Modes

| Mode | Flag | SnapRHI Behavior | Application Responsibility |
|------|------|------------------|---------------------------|
| **Retained** | `CommandBufferCreateFlags::None` | SnapRHI retains strong references to all resources bound to the CommandBuffer | Resources must be alive until **bound** to CommandBuffer |
| **Unretained** | `CommandBufferCreateFlags::UnretainedResources` | SnapRHI does **not** retain resource references | Resources (and CommandBuffer itself) must be alive until **GPU completes** all related work |

### 1.2 Per-Resource Retention Rules

#### Framebuffer

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ SnapRHI does NOT retain attachment texture references.                      │
│                                                                             │
│ Lifetime requirement:                                                       │
│   • CommandBufferCreateFlags::None                                          │
│       → Attachments must be alive until Framebuffer is bound to CommandBuffer│
│   • CommandBufferCreateFlags::UnretainedResources                           │
│       → Attachments must be alive until GPU finishes all related work       │
└─────────────────────────────────────────────────────────────────────────────┘
```

#### DescriptorPool

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ DescriptorPool must outlive ALL DescriptorSets allocated from it.          │
│                                                                             │
│ Destruction order:                                                          │
│   1. Destroy/release all DescriptorSets                                     │
│   2. Destroy DescriptorPool                                                 │
└─────────────────────────────────────────────────────────────────────────────┘
```

#### DescriptorSet

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ SnapRHI does NOT retain bound resource references.                          │
│                                                                             │
│ Lifetime requirement (applies to ALL bound resources AND DescriptorPool):   │
│   • CommandBufferCreateFlags::None                                          │
│       → Resources must be alive until DescriptorSet is bound to CommandBuffer│
│   • CommandBufferCreateFlags::UnretainedResources                           │
│       → Resources must be alive until GPU finishes all related work         │
└─────────────────────────────────────────────────────────────────────────────┘
```

#### CommandBuffer

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ Resource lifetime depends on CommandBufferCreateFlags:                       │
│                                                                             │
│ • CommandBufferCreateFlags::None (default):                                 │
│     Resources must be alive at least until they are first used/bound        │
│     in the CommandBuffer (SnapRHI retains refs after binding)               │
│                                                                             │
│ • CommandBufferCreateFlags::UnretainedResources:                            │
│     ALL resources AND the CommandBuffer itself must remain alive until      │
│     GPU finishes processing the CommandBuffer                               │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 1.3 Synchronization Primitive Reuse

#### Fence
- **Reuse condition:** Can only be reused after GPU has finished all work associated with it
- **Reset requirement:** Call `Fence::reset()` before reusing
- **Validation:** Check `Fence::getStatus() == FenceStatus::Completed` before reset

#### Semaphore
- **Reuse condition:** Can only be reused after GPU has finished all work associated with it
- **Signal/Wait pairing:** Each signal must be matched by exactly one wait before reuse

---

## 2. Multithreading Rules

SnapRHI is designed for multithreaded engines but requires explicit synchronization for most objects. The following table summarizes thread-safety guarantees:

| Object Type | Thread Safety | Notes |
|-------------|---------------|-------|
| **Device** | ✅ Thread-safe | Resource creation methods can be called from any thread |
| **CommandBuffer** | ❌ Not thread-safe | No simultaneous access from multiple threads allowed |
| **CommandQueue** | ❌ Not thread-safe | No simultaneous access from multiple threads allowed |
| **DescriptorPool** | ⚠️ Partially safe | Allocation of new DescriptorSets must be protected from simultaneous access |
| **Fence** | ⚠️ Conditional | Can only be reused after GPU finished all work |
| **Semaphore** | ⚠️ Conditional | Can only be reused after GPU finished all work |
| **All other resources** | ❌ Not thread-safe | No simultaneous access from multiple threads allowed |

### 2.1 Device (Thread-Safe)

```cpp
// Safe: Creating resources from multiple threads
std::thread t1([&device]() {
    auto buffer = device->createBuffer(bufferInfo);
});
std::thread t2([&device]() {
    auto texture = device->createTexture(textureInfo);
});
```

### 2.2 CommandQueue

```cpp
// UNSAFE: Simultaneous submission from multiple threads
// std::thread t1([&queue]() { queue->submitCommandBuffer(cb1); });
// std::thread t2([&queue]() { queue->submitCommandBuffer(cb2); }); // RACE!

// Safe: Serialize submissions
std::lock_guard<std::mutex> lock(queueMutex);
queue->submitCommandBuffer(commandBuffer);
```

**Multi-Queue Hazards:**
- When using multiple queues simultaneously, standard GPU resource hazards apply
- `waitForScheduled()` / `waitForComplete()` only affect commands submitted under the **current DeviceContext**
- Cross-queue synchronization requires explicit semaphores or fences

### 2.3 DescriptorPool

```cpp
// Allocation must be protected
std::lock_guard<std::mutex> lock(poolMutex);
auto descriptorSet = device->createDescriptorSet(setInfo);
```

---

## 3. Memory Hazard Tracking & Synchronization

SnapRHI adopts a **Metal/OpenGL-style automatic hazard tracking model** for most resource access patterns, minimizing the synchronization burden on application code. However, certain cross-encoder scenarios require explicit barriers.

### 3.1 Automatic Hazard Tracking (What SnapRHI Handles)

SnapRHI automatically tracks and resolves memory hazards for the following scenarios:

| Scenario | Automatic Sync | Notes |
|----------|----------------|-------|
| **Render → Render** | ✅ Yes | Sequential render passes on same resources are automatically synchronized |
| **Blit → Render** | ✅ Yes | Blit operations complete before dependent render passes |
| **Render → Blit** | ✅ Yes | Render output is visible to subsequent blit operations |
| **Blit → Blit** | ✅ Yes | Sequential blit operations are properly ordered |
| **Compute → Compute** | ✅ Yes | Sequential compute dispatches within an encoder are synchronized |
| **Texture layout transitions** | ✅ Yes | Vulkan backend handles image layout transitions automatically |

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ AUTOMATIC HAZARD TRACKING PRINCIPLE:                                        │
│                                                                             │
│ Within a single encoder type (Render, Blit, or Compute), SnapRHI            │
│ automatically ensures that:                                                 │
│   • All writes from previous commands are visible to subsequent reads       │
│   • Read-after-write (RAW) hazards are resolved                            │
│   • Write-after-read (WAR) hazards are resolved                            │
│   • Write-after-write (WAW) hazards are resolved                           │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 3.2 Manual Synchronization Required: Compute ↔ Render

> ⚠️ **CRITICAL:** Transitions between Compute and Render encoders require **explicit** `pipelineBarrier()` calls.

SnapRHI does **not** automatically synchronize:
- Compute shader writes read by subsequent render passes
- Render pass output read by subsequent compute shaders

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ MANUAL SYNC REQUIRED:                                                       │
│                                                                             │
│   Compute → Render: Use pipelineBarrier() before render pass               │
│   Render → Compute: Use pipelineBarrier() before compute dispatch          │
│                                                                             │
│ Failure to synchronize results in UNDEFINED BEHAVIOR:                       │
│   • Corrupted texture/buffer data                                           │
│   • Race conditions                                                         │
│   • GPU hangs (driver-dependent)                                            │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 3.3 Using pipelineBarrier()

The `pipelineBarrier()` method inserts explicit synchronization points:

```cpp
void pipelineBarrier(
    PipelineStageBits srcStageMask,      // Stages to wait for
    PipelineStageBits dstStageMask,      // Stages that will wait
    DependencyFlags dependencyFlags,      // Dependency modifiers
    std::span<MemoryBarrierInfo> memoryBarriers,
    std::span<BufferMemoryBarrierInfo> bufferMemoryBarriers,
    std::span<TextureMemoryBarrierInfo> textureMemoryBarriers
);
```

#### Example: Compute writes texture, Render reads it

```cpp
// 1. Compute pass writes to texture
auto computeEncoder = commandBuffer->getComputeCommandEncoder();
computeEncoder->beginEncoding();
computeEncoder->bindComputePipeline(computePipeline);
computeEncoder->bindDescriptorSet(0, computeDescriptorSet, {});
computeEncoder->dispatch(groupsX, groupsY, groupsZ);
computeEncoder->endEncoding();

// 2. Insert barrier: Compute → Render
auto renderEncoder = commandBuffer->getRenderCommandEncoder();
TextureMemoryBarrierInfo textureBarrier{
    .srcAccessMask = AccessFlags::ShaderWrite,
    .dstAccessMask = AccessFlags::ShaderRead,
    .texture = outputTexture,
    .subresourceRange = {.baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1}
};
renderEncoder->beginEncoding(renderPassInfo);
renderEncoder->pipelineBarrier(
    PipelineStageBits::ComputeShaderBit,      // Wait for compute
    PipelineStageBits::FragmentShaderBit,     // Before fragment shader reads
    DependencyFlags::ByRegion,
    {},                                        // No global barriers
    {},                                        // No buffer barriers
    {&textureBarrier, 1}                       // Texture barrier
);
// ... render commands ...
renderEncoder->endEncoding();
```

#### Example: Render writes to texture, Compute reads it

```cpp
// 1. Render pass writes to attachment
auto renderEncoder = commandBuffer->getRenderCommandEncoder();
renderEncoder->beginEncoding(renderPassInfo);
// ... render commands ...
renderEncoder->endEncoding();

// 2. Insert barrier: Render → Compute
auto computeEncoder = commandBuffer->getComputeCommandEncoder();
computeEncoder->beginEncoding();
TextureMemoryBarrierInfo textureBarrier{
    .srcAccessMask = AccessFlags::ColorAttachmentWrite,
    .dstAccessMask = AccessFlags::ShaderRead,
    .texture = renderTarget,
    .subresourceRange = {.baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1}
};
computeEncoder->pipelineBarrier(
    PipelineStageBits::ColorAttachmentOutputBit,  // Wait for render output
    PipelineStageBits::ComputeShaderBit,          // Before compute reads
    DependencyFlags::ByRegion,
    {},
    {},
    {&textureBarrier, 1}
);
computeEncoder->bindComputePipeline(postProcessPipeline);
// ... compute commands ...
computeEncoder->endEncoding();
```

### 3.4 Backend-Specific Behavior

| Backend | pipelineBarrier() Implementation |
|---------|----------------------------------|
| **Vulkan** | Translates directly to `vkCmdPipelineBarrier()` with full stage/access mask support |
| **Metal** | Uses `memoryBarrierWithResources:` / `memoryBarrierWithScope:` for resource-level synchronization. **Note:** On iOS, render encoder barriers have limited support; macOS 10.14+ is required for full functionality |
| **OpenGL** | Converts to `glMemoryBarrier()` / `glMemoryBarrierByRegion()` with appropriate barrier bits |

### 3.5 Access Flags Reference

Use these access flags to specify the type of memory access being synchronized:

| AccessFlags | Description | Typical Stage |
|-------------|-------------|---------------|
| `ShaderRead` | Shader read (sampled texture, UBO, SSBO) | Vertex/Fragment/Compute |
| `ShaderWrite` | Shader write (storage image, SSBO) | Vertex/Fragment/Compute |
| `ColorAttachmentRead` | Framebuffer color read | Color attachment output |
| `ColorAttachmentWrite` | Framebuffer color write | Color attachment output |
| `DepthStencilAttachmentRead` | Depth/stencil read | Early/late fragment tests |
| `DepthStencilAttachmentWrite` | Depth/stencil write | Early/late fragment tests |
| `TransferRead` | Copy/blit source | Transfer |
| `TransferWrite` | Copy/blit destination | Transfer |

### 3.6 Pipeline Stage Reference

| PipelineStageBits | Description |
|-------------------|-------------|
| `TopOfPipeBit` | Pseudo-stage: before any commands |
| `VertexShaderBit` | Vertex shader execution |
| `FragmentShaderBit` | Fragment shader execution |
| `EarlyFragmentTestsBit` | Early depth/stencil tests |
| `LateFragmentTestsBit` | Late depth/stencil tests |
| `ColorAttachmentOutputBit` | Color attachment writes |
| `ComputeShaderBit` | Compute shader execution |
| `TransferBit` | Copy/blit operations |
| `BottomOfPipeBit` | Pseudo-stage: after all commands |
| `AllGraphicsBit` | All graphics pipeline stages |
| `AllCommandsBit` | All pipeline stages |

### 3.7 Common Synchronization Patterns

#### Pattern 1: Compute → Render (most common)
```cpp
// Barrier in render encoder before sampling compute output
pipelineBarrier(
    PipelineStageBits::ComputeShaderBit,
    PipelineStageBits::FragmentShaderBit,
    DependencyFlags::ByRegion,
    {}, {}, {textureBarrier}
);
```

#### Pattern 2: Render → Compute
```cpp
// Barrier in compute encoder before reading render target
pipelineBarrier(
    PipelineStageBits::ColorAttachmentOutputBit,
    PipelineStageBits::ComputeShaderBit,
    DependencyFlags::ByRegion,
    {}, {}, {textureBarrier}
);
```

#### Pattern 3: Compute buffer write → Render vertex read
```cpp
BufferMemoryBarrierInfo bufferBarrier{
    .srcAccessMask = AccessFlags::ShaderWrite,
    .dstAccessMask = AccessFlags::VertexAttributeRead,
    .buffer = vertexBuffer,
    .offset = 0,
    .size = bufferSize
};
pipelineBarrier(
    PipelineStageBits::ComputeShaderBit,
    PipelineStageBits::VertexShaderBit,
    DependencyFlags::ByRegion,
    {}, {&bufferBarrier, 1}, {}
);
```

---

## 4. Memory Management

### 4.1 Memory Properties

SnapRHI exposes explicit memory properties for fine-grained control:

| Property | Description | Typical Use Case |
|----------|-------------|------------------|
| `DeviceLocal` | Optimal for GPU access | Static geometry, textures |
| `HostVisible` | CPU can map and access | Staging buffers, dynamic uniforms |
| `HostCoherent` | No explicit flush/invalidate needed | Convenience upload buffers |
| `HostCached` | CPU reads benefit from cache | Readback buffers |

**Guaranteed Availability:**
- `HostVisible | HostCoherent` — Always available on all backends
- `DeviceLocal` — Always available on all backends

### 4.2 Buffer Mapping Synchronization

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ CRITICAL: Mapped memory MUST be synchronized with GPU through fences.       │
│ Failure to synchronize results in UNDEFINED BEHAVIOR.                       │
│                                                                             │
│ Synchronization workflow:                                                   │
│   1. Submit GPU work that writes to buffer                                  │
│   2. Signal fence on submission                                             │
│   3. Wait for fence completion (CPU-side)                                   │
│   4. Invalidate mapped ranges (if non-coherent)                             │
│   5. Read data                                                              │
│                                                                             │
│ For CPU → GPU:                                                              │
│   1. Write data to mapped memory                                            │
│   2. Flush mapped ranges (if non-coherent)                                  │
│   3. Submit GPU work                                                        │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 4.3 MapUsage::WriteOnly Behavior

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ MapUsage::WriteOnly INVALIDATES the entire mapped range.                    │
│                                                                             │
│ If you need to modify only part of a buffer:                                │
│   • Use a precise subrange in the map() call, OR                            │
│   • Use MapUsage::ReadWrite to preserve existing data                       │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 4.4 UBO/SSBO Packing

SnapRHI does **not** enforce or expose specific rules for uniform/storage buffer packing. The packing layout is backend-dependent:

- **Vulkan:** std140 (UBO) / std430 (SSBO) unless specified otherwise
- **Metal:** Metal Shading Language packing rules
- **OpenGL:** std140 by default

**Application responsibility:** Match the buffer layout in C++ code with the layout declared in shader code.

---

## 5. Resource Creation Contracts

### 5.1 DescriptorSet Allocation

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ Device::createDescriptorSet() may return nullptr on allocation failure.    │
│                                                                             │
│ Fragmentation can cause allocation failures even when the pool has          │
│ sufficient total capacity. This matches Vulkan descriptor pool behavior.    │
│                                                                             │
│ Recovery strategy:                                                          │
│   • Create additional descriptor pools as needed                            │
│   • Consider pool sizing based on expected allocation patterns              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 5.2 DescriptorSetLayout

```cpp
// Bindings MUST be sorted by binding ID in monotonically increasing order
DescriptorSetLayoutCreateInfo layoutInfo;
layoutInfo.bindings = {
    {.binding = 0, .descriptorType = DescriptorType::UniformBuffer, ...},
    {.binding = 1, .descriptorType = DescriptorType::CombinedImageSampler, ...},
    {.binding = 2, .descriptorType = DescriptorType::StorageBuffer, ...},
    // ✅ Correct: 0, 1, 2 is monotonically increasing
};
```

### 5.3 PipelineLayout

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ All set slots up to the maximum binding MUST be filled.                     │
│ Use an empty layout (0 descriptors) for unused set indices.                 │
│                                                                             │
│ Example with sets 0 and 2 used:                                             │
│   setLayouts[0] = usedLayout0;                                              │
│   setLayouts[1] = emptyLayout;  // Placeholder for unused set 1             │
│   setLayouts[2] = usedLayout2;                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 5.4 DescriptorSet IDs

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ DescriptorSet IDs must be in [0, N).                                        │
│                                                                             │
│ IMPORTANT: For Metal, Argument buffer IDs must NOT intersect                │
│ with vertex buffer binding indices. Plan your binding layout accordingly.   │
│                                                                             │
│ See Metal-specific headers for configuration options.                       │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 5.5 OpenGL Specialization Constants

Specialization constants let you patch constant values into a shader at pipeline creation time. Vulkan and Metal have native support; on OpenGL, SnapRHI **emulates** them by injecting `#define` directives into the GLSL source before compilation.

#### Default naming convention

Each constant with ID *N* is injected as `#define SNAP_RHI_SPECIALIZATION_CONSTANT_N <value>`.

**Portable GLSL pattern:**
```glsl
#ifndef SNAP_RHI_SPECIALIZATION_CONSTANT_0
#define SNAP_RHI_SPECIALIZATION_CONSTANT_0 1  // Default when not overridden
#endif
const bool ENABLE_FEATURE = bool(SNAP_RHI_SPECIALIZATION_CONSTANT_0);
```

#### Custom preprocessor names

For more descriptive naming — or when porting Vulkan/SPIR-V shaders — provide custom macro names per constant via `glShaderModuleInfo`:

```cpp
// 1. Define specialization data (cross-backend)
struct MySpecData {
    uint32_t useNormalMap = 1;
    uint32_t maxLightCount = 8;
};
MySpecData specData;

snap::rhi::SpecializationMapEntry entries[] = {
    { .constantID = 0, .offset = offsetof(MySpecData, useNormalMap),  .format = SpecializationConstantFormat::Bool32 },
    { .constantID = 1, .offset = offsetof(MySpecData, maxLightCount), .format = SpecializationConstantFormat::UInt32 },
};

snap::rhi::SpecializationInfo specInfo{
    .mapEntryCount = 2,
    .pMapEntries   = entries,
    .dataSize      = sizeof(specData),
    .pData         = &specData,
};

// 2. (OpenGL only) Map constant IDs to custom GLSL macro names
snap::rhi::opengl::ShaderModuleInfo glInfo;
glInfo.specializationConstantNames = {
    {0, "USE_NORMAL_MAP"},     // → #define USE_NORMAL_MAP 1
    {1, "MAX_LIGHT_COUNT"},    // → #define MAX_LIGHT_COUNT 8
};

// 3. Create shader module
snap::rhi::ShaderModuleCreateInfo ci{};
ci.shaderStage        = snap::rhi::ShaderStage::Fragment;
ci.name               = "main";
ci.shaderLibrary      = myLibrary;
ci.specializationInfo = specInfo;
ci.glShaderModuleInfo = glInfo;  // Ignored by Metal and Vulkan backends
```

The corresponding GLSL would then use the custom names directly:

```glsl
#ifndef USE_NORMAL_MAP
#define USE_NORMAL_MAP 1
#endif

#ifndef MAX_LIGHT_COUNT
#define MAX_LIGHT_COUNT 8
#endif
```

#### Rules

| Rule | Detail |
|------|--------|
| **Custom names are optional** | Any constant ID without a custom name entry falls back to `SNAP_RHI_SPECIALIZATION_CONSTANT_<ID>` |
| **Valid identifiers only** | Custom names must be valid GLSL preprocessor identifiers (no spaces, no leading digits) |
| **Matched by ID** | Entries in `specializationConstantNames` are matched to `SpecializationMapEntry` by `constantID` |
| **Other backends** | The `glShaderModuleInfo` field is silently ignored by Metal and Vulkan |

> **See also:** [API Reference §5.1 — ShaderModule](about.md#51-shadermodule) for the full `ShaderModuleCreateInfo` specification and backend mapping table.

---

## 6. Best Practices & Performance Patterns

### 6.1 The Golden Rule: Persistent Mapping

Unlike OpenGL-style programming, **never** map/unmap every frame in modern graphics APIs.

```cpp
// ✅ RECOMMENDED: Persistent mapping
class DynamicBuffer {
    snap::rhi::Buffer* buffer;
    std::byte* persistentPtr;

public:
    DynamicBuffer(snap::rhi::Device* device, uint32_t size) {
        BufferCreateInfo info{
            .size = size,
            .bufferUsage = BufferUsage::UniformBuffer,
            .memoryProperties = MemoryProperties::HostVisible | MemoryProperties::HostCoherent
        };
        buffer = device->createBuffer(info);

        // Map once at creation, keep pointer forever
        persistentPtr = buffer->map(MemoryAccess::Write, 0, size);
    }

    void update(const void* data, uint32_t size) {
        // Direct write, no map/unmap overhead
        std::memcpy(persistentPtr, data, size);
    }

    ~DynamicBuffer() {
        buffer->unmap();  // Only unmap on destruction
    }
};
```

**Why:** `vkMapMemory` / `glMapBuffer` involve OS kernel calls. Keeping memory mapped has virtually zero overhead.

### 6.2 Staging Buffer Pattern (CPU → GPU)

For static data (meshes, textures) that doesn't change often:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ 1. Allocate staging buffer: HostVisible | HostCoherent                      │
│ 2. Map & copy CPU data into staging buffer                                  │
│ 3. Record CopyBuffer from staging → final buffer                       │
│    (Final buffer should be DeviceLocal for optimal GPU performance)         │
│ 4. Submit and wait for completion                                           │
│ 5. Delete or reuse staging buffer                                           │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 6.3 Ring Buffer Pattern (Streaming Data)

For data that changes every frame (UI, particles, dynamic uniforms):

```cpp
class RingBuffer {
    static constexpr uint32_t FRAME_COUNT = 3;
    static constexpr uint32_t FRAME_SIZE = 1024 * 1024;  // 1MB per frame

    snap::rhi::Buffer* buffer;
    std::byte* mappedPtr;
    uint32_t currentFrame = 0;
    std::array<snap::rhi::Fence*, FRAME_COUNT> frameFences;

public:
    struct Allocation {
        std::byte* ptr;
        uint32_t offset;
    };

    Allocation allocate(uint32_t size) {
        uint32_t frameOffset = currentFrame * FRAME_SIZE;
        // ... offset management within frame ...
        return {mappedPtr + frameOffset + localOffset, frameOffset + localOffset};
    }

    void nextFrame() {
        currentFrame = (currentFrame + 1) % FRAME_COUNT;

        // Wait for GPU to finish with this frame's data before overwriting
        if (frameFences[currentFrame]->getStatus() != FenceStatus::Completed) {
            frameFences[currentFrame]->waitForComplete();
        }
        frameFences[currentFrame]->reset();
    }
};
```

**Benefit:** CPU never waits for GPU while writing new frame data.

### 6.4 GPU → CPU Readback

> ⚠️ **Performance Warning:** GPU readback is the biggest performance trap in graphics programming.

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ NEVER read directly from HostVisible-only memory.                           │
│ Uncached reads trigger slow PCIe bus round-trips for every byte.            │
│                                                                             │
│ CORRECT workflow:                                                           │
│   1. GPU writes to DeviceLocal image/buffer                                 │
│   2. GPU copies to HostVisible | HostCached buffer                          │
│   3. CPU waits for Fence                                                    │
│   4. CPU calls invalidateMappedMemoryRanges() (if not coherent)             │
│   5. CPU reads mapped pointer (fast: hits CPU L1/L2 cache)                  │
└─────────────────────────────────────────────────────────────────────────────┘
```

```cpp
// ✅ Correct readback buffer creation
BufferCreateInfo readbackInfo{
    .size = dataSize,
    .bufferUsage = BufferUsage::CopyDst,
    .memoryProperties = MemoryProperties::HostVisible | MemoryProperties::HostCached
};
auto readbackBuffer = device->createBuffer(readbackInfo);
```

### 6.5 Memory Type Selection Summary

| Use Case | Recommended Properties | Notes |
|----------|----------------------|-------|
| Static GPU data | `DeviceLocal` | Fastest GPU access |
| Dynamic uniforms | `HostVisible \| HostCoherent` | Persistent map, no flush needed |
| Staging upload | `HostVisible \| HostCoherent` | One-shot transfers |
| GPU readback | `HostVisible \| HostCached` | CPU cache benefits |
| Streaming data | `HostVisible \| HostCoherent` | Ring buffer pattern |

---

*Last updated: 2026-02-20*
