#pragma once

#include "snap/rhi/CommandEncoder.h"
#include "snap/rhi/backend/common/CommandEncoder.h"
#include "snap/rhi/backend/common/ResourceResidencySet.h"
#include "snap/rhi/backend/common/ValidationLayer.hpp"
#include "snap/rhi/backend/vulkan/vulkan.h"

namespace snap::rhi::backend::vulkan {
class Device;
class CommandBuffer;

template<typename CommandEncoderBase>
class CommandEncoder : public snap::rhi::backend::common::CommandEncoder<CommandEncoderBase> {
public:
    CommandEncoder(snap::rhi::backend::vulkan::Device* vkDevice,
                   snap::rhi::backend::vulkan::CommandBuffer* commandBuffer);
    ~CommandEncoder() override = default;

    void writeTimestamp(snap::rhi::QueryPool* queryPool,
                        uint32_t query,
                        const snap::rhi::TimestampLocation location) override final;
    void pipelineBarrier(snap::rhi::PipelineStageBits srcStageMask,
                         snap::rhi::PipelineStageBits dstStageMask,
                         snap::rhi::DependencyFlags dependencyFlags,
                         std::span<snap::rhi::MemoryBarrierInfo> memoryBarriers,
                         std::span<snap::rhi::BufferMemoryBarrierInfo> bufferMemoryBarriers,
                         std::span<snap::rhi::TextureMemoryBarrierInfo> textureMemoryBarriers) override final;

    void beginDebugGroup(std::string_view label) override final;
    void endDebugGroup() override final;

protected:
};
} // namespace snap::rhi::backend::vulkan
