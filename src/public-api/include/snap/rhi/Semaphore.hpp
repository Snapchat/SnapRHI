//
//  Semaphore.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 21.09.2020.
//  Copyright © 2021 Snapchat. All rights reserved.
//

#pragma once

#include "snap/rhi/DeviceChild.hpp"
#include "snap/rhi/SemaphoreCreateInfo.h"

namespace snap::rhi {
class Device;

/**
 * @brief Queue-to-queue synchronization primitive.
 *
 * A Semaphore is used to order GPU work submitted to command queues.
 * It is primarily consumed by `snap::rhi::CommandQueue::submitCommands()` as part of the wait/signal lists.
 *
 * Semantics (binary semaphore):
 * - A signal operation performed by a queue submission can satisfy (wake) one wait operation in a subsequent
 *   submission.
 * - Waiting on a semaphore that has not been signaled is invalid; recordings/submissions must establish a proper
 *   signal-before-wait dependency chain.
 *
 * Backend notes:
 * - Vulkan maps this to a native `VkSemaphore`.
 * - OpenGL emulates semaphores using sync objects (GLsync) and CPU coordination.
 * - Metal may implement semaphores using GPU events when available, otherwise a CPU implementation; depending on the
 *   platform, some queue-wait semantics may be approximated.
 */
class Semaphore : public snap::rhi::DeviceChild {
public:
    Semaphore(Device* device, const SemaphoreCreateInfo& info);
    ~Semaphore() override = default;

    /**
     * @brief Returns estimated CPU memory usage attributed to this semaphore.
     */
    [[nodiscard]] uint64_t getCPUMemoryUsage() const override {
        return 0;
    }

    /**
     * @brief Returns estimated GPU/driver memory usage attributed to this semaphore.
     */
    [[nodiscard]] uint64_t getGPUMemoryUsage() const override {
        return 0;
    }

private:
};
} // namespace snap::rhi
