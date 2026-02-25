//
//  DescriptorPoolCreateInfo.h
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 8/23/22.
//

#pragma once

#include "snap/rhi/Enums.h"

namespace snap::rhi {
/**
 * @brief Configuration for creating a @ref snap::rhi::DescriptorPool.
 *
 * This struct loosely mirrors Vulkan's `VkDescriptorPoolCreateInfo` / `VkDescriptorPoolSize`, but is used across all
 * backends.
 *
 * @note Backend behavior
 * - **Vulkan**: uses @ref maxSets and @ref descriptorCount to size a native `VkDescriptorPool`. Descriptor set
 * allocation may fail (returning `nullptr` from `Device::createDescriptorSet`) if either limit is exhausted.
 * - **Metal**: uses these values to size an internal argument-buffer backing store and sub-allocate ranges for
 *   descriptor sets.
 * - **OpenGL/NoOp**: the object exists mostly to satisfy the common API; limits may not be enforced.
 */
struct DescriptorPoolCreateInfo {
    /**
     * @brief Maximum number of descriptor sets that can be allocated from the pool.
     *
     * @note Vulkan uses this to set `VkDescriptorPoolCreateInfo::maxSets`.
     */
    uint32_t maxSets = 0;

    /**
     * @brief Maximum number of descriptors per descriptor type that can be allocated from the pool.
     *
     * This is indexed by `static_cast<uint32_t>(snap::rhi::DescriptorType)`.
     *
     * @note Vulkan uses this to populate `VkDescriptorPoolSize` entries.
     */
    std::array<uint32_t, static_cast<uint32_t>(snap::rhi::DescriptorType::Count)> descriptorCount{};
};
} // namespace snap::rhi
