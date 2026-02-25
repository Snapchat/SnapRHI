//
//  CommandBufferCreateInfo.h
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 21.09.2020.
//  Copyright © 2021 Snapchat. All rights reserved.
//

#pragma once

#include "snap/rhi/Enums.h"
#include <compare>

namespace snap::rhi {
class CommandQueue;

/**
 * @brief Describes how to create an `snap::rhi::CommandBuffer`.
 *
 * This struct is passed to `snap::rhi::Device::createCommandBuffer()`.
 *
 * ## Queue association
 * `commandQueue` indicates which queue the command buffer is intended to be submitted on.
 * Some backends use this pointer to allocate backend-native command buffer objects from the queue (e.g., Vulkan command
 * pools).
 *
 * ## Resource retention model
 * `commandBufferCreateFlags` can control how SnapRHI retains resources referenced during command encoding.
 * The primary performance-related flag is `CommandBufferCreateFlags::UnretainedResources`.
 *
 * When `UnretainedResources` is enabled, SnapRHI may avoid retaining references to resources used by the command
 * buffer (buffers/textures/descriptor pools, etc.) to reduce CPU overhead and allocation pressure. In this mode, the
 * application is responsible for ensuring that all referenced resources remain alive (and not destroyed) until GPU
 * execution has completed.
 *
 * Backend notes:
 * - The common implementation uses this flag to select the `ResourceResidencySet` retention mode.
 * - Command queues may skip submission tracking / fence creation when resources are unretained.
 *
 * @warning Misuse can result in use-after-free on the GPU.
 */
struct CommandBufferCreateInfo {
    /**
     * @brief Command buffer creation flags.
     *
     * @see snap::rhi::CommandBufferCreateFlags
     */
    snap::rhi::CommandBufferCreateFlags commandBufferCreateFlags = snap::rhi::CommandBufferCreateFlags::None;

    /**
     * @brief Queue this command buffer is intended to be submitted on.
     *
     * @warning Must not be `nullptr` for backends that require a queue to allocate backend-native command buffers.
     */
    CommandQueue* commandQueue = nullptr;

    constexpr friend auto operator<=>(const CommandBufferCreateInfo&,
                                      const CommandBufferCreateInfo&) noexcept = default;
};
} // namespace snap::rhi
