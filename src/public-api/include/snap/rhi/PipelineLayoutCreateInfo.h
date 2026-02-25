#pragma once

#include <span>

namespace snap::rhi {
class DescriptorSetLayout;

/**
 * @brief Pipeline layout creation parameters.
 *
 * A pipeline layout defines the resource binding interface that pipelines and command encoders use when binding
 * descriptor sets.
 *
 * The layout is expressed as an array of descriptor set layouts indexed by "set" number.
 *
 * @note This struct is API-agnostic, but the concept is closest to Vulkan's pipeline layout.
 */
struct PipelineLayoutCreateInfo {
    /**
     * @brief Descriptor set layouts indexed by set number.
     *
     * The index in this span maps directly to the shader-visible set index:
     * - `setLayouts[0]` corresponds to set 0
     * - `setLayouts[1]` corresponds to set 1
     * - etc.
     *
     * If a specific set index is not used (a "gap"), you should create an empty descriptor set layout
     * (with zero bindings) and place it at that index. This ensures compatibility with all backends.
     *
     * @warning All entries must be non-null. Some backends (e.g., Vulkan) require valid descriptor set
     * layouts for all indices. Create an empty `DescriptorSetLayout` (with no bindings) for unused set indices
     * rather than using `nullptr`.
     *
     * @note The referenced descriptor set layout objects must remain alive for as long as the pipeline layout is
     * used.
     */
    std::span<snap::rhi::DescriptorSetLayout*> setLayouts;

    // TODO(vdeviatkov): add PushConstant support
    // uint32_t pushConstantRangeCount;
    // const VkPushConstantRange* pPushConstantRanges;
};
} // namespace snap::rhi
