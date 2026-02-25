#pragma once

#include "snap/rhi/DeviceChild.hpp"
#include "snap/rhi/PipelineLayoutCreateInfo.h"

namespace snap::rhi {
class Device;

/**
 * @brief Pipeline resource binding layout.
 *
 * `PipelineLayout` defines the descriptor set layout(s) that will be used by pipelines and command encoders when
 * binding resources.
 *
 * This is an API-agnostic object:
 * - Vulkan creates a native `VkPipelineLayout` from the provided descriptor set layouts.
 * - Other backends may not have a direct native equivalent; they typically store/validate the set layouts and use
 *   them to validate resource bindings.
 */
class PipelineLayout : public snap::rhi::DeviceChild {
public:
    PipelineLayout(Device* device, const PipelineLayoutCreateInfo& info);
    ~PipelineLayout() override = default;

    /**
     * @brief Returns the immutable create info used to build this pipeline layout.
     */
    const PipelineLayoutCreateInfo& getCreateInfo() const {
        return info;
    }

    /**
     * @brief Returns an estimate of CPU memory usage for this object.
     */
    uint64_t getCPUMemoryUsage() const override {
        return 0;
    }

    /**
     * @brief Returns an estimate of GPU/driver memory usage for this object.
     */
    uint64_t getGPUMemoryUsage() const override {
        return 0;
    }

protected:
    PipelineLayoutCreateInfo info{};
};
} // namespace snap::rhi
