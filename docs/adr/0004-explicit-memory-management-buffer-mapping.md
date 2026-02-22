# 4. Explicit Memory Management & Buffer Mapping Interface

Date: 2026-01-28
Author: Vladyslav Deviatkov
Status: Proposed
Supersedes: N/A

## 1. Context

The current memory abstractions in SnapRHI / SnapRHI (e.g., `snap::rhi::MemoryUsage`, `snap::rhi::MapUsage`) are too coarse for modern high-performance graphics workloads. They conceal important platform and hardware behavior related to:

- Where memory resides (device-local vs. host-visible/unified).
- Host cache behavior (cached vs. uncached).
- Cache coherency requirements (coherent vs. non-coherent).
- The need for explicit host cache maintenance (flush / invalidate ranges).

As a result, applications cannot reliably optimize CPU↔GPU data transfers, and backends must either over-synchronize (hurting performance) or rely on implicit behavior that varies across APIs (Vulkan vs. OpenGL vs. Metal).

To achieve parity with modern low-level APIs and enable predictable performance, SnapRHI should expose explicit memory property selection and explicit mapping + cache maintenance, while still providing capability probing and safe fallbacks.

## 2. Problem Statement & Drivers

We need a cross-backend buffer mapping interface that:

- Explicitly expresses the underlying memory properties required by the app.
- Supports both coherent and non-coherent memory models.
- Supports persistent mapping where the backend/API allows it.
- Exposes minimal but sufficient device memory information for selecting memory types.
- Preserves portability: callers can write a single flow and branch only on capabilities.

Primary drivers:

- Performance: avoid extra copies, reduce driver overhead, enable fine-grained flush.
- Correctness: make cache maintenance and preconditions explicit.
- Portability: unify Vulkan/Metal/GL behavior under one contract.
- Extensibility: enable future support for external memory, DMA-BUF, WebGPU interop.

## 3. Decision

1. Deprecate `snap::rhi::MemoryUsage` and `snap::rhi::MapUsage` in favor of explicit bitmask flags:
   - `snap::rhi::MemoryProperties` (allocation / placement traits)
   - `snap::rhi::MemoryAccess` (mapping access semantics)
2. Extend `Capabilities` to report:
   - physical device memory types and their property bits
   - whether persistent mapping is supported for buffers
3. Update `snap::rhi::Buffer` to provide:
   - explicit `map()` / `unmap()`
   - explicit cache maintenance APIs for non-coherent memory: `flushMappedMemoryRanges()` and `invalidateMappedMemoryRanges()`

This trades a small increase in API complexity for predictable cross-platform performance and correctness.

## 4. Considered Options

1. **Keep current `MemoryUsage` / `MapUsage`:** Reject — insufficient control and unclear coherency semantics.
2. **Backend-specific mapping extensions (e.g., VulkanBuffer/MetalBuffer):** Reject — fragments the API and duplicates logic.
3. **Expose explicit flags but keep implicit flush/unmap behavior:** Reject — error-prone; makes performance unpredictable.
4. **Chosen:** explicit property flags + explicit mapping + explicit cache maintenance + capability probing.

## 5. Detailed Design

### 5.1 Core Types

#### 5.1.1 Memory Property Flags

```cpp
namespace snap::rhi {

// Bitmask; combinations are allowed.
enum class MemoryProperties : uint32_t {
    None         = 0,
    DeviceLocal  = 1u << 0,
    HostVisible  = 1u << 1,
    HostCoherent = 1u << 2,
    HostCached   = 1u << 3,
};

} // namespace snap::rhi
```

**Semantics (portable contract):**

- `DeviceLocal`: Prefer memory that is optimal for GPU access.
- `HostVisible`: Memory can be mapped and accessed by the CPU.
- `HostCoherent`: Host cache maintenance is not required (no explicit flush/invalidate).
- `HostCached`: CPU reads benefit from caches. (Note: platforms may implicitly provide cached+coherent, or cached+non-coherent.)

