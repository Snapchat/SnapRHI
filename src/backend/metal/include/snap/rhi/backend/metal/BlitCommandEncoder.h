//
//  BlitCommandEncoder.h
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 07.10.2020.
//  Copyright © 2021 Snapchat. All rights reserved.
//

#pragma once

#include "snap/rhi/BlitCommandEncoder.hpp"
#include "snap/rhi/backend/common/ValidationLayer.hpp"
#include "snap/rhi/backend/metal/CommandEncoder.hpp"
#include "snap/rhi/backend/metal/Context.h"
#include <Metal/Metal.h>

namespace snap::rhi::backend::metal {
class Device;
class CommandBuffer;

using BlitCommandEncoderBase = snap::rhi::backend::metal::CommandEncoder<snap::rhi::BlitCommandEncoder>;
class BlitCommandEncoder final : public BlitCommandEncoderBase {
public:
    BlitCommandEncoder(snap::rhi::backend::metal::Device* mtlDevice, CommandBuffer* commandBuffer);
    ~BlitCommandEncoder() override = default;

    void beginEncoding() override;
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

private:
    Context& context;
};
} // namespace snap::rhi::backend::metal
