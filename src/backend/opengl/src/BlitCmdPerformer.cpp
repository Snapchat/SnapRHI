#include "snap/rhi/backend/opengl/BlitCmdPerformer.hpp"

#include "snap/rhi/Compare.hpp"
#include "snap/rhi/Exception.h"

#include "PlatformSpecific/WebAssembly/ValBuffer.hpp"
#include "snap/rhi/backend/opengl/Buffer.hpp"
#include "snap/rhi/backend/opengl/Device.hpp"
#include "snap/rhi/backend/opengl/DeviceContext.hpp"
#include "snap/rhi/backend/opengl/OpenGL.h"
#include "snap/rhi/backend/opengl/Profile.hpp"
#include "snap/rhi/backend/opengl/Sampler.hpp"
#include "snap/rhi/backend/opengl/Texture.hpp"
#include "snap/rhi/backend/opengl/Utils.hpp"

#include "snap/rhi/common/Throw.h"
#include <snap/rhi/common/Scope.h>

namespace {
void copyTexturesSubData(snap::rhi::backend::opengl::Device* device,
                         const snap::rhi::backend::opengl::CopyTextureToTextureCmd& cmd,
                         snap::rhi::backend::opengl::DeviceContext* dc,
                         const snap::rhi::backend::common::ValidationLayer& validationLayer) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[glCopyImageSubData][CopyTextureToTextureCmd] ");

    auto* srcTexture = snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Texture>(cmd.srcTexture);
    auto* dstTexture = snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Texture>(cmd.dstTexture);
    const auto& gl = device->getOpenGL();

    SNAP_RHI_VALIDATE(validationLayer,
                      (srcTexture->getCreateInfo().textureUsage & snap::rhi::TextureUsage::TransferSrc) !=
                          snap::rhi::TextureUsage::None,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::BlitCommandEncoderOp,
                      "[blitTextures] srcTexture should have TextureUsage::TransferSrc");
    SNAP_RHI_VALIDATE(validationLayer,
                      (dstTexture->getCreateInfo().textureUsage & snap::rhi::TextureUsage::TransferDst) !=
                          snap::rhi::TextureUsage::None,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::BlitCommandEncoderOp,
                      "[blitTextures] dstTexture should have TextureUsage::TransferDst");

    for (uint32_t i = 0; i < cmd.infoCount; ++i) {
        const auto& copyInfo = cmd.infos[i];
        gl.copyImageSubData(static_cast<GLuint>(srcTexture->getTextureID(dc)),
                            static_cast<GLenum>(srcTexture->getTarget()),
                            copyInfo.srcSubresource.mipLevel,
                            copyInfo.srcOffset.x,
                            copyInfo.srcOffset.y,
                            copyInfo.srcOffset.z,

                            static_cast<GLuint>(dstTexture->getTextureID(dc)),
                            static_cast<GLenum>(dstTexture->getTarget()),
                            copyInfo.dstSubresource.mipLevel,
                            copyInfo.dstOffset.x,
                            copyInfo.dstOffset.y,
                            copyInfo.dstOffset.z,

                            copyInfo.extent.width,
                            copyInfo.extent.height,
                            copyInfo.extent.depth);
    }
}

bool canBlitTexturesWithFBO(const snap::rhi::backend::opengl::CopyTextureToTextureCmd& cmd) {
    for (uint32_t i = 0; i < cmd.infoCount; ++i) {
        const auto& copyInfo = cmd.infos[i];
        if (!(copyInfo.srcOffset.z == 0 && copyInfo.extent.depth == 1) ||
            !(copyInfo.dstOffset.z == 0 && copyInfo.extent.depth == 1)) {
            return false;
        }
    }
    return true;
}

