# SnapRHI Public API Reference

This document provides the authoritative reference for the SnapRHI public API surface (`src/public-api/include/snap/rhi/`). It serves as the primary integration guide for engine developers.

> **Related Documentation:**
> - [Resource Management Guide](resource-management.md) — Lifetime rules, threading, memory patterns
> - [Performance Design](performance.md) — Overhead budget, per-backend optimizations
> - [Build Instructions](build.md) — Build system and toolchain setup
> - [Debugging Guide](debugging.md) — Validation layers, sanitizers, diagnostics
> - [Profiling Guide](profiling.md) — Performance analysis tools and workflows

---

## Table of Contents

1. [Architecture Overview](#1-architecture-overview)
2. [Core Interfaces](#2-core-interfaces)
3. [Resource Types](#3-resource-types)
4. [Binding & Descriptor System](#4-binding--descriptor-system)
5. [Pipeline & Shader System](#5-pipeline--shader-system)
6. [Command Recording](#6-command-recording)
7. [Synchronization Primitives](#7-synchronization-primitives)
8. [Utility & Diagnostics](#8-utility--diagnostics)
9. [Usage Patterns](#9-usage-patterns)

---

## 1. Architecture Overview

### 1.1 Design Philosophy

SnapRHI is a **low-level, explicit** graphics abstraction designed for high-performance rendering engines. It exposes primitives similar to Vulkan/Metal/D3D12, giving engine authors explicit control over:

- Resource allocation and memory placement
- Command buffer recording and submission
- Synchronization and resource lifetime
- Pipeline state and shader compilation

**Trade-off:** Explicit control requires explicit responsibility. See [Resource Management Guide](resource-management.md) for lifetime and synchronization contracts.

### 1.2 API Contract

| Aspect | Contract |
|--------|----------|
| **Inputs** | `*CreateInfo` structs configure resource creation; encoder objects record commands |
| **Outputs** | GPU resources (buffers, textures, pipelines); async sync objects (fence/semaphore) |
| **Errors** | Return `nullptr` on allocation failure; throw `snap::rhi::Exception` for critical errors; runtime callbacks via `DebugMessenger` |
| **Success** | Resources create successfully; submissions complete without validation errors on compatible backends |

### 1.3 Backend Support Matrix

| Backend | Platforms | API Version |
|---------|-----------|-------------|
| **Metal** | macOS, iOS | Metal 2+ |
| **Vulkan** | Windows, Linux, Android, macOS/iOS (MoltenVK) | Vulkan 1.0+ |
| **OpenGL / OpenGL ES** | Windows, Linux, macOS, Android, iOS, Web | OpenGL 2.1+ / OpenGL ES 2.0+ |
| **NoOp** | All | N/A (stub backend for testing) |

---

## 2. Core Interfaces

### 2.1 Device

**Header:** `Device.hpp`

The `Device` is the central factory for all GPU resources and the entry point for capability queries.

```cpp
class Device : public std::enable_shared_from_this<Device> {
    // Resource creation (thread-safe)
    std::shared_ptr<Buffer> createBuffer(const BufferCreateInfo& info);
    std::shared_ptr<Texture> createTexture(const TextureCreateInfo& info);
    std::shared_ptr<Texture> createTexture(const std::shared_ptr<TextureInterop>& interop);
    std::shared_ptr<Texture> createTextureView(const TextureViewCreateInfo& info);  // May throw on OpenGL
    std::shared_ptr<Sampler> createSampler(const SamplerCreateInfo& info);
    std::shared_ptr<Framebuffer> createFramebuffer(const FramebufferCreateInfo& info);
    std::shared_ptr<RenderPass> createRenderPass(const RenderPassCreateInfo& info);
    std::shared_ptr<CommandBuffer> createCommandBuffer(const CommandBufferCreateInfo& info);

    // Shader & pipeline creation
    std::shared_ptr<ShaderLibrary> createShaderLibrary(const ShaderLibraryCreateInfo& info);
    std::shared_ptr<ShaderModule> createShaderModule(const ShaderModuleCreateInfo& info);
    std::shared_ptr<RenderPipeline> createRenderPipeline(const RenderPipelineCreateInfo& info);   // May return nullptr
    std::shared_ptr<ComputePipeline> createComputePipeline(const ComputePipelineCreateInfo& info); // May return nullptr
    std::shared_ptr<PipelineLayout> createPipelineLayout(const PipelineLayoutCreateInfo& info);
    std::shared_ptr<PipelineCache> createPipelineCache(const PipelineCacheCreateInfo& info);

    // Descriptor system
    std::shared_ptr<DescriptorSetLayout> createDescriptorSetLayout(const DescriptorSetLayoutCreateInfo& info);
    std::shared_ptr<DescriptorPool> createDescriptorPool(const DescriptorPoolCreateInfo& info);
    std::shared_ptr<DescriptorSet> createDescriptorSet(const DescriptorSetCreateInfo& info);  // May return nullptr

    // Synchronization
    std::shared_ptr<Fence> createFence(const FenceCreateInfo& info);
    std::shared_ptr<Semaphore> createSemaphore(const SemaphoreCreateInfo& info,
                                                const std::shared_ptr<PlatformSyncHandle>& = nullptr);

    // Query and diagnostics
    std::shared_ptr<QueryPool> createQueryPool(const QueryPoolCreateInfo& info);
    std::shared_ptr<DebugMessenger> createDebugMessenger(DebugMessengerCreateInfo&& info);  // May return nullptr
    const Capabilities& getCapabilities() const;
    TextureFormatProperties getTextureFormatProperties(const TextureFormatInfo& info);
    CommandQueue* getCommandQueue(uint32_t queueFamilyIndex, uint32_t queueIndex);

    // Context management (primarily for OpenGL backends)
    DeviceContext* getMainDeviceContext();
    std::shared_ptr<DeviceContext> createDeviceContext(const DeviceContextCreateInfo& info);
    [[nodiscard]] DeviceContext::Guard setCurrent(DeviceContext* ptr);
    DeviceContext* getCurrentDeviceContext();

    // Lifetime & memory
    void waitForTrackedSubmissions();   // Block until all tracked GPU work completes
    uint64_t getCPUMemoryUsage() const;
    uint64_t getGPUMemoryUsage() const;
    DeviceMemorySnapshot captureMemorySnapshot() const;
};
```

**Thread Safety:** Resource creation methods are thread-safe. Returned objects have their own thread-safety rules. Context management methods (`setCurrent`, `getMainDeviceContext`, etc.) are **not** thread-safe.

**Lifetime:** All resources must be released before destroying the Device. Call `waitForTrackedSubmissions()` before destruction to ensure no in-flight GPU work references live resources.

**Error model:**
- `createDescriptorSet()` may return `nullptr` on pool fragmentation.
- `createRenderPipeline()` / `createComputePipeline()` may return `nullptr` when `NullOnPipelineCacheMiss` is set and the cache misses.
- `createTextureView()` may throw on backends that don't support views (e.g., OpenGL).
- `createDebugMessenger()` may return `nullptr` if unsupported by the current backend.

### 2.2 DeviceContext

**Header:** `DeviceContext.hpp`

Higher-level context attached to a Device, primarily used by OpenGL backends for context management.

```cpp
class DeviceContext {
    class Guard;  // RAII guard for context binding
};

// Usage pattern
auto guard = device->setCurrent(context);  // Makes context current
// ... use context ...
// guard destructor restores previous context
```

**Thread Safety:** Not thread-safe. Context binding is per-thread.

### 2.3 CommandQueue

**Header:** `CommandQueue.hpp`

Submission interface for command buffers.

```cpp
class CommandQueue : public DeviceChild {
    void submitCommands(
        std::span<Semaphore*> waitSemaphores,
        std::span<const PipelineStageBits> waitDstStageMask,
        std::span<CommandBuffer*> buffers,
        std::span<Semaphore*> signalSemaphores,
        CommandBufferWaitType waitType,
        Fence* fence
    );

    void submitCommandBuffer(CommandBuffer* buffer, CommandBufferWaitType waitType, Fence* fence);
    void waitUntilScheduled();  // Block until work is scheduled
    void waitIdle();            // Block until all work completes
};
```

**Thread Safety:** Not thread-safe. External synchronization required for concurrent submissions.

**Multi-Queue Notes:**
- `waitForScheduled()` / `waitIdle()` affect only commands from the current DeviceContext
- Cross-queue dependencies require explicit semaphores

### 2.4 CommandBuffer

**Header:** `CommandBuffer.hpp`, `CommandBufferCreateInfo.h`

Records GPU commands for later submission.

```cpp
struct CommandBufferCreateInfo {
    CommandBufferCreateFlags commandBufferCreateFlags;  // None, UnretainedResources
    CommandQueue* commandQueue;                         // Target queue (must not be null)
};

class CommandBuffer : public DeviceChild {
    RenderCommandEncoder* getRenderCommandEncoder();
    BlitCommandEncoder* getBlitCommandEncoder();
    ComputeCommandEncoder* getComputeCommandEncoder();

    void resetQueryPool(QueryPool* pool, uint32_t firstQuery, uint32_t queryCount);
    CommandBufferStatus getStatus() const;  // Initial → Recording → Submitted
    CommandQueue* getCommandQueue();
};
```

**Resource retention:** Controlled by `CommandBufferCreateFlags`:

| Mode | Flag | Behavior |
|------|------|----------|
| **Retained** (default) | `None` | SnapRHI retains strong references to bound resources |
| **Unretained** | `UnretainedResources` | Application must keep all resources alive until GPU completes — reduces CPU overhead |

See [Resource Management Guide §1.1](resource-management.md#11-commandbuffer-retention-modes) for full lifetime rules.

**Thread Safety:** Not thread-safe. Recording must be single-threaded.

**Encoder Lifetime:** Encoder pointers remain valid for the CommandBuffer's lifetime.

---

## 3. Resource Types

### 3.1 Buffer

**Header:** `Buffer.hpp`, `BufferCreateInfo.h`

Linear memory for vertex/index/uniform/storage data.

```cpp
struct BufferCreateInfo {
    uint32_t size;
    BufferUsage bufferUsage;           // VertexBuffer, IndexBuffer, UniformBuffer, etc.
    MemoryProperties memoryProperties; // DeviceLocal, HostVisible, HostCoherent, HostCached
};

class Buffer : public DeviceChild {
    std::byte* map(MemoryAccess access, uint32_t offset, uint32_t size);
    void unmap();
    void flushMappedMemoryRanges(std::span<const MappedMemoryRange> ranges);
    void invalidateMappedMemoryRanges(std::span<const MappedMemoryRange> ranges);
};
```

**Memory Properties:**

| Property | Description | Guaranteed |
|----------|-------------|------------|
| `DeviceLocal` | Optimal GPU access | ✅ Yes |
| `HostVisible` | CPU can map | ✅ Yes |
| `HostCoherent` | No flush/invalidate needed | ✅ Yes |
| `HostCached` | CPU reads use cache | Backend-dependent |

**Mapping Contract:**
- Requires `HostVisible` memory
- If `supportsPersistentMapping` is false, unmap before GPU work
- Non-coherent memory requires explicit flush/invalidate

### 3.2 Texture

**Header:** `Texture.hpp`, `TextureCreateInfo.h`

Image resources for render targets and shader sampling.

```cpp
struct TextureCreateInfo {
    Extent3D size;                // width, height, depth (depth = array layers for 2D arrays)
    uint32_t mipLevels;
    TextureType textureType;      // Texture2D, Texture2DArray, Texture3D, TextureCube, etc.
    TextureUsage textureUsage;    // Sampled, Storage, ColorAttachment, DepthStencilAttachment, etc.
    SampleCount sampleCount;      // Count1 (no MSAA), Count2, Count4, etc.
    PixelFormat format;
    ComponentMapping componentMapping;  // Channel swizzle (default: identity RGBA)
};
```

**Constraints:**
- Cubemap textures require `size.width == size.height`
- Multisampled textures generally do not support multiple mip levels
- MSAA is restricted to 2D textures on most backends

### 3.3 Sampler

**Header:** `Sampler.hpp`, `SamplerCreateInfo.h`

Immutable texture sampling state.

```cpp
struct SamplerCreateInfo {
    SamplerMinMagFilter minFilter, magFilter;
    SamplerMipFilter mipFilter;               // NotMipmapped, Nearest, Linear
    WrapMode wrapU, wrapV, wrapW;             // ClampToEdge, Repeat, MirroredRepeat, etc.
    bool anisotropyEnable;
    AnisotropicFiltering maxAnisotropy;
    bool compareEnable;                       // For shadow map sampling
    CompareFunction compareFunction;
    float lodMin, lodMax;
    SamplerBorderColor borderColor;
    bool unnormalizedCoordinates;              // Texel-space coordinates (backend-dependent)
};
```

**Thread Safety:** Immutable after creation; safe to share between threads.

### 3.4 Framebuffer

**Header:** `Framebuffer.hpp`, `FramebufferCreateInfo.h`

Binds a RenderPass to concrete attachment textures.

```cpp
struct FramebufferCreateInfo {
    RenderPass* renderPass;                  // Must outlive the framebuffer
    std::vector<Texture*> attachments;       // Ordered to match render pass attachment descriptions
    uint32_t width, height;
    uint32_t layers;                         // > 1 enables layered rendering
};
```

**⚠️ Lifetime:** SnapRHI does **not** retain attachment references. See [Resource Management Guide](resource-management.md#12-per-resource-retention-rules).

### 3.5 RenderPass

**Header:** `RenderPass.hpp`, `RenderPassCreateInfo.h`

Describes attachment formats, load/store operations, and subpass structure. Modeled after Vulkan render passes.

```cpp
struct AttachmentDescription {
    AttachmentLoadOp loadOp, stencilLoadOp;    // Clear, Load, DontCare
    AttachmentStoreOp storeOp, stencilStoreOp; // Store, DontCare
    PixelFormat format;
    SampleCount samples;
    uint32_t mipLevel;                         // Which mip level to attach
    uint32_t layer;                            // Which array layer / cube face
    std::optional<AutoresolvedAttachmentDescription> autoresolvedAttachment;
};

struct SubpassDescription {
    std::array<AttachmentReference, MaxColorAttachments> colorAttachments;
    uint32_t colorAttachmentCount;
    std::array<AttachmentReference, MaxColorAttachments> resolveAttachments;
    uint32_t resolveAttachmentCount;
    AttachmentReference depthStencilAttachment;
    AttachmentReference resolveDepthStencilAttachment;
    uint32_t viewMask;                         // Multiview rendering mask
};

struct SubpassDependency {
    uint32_t srcSubpass, dstSubpass;
    PipelineStageBits srcStageMask, dstStageMask;
    AccessFlags srcAccessMask, dstAccessMask;
    DependencyFlags dependencyFlags;
};

struct RenderPassCreateInfo {
    std::array<AttachmentDescription, MaxAttachments> attachments;
    uint32_t attachmentCount;
    std::array<SubpassDescription, MaxSubpasses> subpasses;
    uint32_t subpassCount;
    std::array<SubpassDependency, MaxDependencies> dependencies;
    uint32_t dependencyCount;
};
```

**Key concepts:**
- **Attachments** define the render targets (format, sample count, load/store behavior, mip level, array layer)
- **Subpasses** reference attachments by index and declare which are color, resolve, or depth/stencil targets
- **Dependencies** establish explicit synchronization between subpasses (modeled after `VkSubpassDependency`)
- **Autoresolved attachments** support implicit MSAA resolve (e.g., `EXT_multisampled_render_to_texture` on OpenGL)
- **Multiview** rendering is controlled via `SubpassDescription::viewMask`

**⚠️ Lifetime:** The render pass must outlive all framebuffers and pipelines created from it.

---

## 4. Binding & Descriptor System

### 4.1 DescriptorSetLayout

**Header:** `DescriptorSetLayout.hpp`, `DescriptorSetLayoutCreateInfo.h`

Defines the binding interface for a descriptor set.

```cpp
struct DescriptorSetLayoutBinding {
    uint32_t binding;                    // Binding index in shader
    DescriptorType descriptorType;       // UniformBuffer, CombinedImageSampler, StorageBuffer, etc.
    ShaderStageBits stageBits;           // Vertex, Fragment, Compute
};

struct DescriptorSetLayoutCreateInfo {
    std::vector<DescriptorSetLayoutBinding> bindings;
};
```

**⚠️ Constraint:** Bindings must be sorted by `binding` in monotonically increasing order.

### 4.2 DescriptorPool

**Header:** `DescriptorPool.hpp`, `DescriptorPoolCreateInfo.h`

Allocation pool for descriptor sets.

**⚠️ Lifetime:** Must outlive all allocated DescriptorSets.

**Thread Safety:** Allocation requires external synchronization.

### 4.3 DescriptorSet

**Header:** `DescriptorSet.hpp`, `DescriptorSetCreateInfo.h`

Binds concrete resources to layout bindings.

```cpp
class DescriptorSet : public DeviceChild {
    void bindUniformBuffer(uint32_t binding, Buffer* buffer);
    void bindUniformBuffer(uint32_t binding, Buffer* buffer, uint32_t offset, uint32_t range);
    void bindStorageBuffer(uint32_t binding, Buffer* buffer);
    void bindStorageBuffer(uint32_t binding, Buffer* buffer, uint32_t offset, uint32_t range);
    void bindTexture(uint32_t binding, Texture* texture);
    void bindStorageTexture(uint32_t binding, Texture* texture, uint32_t mipLevel);
    void bindSampler(uint32_t binding, Sampler* sampler);
    void updateDescriptorSet(std::span<Descriptor> descriptorWrites);  // Batch update
    void reset();
};
```

**Buffer offset alignment:** `offset` must satisfy `Capabilities::minUniformBufferOffsetAlignment` (UBO) or the backend's SSBO alignment requirements (SSBO).

**⚠️ Allocation:** `Device::createDescriptorSet()` may return `nullptr` due to pool fragmentation.

**⚠️ Lifetime:** SnapRHI does **not** retain bound resource references.

### 4.4 PipelineLayout

**Header:** `PipelineLayout.hpp`, `PipelineLayoutCreateInfo.h`

Combines descriptor set layouts into a pipeline binding interface.

```cpp
struct PipelineLayoutCreateInfo {
    std::span<DescriptorSetLayout*> setLayouts;  // Indexed by set number
};
```

**⚠️ Constraint:** All set indices up to max must be filled (use empty layouts for gaps).

---

## 5. Pipeline & Shader System

### 5.1 ShaderModule

**Header:** `ShaderModule.hpp`, `ShaderModuleCreateInfo.h`

A shader module represents a **single entry point** (function) selected from a `ShaderLibrary`.

```cpp
struct ShaderModuleCreateInfo {
    ShaderModuleCreateFlags createFlags;  // None, AllowAsyncCreation
    ShaderStage shaderStage;              // Vertex, Fragment, Compute
    std::string_view name;                // Entry point name in shaderLibrary
    SpecializationInfo specializationInfo;
    ShaderLibrary* shaderLibrary;         // Source library (must not be null)

    // OpenGL-only: custom preprocessor names for specialization constants
    std::optional<snap::rhi::opengl::ShaderModuleInfo> glShaderModuleInfo;
};
```

**Key fields:**

| Field | Description |
|-------|-------------|
| `createFlags` | `None` or `AllowAsyncCreation` (backend-dependent) |
| `name` | Entry point name — used as `VkPipelineShaderStageCreateInfo::pName` (Vulkan), `MTLFunction` name (Metal), or exported function name (OpenGL) |
| `shaderLibrary` | The compiled `ShaderLibrary` containing the entry point. Must not be null |
| `specializationInfo` | Typed constant values patched at pipeline creation time (see below) |
| `glShaderModuleInfo` | Optional OpenGL-specific metadata for custom specialization constant naming |

#### Specialization Constants

Specialization constants allow patching constant values into a shader at pipeline creation time without recompiling source. The system is fully typed via `SpecializationConstantFormat`:

```cpp
enum class SpecializationConstantFormat : uint32_t {
    Undefined,
    Bool32,   // 32-bit boolean (Vulkan: VkBool32)
    Float,    // 32-bit IEEE-754 float
    Int32,    // 32-bit signed integer
    UInt32,   // 32-bit unsigned integer
};

struct SpecializationMapEntry {
    uint32_t constantID;                   // ID in [0, maxSpecializationConstants - 1]
    uint32_t offset;                       // Byte offset into SpecializationInfo::pData
    SpecializationConstantFormat format;   // Value type
};

struct SpecializationInfo {
    uint32_t mapEntryCount;
    const SpecializationMapEntry* pMapEntries;
    size_t dataSize;
    const void* pData;  // Raw byte buffer, valid during shader module creation
};
```

**Backend mapping:**

| Backend | Mechanism |
|---------|-----------|
| **Vulkan** | Maps 1:1 to `VkSpecializationMapEntry` / `VkSpecializationInfo` |
| **Metal** | Maps entries to `MTLFunctionConstantValues` |
| **OpenGL** | Injects `#define` directives into GLSL source before compilation |

#### OpenGL Specialization Constants — Custom Naming

By default, the OpenGL backend maps each constant ID *N* to the preprocessor macro `SNAP_RHI_SPECIALIZATION_CONSTANT_N`. Shader authors write:

```glsl
#ifndef SNAP_RHI_SPECIALIZATION_CONSTANT_0
#define SNAP_RHI_SPECIALIZATION_CONSTANT_0 1
#endif
const bool ENABLE_FEATURE = bool(SNAP_RHI_SPECIALIZATION_CONSTANT_0);
```

For more descriptive names — or when porting Vulkan/SPIR-V shaders — the caller can provide **custom preprocessor names** per constant via `glShaderModuleInfo`:

```cpp
// OpenGL-specific: map constant IDs to custom GLSL macro names
snap::rhi::opengl::ShaderModuleInfo glInfo;
glInfo.specializationConstantNames = {
    {0, "USE_NORMAL_MAP"},     // constant 0 → #define USE_NORMAL_MAP <value>
    {1, "MAX_LIGHT_COUNT"},    // constant 1 → #define MAX_LIGHT_COUNT <value>
};

snap::rhi::ShaderModuleCreateInfo ci{};
ci.shaderStage        = snap::rhi::ShaderStage::Fragment;
ci.name               = "main";
ci.shaderLibrary      = myLibrary;
ci.specializationInfo = mySpecInfo;
ci.glShaderModuleInfo = glInfo;   // Other backends ignore this field
```

**Rules:**
- Custom names must be valid GLSL preprocessor identifiers (no spaces, no leading digits).
- Names are matched to `SpecializationMapEntry` by `constantID`.
- Any constant ID **without** a custom name entry falls back to the default `SNAP_RHI_SPECIALIZATION_CONSTANT_<ID>` pattern.
- The `glShaderModuleInfo` field is ignored by Metal and Vulkan backends.

### 5.2 ShaderLibrary

**Header:** `ShaderLibrary.hpp`, `ShaderLibraryCreateInfo.h`

A shader library is a compiled container holding one or more shader entry points. `ShaderModule` objects reference an entry point within a library.

```cpp
struct ShaderLibraryCreateInfo {
    ShaderLibraryCreateFlag libCompileFlag;  // CompileFromSource or CompileFromBinary
    std::span<const uint8_t> code;           // Source text (UTF-8) or binary payload
};

class ShaderLibrary : public DeviceChild {
    const std::optional<reflection::ShaderLibraryInfo>& getReflectionInfo() const;
};
```

**Backend compilation modes:**

| Backend | Supported flag | Payload format |
|---------|----------------|----------------|
| **Vulkan** | `CompileFromBinary` | SPIR-V (`code.size()` must be a multiple of 4, suitably aligned for `uint32_t`) |
| **OpenGL** | `CompileFromSource` | GLSL source text |
| **Metal** | Both | MSL source or metallib binary |

**Reflection:** Some backends populate entry point reflection (names, stages, specialization constants) accessible via `getReflectionInfo()`. Returns `std::nullopt` when reflection is unavailable.

### 5.3 RenderPipeline

**Header:** `RenderPipeline.hpp`, `RenderPipelineCreateInfo.h`

A render pipeline is an immutable graphics pipeline state object (PSO) describing shader stages and all fixed-function state.

```cpp
struct RenderPipelineCreateInfo {
    PipelineCreateFlags pipelineCreateFlags;    // None, AllowAsyncCreation, AcquireNativeReflection, NullOnPipelineCacheMiss
    std::vector<ShaderModule*> stages;          // Vertex + Fragment shader modules

    // Fixed-function state
    VertexInputStateCreateInfo vertexInputState;
    InputAssemblyStateCreateInfo inputAssemblyState;
    RasterizationStateCreateInfo rasterizationState;
    MultisampleStateCreateInfo multisampleState;
    DepthStencilStateCreateInfo depthStencilState;
    ColorBlendStateCreateInfo colorBlendState;

    // Render target compatibility (choose one)
    RenderPass* renderPass;                     // Render-pass-based pipeline
    uint32_t subpass;
    AttachmentFormatsCreateInfo attachmentFormatsCreateInfo;  // Dynamic rendering (renderPass = nullptr)

    // Optional
    RenderPipeline* basePipeline;               // For pipeline derivatives
    PipelineCache* pipelineCache;
    PipelineLayout* pipelineLayout;

    // Backend-specific metadata
    std::optional<snap::rhi::opengl::RenderPipelineInfo> glRenderPipelineInfo;
    std::optional<snap::rhi::metal::RenderPipelineInfo> mtlRenderPipelineInfo;
};
```

#### Pipeline Creation Flags

| Flag | Description |
|------|-------------|
| `None` | Default behavior |
| `AllowAsyncCreation` | Pipeline may be compiled asynchronously (backend-dependent) |
| `AcquireNativeReflection` | Requests native reflection data if supported |
| `NullOnPipelineCacheMiss` | Returns `nullptr` instead of compiling on cache miss |

#### Render Target Compatibility

A render pipeline must know the attachment formats at creation time. Two modes are supported:

| Mode | When to use |
|------|-------------|
| **Render pass** | Set `renderPass` and `subpass`. The render pass must remain alive as long as any pipeline created from it is in use |
| **Dynamic rendering** | Leave `renderPass = nullptr` and fill `attachmentFormatsCreateInfo` with color/depth/stencil formats |

**Render pass compatibility (Vulkan):** Two render passes are compatible if they have the same number of attachments with identical formats and sample counts per subpass. Load/store operations do not need to match.

#### Fixed-Function State

##### Vertex Input

```cpp
struct VertexInputAttributeDescription {
    uint32_t location;              // [0, maxVertexInputAttributes - 1]
    uint32_t binding;               // [0, maxVertexInputBindings - 1]
    uint32_t offset;                // Byte offset (must be a multiple of 4)
    VertexAttributeFormat format;
};

struct VertexInputBindingDescription {
    uint32_t binding;               // [0, maxVertexInputBindings - 1]
    uint32_t divisor;               // Instance divisor (≥ 1, max: Capabilities::maxVertexAttribDivisor)
    uint32_t stride;                // Bytes between vertices (must be a multiple of 4, ≥ 1)
    VertexInputRate inputRate;      // PerVertex or Constant
};

struct VertexInputStateCreateInfo {
    std::array<VertexInputAttributeDescription, MaxVertexAttributes> attributeDescription;
    std::array<VertexInputBindingDescription, MaxVertexBuffers> bindingDescription;
    uint32_t attributesCount;
    uint32_t bindingsCount;
};
```

> **Alignment:** Both `offset` and `stride` must be a multiple of **4 bytes** (Metal restriction adopted as cross-backend rule).

> **Constant input rate:** When `inputRate` is `VertexInputRate::Constant`, only `float` attributes are universally supported. Check `Capabilities::constantInputRateSupportingBits` for other formats.

##### Input Assembly

```cpp
struct InputAssemblyStateCreateInfo {
    Topology primitiveTopology;     // Triangles, TriangleStrip, Lines, Points, etc.
};
```

**Primitive restart** is always enabled when supported (queried via device capabilities). The restart index is chosen automatically based on the index buffer format (`0xFFFF` for uint16, `0xFFFFFFFF` for uint32).

##### Rasterization

```cpp
struct RasterizationStateCreateInfo {
    bool rasterizationEnabled;      // false discards all primitives
    PolygonMode polygonMode;        // Fill, Line
    CullMode cullMode;              // None, Front, Back, FrontAndBack
    Winding windingMode;            // CCW, CW
    bool depthBiasEnable;
    uint32_t clipDistancePlaneCount;  // [0, Capabilities::maxClipDistances]
};
```

##### Depth / Stencil

```cpp
struct DepthStencilStateCreateInfo {
    bool depthTest;
    bool depthWrite;
    CompareFunction depthFunc;      // Always, Less, LessOrEqual, etc.
    bool stencilEnable;
    StencilOpState stencilFront;
    StencilOpState stencilBack;
};

struct StencilOpState {
    StencilOp failOp, passOp, depthFailOp;
    CompareFunction stencilFunc;
    StencilMask readMask, writeMask;
};
```

> **Depth write without testing:** Set `depthFunc = Always`, `depthTest = true`, `depthWrite = true`. When `depthTest` is disabled, writes to the depth buffer are also disabled.

##### Multisample

```cpp
struct MultisampleStateCreateInfo {
    SampleCount samples;            // Count1 (no MSAA), Count2, Count4, etc.
    bool alphaToCoverageEnable;
    bool alphaToOneEnable;
};
```

##### Color Blend

```cpp
struct RenderPipelineColorBlendAttachmentState {
    bool blendEnable;
    BlendFactor srcColorBlendFactor, dstColorBlendFactor;
    BlendOp colorBlendOp;
    BlendFactor srcAlphaBlendFactor, dstAlphaBlendFactor;
    BlendOp alphaBlendOp;
    ColorMask colorWriteMask;
};

struct ColorBlendStateCreateInfo {
    std::array<RenderPipelineColorBlendAttachmentState, MaxColorAttachments> colorAttachmentsBlendState;
    uint32_t colorAttachmentsCount;
};
```

##### Dynamic Rendering Attachments

```cpp
struct AttachmentFormatsCreateInfo {
    std::vector<PixelFormat> colorAttachmentFormats;
    PixelFormat depthAttachmentFormat;       // PixelFormat::Undefined if unused
    PixelFormat stencilAttachmentFormat;     // PixelFormat::Undefined if unused
};
```

#### Backend-Specific Pipeline Metadata

##### OpenGL — `glRenderPipelineInfo`

The OpenGL backend requires explicit resource mapping metadata because OpenGL lacks native descriptor set support:

```cpp
struct opengl::RenderPipelineInfo : public opengl::PipelineInfo {
    // Texture-sampler pairs to combine onto texture units
    std::vector<CombinedTextureSamplerInfo> combinedTextureSamplerInfos;
    // Vertex attribute name → location mapping
    std::vector<VertexAttributeInfo> vertexAttributes;
};

// Base class provides resource binding lookup table
struct opengl::PipelineInfo {
    std::vector<PipelineResourceInfo> resources;  // Maps set/binding → GLSL uniform name
};
```

##### Metal — `mtlRenderPipelineInfo`

Metal shares binding slots 0–30 for both argument buffers and vertex buffers. The pipeline info configures the split:

```cpp
struct metal::RenderPipelineInfo : public metal::PipelineInfo {
    uint32_t vertexBufferBindingBase = 23;  // Vertex buffers start at slot 23+
};

// Base class provides dynamic offsets buffer binding
struct metal::PipelineInfo {
    uint32_t auxiliaryDynamicOffsetsBinding;  // Undefined = dynamic buffers disabled
};
```

> **Both `glRenderPipelineInfo` and `mtlRenderPipelineInfo` are optional.** They are silently ignored by non-matching backends.

### 5.4 ComputePipeline

**Header:** `ComputePipeline.hpp`, `ComputePipelineCreateInfo.h`

A compute pipeline is an immutable pipeline state object for compute shader dispatch.

```cpp
struct ComputePipelineCreateInfo {
    PipelineCreateFlags pipelineCreateFlags;  // None, AllowAsyncCreation, AcquireNativeReflection, NullOnPipelineCacheMiss
    PipelineLayout* pipelineLayout;           // Descriptor set layouts used by the shader
    ShaderModule* stage;                      // Compute shader module
    Extent3D localThreadGroupSize{1, 1, 1};   // Workgroup size (local_size_x/y/z)

    // Optional
    ComputePipeline* basePipeline;            // For pipeline derivatives
    PipelineCache* pipelineCache;

    // Backend-specific metadata
    std::optional<snap::rhi::opengl::PipelineInfo> glPipelineInfo;
    std::optional<snap::rhi::metal::PipelineInfo> mtlPipelineInfo;
};
```

#### Key Fields

| Field | Description |
|-------|-------------|
| `pipelineCreateFlags` | Controls creation behavior (see [Pipeline Creation Flags](#pipeline-creation-flags) in §5.3) |
| `pipelineLayout` | Must match resources declared in the shader. Vulkan validates non-null for executable pipelines |
| `stage` | Compute shader module. Vulkan validates non-null |
| `localThreadGroupSize` | Per-workgroup thread dimensions (`local_size_x/y/z`). Metal uses this to derive `threadsPerThreadgroup` at dispatch time. Vulkan/OpenGL typically encode local size in the shader itself; this value can be used for validation and dispatch calculations |
| `basePipeline` | Optional base pipeline for derivative creation — backends may reuse compilation artifacts |
| `pipelineCache` | Optional cache to speed up creation. Vulkan passes the underlying `VkPipelineCache`; Metal may use an `MTLBinaryArchive` |

#### Pipeline Creation Flags — Compute-Specific Notes

| Flag | Compute behavior |
|------|------------------|
| `AllowAsyncCreation` | Metal: pipeline compilation can happen asynchronously |
| `AcquireNativeReflection` | Populates reflection data from native shader metadata. When set, some backends allow `pipelineLayout = nullptr` and empty `glPipelineInfo` — the resulting pipeline is **invalid for execution** and intended only for reflection queries |
| `NullOnPipelineCacheMiss` | Metal: returns `nullptr` if the binary archive does not contain the pipeline, instead of falling back to normal compilation |

#### Backend-Specific Pipeline Metadata

##### OpenGL — `glPipelineInfo`

OpenGL requires a mapping from GLSL resource names to logical `{set, binding}` pairs:

```cpp
struct opengl::PipelineInfo {
    std::vector<PipelineResourceInfo> resources;  // Maps set/binding → GLSL uniform name
};
```

##### Metal — `mtlPipelineInfo`

Metal uses an auxiliary buffer for dynamic offsets:

```cpp
struct metal::PipelineInfo {
    uint32_t auxiliaryDynamicOffsetsBinding;  // Undefined = dynamic buffers disabled
};
```

> **Both `glPipelineInfo` and `mtlPipelineInfo` are optional.** They are silently ignored by non-matching backends.

### 5.5 PipelineCache

**Header:** `PipelineCache.hpp`, `PipelineCacheCreateInfo.h`

Stores backend-specific pipeline compilation artifacts to reduce creation latency across application runs.

```cpp
struct PipelineCacheCreateInfo {
    PipelineCacheCreateFlags createFlags;  // None, ExternallySynchronized
    std::filesystem::path cachePath;       // Optional path for disk persistence
};

class PipelineCache : public DeviceChild {
    Result serializeToFile(const std::filesystem::path& cachePath) const;
};
```

**Backend behavior:**

| Backend | Implementation |
|---------|---------------|
| **Vulkan** | Wraps `VkPipelineCache`; serializable to disk |
| **Metal** | May use `MTLBinaryArchive` when available |
| **OpenGL** | Caches program binaries (when supported) |

**Thread Safety:** Pipeline caches are safe for concurrent pipeline creation. When `ExternallySynchronized` is set, the application provides its own synchronization; otherwise the backend guards access internally.

**⚠️ Portability:** Cache data is not portable across different GPUs, driver versions, or OS versions.

---

## 6. Command Recording

All three encoder types (`RenderCommandEncoder`, `ComputeCommandEncoder`, `BlitCommandEncoder`) inherit from `CommandEncoder`, which provides shared functionality:

```cpp
class CommandEncoder : public DeviceChild {
    void endEncoding();                        // Close the current encoding scope
    void writeTimestamp(QueryPool* pool, uint32_t query, TimestampLocation location);
    void pipelineBarrier(
        PipelineStageBits srcStageMask,
        PipelineStageBits dstStageMask,
        DependencyFlags dependencyFlags,
        std::span<MemoryBarrierInfo> memoryBarriers,
        std::span<BufferMemoryBarrierInfo> bufferMemoryBarriers,
        std::span<TextureMemoryBarrierInfo> textureMemoryBarriers
    );
};
```

**Call contract:** `beginEncoding()` → record commands → `endEncoding()`. No commands may be recorded outside this scope.

**`pipelineBarrier()`** inserts explicit synchronization — required for Compute ↔ Render transitions. See [Resource Management §3](resource-management.md#3-memory-hazard-tracking--synchronization) for details and examples.

### 6.1 RenderCommandEncoder

**Header:** `RenderCommandEncoder.hpp`

Records rendering commands within a render pass or dynamic rendering scope.

```cpp
class RenderCommandEncoder : public CommandEncoder {
    // Begin/end encoding
    void beginEncoding(const RenderPassBeginInfo& info);   // Render-pass-based
    void beginEncoding(const RenderingInfo& info);         // Dynamic rendering
    // endEncoding() inherited from CommandEncoder

    // Pipeline & resource binding
    void bindRenderPipeline(RenderPipeline* pipeline);
    void bindDescriptorSet(uint32_t binding, DescriptorSet* set, std::span<const uint32_t> dynamicOffsets);
    void bindVertexBuffer(uint32_t binding, Buffer* buffer, uint32_t offset);
    void bindVertexBuffers(uint32_t firstBinding, std::span<Buffer*> buffers, std::span<uint32_t> offsets);
    void bindIndexBuffer(Buffer* buffer, uint32_t offset, IndexType type);

    // Dynamic state
    void setViewport(const Viewport& viewport);
    void setDepthBias(float constantFactor, float slopeFactor, float clamp);
    void setStencilReference(StencilFace face, uint32_t reference);
    void setBlendConstants(float r, float g, float b, float a);

    // Draw calls
    void draw(uint32_t vertexCount, uint32_t firstVertex, uint32_t instanceCount);
    void drawIndexed(uint32_t indexCount, uint32_t firstIndex, uint32_t instanceCount);

    // Inherited: writeTimestamp(), pipelineBarrier(), endEncoding()
};
```

**Dynamic offsets:** When binding a descriptor set that contains dynamic buffers (`UniformBufferDynamic` / `StorageBufferDynamic`), provide byte offsets in `dynamicOffsets` ordered by binding index, then by array element.

### 6.2 ComputeCommandEncoder

**Header:** `ComputeCommandEncoder.hpp`

Records compute dispatch commands.

```cpp
class ComputeCommandEncoder : public CommandEncoder {
    void beginEncoding();
    // endEncoding() inherited from CommandEncoder

    void bindComputePipeline(ComputePipeline* pipeline);
    void bindDescriptorSet(uint32_t binding, DescriptorSet* set, std::span<const uint32_t> dynamicOffsets);
    void dispatch(uint32_t groupSizeX, uint32_t groupSizeY, uint32_t groupSizeZ);

    // Inherited: writeTimestamp(), pipelineBarrier(), endEncoding()
};
```

**Typical usage:** `beginEncoding()` → `bindComputePipeline()` → `bindDescriptorSet()` → `dispatch()` → `endEncoding()`.

### 6.3 BlitCommandEncoder

**Header:** `BlitCommandEncoder.hpp`

Records transfer (copy/blit) operations.

```cpp
class BlitCommandEncoder : public CommandEncoder {
    void beginEncoding();
    // endEncoding() inherited from CommandEncoder

    void copyBuffer(Buffer* src, Buffer* dst, std::span<const BufferCopy> regions);
    void copyBufferToTexture(Buffer* src, Texture* dst, std::span<const BufferTextureCopy> regions);
    void copyTextureToBuffer(Texture* src, Buffer* dst, std::span<const BufferTextureCopy> regions);
    void copyTexture(Texture* src, Texture* dst, std::span<const TextureCopy> regions);
    void generateMipmaps(Texture* texture);
};
```

**Copy info types:**

| Struct | Fields | Notes |
|--------|--------|-------|
| `BufferCopy` | `srcOffset`, `dstOffset`, `size` | Byte-level buffer-to-buffer copy |
| `BufferTextureCopy` | `bufferOffset`, `bytesPerRow`, `bytesPerSlice`, `textureSubresource`, `textureOffset`, `textureExtent` | Vulkan requires `bufferOffset` aligned to 4 bytes |
| `TextureCopy` | `src/dstSubresource`, `src/dstOffset`, `extent` | Texture-to-texture copy |

---

## 7. Synchronization Primitives

### 7.1 Fence

**Header:** `Fence.hpp`, `FenceCreateInfo.h`

CPU/GPU synchronization for host-side waits.

```cpp
class Fence : public DeviceChild {
    FenceStatus getStatus(uint64_t generationID = 0);  // NotReady, Completed, Error
    void waitForScheduled();              // Block until scheduled (Metal-relevant)
    void waitForComplete();               // Block until GPU complete
    void reset();                         // Return to unsignaled state
    uint64_t getGenerationID() const;     // Monotonically increasing cycle counter
    std::unique_ptr<PlatformSyncHandle> exportPlatformSyncHandle();  // Cross-API interop
};
```

**Reuse Contract:** Reset only after `getStatus() == Completed`. Resetting an in-flight fence is undefined behavior.

**Generation ID:** Fences are pooled and reused. Capture `getGenerationID()` at submission time — if `fence->getGenerationID() > capturedID` later, the original workload has completed and the fence was recycled (avoids the ABA problem).

**Backend mapping:**

| Method | Vulkan | Metal | OpenGL |
|--------|--------|-------|--------|
| `getStatus()` | `vkGetFenceStatus` | Tracked `MTLCommandBuffer` status | `glClientWaitSync(timeout=0)` |
| `waitForComplete()` | `vkWaitForFences(UINT64_MAX)` | `waitUntilCompleted` | `glClientWaitSync` |
| `waitForScheduled()` | No-op (scheduled at submit) | `waitUntilScheduled` | No-op |
| `reset()` | `vkResetFences` | Clears tracked buffer | Deletes `GLsync` |

### 7.2 Semaphore

**Header:** `Semaphore.hpp`, `SemaphoreCreateInfo.h`

GPU-only synchronization for queue ordering. Consumed by `CommandQueue::submitCommands()` as part of wait/signal lists.

**Semantics:** Binary semaphore — each signal satisfies exactly one wait. Waiting on an unsignaled semaphore is invalid.

**Reuse Contract:** Can only be reused after GPU finishes all associated work.

**Backend mapping:**

| Backend | Implementation |
|---------|---------------|
| **Vulkan** | Native `VkSemaphore` |
| **Metal** | GPU events when available, otherwise CPU-based |
| **OpenGL** | Emulated via `GLsync` objects and CPU coordination |

**Interop:** `Device::createSemaphore()` accepts an optional `PlatformSyncHandle` for cross-API synchronization.

### 7.3 QueryPool

**Header:** `QueryPool.h`, `QueryPoolCreateInfo.h`

Timestamp query pool for GPU timing.

```cpp
class QueryPool : public DeviceChild {
    enum class Result { Available, NotReady, Disjoint, Error };

    Result getResults(uint32_t firstQuery, uint32_t count, std::span<std::chrono::nanoseconds> queries);
    Result getResultsAndAvailabilities(uint32_t firstQuery, uint32_t count,
                                       std::span<std::chrono::nanoseconds> queries,
                                       std::span<bool> availabilities);
};
```

**Result codes:**

| Result | Meaning |
|--------|---------|
| `Available` | Query finished, timing data is valid |
| `NotReady` | GPU still processing — retry later |
| `Disjoint` | Timing data invalid (GPU throttled, context switched) — discard |
| `Error` | Invalid query ID or internal failure |

**Usage:** Call `CommandBuffer::resetQueryPool()` before reusing queries in a new frame. Use `writeTimestamp()` on any encoder to record timestamps.

---

## 8. Utility & Diagnostics

### 8.1 DebugMessenger

**Header:** `DebugMessenger.h`, `DebugMessengerCreateInfo.h`

Runtime validation and diagnostic callbacks.

```cpp
using DebugMessengerCallback = std::function<void(const DebugCallbackInfo&)>;

struct DebugMessengerCreateInfo {
    DebugMessengerCallback debugMessengerCallback;
};
```

**Setup:** Create via `Device::createDebugMessenger()`. The device must have been created with `DeviceCreateFlags::EnableDebugCallback` for callbacks to be active.

**Thread Safety:** Callbacks may be invoked from arbitrary threads (including internal backend threads). The callback implementation must be thread-safe.

**OpenGL note:** OpenGL allows only a single global debug callback. The backend aggregates multiple `DebugMessenger` instances and fans out messages to all of them.

### 8.2 Capabilities

**Header:** `Capabilities.h`

Runtime device capabilities and limits, queried via `Device::getCapabilities()`. Populated at device creation time.

```cpp
struct Capabilities {
    // Memory
    PhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
    bool supportsPersistentMapping;     // Can hold mapped pointers during GPU access
    uint32_t nonCoherentAtomSize;       // Alignment for flush/invalidate ranges

    // Buffer alignment
    uint32_t minUniformBufferOffsetAlignment;

    // Vertex input
    uint32_t maxVertexInputAttributes;
    uint32_t maxVertexInputBindings;
    uint32_t maxVertexInputAttributeOffset;
    uint32_t maxVertexInputBindingStride;
    uint32_t maxVertexAttribDivisor;
    FormatDataTypeBits constantInputRateSupportingBits;

    // Textures
    uint32_t maxTextureDimension2D;
    uint32_t maxTextureDimension3D;
    uint32_t maxTextureDimensionCube;
    uint32_t maxTextureArrayLayers;

    // Descriptors & pipeline
    uint32_t maxBoundDescriptorSets;
    uint32_t maxPerStageUniformBuffers;
    uint32_t maxTextures, maxSamplers;
    uint32_t maxSpecializationConstants;
    uint32_t maxFramebufferColorAttachmentCount;
    uint32_t maxFramebufferLayers;
    AnisotropicFiltering maxAnisotropic;

    // Compute
    uint32_t maxComputeWorkGroupInvocations;
    uint32_t threadExecutionWidth;           // Warp/wavefront size
    std::array<uint32_t, 3> maxComputeWorkGroupCount;
    std::array<uint32_t, 3> maxComputeWorkGroupSize;

    // Feature flags (selection)
    bool isDynamicRenderingSupported;
    bool isShaderStorageBufferSupported;
    bool isTextureViewSupported;
    bool isFramebufferFetchSupported;
    bool isVertexInputRatePerInstanceSupported;
    bool isPrimitiveRestartIndexEnabled;
    bool isNonFillPolygonModeSupported;
    // ... additional feature flags

    // Conventions
    NDCLayout ndcLayout;                     // Y-axis direction, depth range
    TextureOriginConvention textureConvention;
    APIDescription apiDescription;           // Backend API and version

    // Queue families
    std::array<QueueFamilyProperties, MaxQueueFamilies> queueFamilyProperties;
    uint32_t queueFamiliesCount;

    // Per-format capabilities
    std::array<PixelFormatProperties, PixelFormat::Count> formatProperties;
    std::array<bool, VertexAttributeFormat::Count> vertexAttributeFormatProperties;
};
```

**Key queries for engine integration:**

| Question | Field |
|----------|-------|
| Can I keep buffers mapped? | `supportsPersistentMapping` |
| What's the UBO offset alignment? | `minUniformBufferOffsetAlignment` |
| Max texture size? | `maxTextureDimension2D` |
| Dynamic rendering available? | `isDynamicRenderingSupported` |
| SSBO support? | `isShaderStorageBufferSupported` |
| Compute warp size? | `threadExecutionWidth` |
| Which pixel formats are supported? | `formatProperties[format].textureFeatures` |

### 8.3 TextureInterop

**Header:** `TextureInterop.h`, `TextureInteropCreateInfo.h`

Cross-API texture sharing for platform-native textures (e.g., camera frames, video decoders, external renderers).

```cpp
class TextureInterop {
    struct ImagePlane {
        uint32_t bytesPerRow;
        uint32_t bytesPerPixel;
        void* pixels;
    };

    const TextureCreateInfo& getTextureCreateInfo() const;
    const std::vector<API>& getCompatibleAPIs() const;
    const std::vector<API>& getUsedAPIs() const;

    std::span<const ImagePlane> map(MemoryAccess access, PlatformSyncHandle* syncHandle);
    std::unique_ptr<PlatformSyncHandle> unmap();
};
```

**Typical workflow:**
1. Create a `TextureInterop` instance (platform/backend-specific)
2. Wrap it into an `snap::rhi::Texture` via `Device::createTexture(textureInterop)`
3. Use `PlatformSyncHandle` to synchronize between CPU and GPU or across graphics APIs

> **See also:** [ADR-0003](adr/0003-textureinterop-read-write-api.md) for the design rationale.

### 8.4 Exception

**Header:** `Exception.h`

Exception hierarchy for API error propagation:

| Exception | When thrown |
|-----------|------------|
| `snap::rhi::Exception` | Base class for all SnapRHI runtime exceptions |
| `UnsupportedOperationException` | Requested operation not supported on current backend (e.g., texture views on OpenGL) |
| `InvalidArgumentException` | Argument failed validation |
| `NoBoundDeviceContextException` | No `DeviceContext` was bound to `Device` during command execution |
| `UnexpectedCommandException` | Internal: unknown command type encountered during replay |

---

## 9. Usage Patterns

### 9.1 Minimal Render Flow

```cpp
// 1. Create device
auto device = snap::rhi::backend::vulkan::createDevice(deviceInfo);

// 2. Create resources
auto vertexBuffer = device->createBuffer(vertexBufferInfo);
auto pipeline = device->createRenderPipeline(pipelineInfo);
auto framebuffer = device->createFramebuffer(framebufferInfo);

// 3. Create command buffer
auto queue = device->getCommandQueue(0, 0);
auto commandBuffer = device->createCommandBuffer({.commandQueue = queue});

// 4. Record commands
auto encoder = commandBuffer->getRenderCommandEncoder();
encoder->beginEncoding(renderPassBeginInfo);
encoder->bindRenderPipeline(pipeline.get());
encoder->bindVertexBuffer(0, vertexBuffer.get(), 0);
encoder->draw(3, 0, 1);
encoder->endEncoding();

// 5. Submit and synchronize
auto fence = device->createFence({});
queue->submitCommandBuffer(commandBuffer.get(), CommandBufferWaitType::DoNotWait, fence.get());
fence->waitForComplete();
```

### 9.2 Header Locations

| Category | Path |
|----------|------|
| Public headers | `src/public-api/include/snap/rhi/` |
| Backend implementations | `src/backend/{metal,vulkan,opengl,noop}/` |
| Backend common | `src/backend/common/` |
| Interop | `src/interop/` |

---

## Appendix: Architecture Decision Records

Key design decisions are documented in [ADRs](adr/README.md):

| ADR | Topic |
|-----|-------|
| [ADR-0001](adr/0001-opengl-vulkan-sync.md) | Cross-API GPU Synchronization |
| [ADR-0002](adr/0002-timestamp-query-support.md) | Timestamp Query Support |
| [ADR-0003](adr/0003-textureinterop-read-write-api.md) | TextureInterop Read/Write API |
| [ADR-0004](adr/0004-explicit-memory-management-buffer-mapping.md) | Explicit Memory Management |

---

*Last updated: 2026-02-20*
