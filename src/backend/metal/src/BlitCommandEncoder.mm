#include "snap/rhi/backend/metal/BlitCommandEncoder.h"
#include "snap/rhi/Common.h"
#include "snap/rhi/backend/common/CommandBuffer.h"
#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/metal/Buffer.h"
#include "snap/rhi/backend/metal/CommandBuffer.h"
#include "snap/rhi/backend/metal/Device.h"
#include "snap/rhi/backend/metal/Texture.h"
#include <snap/rhi/common/Throw.h>

namespace snap::rhi::backend::metal {
BlitCommandEncoder::BlitCommandEncoder(snap::rhi::backend::metal::Device* mtlDevice, CommandBuffer* commandBuffer)
    : BlitCommandEncoderBase(mtlDevice, commandBuffer), context(commandBuffer->getContext()) {}

void BlitCommandEncoder::beginEncoding() {
    const id<MTLBlitCommandEncoder>& encoder = context.beginBlit();
    BlitCommandEncoderBase::onBeginEncoding(encoder);
}

void BlitCommandEncoder::copyBuffer(snap::rhi::Buffer* srcBuffer,
                                    snap::rhi::Buffer* dstBuffer,
                                    std::span<const BufferCopy> info) {
    Buffer* srcMtlBuffer = snap::rhi::backend::common::smart_cast<Buffer>(srcBuffer);
    Buffer* dstMtlBuffer = snap::rhi::backend::common::smart_cast<Buffer>(dstBuffer);

    for (const auto& copyInfo : info) {
        [context.getBlitEncoder() copyFromBuffer:srcMtlBuffer->getBuffer()
                                    sourceOffset:copyInfo.srcOffset
                                        toBuffer:dstMtlBuffer->getBuffer()
                               destinationOffset:copyInfo.dstOffset
                                            size:copyInfo.size];
    }

    {
        resourceResidencySet.track(srcBuffer);
        resourceResidencySet.track(dstBuffer);
    }
}

void BlitCommandEncoder::copyBufferToTexture(snap::rhi::Buffer* srcBuffer,
                                             snap::rhi::Texture* dstTexture,
                                             std::span<const BufferTextureCopy> info) {
    Buffer* srcMtlBuffer = snap::rhi::backend::common::smart_cast<Buffer>(srcBuffer);
    Texture* dstMtlTexture = snap::rhi::backend::common::smart_cast<Texture>(dstTexture);

    commandBuffer->tryPreserveInteropTexture(dstTexture);

    const auto& texInfo = dstTexture->getCreateInfo();

    for (const auto& copyInfo : info) {
        const uint32_t bytesPerRow =
            copyInfo.bytesPerRow ?
                copyInfo.bytesPerRow :
                snap::rhi::bytesPerRow(copyInfo.textureExtent.width, copyInfo.textureExtent.height, texInfo.format);
        const uint64_t bytesPerSlice =
            copyInfo.bytesPerSlice ?
                copyInfo.bytesPerSlice :
                snap::rhi::bytesPerSliceWithRow(bytesPerRow, copyInfo.textureExtent.height, texInfo.format);

        MTLOrigin mtlOrigin =
            MTLOriginMake(copyInfo.textureOffset.x, copyInfo.textureOffset.y, copyInfo.textureOffset.z);
        MTLSize mtlSize =
            MTLSizeMake(copyInfo.textureExtent.width, copyInfo.textureExtent.height, copyInfo.textureExtent.depth);

        switch (texInfo.textureType) {
            case snap::rhi::TextureType::Texture2D:
            case snap::rhi::TextureType::Texture3D: {
                assert(copyInfo.textureOffset.z + copyInfo.textureExtent.depth <= texInfo.size.depth);

                [context.getBlitEncoder() copyFromBuffer:srcMtlBuffer->getBuffer()
                                            sourceOffset:copyInfo.bufferOffset
                                       sourceBytesPerRow:bytesPerRow
                                     sourceBytesPerImage:bytesPerSlice
                                              sourceSize:mtlSize
                                               toTexture:dstMtlTexture->getTexture()
                                        destinationSlice:0 // If the texture type is neither an array nor a cube, use 0
                                        destinationLevel:copyInfo.textureSubresource.mipLevel
                                       destinationOrigin:mtlOrigin];
            } break;

            case snap::rhi::TextureType::Texture2DArray:
            case snap::rhi::TextureType::TextureCubemap: {
                mtlSize.depth = 1;
                mtlOrigin.z = 0;

                [[maybe_unused]] const uint32_t depth =
                    texInfo.size.depth * (texInfo.textureType == snap::rhi::TextureType::TextureCubemap ? 6 : 1);
                assert(copyInfo.textureOffset.z + copyInfo.textureExtent.depth <= depth);

                uint64_t offset = copyInfo.bufferOffset;
                for (uint32_t slice = 0; slice < copyInfo.textureExtent.depth; ++slice) {
                    const uint32_t destinationSlice = copyInfo.textureOffset.z + slice;
                    [context.getBlitEncoder() copyFromBuffer:srcMtlBuffer->getBuffer()
                                                sourceOffset:offset
                                           sourceBytesPerRow:bytesPerRow
                                         sourceBytesPerImage:bytesPerSlice
                                                  sourceSize:mtlSize
                                                   toTexture:dstMtlTexture->getTexture()
                                            destinationSlice:destinationSlice
                                            destinationLevel:copyInfo.textureSubresource.mipLevel
                                           destinationOrigin:mtlOrigin];
                    offset += bytesPerSlice;
                }
            } break;

            default:
                snap::rhi::common::throwException("unsupported texture type");
        }
    }

    {
        resourceResidencySet.track(srcBuffer);
        resourceResidencySet.track(dstTexture);
    }
}

void BlitCommandEncoder::copyTextureToBuffer(snap::rhi::Texture* srcTexture,
                                             snap::rhi::Buffer* dstBuffer,
                                             std::span<const BufferTextureCopy> info) {
    Texture* srcMtlTexture = snap::rhi::backend::common::smart_cast<Texture>(srcTexture);
    Buffer* dstMtlBuffer = snap::rhi::backend::common::smart_cast<Buffer>(dstBuffer);

    commandBuffer->tryPreserveInteropTexture(srcTexture);

    const auto& texInfo = srcTexture->getCreateInfo();

    for (const auto& copyInfo : info) {
        const uint32_t bytesPerRow =
            copyInfo.bytesPerRow ?
                copyInfo.bytesPerRow :
                snap::rhi::bytesPerRow(copyInfo.textureExtent.width, copyInfo.textureExtent.height, texInfo.format);
        const uint64_t bytesPerSlice =
            copyInfo.bytesPerSlice ?
                copyInfo.bytesPerSlice :
                snap::rhi::bytesPerSliceWithRow(bytesPerRow, copyInfo.textureExtent.height, texInfo.format);

        MTLOrigin mtlOrigin =
            MTLOriginMake(copyInfo.textureOffset.x, copyInfo.textureOffset.y, copyInfo.textureOffset.z);
        MTLSize mtlSize =
            MTLSizeMake(copyInfo.textureExtent.width, copyInfo.textureExtent.height, copyInfo.textureExtent.depth);

        switch (texInfo.textureType) {
            case snap::rhi::TextureType::Texture2D:
            case snap::rhi::TextureType::Texture3D: {
                assert(copyInfo.textureOffset.z + copyInfo.textureExtent.depth <= texInfo.size.depth);

                [context.getBlitEncoder() copyFromTexture:srcMtlTexture->getTexture()
                                              sourceSlice:0 // If the texture type is neither an array nor a cube, use 0
                                              sourceLevel:copyInfo.textureSubresource.mipLevel
                                             sourceOrigin:mtlOrigin
                                               sourceSize:mtlSize
                                                 toBuffer:dstMtlBuffer->getBuffer()
                                        destinationOffset:copyInfo.bufferOffset
                                   destinationBytesPerRow:bytesPerRow
                                 destinationBytesPerImage:bytesPerSlice];
            } break;

            case snap::rhi::TextureType::Texture2DArray:
            case snap::rhi::TextureType::TextureCubemap: {
                mtlSize.depth = 1;
                mtlOrigin.z = 0;

                [[maybe_unused]] const uint32_t depth =
                    texInfo.size.depth * (texInfo.textureType == snap::rhi::TextureType::TextureCubemap ? 6 : 1);
                assert(copyInfo.textureOffset.z + copyInfo.textureExtent.depth <= depth);

                uint64_t offset = copyInfo.bufferOffset;
                for (uint32_t slice = 0; slice < copyInfo.textureExtent.depth; ++slice) {
                    const uint32_t sourceSlice = copyInfo.textureOffset.z + slice;
                    [context.getBlitEncoder() copyFromTexture:srcMtlTexture->getTexture()
                                                  sourceSlice:sourceSlice
                                                  sourceLevel:copyInfo.textureSubresource.mipLevel
                                                 sourceOrigin:mtlOrigin
                                                   sourceSize:mtlSize
                                                     toBuffer:dstMtlBuffer->getBuffer()
                                            destinationOffset:offset
                                       destinationBytesPerRow:bytesPerRow
                                     destinationBytesPerImage:bytesPerSlice];
                    offset += bytesPerSlice;
                }
            } break;

            default:
                snap::rhi::common::throwException("unsupported texture type");
        }
    }

    {
        resourceResidencySet.track(srcTexture);
        resourceResidencySet.track(dstBuffer);
    }
}

void BlitCommandEncoder::copyTexture(snap::rhi::Texture* srcTexture,
                                     snap::rhi::Texture* dstTexture,
                                     std::span<const TextureCopy> info) {
    Texture* srcMtlTexture = snap::rhi::backend::common::smart_cast<Texture>(srcTexture);
    Texture* dstMtlTexture = snap::rhi::backend::common::smart_cast<Texture>(dstTexture);

    commandBuffer->tryPreserveInteropTexture(srcTexture);
    commandBuffer->tryPreserveInteropTexture(dstTexture);

    for (const auto& copyInfo : info) {
        MTLOrigin srcOrigin = MTLOriginMake(copyInfo.srcOffset.x, copyInfo.srcOffset.y, copyInfo.srcOffset.z);
        MTLOrigin dstOrigin = MTLOriginMake(copyInfo.dstOffset.x, copyInfo.dstOffset.y, copyInfo.dstOffset.z);
        MTLSize mtlSize = MTLSizeMake(copyInfo.extent.width, copyInfo.extent.height, copyInfo.extent.depth);

        //        for (uint32_t slice = 0; slice < size.depth; ++slice) {
        //            uint32_t sourceSlice = srcInfo.origin.z + slice;
        //            uint32_t destinationSlice = dstInfo.origin.z + slice;
        /**
         * Tex2D -> Tex2D => done
         * Tex2D -> Tex2DArr with slice => need to implement
         * Tex2D -> TexCube with slice => need to implement
         * Tex2D -> Tex3D with slice => need to implement
         *
         * Tex2DArr -> Tex2D with slice => need to implement
         * Tex2DArr -> Tex2DArr with slice  => need to implement
         * Tex2DArr -> TexCube with slice  => need to implement
         * Tex2DArr -> Tex3D with slice  => need to implement
         *
         * TexCube -> Tex2D with slice => need to implement
         * TexCube -> Tex2DArr with slice => need to implement
         * TexCube -> TexCube with slice => need to implement
         * TexCube -> Tex3D with slice => need to implement
         *
         * Tex3D -> Tex2D with slice => need to implement
         * Tex3D -> Tex2DArr with slice => need to implement
         * Tex3D -> TexCube with slice => need to implement
         * Tex3D -> Tex3D => done
         **/
        [context.getBlitEncoder() copyFromTexture:srcMtlTexture->getTexture()
                                      sourceSlice:0
                                      sourceLevel:copyInfo.srcSubresource.mipLevel
                                     sourceOrigin:srcOrigin
                                       sourceSize:mtlSize
                                        toTexture:dstMtlTexture->getTexture()
                                 destinationSlice:0
                                 destinationLevel:copyInfo.dstSubresource.mipLevel
                                destinationOrigin:dstOrigin];
        //        }
    }

    {
        resourceResidencySet.track(srcTexture);
        resourceResidencySet.track(dstTexture);
    }
}

void BlitCommandEncoder::generateMipmaps(snap::rhi::Texture* texture) {
    Texture* mtlTexture = snap::rhi::backend::common::smart_cast<Texture>(texture);

    commandBuffer->tryPreserveInteropTexture(texture);

    [context.getBlitEncoder() generateMipmapsForTexture:mtlTexture->getTexture()];

    {
        resourceResidencySet.track(texture);
    }
}

void BlitCommandEncoder::pipelineBarrier(snap::rhi::PipelineStageBits srcStageMask,
                                         snap::rhi::PipelineStageBits dstStageMask,
                                         snap::rhi::DependencyFlags dependencyFlags,
                                         std::span<snap::rhi::MemoryBarrierInfo> memoryBarriers,
                                         std::span<snap::rhi::BufferMemoryBarrierInfo> bufferMemoryBarriers,
                                         std::span<snap::rhi::TextureMemoryBarrierInfo> textureMemoryBarriers) {
    /*
     * In Metal, resource hazards are generally managed automatically by the system for Blit command encoders.
     * */
}

void BlitCommandEncoder::endEncoding() {
    context.endBlit();
    BlitCommandEncoderBase::onEndEncoding();
}
} // namespace snap::rhi::backend::metal
