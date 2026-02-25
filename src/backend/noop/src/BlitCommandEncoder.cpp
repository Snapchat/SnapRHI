#include "snap/rhi/backend/noop/BlitCommandEncoder.hpp"

#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/noop/Device.hpp"

#include "snap/rhi/backend/noop/CommandBuffer.hpp"
#include "snap/rhi/backend/noop/Texture.hpp"

#include <algorithm>
#include <sstream>

namespace snap::rhi::backend::noop {
BlitCommandEncoder::BlitCommandEncoder(snap::rhi::backend::common::Device* device,
                                       snap::rhi::backend::noop::CommandBuffer* commandBuffer)
    : BlitCommandEncoderBase(device, commandBuffer) {}

void BlitCommandEncoder::beginEncoding() {
    BlitCommandEncoderBase::onBeginEncoding();
}

void BlitCommandEncoder::writeTimestamp(snap::rhi::QueryPool* queryPool,
                                        uint32_t query,
                                        const snap::rhi::TimestampLocation location) {}

void BlitCommandEncoder::copyBuffer(snap::rhi::Buffer* srcBuffer,
                                    snap::rhi::Buffer* dstBuffer,
                                    std::span<const BufferCopy> info) {}

void BlitCommandEncoder::copyBufferToTexture(snap::rhi::Buffer* srcBuffer,
                                             snap::rhi::Texture* dstTexture,
                                             std::span<const BufferTextureCopy> info) {}

void BlitCommandEncoder::copyTextureToBuffer(snap::rhi::Texture* srcTexture,
                                             snap::rhi::Buffer* dstBuffer,
                                             std::span<const BufferTextureCopy> info) {}

void BlitCommandEncoder::copyTexture(snap::rhi::Texture* srcTexture,
                                     snap::rhi::Texture* dstTexture,
                                     std::span<const TextureCopy> info) {}

void BlitCommandEncoder::generateMipmaps(snap::rhi::Texture* texture) {}

void BlitCommandEncoder::pipelineBarrier(snap::rhi::PipelineStageBits srcStageMask,
                                         snap::rhi::PipelineStageBits dstStageMask,
                                         snap::rhi::DependencyFlags dependencyFlags,
                                         std::span<snap::rhi::MemoryBarrierInfo> memoryBarriers,
                                         std::span<snap::rhi::BufferMemoryBarrierInfo> bufferMemoryBarriers,
                                         std::span<snap::rhi::TextureMemoryBarrierInfo> textureMemoryBarriers) {}

void BlitCommandEncoder::endEncoding() {
    BlitCommandEncoderBase::onEndEncoding();
}

void BlitCommandEncoder::beginDebugGroup(std::string_view name) {}

void BlitCommandEncoder::endDebugGroup() {}

} // namespace snap::rhi::backend::noop
