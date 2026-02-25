#pragma once

#include "snap/rhi/ComputeCommandEncoder.hpp"
#include "snap/rhi/backend/common/CommandEncoder.h"
#include "snap/rhi/backend/common/ValidationLayer.hpp"

namespace snap::rhi::backend::common {
class Device;
} // namespace snap::rhi::backend::common

namespace snap::rhi::backend::noop {
class CommandBuffer;

using ComputeCommandEncoderBase = snap::rhi::backend::common::CommandEncoder<snap::rhi::ComputeCommandEncoder>;
class ComputeCommandEncoder final : public ComputeCommandEncoderBase {
public:
    ComputeCommandEncoder(snap::rhi::backend::common::Device* device,
                          snap::rhi::backend::noop::CommandBuffer* commandBuffer);
    ~ComputeCommandEncoder() = default;

    void beginEncoding() override;
    void writeTimestamp(snap::rhi::QueryPool* queryPool,
                        uint32_t query,
                        const snap::rhi::TimestampLocation location) override;
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

    void beginDebugGroup(std::string_view label) override;
    void endDebugGroup() override;

private:
    const snap::rhi::backend::common::ValidationLayer& validationLayer;
};
} // namespace snap::rhi::backend::noop
