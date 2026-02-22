#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

#include "snap/rhi/DeviceChild.hpp"
#include "snap/rhi/RenderPipelineCreateInfo.h"
#include "snap/rhi/reflection/RenderPipelineInfo.h"

namespace snap::rhi {
class Device;

/**
 * @brief Compiled graphics pipeline state object.
 *
 * A RenderPipeline encapsulates the GPU state required to render: shader stages, fixed-function state, vertex input
 * layout, and (backend-dependent) compiled pipeline objects.
 *
 * RenderPipeline objects are created by `snap::rhi::Device::createRenderPipeline()`.
 *
 * Reflection:
 * - When `PipelineCreateFlags::AcquireNativeReflection` (or equivalent) is enabled and supported by the backend,
 *   `getReflectionInfo()` may return a populated `reflection::RenderPipelineInfo`.
 * - Otherwise, `getReflectionInfo()` returns an empty optional.
 */
class RenderPipeline : public snap::rhi::DeviceChild {
public:
    RenderPipeline(Device* device, const RenderPipelineCreateInfo& info);
    ~RenderPipeline() override = default;

    /**
     * @brief Returns the creation parameters used to construct this pipeline.
     */
    const RenderPipelineCreateInfo& getCreateInfo() const {
        return info;
    }

    /**
     * @brief Returns optional reflection information for the pipeline.
     */
    virtual const std::optional<reflection::RenderPipelineInfo>& getReflectionInfo() const;

    /**
     * @brief Returns an estimate of CPU-side memory used by this object.
     */
    uint64_t getCPUMemoryUsage() const override {
        return 0;
    }

    /**
     * @brief Returns an estimate of GPU-side memory used by this object.
     */
    uint64_t getGPUMemoryUsage() const override {
        return 0;
    }

protected:
    RenderPipelineCreateInfo info{};

    /// Backends may populate reflection data synchronously or lazily.
    mutable std::optional<reflection::RenderPipelineInfo> reflection = std::nullopt;

private:
};
} // namespace snap::rhi