void blitTexturesWithFBO(snap::rhi::backend::opengl::Device* device,
                         const snap::rhi::backend::opengl::CopyTextureToTextureCmd& cmd,
                         snap::rhi::backend::opengl::DeviceContext* dc,
                         const snap::rhi::backend::common::ValidationLayer& validationLayer) {
    /**
     * This function support only color blit
     */

    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[blitTexturesWithFBO][blitTextures]");
    assert(dc);

    auto* srcTexture = snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Texture>(cmd.srcTexture);
    auto* dstTexture = snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Texture>(cmd.dstTexture);

    for (uint32_t i = 0; i < cmd.infoCount; ++i) {
        const auto& copyInfo = cmd.infos[i];
        if (!(copyInfo.srcOffset.z == 0 && copyInfo.extent.depth == 1) ||
            !(copyInfo.dstOffset.z == 0 && copyInfo.extent.depth == 1)) {
            snap::rhi::common::throwException<snap::rhi::UnsupportedOperationException>(
                "[copyTexture] texture arrays and 3D textures are not yet supported");
        }

        snap::rhi::TextureSubresourceRange srcSubres = {};
        srcSubres.baseMipLevel = copyInfo.srcSubresource.mipLevel;
        srcSubres.baseArrayLayer = 0;
        srcSubres.levelCount = 1;
        srcSubres.layerCount = 1;

        snap::rhi::TextureSubresourceRange dstSubres = {};
        dstSubres.baseMipLevel = copyInfo.dstSubresource.mipLevel;
        dstSubres.baseArrayLayer = 0;
        dstSubres.levelCount = 1;
        dstSubres.layerCount = 1;

        GLbitfield mask = GL_NONE;
        const snap::rhi::DepthStencilFormatTraits depthStencilFormatTraits =
            snap::rhi::getDepthStencilFormatTraits(dstTexture->getCreateInfo().format);
        if (depthStencilFormatTraits == snap::rhi::DepthStencilFormatTraits::None) {
            mask |= GL_COLOR_BUFFER_BIT;
        }

        if ((depthStencilFormatTraits & snap::rhi::DepthStencilFormatTraits::HasDepthAspect) ==
            snap::rhi::DepthStencilFormatTraits::HasDepthAspect) {
            mask |= GL_DEPTH_BUFFER_BIT;
        }

        if ((depthStencilFormatTraits & snap::rhi::DepthStencilFormatTraits::HasStencilAspect) ==
            snap::rhi::DepthStencilFormatTraits::HasStencilAspect) {
            mask |= GL_STENCIL_BUFFER_BIT;
        }

        snap::rhi::backend::opengl::blitTextures(dc,
                                                 nullptr,
                                                 srcTexture,
                                                 copyInfo.srcOffset,
                                                 srcSubres,
                                                 dstTexture,
                                                 copyInfo.dstOffset,
                                                 dstSubres,
                                                 copyInfo.extent,
                                                 copyInfo.extent,
                                                 mask);
    }
}

void copyTexturesSubImage(snap::rhi::backend::opengl::Device* device,
                          const snap::rhi::backend::opengl::CopyTextureToTextureCmd& cmd,
                          snap::rhi::backend::opengl::DeviceContext* dc,
                          const snap::rhi::backend::common::ValidationLayer& validationLayer) {
    /**
     * This function support only color blit
     */

    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[copyTextures][copyTexSubImage2D]");
    assert(dc);

    auto* srcTexture = snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Texture>(cmd.srcTexture);
    auto* dstTexture = snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Texture>(cmd.dstTexture);
    const auto& gl = device->getOpenGL();

    SNAP_RHI_VALIDATE(
        validationLayer,
        (srcTexture->getCreateInfo().textureUsage & snap::rhi::TextureUsage::TransferSrc) !=
            snap::rhi::TextureUsage::None,
        snap::rhi::ReportLevel::Error,
        snap::rhi::ValidationTag::BlitCommandEncoderOp,
        "[snap::rhi::backend::opengl::Legacy][copyTexture] srcTexture should have TextureUsage::TransferSrc");
    SNAP_RHI_VALIDATE(
        validationLayer,
        (dstTexture->getCreateInfo().textureUsage & snap::rhi::TextureUsage::TransferDst) !=
            snap::rhi::TextureUsage::None,
        snap::rhi::ReportLevel::Error,
        snap::rhi::ValidationTag::BlitCommandEncoderOp,
        "[snap::rhi::backend::opengl::Legacy][copyTexture] dstTexture should have TextureUsage::TransferDst");
    for (uint32_t i = 0; i < cmd.infoCount; ++i) {
        const auto& copyInfo = cmd.infos[i];
        snap::rhi::TextureSubresourceRange srcSubres = {};
        srcSubres.baseMipLevel = copyInfo.srcSubresource.mipLevel;
        srcSubres.baseArrayLayer = 0;
        srcSubres.levelCount = 1;

        uint32_t layerCount = 1;

        if (srcTexture->getTarget() == snap::rhi::backend::opengl::TextureTarget::TextureCubeMap ||
            srcTexture->getTarget() == snap::rhi::backend::opengl::TextureTarget::Texture2DArray ||
            srcTexture->getTarget() == snap::rhi::backend::opengl::TextureTarget::Texture3D) {
            layerCount = copyInfo.extent.depth;
        }

        for (uint32_t layer = 0; layer < layerCount; ++layer) {
            srcSubres.baseArrayLayer = copyInfo.srcOffset.z + layer;
            srcSubres.layerCount = 1;

            // For OpenGL ES 2.0 we have to use FramebufferTarget::Framebuffer instead of
            // FramebufferTarget::ReadFramebuffer
            snap::rhi::backend::opengl::FramebufferTarget target =
                snap::rhi::backend::opengl::FramebufferTarget::Framebuffer;
            snap::rhi::backend::opengl::FramebufferDescription fboDesc =
                snap::rhi::backend::opengl::buildFBODesc(dc, srcSubres, srcTexture, nullptr);
            [[maybe_unused]] snap::rhi::backend::opengl::FramebufferId fboId = dc->bindFramebuffer(target, fboDesc);
            gl.readBuffer(snap::rhi::backend::opengl::FramebufferAttachmentTarget::ColorAttachment0);
            gl.drawBuffers(0, nullptr);
            gl.validateFramebuffer(target, fboId, fboDesc);

            SNAP_RHI_ON_SCOPE_EXIT {
                if (device->shouldDetachFramebufferOnScopeExit()) {
                    // We shouldn't make this call because OpenGL can waste a lot of time on it.
                    gl.bindFramebuffer(target, snap::rhi::backend::opengl::FramebufferId::CurrSurfaceBackbuffer, dc);
                }
            };

            gl.bindTexture(dstTexture->getTarget(), dstTexture->getTextureID(dc), dc); // bind copy dest texture
            SNAP_RHI_ON_SCOPE_EXIT {
                gl.bindTexture(dstTexture->getTarget(), snap::rhi::backend::opengl::TextureId::Null, dc);
            };
            if (layer > 1 || copyInfo.dstOffset.z > 0) {
                gl.copyTexSubImage3D(dstTexture->getTarget(),
                                     copyInfo.dstSubresource.mipLevel, // perform copy operation
                                     copyInfo.dstOffset.x,
                                     copyInfo.dstOffset.y,
                                     copyInfo.dstOffset.z + layer,
                                     copyInfo.srcOffset.x,
                                     copyInfo.srcOffset.y,
                                     copyInfo.extent.width,
                                     copyInfo.extent.height);
            } else {
                gl.copyTexSubImage2D(dstTexture->getTarget(),
                                     copyInfo.dstSubresource.mipLevel, // perform copy operation
                                     copyInfo.dstOffset.x,
                                     copyInfo.dstOffset.y,
                                     copyInfo.srcOffset.x,
                                     copyInfo.srcOffset.y,
                                     copyInfo.extent.width,
                                     copyInfo.extent.height);
            }
        }
    }
}