Backends may ignore unsupported bits or fail creation when a requested combination cannot be satisfied. This behavior is explicitly controlled via backend capability detection (see §5.3).

#### 5.1.2 Memory Map Flags

```cpp
namespace snap::rhi {

enum class MemoryAccess : uint32_t {
    None  = 0,
    Read  = 1u << 0,
    Write = 1u << 1,
};

} // namespace snap::rhi
```

#### 5.1.3 Memory Property Tables

```cpp
namespace snap::rhi {

static constexpr uint32_t MAX_MEMORY_TYPES = 32u;

struct MappedMemoryRange {
    uint32_t offset = 0;
    uint32_t size = 0;
};

struct MemoryType {
    snap::rhi::MemoryProperties memoryProperties = snap::rhi::MemoryProperties::None;
};

struct PhysicalDeviceMemoryProperties {
    std::array<MemoryType, MAX_MEMORY_TYPES> memoryTypes{};
};

} // namespace snap::rhi
```

### 5.2 Buffer Creation

```cpp
namespace snap::rhi {

struct BufferCreateInfo {
    uint32_t size = 0;
    snap::rhi::BufferUsage bufferUsage = snap::rhi::BufferUsage::None;

    // Requested properties. Typical patterns:
    // - Device-local GPU buffers: DeviceLocal
    // - Staging/upload: HostVisible | HostCoherent
    // - Readback: HostVisible | HostCached (often non-coherent)
    snap::rhi::MemoryProperties memoryProperties = snap::rhi::MemoryProperties::DeviceLocal;
};

} // namespace snap::rhi
```

### 5.3 Capabilities

```cpp
namespace snap::rhi {

struct Capabilities {
    // ... existing members

    snap::rhi::PhysicalDeviceMemoryProperties physicalDeviceMemoryProperties{};

    /**
     * Indicates whether buffers can remain mapped while the GPU is accessing them.
     *
     * - true (Persistent Mapping):
     *   Pointer returned from `Buffer::map()` remains valid across GPU submissions.
     *   (Still requires correct synchronization and cache maintenance.)
     *
     * - false (Must Unmap):
     *   App must call `Buffer::unmap()` before submitting GPU work that uses the buffer.
     */
    bool supportsPersistentMapping = false;

    /**
     * @brief The size of the non-coherent memory atom.
     * * @details
     * When flushing or invalidating non-coherent memory ranges:
     * 1. The 'offset' must be a multiple of this value.
     * 2. The 'size' must be a multiple of this value (unless size is VK_WHOLE_SIZE).
     * * This effectively maps to VkPhysicalDeviceLimits::nonCoherentAtomSize.
     * On most desktop GPUs this is 64 bytes (cache line size).
     */
    uint32_t nonCoherentAtomSize = 1;
};
} // namespace snap::rhi
```

### 5.4 Buffer Mapping Interface

```cpp
namespace snap::rhi {

class Buffer {
public:
    virtual ~Buffer() = default;

    // Maps the buffer for host access.
    // Returns nullptr on failure.
    virtual std::byte* map(snap::rhi::MemoryAccess access, uint32_t offset, uint32_t size) = 0;

    // Ends the current mapping scope.
    // On some backends this is required before GPU usage; see Capabilities::supportsPersistentMapping.
    // Backends may also use this as a hook to perform implicit whole-range flushes.
    virtual void unmap() = 0;

    // CPU -> GPU visibility for non-coherent memory.
    // For coherent memory this is a no-op.
    virtual void flushMappedMemoryRanges(std::span<const snap::rhi::MappedMemoryRange> ranges) = 0;

    // GPU -> CPU visibility for non-coherent memory.
    // For coherent memory this is a no-op.
    // Precondition: GPU writes to the buffer finished (fence / queue idle / etc.).
    virtual void invalidateMappedMemoryRanges(std::span<const snap::rhi::MappedMemoryRange> ranges) = 0;
};

} // namespace snap::rhi
```

