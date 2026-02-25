//
//  ComputeCommandEncoder.h
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 12/14/22.
//

#pragma once

#include "snap/rhi/ComputeCommandEncoder.hpp"
#include "snap/rhi/backend/common/ValidationLayer.hpp"
#include "snap/rhi/backend/metal/CommandEncoder.hpp"
#include "snap/rhi/backend/metal/Context.h"
#include "snap/rhi/backend/metal/PipelineResourceState.h"
#include <Metal/Metal.h>

namespace snap::rhi::backend::metal {
class Device;
class CommandBuffer;

using ComputeCommandEncoderBase = snap::rhi::backend::metal::CommandEncoder<snap::rhi::ComputeCommandEncoder>;
class ComputeCommandEncoder final : public ComputeCommandEncoderBase {
public:
    ComputeCommandEncoder(Device* mtlDevice, CommandBuffer* commandBuffer);
    ~ComputeCommandEncoder() override = default;

    void beginEncoding() override;
    void bindDescriptorSet(uint32_t binding,
                           snap::rhi::DescriptorSet* descriptorSet,
                           std::span<const uint32_t> dynamicOffsets) override;
    void bindComputePipeline(snap::rhi::ComputePipeline* pipeline) override;
    void dispatch(const uint32_t groupSizeX, const uint32_t groupSizeY, const uint32_t groupSizeZ) override;
    void pipelineBarrier(snap::rhi::PipelineStageBits srcStageMask,
                         snap::rhi::PipelineStageBits dstStageMask,
                         snap::rhi::DependencyFlags dependencyFlags,
                         std::span<snap::rhi::MemoryBarrierInfo> memoryBarriers,
                         std::span<snap::rhi::BufferMemoryBarrierInfo> bufferMemoryBarriers,
                         std::span<snap::rhi::TextureMemoryBarrierInfo> textureMemoryBarriers) override;
    void endEncoding() override;

private:
    Context& context;
    MTLSize threadsPerThreadgroup{};
    PipelineResourceState pipelineState;
};
} // namespace snap::rhi::backend::metal