void copyWithPBO(snap::rhi::backend::opengl::Device* device,
                 const snap::rhi::backend::opengl::CopyTextureToBufferCmd& cmd,
                 snap::rhi::backend::opengl::DeviceContext* dc,
                 const snap::rhi::backend::common::ValidationLayer& validationLayer) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[copyWithPBO][CopyTextureToBufferCmd]");
    assert(dc);

    auto* dstBuffer = snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Buffer>(cmd.dstBuffer);
    auto* srcTexture = snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Texture>(cmd.srcTexture);
    const auto& gl = device->getOpenGL();

    SNAP_RHI_VALIDATE(validationLayer,
                      (srcTexture->getCreateInfo().textureUsage & snap::rhi::TextureUsage::TransferSrc) !=
                          snap::rhi::TextureUsage::None,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::TextureOp,
                      "[CopyTextureToBuffer] srcTexture should have TextureUsage::TransferSrc");
    SNAP_RHI_VALIDATE(validationLayer,
                      (dstBuffer->getCreateInfo().bufferUsage & snap::rhi::BufferUsage::TransferDst) !=
                          snap::rhi::BufferUsage::None,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::BufferOp,
                      "[CopyTextureToBuffer] dstBuffer should have BufferUsage::TransferDst");

    const auto& textureInfo = srcTexture->getCreateInfo();
    const auto& textureFormatInfo = gl.getTextureFormat(textureInfo.format);

    gl.bindBuffer(GL_PIXEL_PACK_BUFFER, dstBuffer->getGLBuffer(dc), dc);
    SNAP_RHI_ON_SCOPE_EXIT {
        gl.bindBuffer(GL_PIXEL_PACK_BUFFER, 0, dc);
    };

    assert(gl.getFeatures().isCustomTexturePackingSupported);
    for (uint32_t i = 0; i < cmd.infoCount; ++i) {
        const auto& copyInfo = cmd.infos[i];
        snap::rhi::TextureSubresourceRange srcSubres = {};
        srcSubres.baseMipLevel = copyInfo.textureSubresource.mipLevel;
        srcSubres.baseArrayLayer = copyInfo.textureOffset.z;
        srcSubres.levelCount = 1;

        uint32_t layerCount = 1;

        if (srcTexture->getTarget() == snap::rhi::backend::opengl::TextureTarget::TextureCubeMap ||
            srcTexture->getTarget() == snap::rhi::backend::opengl::TextureTarget::Texture2DArray ||
            srcTexture->getTarget() == snap::rhi::backend::opengl::TextureTarget::Texture3D) {
            layerCount = copyInfo.textureExtent.depth;
        }

        for (uint32_t layer = 0; layer < layerCount; ++layer) {
            srcSubres.baseArrayLayer = copyInfo.textureOffset.z + layer;
            srcSubres.layerCount = 1;

            snap::rhi::backend::opengl::FramebufferDescription fboDesc =
                snap::rhi::backend::opengl::buildFBODesc(dc, srcSubres, srcTexture, nullptr);
            [[maybe_unused]] snap::rhi::backend::opengl::FramebufferId fboId =
                dc->bindFramebuffer(snap::rhi::backend::opengl::FramebufferTarget::ReadFramebuffer, fboDesc);
            gl.readBuffer(snap::rhi::backend::opengl::FramebufferAttachmentTarget::ColorAttachment0);
            gl.validateFramebuffer(snap::rhi::backend::opengl::FramebufferTarget::ReadFramebuffer, fboId, fboDesc);

            SNAP_RHI_ON_SCOPE_EXIT {
                if (device->shouldDetachFramebufferOnScopeExit()) {
                    // We shouldn't make this call because OpenGL can waste a lot of time on it.
                    gl.bindFramebuffer(snap::rhi::backend::opengl::FramebufferTarget::ReadFramebuffer,
                                       snap::rhi::backend::opengl::FramebufferId::CurrSurfaceBackbuffer,
                                       dc);
                }
            };

            uint32_t bytesPerRow = 0;
            uint64_t bytesPerSlice = 0;

            uint32_t unpackRowLength = 0;
            uint32_t unpackImageHeight = 0;

            snap::rhi::backend::opengl::computeUnpackImageSize(copyInfo.textureExtent.width,
                                                               copyInfo.textureExtent.height,
                                                               textureInfo.format,
                                                               copyInfo.bytesPerRow,
                                                               copyInfo.bytesPerSlice,
                                                               bytesPerRow,
                                                               bytesPerSlice,
                                                               unpackRowLength,
                                                               unpackImageHeight);

            /**
             * Since glReadPixels doesn't read compressed texture, we can safety change GL_PACK_ROW_LENGTH for
             * glPixelStorei
             */
            gl.pixelStorei(GL_PACK_ROW_LENGTH, unpackRowLength == copyInfo.textureExtent.width ? 0 : unpackRowLength);
            auto layerOffset = copyInfo.bytesPerSlice * layer;
            uint8_t* data = reinterpret_cast<uint8_t*>(copyInfo.bufferOffset + layerOffset);
            assert(copyInfo.bytesPerSlice * layer < dstBuffer->getCreateInfo().size);
            gl.readPixels(copyInfo.textureOffset.x,
                          copyInfo.textureOffset.y,
                          copyInfo.textureExtent.width,
                          copyInfo.textureExtent.height,
                          textureFormatInfo.format,
                          textureFormatInfo.dataType,
                          data); // load pixels
        }
    }
}

