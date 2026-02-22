#pragma once

namespace snap::rhi {
class DescriptorSetLayout;
class DescriptorPool;

/**
 * @brief Configuration for creating a @ref snap::rhi::DescriptorSet.
 *
 * Descriptor sets are allocated from a @ref snap::rhi::DescriptorPool and must be compatible with the provided
 * @ref snap::rhi::DescriptorSetLayout.
 *
 * @note Backend behavior
 * - **Vulkan**: allocates a native `VkDescriptorSet` from @ref descriptorPool using @ref descriptorSetLayout.
 *   Allocation can fail if the pool is exhausted; in that case, `Device::createDescriptorSet()` may return `nullptr`.
 * - **Metal**: sub-allocates an argument-buffer region from @ref descriptorPool. If allocation fails,
 *   `Device::createDescriptorSet()` returns `nullptr`.
 * - **OpenGL**: creates a lightweight object that stores bound resources; allocation typically does not fail.
 */
struct DescriptorSetCreateInfo {
    /**
     * @brief Descriptor set layout describing the binding numbers and descriptor types.
     *
     * This layout defines which bindings are valid and what resource type is expected at each binding.
     * It must match the shader/pipeline layout used when binding the descriptor set.
     */
    DescriptorSetLayout* descriptorSetLayout = nullptr;

    /**
     * @brief Descriptor pool from which the descriptor set is allocated.
     *
     * @warning Lifetime
     * The pool is expected to outlive any descriptor sets allocated from it.
     */
    DescriptorPool* descriptorPool = nullptr;
};
} // namespace snap::rhi
