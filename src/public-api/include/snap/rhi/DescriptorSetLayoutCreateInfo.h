#pragma once

#include "snap/rhi/Enums.h"
#include "snap/rhi/common/HashCombine.h"
#include <compare>
#include <vector>

namespace snap::rhi {
/**
 * @brief A single binding entry within a descriptor set layout.
 *
 * This structure describes what resource type is expected at a given binding index and which shader stages may access
 * it.
 *
 * Backend notes:
 * - Vulkan: maps to `VkDescriptorSetLayoutBinding`.
 * - Some backends may use this purely as metadata for validation and binding.
 */
struct DescriptorSetLayoutBinding {
    /**
     * @brief Binding number within the descriptor set.
     *
     * This value corresponds to the binding index declared in shader code (for example GLSL/SPIR-V `layout(binding =
     * N)`).
     */
    uint32_t binding = 0;

    /**
     * @brief Descriptor/resource type expected at this binding.
     */
    DescriptorType descriptorType = DescriptorType::Undefined;

    /**
     * @brief Shader stages that can access this binding.
     */
    ShaderStageBits stageBits = ShaderStageBits::None;

    constexpr friend auto operator<=>(const DescriptorSetLayoutBinding&,
                                      const DescriptorSetLayoutBinding&) noexcept = default;
};

/**
 * @brief Descriptor set layout creation parameters.
 *
 * A descriptor set layout defines the list of bindings (by index) that a descriptor set can contain.
 */
struct DescriptorSetLayoutCreateInfo {
    std::vector<DescriptorSetLayoutBinding> bindings;
};
} // namespace snap::rhi

namespace std {
/**
 * @brief Hash specialization for `snap::rhi::DescriptorSetLayoutCreateInfo`.
 *
 * This hash combines all binding entries in order. If the stored bindings order changes, the hash changes.
 */
template<>
struct hash<snap::rhi::DescriptorSetLayoutCreateInfo> {
    size_t operator()(const snap::rhi::DescriptorSetLayoutCreateInfo& info) const noexcept {
        size_t result = 0;
        for (const auto& descInfo : info.bindings) {
            result = snap::rhi::common::hash_combine(result,
                                                     descInfo.binding,
                                                     static_cast<uint32_t>(descInfo.descriptorType),
                                                     static_cast<uint32_t>(descInfo.stageBits));
        }
        return result;
    }
};
} // namespace std
