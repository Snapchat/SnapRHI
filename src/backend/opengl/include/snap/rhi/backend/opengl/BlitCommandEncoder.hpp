//
//  BlitCommandEncoder.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 13.10.2021.
//

#pragma once

#include "snap/rhi/BlitCommandEncoder.hpp"
#include "snap/rhi/backend/common/CommandEncoder.h"
#include "snap/rhi/backend/common/ResourceResidencySet.h"
#include "snap/rhi/backend/common/ValidationLayer.hpp"
#include "snap/rhi/backend/opengl/CommandAllocator.hpp"
#include "snap/rhi/backend/opengl/CommandEncoder.hpp"

#include <string_view>

namespace snap::rhi::backend::opengl {
class Device;
class CommandBuffer;

using BlitCommandEncoderBase = CommandEncoder<snap::rhi::BlitCommandEncoder>;
class BlitCommandEncoder final : public BlitCommandEncoderBase {
public:
    BlitCommandEncoder(snap::rhi::backend::opengl::Device* device,
                       snap::rhi::backend::opengl::CommandBuffer* commandBuffer);
    ~BlitCommandEncoder() = default;

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
    void endEncoding() override;

private:
};
} // namespace snap::rhi::backend::opengl
