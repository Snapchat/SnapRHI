//
//  DescriptorSetLayout.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 8/2/22.
//

#pragma once

#include <cstdint>

#include "snap/rhi/DescriptorSetLayoutCreateInfo.h"
#include "snap/rhi/DeviceChild.hpp"

namespace snap::rhi {
class Device;

/**
 * @brief Describes the binding interface of a descriptor set.
 *
 * A descriptor set layout is a (typically immutable) description of the resources (buffers, textures, samplers, etc.)
 * a descriptor set may contain and in which shader stages those resources are visible.
 *
 * Backend notes:
 * - Vulkan: creates an underlying `VkDescriptorSetLayout`.
 * - Other backends (for example Metal/OpenGL/noop): may treat this object as a CPU-side description used for validation
 *   and/or resource binding.
 *
 * SnapRHI requires that descriptors sort their bindings by `binding` index and that duplicate binding indices are
 * removed.
 *
 */
class DescriptorSetLayout : public snap::rhi::DeviceChild {
public:
    DescriptorSetLayout(Device* device, const DescriptorSetLayoutCreateInfo& info);
    ~DescriptorSetLayout() override = default;

    /**
     * @brief Returns the create info used to build this layout.
     */
    const DescriptorSetLayoutCreateInfo& getCreateInfo() const {
        return info;
    }

    /**
     * @brief Returns estimated CPU memory usage for this object.
     *
     * This value currently accounts only for CPU-side bookkeeping (the stored bindings array) and does not include
     * backend driver allocations.
     *
     * @return Estimated CPU memory usage in bytes.
     */
    uint64_t getCPUMemoryUsage() const override {
        const uint64_t size = static_cast<uint64_t>(info.bindings.size() * sizeof(DescriptorSetLayoutBinding));
        return size;
    }

    /**
     * @brief Returns estimated GPU/driver memory usage for this object.
     *
     * GPU/driver memory tracking for descriptor set layouts is currently not implemented, so this always returns 0.
     *
     * @return Estimated GPU/driver memory usage in bytes (always 0).
     */
    uint64_t getGPUMemoryUsage() const override {
        return 0;
    }

protected:
    DescriptorSetLayoutCreateInfo info{};
};
} // namespace snap::rhi
