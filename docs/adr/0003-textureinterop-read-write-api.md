# 3. TextureInterop Read/Write API

Date: 2025-11-05
Author: Vladyslav Deviatkov
Status: Accepted
Supersedes: N/A

## 1. Context

Cross-platform CPU access to GPU texture memory is required for several high-priority product features (camera processing, on-device ML pre/post-processing, video encoding, screen sharing, remote rendering diagnostics). Today, SnapRHI exposes buffer mapping APIs but lacks a unified, *safely synchronized* interface for image/texture resources. Platforms differ dramatically:

- Apple (CVPixelBuffer / IOSurface): Implicit driver-managed coherency; CPU waits via command buffer or GL fence completion, then locks the pixel buffer; unlock implicitly makes data available.
- Android (AHardwareBuffer): Explicit acquire/release fence file descriptors (FDs) required; CPU and GPU coordination must be manually expressed by passing these FDs.

Without a unified abstraction, app code must branch per platform, duplicating synchronization logic and increasing the likelihood of race conditions (e.g., reading incomplete GPU results or submitting GPU work using partially written CPU data). This increases maintenance cost and reduces reliability under high-throughput scenarios (multi-frame pipelines, concurrent encoders).

We also need an API that: (1) Preserves zero‑copy semantics where supported, (2) Integrates with existing semaphore/fence primitives, (3) Degrades gracefully to CPU-blocking waits when export/import is not possible.

## 2. Problem Statement & Drivers

Provide host read/write access to multi-plane texture resources with correctness guarantees across Vulkan, OpenGL(EGL), Metal, and GL-on-Apple. Key drivers:

- Performance: Avoid needless staging copies; enable direct plane access for YUV/RGBA camera frames and ML tensors.
- Predictable synchronization: Single flow covering explicit (Android) and implicit (Apple) models.
- Extensibility: Future support for DMA-BUF (Linux), WebGPU interop, and remote capture tooling.
- Safety: Prevent use-after-write/partial-read hazards by making required synchronization steps first-class.

## 3. Decision

Introduce `snap::rhi::TextureInterop` as the canonical cross-platform CPU mapping interface for textures. Adopt explicit synchronization handles (`snap::rhi::PlatformSyncHandle`) returned from and consumed by fences/semaphores.

Unify fence export naming as `Fence::exportPlatformSyncHandle()`. Provide a single `Device::createSemaphore(..., PlatformSyncHandle*)` overload for importing release fences.

The decision trades a small increase in API complexity (extra handle object) for a large reduction in platform branching, clearer lifetime semantics, and portable correctness.

## 4. Considered Options

1. Keep buffer-only mapping: Reject – too slow and cannot satisfy texture use cases (multi-plane, swizzle, stride).
2. Provide per-platform extensions (AppleTexture, AndroidTexture): Reject – fragments code, duplicates logic, hurts portability.
3. Unified mapping without synchronization abstraction (caller manually performs all waits/imports): Reject – high risk of misuse; error-prone.
4. Chosen: Unified `TextureInterop` + abstracted platform sync handle + fence/semaphore integration.

## 5. Detailed Design

### 5.1 Core Types

```cpp
struct TextureInteropCreateInfo {
    // Existing texture creation fields (format, dimensions, usage, etc.)
    snap::rhi::MemoryUsage memoryProperties = snap::rhi::MemoryProperties::DeviceLocal; // default
};

class PlatformSyncHandle { // Opaque, non-copyable
public:
    PlatformSyncHandle() = default;
    virtual ~PlatformSyncHandle() = default;
    PlatformSyncHandle(const PlatformSyncHandle&) = delete;
    PlatformSyncHandle& operator=(const PlatformSyncHandle&) = delete;
};

class TextureInterop { // Obtained via Device / texture creation path
public:
    struct ImagePlane {
        uint32_t bytesPerRow = 0;
        uint32_t bytesPerPixel = 0;
        void*    pixels = nullptr; // Valid only between map()/unmap()
    };

    // Maps for host read/write;  consume an acquire fence handle.
    std::span<const ImagePlane> map(MapUsage usage, PlatformSyncHandle* platformSyncHandle);

    // Unmaps; produce a release fence to be imported as a wait semaphore.
    std::unique_ptr<PlatformSyncHandle> unmap();
};

class Fence : public snap::rhi::DeviceChild {
public:
    virtual FenceStatus getStatus() = 0;
    virtual void waitForComplete() = 0;
    virtual std::unique_ptr<PlatformSyncHandle> exportPlatformSyncHandle() = 0;
    virtual void reset() = 0;
};

class Device {
public:
    virtual std::shared_ptr<Semaphore> createSemaphore(const SemaphoreCreateInfo& info,
                                                      std::unique_ptr<PlatformSyncHandle> platformSyncHandle = {}) = 0;
};
```

### 5.2 Usage Workflow

1. Create `TextureInterop` + associated `Texture`.
2. Submit GPU work rendering into texture; signal fence.
3. Attempt `Fence::exportPlatformSyncHandle()`.
   - Pass to `interop->map()`.
4. CPU reads/writes pixel planes.
5. Call `unmap()`.
   - Import via `Device::createSemaphore(..., std::move(handle))` and submit next GPU work waiting on this semaphore.

### 5.3 Platform Behavior Summary

| Platform | Acquire Sync (map) | Release Sync (unmap)               |
|----------|--------------------|------------------------------------|
| Apple (Metal/GL) | CPU waits on fence completion; | Empty fence produced on unlock   |
| Android (Vulkan/EGL) | Acquire fence FD consumed by lock | Release fence FD produced on unlock |

### 5.4 Android Implementation Notes

