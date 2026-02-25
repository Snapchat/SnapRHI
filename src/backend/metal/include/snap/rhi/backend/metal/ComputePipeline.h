//
//  ComputePipeline.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 12/13/22.
//

#pragma once

#include "snap/rhi/ComputePipeline.hpp"
#include "snap/rhi/backend/common/ValidationLayer.hpp"
#include "snap/rhi/reflection/ComputePipelineInfo.h"

#include <future>
#include <mutex>
#include <span>

#include <Metal/Metal.h>

namespace snap::rhi {
class ShaderModule;
class PipelineCache;
} // namespace snap::rhi

namespace snap::rhi::backend::metal {
class Device;

class ComputePipeline final : public snap::rhi::ComputePipeline {
public:
    ComputePipeline(Device* mtlDevice, const snap::rhi::ComputePipelineCreateInfo& info);
    ComputePipeline(Device* mtlDevice,
                    const snap::rhi::ComputePipelineCreateInfo& info,
                    const std::optional<reflection::ComputePipelineInfo>& reflectionInfo,
                    const id<MTLFunction>& computeFunction,
                    const id<MTLComputePipelineState>& pipelineState);
    ~ComputePipeline() override = default;

    const id<MTLFunction>& getComputeFunction() const;
    const id<MTLComputePipelineState>& getComputePipeline() const;
    const std::optional<reflection::ComputePipelineInfo>& getReflectionInfo() const override;

    void setDebugLabel(std::string_view label) override;

private:
    void sync() const;

    mutable std::once_flag syncFlag;
    mutable std::future<void> asyncFuture;
    std::promise<void> asyncPromise;

    const snap::rhi::backend::common::ValidationLayer& validationLayer;

private:
    id<MTLFunction> computeFunction = nil;
    id<MTLComputePipelineState> pipelineState = nil;
};
} // namespace snap::rhi::backend::metal