### 5.5 API Contracts

#### 5.5.1 Mapping

- `map()` requires the buffer to be created with `HostVisible`.
- If `Capabilities::supportsPersistentMapping` is `false`, the app must not submit GPU work referencing this buffer while it is mapped.
- Returned pointer is valid until `unmap()` (or indefinitely if persistent mapping is supported and the buffer remains alive).

#### 5.5.2 Cache Maintenance

- For coherent memory (`HostCoherent`), `flushMappedMemoryRanges()` and `invalidateMappedMemoryRanges()` are no-ops.
- For non-coherent memory, callers must:
  - Call `flushMappedMemoryRanges()` after CPU writes and before GPU reads.
  - Call `invalidateMappedMemoryRanges()` after GPU writes and before CPU reads.
- Ranges may need to be aligned/expanded to `Capabilities::nonCoherentAtomSize` (backend will validate and/or internally fix up).

## 6. Backend Implementation Strategy

### 6.1 Vulkan

- Mapping:
  - `map()` → `vkMapMemory`
  - `unmap()` → `vkUnmapMemory` (or a no-op if persistent mapping chosen internally)
- Memory properties map 1:1:
  - `DeviceLocal` → `VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT`
  - `HostVisible` → `VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT`
  - `HostCoherent` → `VK_MEMORY_PROPERTY_HOST_COHERENT_BIT`
  - `HostCached` → `VK_MEMORY_PROPERTY_HOST_CACHED_BIT`
- Cache maintenance:
  - `flushMappedMemoryRanges()` → `vkFlushMappedMemoryRanges`
  - `invalidateMappedMemoryRanges()` → `vkInvalidateMappedMemoryRanges`

### 6.2 OpenGL / OpenGL ES

- Mapping:
  - `map()` → `glMapBufferRange`
  - `unmap()` → `glUnmapBuffer`
- Optimization:
  - If mapping with Write-only intent, backend may use `GL_MAP_INVALIDATE_BUFFER_BIT` to orphan.
- Cache maintenance:
  - Coherent behavior is typically achieved by unmapping (implicit flush).
  - Explicit flushing uses `GL_MAP_FLUSH_EXPLICIT_BIT` and `glFlushMappedBufferRange`.
- Persistent mapping:
  - Available via `GL_MAP_PERSISTENT_BIT` (requires `ARB_buffer_storage` / `EXT_buffer_storage`).
  - Many Android OpenGL ES devices lack these extensions; therefore `supportsPersistentMapping` is expected to be `false` on those backends.

### 6.3 Metal

- Mapping:
  - `map()` returns `[MTLBuffer contents]`.
  - `unmap()` is typically a no-op except as a convenient hook for whole-range `didModifyRange:`.
- Memory properties:
  - Unified memory (iOS / Apple Silicon): `MTLStorageModeShared` (host visible, coherent).
  - Discrete memory (some macOS): `MTLStorageModeManaged` (host visible, non-coherent).
- Cache maintenance:
  - `flushMappedMemoryRanges()` → `-[MTLBuffer didModifyRange:]` (CPU → GPU)
  - `invalidateMappedMemoryRanges()` → `-[MTLBlitCommandEncoder synchronizeResource:]` (GPU → CPU)

## 7. Consequences

### Positive

- Enables zero-copy / minimal-copy buffer workflows where supported.
- Enables explicit tuning for CPU readback (`HostCached`) and uploads (`HostCoherent`).
- Reduces driver overhead on GL by enabling explicit flushing and write-only mapping optimizations.
- Unifies the mental model for Vulkan, Metal, and modern GL.

### Negative

- Increases API surface area and complexity: callers must understand coherency and synchronization.
- Requires the app to respect `supportsPersistentMapping` to remain correct on older OpenGL ES devices.
- Introduces backend-specific availability constraints for certain combinations of property bits.