- `PlatformSyncHandle` concrete class wraps `int fenceFd`; destructor `close()` if >=0.
- `map()` consumes acquire FD (after downcast) and sets internal FD to -1.
- `unmap()` obtains release FD; returns new handle if valid, or empty handle.
- Vulkan: requires `VK_KHR_external_{fence,semaphore}_fd`.
- EGL: requires `EGL_ANDROID_native_fence_sync`.

### 5.5 Apple Implementation Notes

- `map()` consumes snap::rhi::Fence, call snap::rhi::Fence::waitUntilCompleted(), performs `CVPixelBufferLockBaseAddress()`; planes precomputed in constructor.
- `unmap()` calls `CVPixelBufferUnlockBaseAddress()`, returns empty handle.
- Fence export always returns nullptr; CPU wait is mandatory pre-map.

### 5.6 Error & Edge Cases

- Attempting to pass stale/used `PlatformSyncHandle` results in map failure → must validate FD >= 0 before consumption.
- Missing required extensions → export returns nullptr; callers must fallback to CPU wait path (documented contract).
- Multi-plane coherence: All planes locked/unlocked atomically under platform calls; partial plane mapping is not supported initially.
- Concurrent maps: Not supported; second `map()` without prior `unmap()` is undefined → implementation will assert or return empty span.
- Lifetime: ImagePlane `pixels` pointers invalidated immediately after `unmap()`.

### 5.7 Threading & Concurrency

- Fence export/import operations are thread-safe provided the underlying backend handles fence/semaphore creation on a single Device dispatcher thread or protects with internal mutexes.
- `TextureInterop::map()` must internally guard against concurrent calls (mutex or atomic state flag) to prevent double-lock.

### 5.8 Performance Considerations

- Zero copy: Achieved by locking native texture storage, avoiding staging buffers.
- Avoid extra waits: Only perform CPU wait when export not available; otherwise rely on kernel-level fence.
- FD lifecycle: Immediate `close()` after native lock/unlock consumption to reduce FD pressure.
- Cache locality: Callers can batch CPU operations within a single map window; guidance to minimize map/unmap frequency added to docs.

### 5.9 Security & Robustness

- FD ownership transfer explicit (`releaseFd()` semantics); prevents double-close.
- Invalid handles (negative FD) short-circuit import path, forcing safer blocking wait.
- No raw file descriptor exposure beyond opaque handle boundary.
- Mitigates accidental misuse by making acquire/release explicit in API signatures.

### 5.10 Testing Strategy

- Unit tests:
  - Apple mock: Ensure map after fence wait succeeds; unmap returns nullptr.
  - Android mock: Simulate acquire/release FD cycle; assert FD consumed and produced once.
- Integration tests:
  - Vulkan path with external semaphore/fence extensions: round-trip render → CPU modify → subsequent GPU read.
  - EGL path: native fence duplication/import.
- Stress tests: Rapid map/unmap cycles across frames (1000+ iterations) verifying no FD leaks.
- Negative tests: Missing extensions → fallback path correctness & data integrity.
- Performance baseline: Benchmark CPU access latency vs legacy staging copy approach.

### 5.11 Rollout Plan

1. Backend capability detection + graceful fallback implementation.
2. Introduce Public API.
3. Add tests to CI.
4. Add documentation.

### 5.12 Future Work

- DMA-BUF / Linux support (Wayland, external memory sync primitives).
- WebGPU interop once browser APIs stabilize.
- Partial plane mapping & format conversion helpers.

## 6. API Contracts

- Precondition: All prior GPU writes to texture completed (either by exported handle or manual CPU wait) before `map()`.
- Postcondition after `map()`: Returned planes remain valid until `unmap()`.
- Postcondition after `unmap()`: If handle returned, caller MUST import as wait semaphore before next GPU access.
- Failure Mode: Missing extensions → export returns nullptr; caller performs CPU wait; correctness preserved.

## 7. Risks & Mitigations

| Risk | Impact                          | Mitigation |
|------|---------------------------------|-----------|
| Extension absence on subset of Android devices | Reduced performance (CPU waits) | Capability probing, fallback path validated |
| FD leak due to misuse | Resource exhaustion             | Opaque handle consumes/invalidates FD, tests detect leaks |
| Race from skipped CPU wait when export unsupported | Corrupted reads                 | Clear contract + assertions in debug builds |
| Multiple imports of same FD | Undefined sync state            | `releaseFd()` empties handle; second import fails early |

## 8. Examples

```cpp
// Acquire path (Android preferred)
auto fence = device->createFence(...);
queue->submitCommandBuffer(..., fence.get());
auto acquireHandle = fence->exportPlatformSyncHandle();
auto planes = interop->map(snap::rhi::MapUsage::ReadWrite, acquireHandle.get());
// CPU work using planes[0].pixels, planes[0].bytesPerRow, etc.
auto releaseHandle = interop->unmap();
auto waitSemaphore = device->createSemaphore({}, std::move(releaseHandle));
queue->submitCommandBuffer({&waitSemaphore,1}, {&stageBits,1}, {...}, {}, nullptr);
```

## 9. References

- Vulkan External Synchronization: https://registry.khronos.org/vulkan/specs/1.3-extensions/html/chap40.html
- EGL Native Fence Sync: https://www.khronos.org/registry/EGL/extensions/ANDROID/EGL_ANDROID_native_fence_sync.txt
- CVPixelBuffer Docs: [Apple Developer Documentation](https://developer.apple.com/documentation/corevideo/cvpixelbuffer-q2e?language=objc)
- AHardwareBuffer [NDK Reference](https://developer.android.com/ndk/reference/group/a-hardware-buffer)