void copy(snap::rhi::backend::opengl::Device* device,
          const snap::rhi::backend::opengl::CopyTextureToBufferCmd& cmd,
          snap::rhi::backend::opengl::DeviceContext* dc,
          const snap::rhi::backend::common::ValidationLayer& validationLayer) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[copy][CopyTextureToBufferCmd]");
    assert(dc);

    auto* dstBuffer = snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Buffer>(cmd.dstBuffer);
    auto* srcTexture = snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Texture>(cmd.srcTexture);
    const auto& gl = device->getOpenGL();

    SNAP_RHI_VALIDATE(validationLayer,
                      (srcTexture->getCreateInfo().textureUsage & snap::rhi::TextureUsage::TransferSrc) !=
                          snap::rhi::TextureUsage::None,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::BlitCommandEncoderOp,
                      "[CopyTextureToBuffer] srcTexture should have TextureUsage::TransferSrc");
    SNAP_RHI_VALIDATE(validationLayer,
                      (dstBuffer->getCreateInfo().bufferUsage & snap::rhi::BufferUsage::TransferDst) !=
                          snap::rhi::BufferUsage::None,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::BlitCommandEncoderOp,
                      "[CopyTextureToBuffer] dstBuffer should have BufferUsage::TransferDst");

    const auto& textureInfo = srcTexture->getCreateInfo();
    const auto& textureFormatInfo = gl.getTextureFormat(textureInfo.format);

    const uint64_t unpackMemorySize = snap::rhi::backend::opengl::computeStagingDataSize(
        std::span<const snap::rhi::BufferTextureCopy>(cmd.infos.data(), cmd.infoCount),
        textureInfo,
        gl.getFeatures().isCustomTexturePackingSupported);
    std::vector<uint8_t> unpackedReadingData(unpackMemorySize);

    if (gl.getFeatures().isPBOSupported) {
        gl.bindBuffer(GL_PIXEL_PACK_BUFFER, 0, dc);
    }

    for (uint32_t i = 0; i < cmd.infoCount; ++i) {
        const auto& copyInfo = cmd.infos[i];
        snap::rhi::TextureSubresourceRange srcSubres = {};
        srcSubres.baseMipLevel = copyInfo.textureSubresource.mipLevel;
        srcSubres.baseArrayLayer = copyInfo.textureOffset.z;
        srcSubres.levelCount = 1;

        uint32_t layerCount = 1;

        if (srcTexture->getTarget() == snap::rhi::backend::opengl::TextureTarget::TextureCubeMap ||
            srcTexture->getTarget() == snap::rhi::backend::opengl::TextureTarget::Texture2DArray ||
            srcTexture->getTarget() == snap::rhi::backend::opengl::TextureTarget::Texture3D) {
            layerCount = copyInfo.textureExtent.depth;
        }

        // Copy layer by layer
        for (uint32_t layer = 0; layer < layerCount; ++layer) {
            srcSubres.baseArrayLayer = copyInfo.textureOffset.z + layer;
            srcSubres.layerCount = 1;

            // For OpenGL ES 2.0 we have to use FramebufferTarget::Framebuffer instead of
            // FramebufferTarget::ReadFramebuffer
            snap::rhi::backend::opengl::FramebufferTarget target =
                snap::rhi::backend::opengl::FramebufferTarget::Framebuffer;
            snap::rhi::backend::opengl::FramebufferDescription fboDesc =
                snap::rhi::backend::opengl::buildFBODesc(dc, srcSubres, srcTexture, nullptr);
            [[maybe_unused]] snap::rhi::backend::opengl::FramebufferId fboId = dc->bindFramebuffer(target, fboDesc);
            gl.readBuffer(snap::rhi::backend::opengl::FramebufferAttachmentTarget::ColorAttachment0);
            gl.drawBuffers(0, nullptr);
            gl.validateFramebuffer(target, fboId, fboDesc);

            SNAP_RHI_ON_SCOPE_EXIT {
                if (device->shouldDetachFramebufferOnScopeExit()) {
                    // We shouldn't make this call because OpenGL can waste a lot of time on it.
                    gl.bindFramebuffer(target, snap::rhi::backend::opengl::FramebufferId::CurrSurfaceBackbuffer, dc);
                }
            };

            assert(dstBuffer->getGLBuffer(dc) == GL_NONE);
            /**
             * WholeSize is fine here since buffer is logical buffer
             */
            auto layerOffset = copyInfo.bytesPerSlice * layer;
            auto* ptr = dstBuffer->map(snap::rhi::MemoryAccess::Write, 0, snap::rhi::WholeSize, dc) +
                        copyInfo.bufferOffset + layerOffset;
            SNAP_RHI_ON_SCOPE_EXIT {
                dstBuffer->unmap(dc);
            };

            uint32_t bytesPerRow = 0;
            uint64_t bytesPerSlice = 0;

            uint32_t unpackRowLength = 0;
            uint32_t unpackImageHeight = 0;

            snap::rhi::backend::opengl::computeUnpackImageSize(copyInfo.textureExtent.width,
                                                               copyInfo.textureExtent.height,
                                                               textureInfo.format,
                                                               copyInfo.bytesPerRow,
                                                               copyInfo.bytesPerSlice,
                                                               bytesPerRow,
                                                               bytesPerSlice,
                                                               unpackRowLength,
                                                               unpackImageHeight);

            /**
             * Since glReadPixels doesn't read compressed texture, we can safety change GL_PACK_ROW_LENGTH for
             * glPixelStorei
             */
            if (gl.getFeatures().isCustomTexturePackingSupported ||
                ((unpackRowLength == copyInfo.textureExtent.width) &&
                 (unpackImageHeight == copyInfo.textureExtent.height))) {
                gl.pixelStorei(GL_PACK_ROW_LENGTH,
                               unpackRowLength == copyInfo.textureExtent.width ? 0 : unpackRowLength);
                gl.readPixels(copyInfo.textureOffset.x,
                              copyInfo.textureOffset.y,
                              copyInfo.textureExtent.width,
                              copyInfo.textureExtent.height,
                              textureFormatInfo.format,
                              textureFormatInfo.dataType,
                              ptr); // load pixels
            } else {
                gl.readPixels(copyInfo.textureOffset.x,
                              copyInfo.textureOffset.y,
                              copyInfo.textureExtent.width,
                              copyInfo.textureExtent.height,
                              textureFormatInfo.format,
                              textureFormatInfo.dataType,
                              unpackedReadingData.data()); // load pixels

                const uint32_t dstBytesPerRow =
                    snap::rhi::bytesPerRow(unpackRowLength, unpackImageHeight, textureInfo.format);
                const uint64_t dstBytesPerSlice =
                    snap::rhi::bytesPerSlice(unpackRowLength, unpackImageHeight, textureInfo.format);

                assert(dstBytesPerSlice % dstBytesPerRow == 0);
                const uint32_t dstHeight = static_cast<uint32_t>(dstBytesPerSlice / dstBytesPerRow);

                const uint32_t srcBytesPerRow = snap::rhi::bytesPerRow(
                    copyInfo.textureExtent.width, copyInfo.textureExtent.height, textureInfo.format);

                auto* dst = ptr;
                const auto* src = unpackedReadingData.data();

                for (uint32_t y = 0; y < dstHeight; ++y) {
                    std::memcpy(dst, src, srcBytesPerRow);

                    src += srcBytesPerRow;
                    dst += dstBytesPerRow;
                }
            }
        }
    }
}

