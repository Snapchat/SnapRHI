#pragma once

#include "snap/rhi/backend/common/CommandEncoder.h"
#include "snap/rhi/backend/common/Device.hpp"
#include "snap/rhi/backend/common/ResourceResidencySet.h"
#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/common/ValidationLayer.hpp"
#include "snap/rhi/backend/opengl/CommandAllocator.hpp"

namespace snap::rhi::backend::opengl {
class Device;
class CommandBuffer;

template<typename CommandEncoderBase>
class CommandEncoder : public snap::rhi::backend::common::CommandEncoder<CommandEncoderBase> {
public:
    CommandEncoder(snap::rhi::backend::opengl::Device* device,
                   snap::rhi::backend::opengl::CommandBuffer* commandBuffer);
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

    void beginDebugGroup(std::string_view label) final;

    void endDebugGroup() final;

protected:
    snap::rhi::backend::opengl::CommandAllocator& commandAllocator;
};
} // namespace snap::rhi::backend::opengl
