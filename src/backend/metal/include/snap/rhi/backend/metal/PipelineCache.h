//
//  PipelineCache.h
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 01.06.2021.
//

#pragma once

#include "snap/rhi/ComputePipelineCreateInfo.h"
#include "snap/rhi/PipelineCache.hpp"
#include "snap/rhi/RenderPassCreateInfo.h"
#include "snap/rhi/RenderPipelineCreateInfo.h"

#include <Metal/Metal.h>
#include <mutex>

namespace snap::rhi::backend::metal {
class Device;
class Context;

// https://developer.apple.com/documentation/metal/mtlbinaryarchive?language=objc
// https://developer.apple.com/documentation/metal/mtlrenderpipelinedescriptor/3554050-binaryarchives?language=objc
// https://developer.apple.com/documentation/metal/mtlcomputepipelinedescriptor/3553961-binaryarchives?language=objc

/**
 * When you create a Metal library, Metal compiles shader functions into an intermediate representation.
 * When you create the pipeline state object, this intermediate code is compiled for the specific GPU.
 * If you specify an array of MTLBinaryArchive objects, when Metal creates the pipeline state object,
 * it first checks the archives to see if there is already a compiled function. If so, Metal uses it instead.
 *
 * https://developer.apple.com/documentation/metal/mtlcomputepipelinedescriptor/3553961-binaryarchives?language=objc
 */
class PipelineCache final : public snap::rhi::PipelineCache {
public:
    explicit PipelineCache(Device* mtlDevice, const snap::rhi::PipelineCacheCreateInfo& info);
    ~PipelineCache() override = default;

    API_AVAILABLE(macos(11.0), ios(14.0))
    const id<MTLBinaryArchive>& getBinaryArchive() const;

    void addRenderPipeline(const snap::rhi::RenderPipelineCreateInfo& info,
                           const id<MTLFunction>& vertex,
                           const id<MTLFunction>& fragment);

    void addComputePipeline(const snap::rhi::ComputePipelineCreateInfo& info, const id<MTLFunction>& compute);

    Result serializeToFile(const std::filesystem::path& cachePath) const override;

    void setDebugLabel(std::string_view label) override;

private:
    struct RenderPipelineInfo {
        snap::rhi::RenderPipelineCreateInfo info{};
        id<MTLFunction> vertex = nil;
        id<MTLFunction> fragment = nil;
    };
    mutable std::vector<RenderPipelineInfo> renderPipelines;

    struct ComputePipelineInfo {
        snap::rhi::ComputePipelineCreateInfo info{};
        id<MTLFunction> compute = nil;
    };
    mutable std::vector<ComputePipelineInfo> computePipelines;

    API_AVAILABLE(macos(11.0), ios(14.0))
    id<MTLBinaryArchive> archive = nil;
};
} // namespace snap::rhi::backend::metal