void copyWithBufferSubData(snap::rhi::backend::opengl::Device* device,
                           const snap::rhi::backend::opengl::CopyBufferToBufferCmd& cmd,
                           snap::rhi::backend::opengl::DeviceContext* dc,
                           const snap::rhi::backend::common::ValidationLayer& validationLayer) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[copyWithBufferSubData][CopyBufferToBufferCmd]");

    auto* srcBuffer = snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Buffer>(cmd.srcBuffer);
    auto* dstBuffer = snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Buffer>(cmd.dstBuffer);
    const auto& gl = device->getOpenGL();

    SNAP_RHI_VALIDATE(validationLayer,
                      (srcBuffer->getCreateInfo().bufferUsage & snap::rhi::BufferUsage::CopySrc) !=
                          snap::rhi::BufferUsage::None,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::BufferOp,
                      "[CopyBufferToBuffer] srcBuffer should have BufferUsage::CopySrc");
    SNAP_RHI_VALIDATE(validationLayer,
                      (dstBuffer->getCreateInfo().bufferUsage & snap::rhi::BufferUsage::CopyDst) !=
                          snap::rhi::BufferUsage::None,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::BufferOp,
                      "[CopyBufferToBuffer] dstBuffer should have BufferUsage::CopyDst");

    gl.bindBuffer(GL_COPY_READ_BUFFER, srcBuffer->getGLBuffer(dc), dc);
    SNAP_RHI_ON_SCOPE_EXIT {
        gl.bindBuffer(GL_COPY_READ_BUFFER, 0, dc);
    };

    gl.bindBuffer(GL_COPY_WRITE_BUFFER, dstBuffer->getGLBuffer(dc), dc);
    SNAP_RHI_ON_SCOPE_EXIT {
        gl.bindBuffer(GL_COPY_WRITE_BUFFER, 0, dc);
    };

    for (uint32_t i = 0; i < cmd.infoCount; ++i) {
        const auto& copyInfo = cmd.infos[i];
        gl.copyBufferSubData(
            GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, copyInfo.srcOffset, copyInfo.dstOffset, copyInfo.size);
    }
}

