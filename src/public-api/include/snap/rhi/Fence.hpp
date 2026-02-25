//
//  Fence.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 21.09.2020.
//  Copyright © 2021 Snapchat. All rights reserved.
//

#pragma once

#include "snap/rhi/DeviceChild.hpp"
#include "snap/rhi/Enums.h"
#include "snap/rhi/FenceCreateInfo.h"
#include "snap/rhi/PlatformSyncHandle.h"
#include <atomic>
#include <memory>
#include <mutex>

namespace snap::rhi {
class Device;

class Fence : public snap::rhi::DeviceChild {
public:
    Fence(Device* device, const FenceCreateInfo& info);
    ~Fence() override = default;

    /**
     * @brief Query the current completion status of the fence.
     *
     * A fence transitions from an unsignaled state to a signaled/completed state once all GPU work associated with the
     * submission that signaled the fence has finished.
     *
     * Backend mappings:
     * - **Vulkan:** `vkGetFenceStatus`.
     * - **Metal:** reflects the tracked `MTLCommandBuffer` status.
     * - **OpenGL:** uses `glClientWaitSync(..., timeout=0)` to poll the `GLsync` object.
     *
     * @return Current fence status:
     * - `FenceStatus::NotReady` if work is still in-flight.
     * - `FenceStatus::Completed` if the fence is signaled.
     * - `FenceStatus::Error` only on backends that can detect/report an error while querying.
     */
    virtual FenceStatus getStatus(uint64_t generationID = 0) = 0;

    /**
     * @brief Block the calling thread until the submission associated with this fence is *scheduled* on the GPU.
     *
     * This is a weaker guarantee than @ref waitForComplete(): it only ensures the driver/runtime has accepted the work
     * and it is eligible for GPU execution (not necessarily finished).
     *
     * Backend behavior:
     * - **Metal:** waits until the tracked `MTLCommandBuffer` reaches `MTLCommandBufferStatusScheduled`.
     * - **Vulkan:** no-op, because a successful `vkQueueSubmit` implies the submission is scheduled.
     * - **OpenGL:** effectively a no-op after `glFenceSync`; the sync is considered scheduled once created/queued.
     *
     * @note This is used by the interop synchronization path on Apple platforms, where waiting until scheduled is
     * sufficient for cross-API texture sharing.
     * @see https://developer.apple.com/documentation/metal/mtlcommandbuffer/waituntilscheduled
     */
    virtual void waitForScheduled() = 0;

    /**
     * @brief Block the calling thread until the fence is signaled (GPU work is complete).
     *
     * This provides a host-side synchronization point. When this function returns successfully, all work associated
     * with the submission that signaled the fence has finished executing on the GPU.
     *
     * Backend mappings:
     * - **Vulkan:** `vkWaitForFences(..., timeout=UINT64_MAX)`.
     * - **Metal:** `-[MTLCommandBuffer waitUntilCompleted]`.
     * - **OpenGL:** waits on `GLsync` via `glClientWaitSync` (and may fall back to `glFinish` on WebAssembly).
     *
     * @warning This call can stall the CPU and severely impact performance. Prefer GPU-side synchronization
     * primitives (semaphores/events) where applicable.
     */
    virtual void waitForComplete() = 0;

    /**
     * @brief Export the fence as an opaque, platform-specific synchronization handle.
     *
     * This is primarily intended for cross-API interop (e.g. sharing textures between different graphics APIs). The
     * returned handle is opaque and platform dependent:
     * - On **Android**, backends may export an OS-native sync FD when supported (e.g. Vulkan external fence FD, EGL
     * native fence FD). If not supported, a generic opaque handle is returned.
     * - On other platforms, a generic opaque handle wrapper is typically returned.
     *
     * @return A unique pointer owning the exported handle. Implementations may return a generic handle wrapper when
     * native export is not available.
     */
    virtual std::unique_ptr<snap::rhi::PlatformSyncHandle> exportPlatformSyncHandle() = 0;

    /**
     * @brief Reset the fence back to the unsignaled state.
     *
     * After reset, the fence can be reused for a subsequent submission.
     *
     * Backend mappings:
     * - **Vulkan:** `vkResetFences`.
     * - **Metal:** clears the tracked command buffer and status.
     * - **OpenGL:** deletes the `GLsync` object and returns to an unsignaled state.
     *
     * @warning It is the caller's responsibility to ensure the fence is not still associated with in-flight work
     * (i.e. it is completed) before resetting. Resetting a fence that is still in use can lead to undefined behavior.
     */

    virtual void reset();

    [[nodiscard]] uint64_t getCPUMemoryUsage() const override {
        return 0;
    }

    [[nodiscard]] uint64_t getGPUMemoryUsage() const override {
        return 0;
    }

    /**
     * @brief Returns the current generation ID of this recycled fence.
     *
     * This value is a monotonically increasing counter that increments every time the
     * fence is reset. It is used to validate that the fence is still representing
     * the specific submission cycle you are interested in.
     *
     * @details
     * **The ABA Problem:**
     * Because Fences are pooled and reused, a pointer to a Fence is not a unique
     * identifier for a specific GPU workload.
     *
     * **Usage:**
     * Capture the generation ID at submission time. When checking status later,
     * if @c fence->getGenerationID() > capturedID, you know the original
     * workload has definitely completed and the fence has already been recycled
     * for new work.
     *
     * @return The unique generation index for the current cycle.
     */
    uint64_t getGenerationID() const {
        std::lock_guard lock(accessMutex);
        return generationID;
    }

protected:
    mutable std::mutex accessMutex;
    uint64_t generationID = 0;
};
} // namespace snap::rhi