## 8. Examples

### 8.1 Coherent memory (no flush/invalidate) + persistent mapping

Typical use-case: upload buffer updated every frame.

```cpp
// Create a host-visible coherent buffer.
snap::rhi::BufferCreateInfo info{};
info.size = kSize;
info.bufferUsage = snap::rhi::BufferUsage::TransferSrc;
info.memoryProperties = snap::rhi::MemoryProperties::HostVisible |
                         snap::rhi::MemoryProperties::HostCoherent;

auto buffer = device->createBuffer(info);

// Map once and keep mapped (only if supported).
auto* ptr = buffer->map(snap::rhi::MemoryAccess::Write, 0, info.size);
if (!ptr) {
    // handle failure
}

// Write CPU data.
std::memcpy(ptr, data, info.size);

// Coherent: no flush required.

// If persistent mapping isn't supported by this backend, unmap before GPU submission.
if (!device->getCapabilities().supportsPersistentMapping) {
    buffer->unmap();
}

// Submit GPU work that reads the buffer (synchronization omitted).
queue->submit(...);

// If the backend requires unmap-before-use, you may map again next frame.
```

### 8.2 Non-coherent memory (explicit flush/invalidate)

Typical use-case: cached readback buffer (GPU writes, CPU reads) and/or non-coherent upload.

```cpp
// Create a host-visible cached buffer. This may be non-coherent on some platforms.
snap::rhi::BufferCreateInfo info{};
info.size = kSize;
info.bufferUsage = snap::rhi::BufferUsage::TransferDst;
info.memoryProperties = snap::rhi::MemoryProperties::HostVisible |
                         snap::rhi::MemoryProperties::HostCached;

auto buffer = device->createBuffer(info);

// --- GPU -> CPU readback path ---
// 1) GPU writes into buffer.
queue->submitCopyToBuffer(...);

// 2) Wait for GPU completion (e.g., fence wait).
fence->waitForComplete();

// 3) Invalidate range BEFORE CPU reads if memory is non-coherent.
//    (For coherent memory this is a no-op.)
snap::rhi::MappedMemoryRange readRange{.offset = 0, .size = info.size};
buffer->invalidateMappedMemoryRanges(std::span{&readRange, 1});

// 4) Map and read.
auto* ptr = buffer->map(snap::rhi::MemoryAccess::Read, 0, info.size);
// read from ptr ...

// 5) Unmap if required.
if (!device->getCapabilities().supportsPersistentMapping) {
    buffer->unmap();
}

// --- CPU -> GPU upload path (non-coherent) ---
// 1) Map and write.
ptr = buffer->map(snap::rhi::MemoryAccess::Write, 0, info.size);
std::memcpy(ptr, data, info.size);

// 2) Flush AFTER CPU writes and BEFORE GPU reads if non-coherent.
snap::rhi::MappedMemoryRange writeRange{.offset = 0, .size = info.size};
buffer->flushMappedMemoryRanges(std::span{&writeRange, 1});

// 3) Unmap if required before using the buffer on the GPU.
if (!device->getCapabilities().supportsPersistentMapping) {
    buffer->unmap();
}

queue->submit(...);
```

## 9. Rollout / Action Items

- [ ] Add `MemoryProperties` and `MemoryAccess` to public headers.
- [ ] Mark `MemoryUsage` and `MapUsage` as deprecated, with a migration path.
- [ ] Extend `Capabilities` with memory properties and persistent mapping support.
- [ ] Update `BufferCreateInfo` and `Buffer` interface across all backends.
- [ ] Implement per-backend coherency behavior, flush/invalidate, and validation.
- [ ] Add tests:
  - Correct flush/invalidate calls on Vulkan.
  - GL path uses explicit flush when non-coherent.
  - Metal Managed path calls `didModifyRange` / `synchronizeResource` as expected.
