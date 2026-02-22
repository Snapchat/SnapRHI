#pragma once

#include "snap/rhi/BlitCommandEncoder.hpp"
#include "snap/rhi/backend/common/CommandEncoder.h"
#include "snap/rhi/backend/common/ResourceResidencySet.h"
#include "snap/rhi/backend/common/ValidationLayer.hpp"
#include "snap/rhi/backend/vulkan/CommandEncoder.hpp"
#include "snap/rhi/backend/vulkan/vulkan.h"

namespace snap::rhi::backend::vulkan {
class Device;
class CommandBuffer;

using BlitCommandEncoderBase = CommandEncoder<snap::rhi::BlitCommandEncoder>;
class BlitCommandEncoder final : public BlitCommandEncoderBase {
public:
    BlitCommandEncoder(snap::rhi::backend::vulkan::Device* vkDevice,
                       snap::rhi::backend::vulkan::CommandBuffer* commandBuffer);
    ~BlitCommandEncoder() final;

    void beginEncoding() final;
    void copyBuffer(snap::rhi::Buffer* srcBuffer, snap::rhi::Buffer* dstBuffer, std::span<const BufferCopy> info) final;
    void copyBufferToTexture(snap::rhi::Buffer* srcBuffer,
                             snap::rhi::Texture* dstTexture,
                             std::span<const BufferTextureCopy> info) final;
    void copyTextureToBuffer(snap::rhi::Texture* srcTexture,
                             snap::rhi::Buffer* dstBuffer,
                             std::span<const BufferTextureCopy> info) final;
    void copyTexture(snap::rhi::Texture* srcTexture,
                     snap::rhi::Texture* dstTexture,
                     std::span<const TextureCopy> info) final;
    void generateMipmaps(snap::rhi::Texture* texture) final;
    void endEncoding() final;

private:
};
} // namespace snap::rhi::backend::vulkan
