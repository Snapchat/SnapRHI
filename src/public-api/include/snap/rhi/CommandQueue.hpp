//
//  CommandQueue.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 21.09.2020.
//  Copyright © 2021 Snapchat. All rights reserved.
//

#pragma once

#include "snap/rhi/DeviceChild.hpp"
#include "snap/rhi/Structs.h"
#include <span>

namespace snap::rhi {
class Device;
class CommandBuffer;
class Semaphore;
class Fence;

/**
 * @brief GPU command queue used to submit command buffers for execution.
 *
 * Command queues are created by the backend `snap::rhi::Device` and represent the execution context for recorded
 * command buffers. Submission semantics (scheduling, synchronization primitives, and resource lifetime tracking) are
 * backend-specific.
 *
 * ## Thread-safety
 * Backends typically allow concurrent submissions, but `CommandQueue` itself does not guarantee thread-safety.
 * If you submit from multiple threads, externally synchronize as required by the backend/device.
 */
class CommandQueue : public snap::rhi::DeviceChild {
public:
    explicit CommandQueue(Device* device);
    ~CommandQueue() override = default;

    /**
     * @brief Submits one or more command buffers to the queue.
     *
     * @param waitSemaphores Semaphores to wait on before execution begins.
     * @param waitDstStageMask Pipeline stage masks associated with @p waitSemaphores.
     * @param buffers Command buffers to submit, in submission order.
     * @param signalSemaphores Semaphores to signal when the submission completes.
     * @param waitType Whether to block the CPU until the submission is scheduled/completed.
     * @param fence Optional fence that will be signaled when the submission completes.
     *
     * ## Lifetime and synchronization contract
     * - If @fence is not `nullptr`, it must be in the unsignaled state prior to submission.
     * - The application must ensure that submitted command buffers (and any resources they reference, depending on
     *   the command buffer create flags) remain valid until GPU execution has completed.
     *
     * ## Backend notes
     * - Vulkan ultimately maps to `vkQueueSubmit` and may generate additional internal command buffers to perform
     *   resource initialization (e.g., image layout transitions).
     * - OpenGL replays recorded commands on the current context and optionally uses `glFlush` to ensure work is
     * scheduled when a fence is requested or when waiting-until-scheduled.
     * - Metal commits `MTLCommandBuffer` objects and may queue semaphore waits/signals on an auxiliary buffer.
     * - NoOp backends may implement this as a no-op.
     */
    virtual void submitCommands(std::span<snap::rhi::Semaphore*> waitSemaphores,
                                std::span<const snap::rhi::PipelineStageBits> waitDstStageMask,
                                std::span<snap::rhi::CommandBuffer*> buffers,
                                std::span<snap::rhi::Semaphore*> signalSemaphores,
                                snap::rhi::CommandBufferWaitType waitType,
                                snap::rhi::Fence* fence) = 0;

    /**
     * @brief Convenience overload to submit a single command buffer.
     *
     * Equivalent to calling `submitCommands()` with:
     * - a single-element `buffers` span,
     * - empty wait/signal semaphore lists.
     *
     * @param commandBuffer Command buffer to submit.
     * @param waitType Whether to block the CPU until the submission is scheduled/completed.
     * @param fence Optional fence that will be signaled when the submission completes.
     */
    void submitCommandBuffer(CommandBuffer* commandBuffer,
                             CommandBufferWaitType waitType = CommandBufferWaitType::DoNotWait,
                             Fence* fence = nullptr);

    /**
     * @brief Blocks until previously submitted work is at least scheduled for execution.
     *
     * @note On Vulkan, work is effectively scheduled at `vkQueueSubmit` time. On OpenGL, implementations may use
     * `glFlush` to push commands to the driver.
     */
    virtual void waitUntilScheduled() = 0;

    /**
     * @brief Blocks until all previously submitted work to this queue is completed.
     *
     * @note Commonly maps to `vkQueueWaitIdle` (Vulkan), `-[MTLCommandBuffer waitUntilCompleted]` for the last
     * submitted command buffer (Metal), or `glFinish`/equivalent mechanisms (OpenGL).
     */
    virtual void waitIdle() = 0;

    /**
     * @brief Returns estimated CPU memory usage attributed to this queue.
     */
    [[nodiscard]] uint64_t getCPUMemoryUsage() const override {
        return 0;
    }

    /**
     * @brief Returns estimated GPU/driver memory usage attributed to this queue.
     */
    [[nodiscard]] uint64_t getGPUMemoryUsage() const override {
        return 0;
    }

private:
};
} // namespace snap::rhi
