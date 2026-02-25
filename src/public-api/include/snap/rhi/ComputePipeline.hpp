//
//  ComputePipeline.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 12/6/22.
//

#pragma once

#include <optional>
#include <string_view>

#include "snap/rhi/ComputePipelineCreateInfo.h"
#include "snap/rhi/DeviceChild.hpp"
#include "snap/rhi/reflection/ComputePipelineInfo.h"

namespace snap::rhi {
class Device;

/**
 * @brief Compute pipeline object.
 *
 * A compute pipeline encapsulates the backend-specific compute pipeline state created from a
 * `snap::rhi::ComputePipelineCreateInfo` (shader stage, pipeline layout, optional cache/base pipeline, and creation
 * flags).
 *
 * Pipelines are created via `snap::rhi::Device::createComputePipeline()`.
 *
 * ## Reflection
 * `getReflectionInfo()` may return shader/pipeline reflection information (descriptor set layouts, bindings, etc.).
 * Reflection availability is backend- and flag-dependent:
 * - OpenGL/Metal typically populate reflection after pipeline creation/synchronization.
 * - Vulkan may populate reflection only if `PipelineCreateFlags::AcquireNativeReflection` was requested and the shader
 *   module provides reflection data.
 */
class ComputePipeline : public snap::rhi::DeviceChild {
public:
    ComputePipeline(Device* device, const ComputePipelineCreateInfo& info);
    ~ComputePipeline() override = default;

    /**
     * @brief Returns the immutable creation info used to create this pipeline.
     */
    const ComputePipelineCreateInfo& getCreateInfo() const {
        return info;
    }

    /**
     * @brief Returns reflection information for this pipeline, if available.
     *
     * @return An optional reflection structure. If reflection is not available on the active backend (or was not
     * requested), the optional may be empty.
     *
     * @note Some backends may perform lazy synchronization before returning reflection (e.g., OpenGL/Metal ensure the
     * pipeline is compiled/ready before exposing reflection).
     */
    virtual const std::optional<reflection::ComputePipelineInfo>& getReflectionInfo() const;

    /**
     * @brief Returns CPU memory usage attributed to this pipeline.
     */
    uint64_t getCPUMemoryUsage() const override {
        return 0;
    }

    /**
     * @brief Returns GPU memory usage attributed to this pipeline.
     */
    uint64_t getGPUMemoryUsage() const override {
        return 0;
    }

protected:
    snap::rhi::ComputePipelineCreateInfo info{};
    mutable std::optional<reflection::ComputePipelineInfo> reflection = std::nullopt;

private:
};
} // namespace snap::rhi
