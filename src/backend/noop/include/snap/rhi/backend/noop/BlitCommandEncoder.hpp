#pragma once

#include "snap/rhi/BlitCommandEncoder.hpp"
#include "snap/rhi/Device.hpp"
#include "snap/rhi/backend/common/CommandEncoder.h"

#include <string_view>

namespace snap::rhi::backend::common {
class Device;
} // namespace snap::rhi::backend::common

namespace snap::rhi::backend::noop {
class CommandBuffer;

using BlitCommandEncoderBase = snap::rhi::backend::common::CommandEncoder<snap::rhi::BlitCommandEncoder>;
class BlitCommandEncoder final : public BlitCommandEncoderBase {
public:
    BlitCommandEncoder(snap::rhi::backend::common::Device* device,
                       snap::rhi::backend::noop::CommandBuffer* commandBuffer);
    ~BlitCommandEncoder() = default;

    void beginEncoding() override;
    void writeTimestamp(snap::rhi::QueryPool* queryPool,
                        uint32_t query,
                        const snap::rhi::TimestampLocation location) override;
    void copyBuffer(snap::rhi::Buffer* srcBuffer,
                    snap::rhi::Buffer* dstBuffer,
                    std::span<const BufferCopy> info) override;
    void copyBufferToTexture(snap::rhi::Buffer* srcBuffer,
                             snap::rhi::Texture* dstTexture,
                             std::span<const BufferTextureCopy> info) override;
    void copyTextureToBuffer(snap::rhi::Texture* srcTexture,
                             snap::rhi::Buffer* dstBuffer,
                             std::span<const BufferTextureCopy> info) override;
    void copyTexture(snap::rhi::Texture* srcTexture,
                     snap::rhi::Texture* dstTexture,
                     std::span<const TextureCopy> info) override;
    void generateMipmaps(snap::rhi::Texture* texture) override;
    void pipelineBarrier(snap::rhi::PipelineStageBits srcStageMask,
                         snap::rhi::PipelineStageBits dstStageMask,
                         snap::rhi::DependencyFlags dependencyFlags,
                         std::span<snap::rhi::MemoryBarrierInfo> memoryBarriers,
                         std::span<snap::rhi::BufferMemoryBarrierInfo> bufferMemoryBarriers,
                         std::span<snap::rhi::TextureMemoryBarrierInfo> textureMemoryBarriers) final;
    void endEncoding() override;

    void beginDebugGroup(std::string_view label) override;
    void endDebugGroup() override;

private:
};
} // namespace snap::rhi::backend::noop