void copy(snap::rhi::backend::opengl::Device* device,
          const snap::rhi::backend::opengl::CopyBufferToBufferCmd& cmd,
          snap::rhi::backend::opengl::DeviceContext* dc,
          const snap::rhi::backend::common::ValidationLayer& validationLayer) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[copy][CopyBufferToBufferCmd]");

    auto* srcBuffer = snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Buffer>(cmd.srcBuffer);
    auto* dstBuffer = snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Buffer>(cmd.dstBuffer);

    SNAP_RHI_VALIDATE(
        validationLayer,
        (srcBuffer->getCreateInfo().bufferUsage & snap::rhi::BufferUsage::CopySrc) != snap::rhi::BufferUsage::None,
        snap::rhi::ReportLevel::Error,
        snap::rhi::ValidationTag::BlitCommandEncoderOp,
        "[snap::rhi::backend::opengl::Legacy][CopyBufferToBuffer] srcBuffer should have BufferUsage::CopySrc");
    SNAP_RHI_VALIDATE(
        validationLayer,
        (dstBuffer->getCreateInfo().bufferUsage & snap::rhi::BufferUsage::CopyDst) != snap::rhi::BufferUsage::None,
        snap::rhi::ReportLevel::Error,
        snap::rhi::ValidationTag::BlitCommandEncoderOp,
        "[snap::rhi::backend::opengl::Legacy][CopyBufferToBuffer] dstBuffer should have BufferUsage::CopyDst");

    const auto* ptr = srcBuffer->map(snap::rhi::MemoryAccess::Read, 0, snap::rhi::WholeSize, dc);
    SNAP_RHI_ON_SCOPE_EXIT {
        srcBuffer->unmap(dc);
    };
    for (uint32_t i = 0; i < cmd.infoCount; ++i) {
        const auto& copyInfo = cmd.infos[i];
        dstBuffer->uploadData(
            copyInfo.dstOffset, std::span<const std::byte>{ptr + copyInfo.srcOffset, copyInfo.size}, dc);
    }
}
} // unnamed namespace

