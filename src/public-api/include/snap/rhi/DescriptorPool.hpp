//
//  DescriptorPool.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 8/23/22.
//

#pragma once

#include "snap/rhi/DescriptorPoolCreateInfo.h"
#include "snap/rhi/DeviceChild.hpp"

namespace snap::rhi {
class Device;

/**
 * @brief Descriptor pool object.
 *
 * A `DescriptorPool` is an allocation arena used by backends to create descriptor sets.
 *
 * Instances are created via `Device::createDescriptorPool()` and are typically referenced by `DescriptorSetCreateInfo`.
 *
 * @note Backend behavior
 * - **Vulkan**: wraps a `VkDescriptorPool` and uses it to allocate/free `VkDescriptorSet` objects. Allocation can fail
 * when `maxSets` or per-type `descriptorCount` is exhausted.
 * - **Metal**: manages an internal argument-buffer backing store and sub-allocates regions for descriptor sets.
 * - **OpenGL**: there is no native descriptor pool concept; the object exists mainly to match the public API.
 * - **NoOp**: performs no work.
 *
 * @warning Lifetime
 * Backends assume the pool outlives any descriptor sets allocated from it (e.g., Metal stores subrange deleters that
 * reference the pool).
 */
class DescriptorPool : public snap::rhi::DeviceChild {
public:
    DescriptorPool(Device* device, const DescriptorPoolCreateInfo& info);
    ~DescriptorPool() override = default;

    /**
     * @brief Returns the immutable creation info used to create this pool.
     */
    [[nodiscard]] const DescriptorPoolCreateInfo& getCreateInfo() const {
        return info;
    }

    /**
     * @brief Returns CPU memory usage attributed to this pool.
     */
    [[nodiscard]] uint64_t getCPUMemoryUsage() const override {
        return 0;
    }

    /**
     * @brief Returns GPU memory usage attributed to this pool.
     */
    [[nodiscard]] uint64_t getGPUMemoryUsage() const override {
        return 0;
    }

protected:
    DescriptorPoolCreateInfo info{};
};
} // namespace snap::rhi
