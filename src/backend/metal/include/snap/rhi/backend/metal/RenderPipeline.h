//
//  GraphicsPipeline.h
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 1/20/20.
//  Copyright © 2020 Snapchat. All rights reserved.
//

#pragma once

#include "snap/rhi/RenderPipeline.hpp"
#include "snap/rhi/backend/common/ValidationLayer.hpp"
#include "snap/rhi/reflection/RenderPipelineInfo.h"

#include <Metal/Metal.h>

#include <future>
#include <mutex>
#include <span>

namespace snap::rhi {
class ShaderModule;
class PipelineCache;
} // namespace snap::rhi

namespace snap::rhi::backend::metal {
class Device;

class RenderPipeline final : public snap::rhi::RenderPipeline {
public:
    RenderPipeline(Device* mtlDevice, const snap::rhi::RenderPipelineCreateInfo& info);
    RenderPipeline(Device* mtlDevice,
                   const snap::rhi::RenderPipelineCreateInfo& info,
                   const snap::rhi::reflection::RenderPipelineInfo& reflectionInfo,
                   const id<MTLRenderPipelineState>& state);
    ~RenderPipeline() override = default;

    const id<MTLRenderPipelineState>& getRenderPipeline() const;
    const id<MTLDepthStencilState>& getDepthStencilState() const;

    const std::optional<reflection::RenderPipelineInfo>& getReflectionInfo() const override;

private:
    void initDepthStencilState(Device* mtlDevice, snap::rhi::RenderPipeline* basePipeline);
    void sync() const;

    mutable std::once_flag syncFlag;
    mutable std::future<void> asyncFuture;
    std::promise<void> asyncPromise;
    const snap::rhi::backend::common::ValidationLayer& validationLayer;

private:
    id<MTLRenderPipelineState> pipelineState = nil;
    id<MTLDepthStencilState> depthStencilState = nil;
};
} // namespace snap::rhi::backend::metal