namespace snap::rhi::backend::opengl {
BlitCmdPerformer::BlitCmdPerformer(Device* device) : CmdPerformer(device) {}

void BlitCmdPerformer::copyBuffer(const snap::rhi::backend::opengl::CopyBufferToBufferCmd& cmd) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[BlitCmdPerformer][copyBuffer][CopyBufferToBufferCmd]");

    const auto& glFeatures = gl.getFeatures();
    auto* srcBuffer = snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Buffer>(cmd.srcBuffer);
    auto* dstBuffer = snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Buffer>(cmd.dstBuffer);

    if (glFeatures.isCopyBufferSubDataSupported && srcBuffer->getGLBuffer(dc) != GL_NONE &&
        dstBuffer->getGLBuffer(dc) != GL_NONE) {
        copyWithBufferSubData(device, cmd, dc, validationLayer);
    } else {
        copy(device, cmd, dc, validationLayer);
    }
}

void BlitCmdPerformer::copyBufferToTexture(const snap::rhi::backend::opengl::CopyBufferToTextureCmd& cmd) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[BlitCmdPerformer][copyBufferToTexture][CopyBufferToTextureCmd]");

    auto* srcBuffer = snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Buffer>(cmd.srcBuffer);
    auto* dstTexture = snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Texture>(cmd.dstTexture);

    uploadBufferToTexture(srcBuffer,
                          dstTexture,
                          std::span<const BufferTextureCopy>(cmd.infos.data(), cmd.infoCount),
                          gl,
                          validationLayer,
                          dc);
}

void BlitCmdPerformer::copyTextureToBuffer(const snap::rhi::backend::opengl::CopyTextureToBufferCmd& cmd) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[BlitCmdPerformer][copyTextureToBuffer][CopyTextureToBufferCmd]");

    const auto& glFeatures = gl.getFeatures();

    auto* dstBuffer = snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Buffer>(cmd.dstBuffer);
    if (glFeatures.isPBOSupported && dstBuffer->getGLBuffer(dc) != GL_NONE) {
        copyWithPBO(device, cmd, dc, validationLayer);
    } else {
        copy(device, cmd, dc, validationLayer);
    }

    gl.pixelStorei(GL_PACK_ROW_LENGTH, 0);
}

void BlitCmdPerformer::copyTexture(const snap::rhi::backend::opengl::CopyTextureToTextureCmd& cmd) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[BlitCmdPerformer][copyTexture][CopyTextureToTextureCmd]");
    assert(dc);

    const auto& glFeatures = gl.getFeatures();

    auto* srcTexture = snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Texture>(cmd.srcTexture);
    auto* dstTexture = snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Texture>(cmd.dstTexture);

    const auto& srcInfo = srcTexture->getCreateInfo();
    const auto& dstInfo = dstTexture->getCreateInfo();

    const auto& caps = device->getCapabilities();

    const auto& srcTextureFeatures = caps.formatProperties[static_cast<uint32_t>(srcInfo.format)].textureFeatures;
    const auto& dstTextureFeatures = caps.formatProperties[static_cast<uint32_t>(dstInfo.format)].textureFeatures;

    SNAP_RHI_VALIDATE(
        validationLayer,
        ((srcTextureFeatures & snap::rhi::FormatFeatures::BlitSrc) != snap::rhi::FormatFeatures::None) &&
            ((dstTextureFeatures & snap::rhi::FormatFeatures::BlitDst) != snap::rhi::FormatFeatures::None),
        snap::rhi::ReportLevel::Error,
        snap::rhi::ValidationTag::CommandBufferOp,
        "[GL][copyTexture] Src texture format should have FormatFeatures::BlitSrc, Dst texture format should have  "
        "FormatFeatures::BlitDst");

    const auto fboPoolOptions = device->getFramebufferPoolOptions();
    const auto& dcInfo = dc->getCreateInfo();
    bool isFBOPoolForced = (fboPoolOptions != GLFramebufferPoolOption::None) && (dcInfo.internalResourcesAllowed);
    auto textureBlitOptions = device->getTextureBlitOptions();
    bool isFBOBlitForced = (textureBlitOptions == snap::rhi::backend::opengl::TextureBlitOptions::BlitFramebuffer);

    /**
     * Don't remove it, because for an FBO pool, blitting with FBO is more efficient than copyTexturesSubData.
     */
    if ((isFBOBlitForced || isFBOPoolForced) && glFeatures.isBlitFramebufferSupported && canBlitTexturesWithFBO(cmd)) {
        blitTexturesWithFBO(device, cmd, dc, validationLayer);
        return;
    }

    if (textureBlitOptions == snap::rhi::backend::opengl::TextureBlitOptions::CopyImageSubData &&
        glFeatures.isCopyImageSubDataSupported) {
        copyTexturesSubData(device, cmd, dc, validationLayer);
        return;
    }

    if (textureBlitOptions == snap::rhi::backend::opengl::TextureBlitOptions::CopyTexSubImage) {
        copyTexturesSubImage(device, cmd, dc, validationLayer);
        return;
    }

    // Default blit approach selection logic
    if (glFeatures.isCopyImageSubDataSupported) {
        copyTexturesSubData(device, cmd, dc, validationLayer);
    } else if (glFeatures.isBlitFramebufferSupported) {
        blitTexturesWithFBO(device, cmd, dc, validationLayer);
    } else {
        copyTexturesSubImage(device, cmd, dc, validationLayer);
    }
}

void BlitCmdPerformer::generateMipmap(const snap::rhi::backend::opengl::GenerateMipmapCmd& cmd) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[BlitCmdPerformer][generateMipmap][GenerateMipmapCmd]");

    auto* texture = snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Texture>(cmd.texture);

    const auto& info = texture->getCreateInfo();
    const snap::rhi::backend::opengl::TextureTarget target = texture->getTarget();

    if (info.mipLevels == 1) {
        return; // We should not generate mip levels for textures that only have 1 mip level.
    }

    // https://registry.khronos.org/OpenGL-Refpages/es3/html/glGenerateMipmap.xhtml
    // GL_INVALID_OPERATION is generated if the levelbase
    // array was not specified with an unsized internal format or a sized internal format that is both color-renderable
    // and texture-filterable
    // or unsized internal format
    //    const auto& textureFeatures = caps.formatProperties[static_cast<uint32_t>(info.format)].textureFeatures;
    //
    //    SNAP_RHI_VALIDATE(gl.getDevice()->getValidationLayer(),
    //        info.format == snap::rhi::PixelFormat::Grayscale ||
    //            (((textureFeatures & snap::rhi::FormatFeatures::ColorRenderable) != snap::rhi::FormatFeatures::None)
    //            &&
    //             ((textureFeatures & snap::rhi::FormatFeatures::Sampled) != snap::rhi::FormatFeatures::None)),
    //        snap::rhi::ReportLevel::Error,
    //        snap::rhi::ValidationTag::CommandBufferOp,
    //        "[GL][generateMipmap] texture format should be both color-renderable and texture-filterable");

    gl.bindTexture(target, texture->getTextureID(dc), dc);
    gl.hint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
    gl.generateMipmap(target);
    gl.bindTexture(target, snap::rhi::backend::opengl::TextureId::Null, dc);

    /**
     * Driver may reallocate texture memory after mipmap generation
     * In order to prevent issues with FBO pool, we have to regenerate texture UUID.
     **/
    //
    // According to @vserhienko research, Apple's GL-on-MTL engine implementation
    // may change texture's layout on glGenerateMipmap, which will cause
    // texture re-initialization. All framebuffers referencing the texture
    // will still be valid due to Objective-C reference counting mechanism.
    // But even though they seem to be technically valid, they will reference
    // some stale texture or some block of memory, which may not be the one we
    // attached earlier during framebuffer initialization.
    texture->updateTextureUUID();
}

void BlitCmdPerformer::reset() {
    // Blit Encoder doesn't have any internal states, so do nothing
}
} // namespace snap::rhi::backend::opengl
