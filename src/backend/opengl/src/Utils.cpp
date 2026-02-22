#include "snap/rhi/backend/opengl/Utils.hpp"

#include "PlatformSpecific/WebAssembly/ValBuffer.hpp"
#include "snap/rhi/backend/opengl/Buffer.hpp"
#include "snap/rhi/backend/opengl/Device.hpp"
#include "snap/rhi/backend/opengl/DeviceContext.hpp"
#include "snap/rhi/backend/opengl/Profile.hpp"
#include "snap/rhi/backend/opengl/RenderPipeline.hpp"
#include "snap/rhi/common/Throw.h"
#include <array>
#include <snap/rhi/common/Scope.h>
#include <string_view>

namespace {
void copyWithPBO(snap::rhi::backend::opengl::Buffer* srcBuffer,
                 snap::rhi::backend::opengl::Texture* dstTexture,
                 std::span<const snap::rhi::BufferTextureCopy> infos,
                 snap::rhi::backend::opengl::Profile& gl,
                 const snap::rhi::backend::common::ValidationLayer& validationLayer,
                 snap::rhi::backend::opengl::DeviceContext* dc) {
    snap::rhi::Device* device = gl.getDevice();
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[copyWithPBO][CopyBufferToTexture]");

    const snap::rhi::TextureCreateInfo& textureInfo = dstTexture->getCreateInfo();
    const bool isCompressedTexture = snap::rhi::isCompressedFormat(textureInfo.format);

    assert(gl.getFeatures().isCustomTexturePackingSupported);
    SNAP_RHI_VALIDATE(validationLayer,
                      (srcBuffer->getCreateInfo().bufferUsage & snap::rhi::BufferUsage::TransferSrc) !=
                          snap::rhi::BufferUsage::None,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::BufferOp,
                      "[CopyBufferToTexture] srcBuffer should have BufferUsage::TransferSrc");
    SNAP_RHI_VALIDATE(validationLayer,
                      (dstTexture->getCreateInfo().textureUsage & snap::rhi::TextureUsage::TransferDst) !=
                          snap::rhi::TextureUsage::None,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::TextureOp,
                      "[CopyBufferToTexture] dstTexture should have TextureUsage::TransferDst");

    gl.bindBuffer(GL_PIXEL_UNPACK_BUFFER, srcBuffer->getGLBuffer(dc), dc);
    SNAP_RHI_ON_SCOPE_EXIT {
        gl.bindBuffer(GL_PIXEL_UNPACK_BUFFER, 0, dc);
    };

    const uint64_t unpackMemorySize = snap::rhi::backend::opengl::computeStagingDataSize(
        infos, textureInfo, gl.getFeatures().isCustomTexturePackingSupported);
    GLuint stagingGLBuffer = GL_NONE;
    std::shared_ptr<snap::rhi::Buffer> stagingBuffer = nullptr;
    if (unpackMemorySize) {
        snap::rhi::BufferCreateInfo bufferCreateInfo{};
        bufferCreateInfo.bufferUsage = snap::rhi::BufferUsage::TransferSrc;
        bufferCreateInfo.memoryProperties =
            snap::rhi::MemoryProperties::HostVisible | snap::rhi::MemoryProperties::HostCoherent;
        bufferCreateInfo.size = unpackMemorySize;

        stagingBuffer = device->createBuffer(bufferCreateInfo);

        snap::rhi::backend::opengl::Buffer* stagingNativeBuffer =
            snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Buffer>(stagingBuffer.get());
        stagingGLBuffer = stagingNativeBuffer->getGLBuffer(dc);
    }

    gl.bindBuffer(GL_COPY_READ_BUFFER, srcBuffer->getGLBuffer(dc), dc);
    SNAP_RHI_ON_SCOPE_EXIT {
        gl.bindBuffer(GL_COPY_READ_BUFFER, 0, dc);
    };

    gl.bindBuffer(GL_COPY_WRITE_BUFFER, stagingGLBuffer, dc);
    SNAP_RHI_ON_SCOPE_EXIT {
        gl.bindBuffer(GL_COPY_WRITE_BUFFER, 0, dc);
    };

    for (const auto& copyInfo : infos) {
        const uint32_t dstBytesPerRow =
            snap::rhi::bytesPerRow(copyInfo.textureExtent.width, copyInfo.textureExtent.height, textureInfo.format);
        const uint64_t dstBytesPerSlice =
            snap::rhi::bytesPerSlice(copyInfo.textureExtent.width, copyInfo.textureExtent.height, textureInfo.format);

        const uint32_t bytesPerRow = copyInfo.bytesPerRow ? copyInfo.bytesPerRow : dstBytesPerRow;
        const uint64_t bytesPerSlice =
            copyInfo.bytesPerSlice ?
                copyInfo.bytesPerSlice :
                snap::rhi::bytesPerSliceWithRow(bytesPerRow, copyInfo.textureExtent.height, textureInfo.format);

        bool shouldUnpack = !((bytesPerRow == dstBytesPerRow) && (bytesPerSlice == dstBytesPerSlice));
        if (shouldUnpack && isCompressedTexture) {
            uint64_t srcOffset = copyInfo.bufferOffset;
            uint64_t dstOffset = 0;

            const uint32_t srcBytesPerRow = bytesPerRow;
            const uint64_t srcBytesPerSlice = bytesPerSlice;

            assert(srcBytesPerSlice % srcBytesPerRow == 0);
            const uint32_t srcHeight = static_cast<uint32_t>(srcBytesPerSlice / srcBytesPerRow);

            for (uint32_t z = 0; z < copyInfo.textureExtent.depth; ++z) {
                uint64_t srcSliceOffset = srcOffset;

                for (uint32_t y = 0; y < srcHeight; ++y) {
                    gl.copyBufferSubData(
                        GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, srcSliceOffset, dstOffset, dstBytesPerRow);

                    srcSliceOffset += srcBytesPerRow;
                    dstOffset += dstBytesPerRow;
                }
                srcOffset += srcBytesPerSlice;
            }
            gl.bindBuffer(GL_PIXEL_UNPACK_BUFFER, stagingGLBuffer, dc);
            dstTexture->upload(
                copyInfo.textureOffset, copyInfo.textureExtent, copyInfo.textureSubresource.mipLevel, 0, 0, 0, dc);
        } else {
            gl.bindBuffer(GL_PIXEL_UNPACK_BUFFER, srcBuffer->getGLBuffer(dc), dc);
            dstTexture->upload(copyInfo.textureOffset,
                               copyInfo.textureExtent,
                               copyInfo.textureSubresource.mipLevel,
                               reinterpret_cast<const uint8_t*>(copyInfo.bufferOffset),
                               bytesPerRow,
                               bytesPerSlice,
                               dc);
        }
    }
}

void copy(snap::rhi::backend::opengl::Buffer* srcBuffer,
          snap::rhi::backend::opengl::Texture* dstTexture,
          std::span<const snap::rhi::BufferTextureCopy> infos,
          snap::rhi::backend::opengl::Profile& gl,
          const snap::rhi::backend::common::ValidationLayer& validationLayer,
          snap::rhi::backend::opengl::DeviceContext* dc) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(gl.getDevice(), "[copy][CopyBufferToTexture]");

    const snap::rhi::TextureCreateInfo& textureInfo = dstTexture->getCreateInfo();
    const bool isCompressedTexture = snap::rhi::isCompressedFormat(textureInfo.format);

    SNAP_RHI_VALIDATE(validationLayer,
                      (srcBuffer->getCreateInfo().bufferUsage & snap::rhi::BufferUsage::TransferSrc) !=
                          snap::rhi::BufferUsage::None,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::BufferOp,
                      "[CopyBufferToTexture] srcBuffer should have BufferUsage::TransferSrc");
    SNAP_RHI_VALIDATE(validationLayer,
                      (dstTexture->getCreateInfo().textureUsage & snap::rhi::TextureUsage::TransferDst) !=
                          snap::rhi::TextureUsage::None,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::TextureOp,
                      "[CopyBufferToTexture] dstTexture should have TextureUsage::TransferDst");

    if (gl.getFeatures().isPBOSupported) {
        gl.bindBuffer(GL_PIXEL_UNPACK_BUFFER, 0, dc);
    }

    const uint64_t unpackMemorySize = snap::rhi::backend::opengl::computeStagingDataSize(
        infos, textureInfo, gl.getFeatures().isCustomTexturePackingSupported);

    std::vector<uint8_t> unpackedUploadingData;
    const auto* ptr = srcBuffer->map(snap::rhi::MemoryAccess::Read, 0, snap::rhi::WholeSize, dc);
    SNAP_RHI_ON_SCOPE_EXIT {
        srcBuffer->unmap(dc);
    };
    for (const auto& copyInfo : infos) {
        const uint32_t dstBytesPerRow =
            snap::rhi::bytesPerRow(copyInfo.textureExtent.width, copyInfo.textureExtent.height, textureInfo.format);
        const uint64_t dstBytesPerSlice =
            snap::rhi::bytesPerSlice(copyInfo.textureExtent.width, copyInfo.textureExtent.height, textureInfo.format);

        const uint32_t bytesPerRow = copyInfo.bytesPerRow ? copyInfo.bytesPerRow : dstBytesPerRow;
        const uint64_t bytesPerSlice =
            copyInfo.bytesPerSlice ?
                copyInfo.bytesPerSlice :
                snap::rhi::bytesPerSliceWithRow(bytesPerRow, copyInfo.textureExtent.height, textureInfo.format);
#if SNAP_RHI_PLATFORM_WEBASSEMBLY()
        auto* valBuffer = dynamic_cast<snap::rhi::backend::opengl::ValBuffer*>(srcBuffer);
        if (valBuffer) {
            dstTexture->upload(copyInfo.textureOffset,
                               copyInfo.textureExtent,
                               copyInfo.textureSubresource.mipLevel,
                               valBuffer->getJsBuffer(),
                               bytesPerRow,
                               bytesPerSlice,
                               dc);
            continue;
        }
#endif

        bool shouldUnpack = !((bytesPerRow == dstBytesPerRow) && (bytesPerSlice == dstBytesPerSlice));

        if ((!gl.getFeatures().isCustomTexturePackingSupported && shouldUnpack) ||
            (gl.getFeatures().isCustomTexturePackingSupported && shouldUnpack && isCompressedTexture)) {
            const auto* src = ptr + copyInfo.bufferOffset;
            unpackedUploadingData.resize(unpackMemorySize);
            uint8_t* dst = unpackedUploadingData.data();

            const uint32_t srcBytesPerRow = bytesPerRow;
            const uint64_t srcBytesPerSlice = bytesPerSlice;

            assert(srcBytesPerSlice % srcBytesPerRow == 0);
            const uint32_t srcHeight = static_cast<uint32_t>(srcBytesPerSlice / srcBytesPerRow);

            for (uint32_t z = 0; z < copyInfo.textureExtent.depth; ++z) {
                const auto* srcSlice = src;

                for (uint32_t y = 0; y < srcHeight; ++y) {
                    std::memcpy(dst, srcSlice, dstBytesPerRow);

                    srcSlice += srcBytesPerRow;
                    dst += dstBytesPerRow;
                }
                src += srcBytesPerSlice;
            }

            dstTexture->upload(copyInfo.textureOffset,
                               copyInfo.textureExtent,
                               copyInfo.textureSubresource.mipLevel,
                               unpackedUploadingData.data(),
                               0,
                               0,
                               dc);
        } else {
            dstTexture->upload(copyInfo.textureOffset,
                               copyInfo.textureExtent,
                               copyInfo.textureSubresource.mipLevel,
                               reinterpret_cast<const uint8_t*>(ptr + copyInfo.bufferOffset),
                               bytesPerRow,
                               bytesPerSlice,
                               dc);
        }
    }
}

constexpr GLbitfield toGLFlags(const snap::rhi::AccessFlags flags) {
    constexpr std::array<std::pair<snap::rhi::AccessFlags, GLbitfield>, 12> states{
        std::make_pair(snap::rhi::AccessFlags::IndexRead, GL_ELEMENT_ARRAY_BARRIER_BIT),
        std::make_pair(snap::rhi::AccessFlags::VertexAttributeRead, GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT),
        std::make_pair(snap::rhi::AccessFlags::UniformRead, GL_UNIFORM_BARRIER_BIT),
        std::make_pair(snap::rhi::AccessFlags::ShaderRead,
                       GL_UNIFORM_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT),
        std::make_pair(snap::rhi::AccessFlags::ShaderWrite,
                       GL_UNIFORM_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT),
        std::make_pair(snap::rhi::AccessFlags::ColorAttachmentRead, GL_FRAMEBUFFER_BARRIER_BIT),
        std::make_pair(snap::rhi::AccessFlags::ColorAttachmentWrite, GL_FRAMEBUFFER_BARRIER_BIT),

        std::make_pair(snap::rhi::AccessFlags::TransferRead,
                       GL_PIXEL_BUFFER_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT),
        std::make_pair(snap::rhi::AccessFlags::TransferWrite,
                       GL_PIXEL_BUFFER_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT),

        std::make_pair(snap::rhi::AccessFlags::MemoryRead,
                       GL_PIXEL_BUFFER_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT),
        std::make_pair(snap::rhi::AccessFlags::MemoryWrite,
                       GL_PIXEL_BUFFER_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT),

        std::make_pair(snap::rhi::AccessFlags::All, GL_ALL_BARRIER_BITS),
    };

    GLbitfield result = 0;
    for (size_t i = 0; i < states.size(); ++i) {
        if ((flags & states[i].first) == states[i].first) {
            result |= states[i].second;
        }
    }

    return result;
}
} // unnamed namespace

namespace snap::rhi::backend::opengl {
snap::rhi::SampleCount getFramebufferAttachmentSampleCount(const snap::rhi::TextureCreateInfo& createInfo,
                                                           snap::rhi::SampleCount numAttachmentSamples) noexcept {
    // clang-format off
    const bool isArrayTexture = createInfo.textureType == snap::rhi::TextureType::Texture2DArray;
    const snap::rhi::SampleCount numUnresolvedSamples = createInfo.sampleCount;
    const bool isUnresolved = numUnresolvedSamples > snap::rhi::SampleCount::Count1;
    const bool isAutoresolved = numAttachmentSamples > snap::rhi::SampleCount::Count1;
    const bool isAutoresolvedMultiviewAttachment = !isUnresolved && isArrayTexture && isAutoresolved;
    const snap::rhi::SampleCount numSamples = isAutoresolvedMultiviewAttachment ? numAttachmentSamples : numUnresolvedSamples;
    // clang-format on
    return numSamples;
}

snap::rhi::SampleCount getFramebufferAttachmentSampleCount(const snap::rhi::TextureCreateInfo& createInfo,
                                                           const snap::rhi::AttachmentDescription& attDesc) noexcept {
    return getFramebufferAttachmentSampleCount(createInfo, attDesc.samples);
}

snap::rhi::backend::opengl::FramebufferAttachment buildFramebufferAttachment(
    DeviceContext* dc,
    const snap::rhi::backend::opengl::Texture* texture,
    const snap::rhi::TextureSubresourceRange& subresRange,
    const snap::rhi::SampleCount numAttachmentSamples,
    const uint32_t viewMask) noexcept {
    assert(subresRange.levelCount == 1);
    const TextureCreateInfo& createInfo = texture->getCreateInfo();
    return snap::rhi::backend::opengl::FramebufferAttachment{.texUUID = texture->getTextureUUID(),
                                                             .target = texture->getTarget(),
                                                             .texId = texture->getTextureID(dc),
                                                             .level = subresRange.baseMipLevel,
                                                             .firstLayer = subresRange.baseArrayLayer,
                                                             .viewMask = viewMask,
                                                             .size = createInfo.size,
                                                             .format = createInfo.format,
                                                             .sampleCount = numAttachmentSamples};
}

snap::rhi::backend::opengl::FramebufferDescription buildFBODesc(DeviceContext* dc,
                                                                const snap::rhi::TextureSubresourceRange& subresRange,
                                                                const snap::rhi::Texture* fbAtt,
                                                                const snap::rhi::Texture* fbColorAtt) {
    snap::rhi::backend::opengl::FramebufferDescription result{};

    auto* texture = snap::rhi::backend::common::smart_cast<const snap::rhi::backend::opengl::Texture>(fbAtt);
    const auto& texInfo = texture->getCreateInfo();
    if (!texture) {
        snap::rhi::common::throwException<snap::rhi::InvalidArgumentException>("Unexpected texture type");
    }

    snap::rhi::backend::opengl::FramebufferAttachment attachment = buildFramebufferAttachment(dc, texture, subresRange);
    snap::rhi::backend::opengl::FramebufferAttachment colorAttachment;
    if (fbColorAtt) {
        auto* colorTexture =
            snap::rhi::backend::common::smart_cast<const snap::rhi::backend::opengl::Texture>(fbColorAtt);
        if (!colorTexture) {
            snap::rhi::common::throwException<snap::rhi::InvalidArgumentException>("Unexpected texture type");
        }
        colorAttachment = buildFramebufferAttachment(dc, colorTexture, subresRange);
    }

    auto dsTraits = snap::rhi::getDepthStencilFormatTraits(texInfo.format);
    if (dsTraits != snap::rhi::DepthStencilFormatTraits::None) {
        result.depthStencilAttachment = attachment;

        if (fbColorAtt) {
            result.colorAttachments[result.numColorAttachments++] = colorAttachment;
        }
    } else {
        result.colorAttachments[result.numColorAttachments++] = attachment;
    }

    return result;
}

bool isSystemResource(const std::string& name) {
    static constexpr std::string_view prefix = "gl_";

    return name.length() >= prefix.length() && prefix == name.substr(0, prefix.length());
}

void restoreAttachmentsMask(DeviceContext* dc,
                            snap::rhi::backend::opengl::RenderPipeline* renderPipeline,
                            GLbitfield& clearMask) {
    if (dc && renderPipeline && clearMask) {
        auto& gl = dc->getOpenGL();

        const auto& createInfo = renderPipeline->getCreateInfo();
        const auto& depthStencilState = createInfo.depthStencilState;
        const auto& colorBlendState = createInfo.colorBlendState;

        if (clearMask & GL_DEPTH_BUFFER_BIT) {
            gl.depthMask(depthStencilState.depthWrite ? GL_TRUE : GL_FALSE);
        }

        if (clearMask & GL_STENCIL_BUFFER_BIT) {
            gl.stencilMaskSeparate(GL_FRONT, static_cast<uint32_t>(depthStencilState.stencilFront.writeMask), dc);
            gl.stencilMaskSeparate(GL_BACK, static_cast<uint32_t>(depthStencilState.stencilBack.writeMask), dc);
        }

        if (clearMask & GL_COLOR_BUFFER_BIT) {
            const auto& features = gl.getFeatures();
            if (features.isDifferentBlendSettingsSupported) {
                for (uint32_t i = 0; i < colorBlendState.colorAttachmentsCount; ++i) {
                    const auto& state = colorBlendState.colorAttachmentsBlendState[i];

                    gl.colorMaski(i,
                                  (state.colorWriteMask & snap::rhi::ColorMask::R) != snap::rhi::ColorMask::None,
                                  (state.colorWriteMask & snap::rhi::ColorMask::G) != snap::rhi::ColorMask::None,
                                  (state.colorWriteMask & snap::rhi::ColorMask::B) != snap::rhi::ColorMask::None,
                                  (state.colorWriteMask & snap::rhi::ColorMask::A) != snap::rhi::ColorMask::None,
                                  dc);
                }
            } else if (colorBlendState.colorAttachmentsCount > 0) {
                gl.colorMask((colorBlendState.colorAttachmentsBlendState[0].colorWriteMask & snap::rhi::ColorMask::R) !=
                                 snap::rhi::ColorMask::None,
                             (colorBlendState.colorAttachmentsBlendState[0].colorWriteMask & snap::rhi::ColorMask::G) !=
                                 snap::rhi::ColorMask::None,
                             (colorBlendState.colorAttachmentsBlendState[0].colorWriteMask & snap::rhi::ColorMask::B) !=
                                 snap::rhi::ColorMask::None,
                             (colorBlendState.colorAttachmentsBlendState[0].colorWriteMask & snap::rhi::ColorMask::A) !=
                                 snap::rhi::ColorMask::None,
                             dc);
            }
        }
    }
    clearMask = GL_NONE;
}

void blitTextures(DeviceContext* dc,
                  snap::rhi::backend::opengl::RenderPipeline* renderPipeline,
                  const snap::rhi::Texture* srcTexture,
                  snap::rhi::Offset3D srcOrigin,
                  const snap::rhi::TextureSubresourceRange& srcDesc,
                  const snap::rhi::Texture* dstTexture,
                  snap::rhi::Offset3D dstOrigin,
                  const snap::rhi::TextureSubresourceRange& dstDesc,
                  snap::rhi::Extent3D srcExtent,
                  snap::rhi::Extent3D destExtent,
                  uint32_t mask,
                  // If blitting a depth texture (for msaa), we must provide a color texture for the fbo or the fbo will
                  // be considered incomplete for compatibility OpenGL
                  const snap::rhi::Texture* srcColorTexture,
                  const snap::rhi::Texture* destColorTexture,
                  uint32_t filter) {
    auto& gl = dc->getOpenGL();

    FramebufferDescription readFBODesc = buildFBODesc(dc, srcDesc, srcTexture, srcColorTexture);
    [[maybe_unused]] FramebufferId readFBOId =
        dc->bindFramebuffer(snap::rhi::backend::opengl::FramebufferTarget::ReadFramebuffer, readFBODesc);
    gl.readBuffer(snap::rhi::backend::opengl::FramebufferAttachmentTarget::ColorAttachment0);

    SNAP_RHI_ON_SCOPE_EXIT {
        if (common::smart_cast<Device>(gl.getDevice())->shouldDetachFramebufferOnScopeExit()) {
            // We shouldn't make this call because OpenGL can waste a lot of time on it.
            gl.bindFramebuffer(snap::rhi::backend::opengl::FramebufferTarget::ReadFramebuffer,
                               snap::rhi::backend::opengl::FramebufferId::CurrSurfaceBackbuffer,
                               dc);
        }
    };

    FramebufferDescription drawFBODesc = buildFBODesc(dc, dstDesc, dstTexture, destColorTexture);
    [[maybe_unused]] FramebufferId drawFBOId =
        dc->bindFramebuffer(snap::rhi::backend::opengl::FramebufferTarget::DrawFramebuffer, drawFBODesc);
    FramebufferAttachmentTarget drawBuffer = FramebufferAttachmentTarget::ColorAttachment0;
    gl.drawBuffers(1, &drawBuffer);

    gl.validateFramebuffer(snap::rhi::backend::opengl::FramebufferTarget::ReadFramebuffer, readFBOId, readFBODesc);
    gl.validateFramebuffer(snap::rhi::backend::opengl::FramebufferTarget::DrawFramebuffer, drawFBOId, drawFBODesc);

    SNAP_RHI_ON_SCOPE_EXIT {
        if (common::smart_cast<Device>(gl.getDevice())->shouldDetachFramebufferOnScopeExit()) {
            // We shouldn't make this call because OpenGL can waste a lot of time on it.
            gl.bindFramebuffer(snap::rhi::backend::opengl::FramebufferTarget::DrawFramebuffer,
                               snap::rhi::backend::opengl::FramebufferId::CurrSurfaceBackbuffer,
                               dc);
        }
    };

    gl.colorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, dc);
    SNAP_RHI_ON_SCOPE_EXIT {
        GLbitfield clearMask = GL_COLOR_BUFFER_BIT;
        restoreAttachmentsMask(dc, renderPipeline, clearMask);
    };

    gl.blitFramebuffer(srcOrigin.x,
                       srcOrigin.y,
                       srcOrigin.x + srcExtent.width,
                       srcOrigin.y + srcExtent.height,
                       dstOrigin.x,
                       dstOrigin.y,
                       dstOrigin.x + destExtent.width,
                       dstOrigin.y + destExtent.height,
                       mask,
                       filter);
}

void blitTextures(DeviceContext* dc,
                  snap::rhi::backend::opengl::RenderPipeline* renderPipeline,
                  const snap::rhi::Texture* srcTexture,
                  const snap::rhi::TextureSubresourceRange& srcDesc,
                  const snap::rhi::Texture* dstTexture,
                  const snap::rhi::TextureSubresourceRange& dstDesc,
                  uint32_t srcWidth,
                  uint32_t srcHeight,
                  uint32_t destWidth,
                  uint32_t destHeight,
                  uint32_t mask,
                  // If blitting a depth texture (for msaa), we must provide a color texture for the fbo or the fbo will
                  // be considered incomplete for compatibility OpenGL
                  const snap::rhi::Texture* srcColorTexture,
                  const snap::rhi::Texture* destColorTexture,
                  uint32_t filter) {
    return blitTextures(dc,
                        renderPipeline,
                        srcTexture,
                        {0, 0},
                        srcDesc,
                        dstTexture,
                        {0, 0},
                        dstDesc,
                        {srcWidth, srcHeight},
                        {destWidth, destHeight},
                        mask,
                        srcColorTexture,
                        destColorTexture,
                        filter);
}

void blitTextures(DeviceContext* dc,
                  snap::rhi::backend::opengl::RenderPipeline* renderPipeline,
                  const snap::rhi::Texture* srcTexture,
                  const snap::rhi::AttachmentDescription& srcDesc,
                  const snap::rhi::Texture* dstTexture,
                  const snap::rhi::AttachmentDescription& dstDesc,
                  uint32_t srcWidth,
                  uint32_t srcHeight,
                  uint32_t destWidth,
                  uint32_t destHeight,
                  uint32_t mask,
                  // If blitting a depth texture (for msaa), we must provide a color texture for the fbo or the fbo will
                  // be considered incomplete for compatibility OpenGL
                  const snap::rhi::Texture* srcColorTexture,
                  const snap::rhi::Texture* destColorTexture,
                  uint32_t filter) {
    snap::rhi::TextureSubresourceRange srcSubres = {};
    srcSubres.baseMipLevel = srcDesc.mipLevel;
    srcSubres.baseArrayLayer = srcDesc.layer;
    srcSubres.layerCount = 1;
    srcSubres.levelCount = 1;

    snap::rhi::TextureSubresourceRange dstSubres = {};
    dstSubres.baseMipLevel = dstDesc.mipLevel;
    dstSubres.baseArrayLayer = dstDesc.layer;
    dstSubres.layerCount = 1;
    dstSubres.levelCount = 1;

    return blitTextures(dc,
                        renderPipeline,
                        srcTexture,
                        srcSubres,
                        dstTexture,
                        dstSubres,
                        srcWidth,
                        srcHeight,
                        destWidth,
                        destHeight,
                        mask,
                        srcColorTexture,
                        destColorTexture,
                        filter);
}

void blitTextures(DeviceContext* dc,
                  snap::rhi::backend::opengl::RenderPipeline* renderPipeline,
                  const snap::rhi::TextureView& srcTexture,
                  const snap::rhi::TextureView& dstTexture,
                  uint32_t mask,
                  const snap::rhi::Texture* srcColorTexture,
                  const snap::rhi::Texture* destColorTexture,
                  uint32_t filter) {
    const auto& srcTextureInfo = srcTexture.texture->getCreateInfo();
    const auto& dstTextureInfo = dstTexture.texture->getCreateInfo();

    snap::rhi::TextureSubresourceRange srcSubres = {};
    srcSubres.baseMipLevel = srcTexture.mipLevel;
    srcSubres.baseArrayLayer = srcTexture.layer;
    srcSubres.layerCount = 1;
    srcSubres.levelCount = 1;

    snap::rhi::TextureSubresourceRange dstSubres = {};
    dstSubres.baseMipLevel = dstTexture.mipLevel;
    dstSubres.baseArrayLayer = dstTexture.layer;
    dstSubres.layerCount = 1;
    dstSubres.levelCount = 1;

    return blitTextures(dc,
                        renderPipeline,
                        srcTexture.texture,
                        srcSubres,
                        dstTexture.texture,
                        dstSubres,
                        srcTextureInfo.size.width,
                        srcTextureInfo.size.height,
                        dstTextureInfo.size.width,
                        dstTextureInfo.size.height,
                        mask,
                        srcColorTexture,
                        destColorTexture,
                        filter);
}

void computeUnpackImageSize(const uint32_t width,
                            const uint32_t height,
                            const snap::rhi::PixelFormat format,
                            const uint32_t srcBytesPerRow,
                            const uint64_t srcBytesPerSlice,
                            uint32_t& bytesPerRow,
                            uint64_t& bytesPerSlice,
                            uint32_t& unpackRowLength,
                            uint32_t& unpackImageHeight) {
    const uint32_t dstBytesPerRow = snap::rhi::bytesPerRow(width, height, format);

    bytesPerRow = srcBytesPerRow ? srcBytesPerRow : dstBytesPerRow;
    bytesPerSlice = srcBytesPerSlice ? srcBytesPerSlice : snap::rhi::bytesPerSliceWithRow(bytesPerRow, height, format);

    const uint32_t bpp = snap::rhi::backend::common::divCeil(dstBytesPerRow, width);
    const uint32_t bh = static_cast<uint32_t>(
        snap::rhi::backend::common::divCeil(static_cast<uint64_t>(dstBytesPerRow) * height, bytesPerSlice));

    unpackRowLength = snap::rhi::backend::common::divCeil(bytesPerRow, bpp);
    unpackImageHeight = static_cast<uint32_t>(snap::rhi::backend::common::divCeil(
        bytesPerSlice * static_cast<uint64_t>(bh), static_cast<uint64_t>(bytesPerRow)));
}

void barrier(snap::rhi::backend::opengl::Profile& gl, const snap::rhi::backend::opengl::PipelineBarrierCmd& cmd) {
    if (cmd.dependencyFlags == snap::rhi::DependencyFlags::ByRegion) {
        GLbitfield barriers = 0;
        for (uint32_t i = 0; i < cmd.memoryBarriersCount; ++i) {
            barriers |= toGLFlags(cmd.memoryBarriers[i].dstAccessMask);
        }
        for (uint32_t i = 0; i < cmd.bufferMemoryBarriersCount; ++i) {
            barriers |= toGLFlags(cmd.bufferMemoryBarriers[i].dstAccessMask);
        }
        for (uint32_t i = 0; i < cmd.textureMemoryBarriersCount; ++i) {
            barriers |= toGLFlags(cmd.textureMemoryBarriers[i].dstAccessMask);
        }
        gl.memoryBarrierByRegion(barriers);
    } else {
        snap::rhi::common::throwException("[RenderCmdPerformer::pipelineBarrier] invalid DependencyFlags");
    }
}

uint64_t computeStagingDataSize(std::span<const snap::rhi::BufferTextureCopy> infos,
                                const snap::rhi::TextureCreateInfo& textureInfo,
                                const bool isCustomTexturePackingSupported) {
    const bool isCompressedTexture = snap::rhi::isCompressedFormat(textureInfo.format);

    uint64_t memorySize = 0;
    for (const auto& copyInfo : infos) {
        const uint32_t dstBytesPerRow =
            snap::rhi::bytesPerRow(copyInfo.textureExtent.width, copyInfo.textureExtent.height, textureInfo.format);
        const uint64_t dstBytesPerSlice =
            snap::rhi::bytesPerSlice(copyInfo.textureExtent.width, copyInfo.textureExtent.height, textureInfo.format);

        const uint32_t bytesPerRow = copyInfo.bytesPerRow ? copyInfo.bytesPerRow : dstBytesPerRow;
        const uint64_t bytesPerSlice =
            copyInfo.bytesPerSlice ?
                copyInfo.bytesPerSlice :
                snap::rhi::bytesPerSliceWithRow(bytesPerRow, copyInfo.textureExtent.height, textureInfo.format);

        bool shouldUnpack = !((bytesPerRow == dstBytesPerRow) && (bytesPerSlice == dstBytesPerSlice));
        if ((!isCustomTexturePackingSupported && shouldUnpack) ||
            (isCustomTexturePackingSupported && shouldUnpack && isCompressedTexture)) {
            memorySize = std::max(memorySize, static_cast<uint64_t>(dstBytesPerSlice) * copyInfo.textureExtent.depth);
        }
    }

    return memorySize;
}

void uploadBufferToTexture(snap::rhi::backend::opengl::Buffer* srcBuffer,
                           snap::rhi::backend::opengl::Texture* dstTexture,
                           std::span<const snap::rhi::BufferTextureCopy> infos,
                           snap::rhi::backend::opengl::Profile& gl,
                           const snap::rhi::backend::common::ValidationLayer& validationLayer,
                           snap::rhi::backend::opengl::DeviceContext* dc) {
    const auto& glFeatures = gl.getFeatures();

    if (glFeatures.isPBOSupported && (srcBuffer->getGLBuffer(dc) != GL_NONE)) {
        copyWithPBO(srcBuffer, dstTexture, infos, gl, validationLayer, dc);
    } else {
        copy(srcBuffer, dstTexture, infos, gl, validationLayer, dc);
    }

    gl.pixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    gl.pixelStorei(GL_UNPACK_IMAGE_HEIGHT, 0);
}

std::string_view tryFindGLConstantName(int64_t v, std::string_view fallback) noexcept {
    switch (v) { // clang-format off
#if SNAP_RHI_ENABLE_SLOW_VALIDATIONS()

        //
        // Possible permutations seen and useful IRL:
        //

        case 0x00000100 | 0x00000400: return "GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT (0x0500)";
        case 0x00004000 | 0x00000100: return "GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT (0x4100)";
        case 0x00004000 | 0x00000100 | 0x00000400: return "GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT (0x4500)";

        //
        // GL constants:
        // (use https://github.com/javagl/javagl.github.io/blob/master/GLConstantsTranslator/scripts/GLConstantsData.js to generate this list):
        // (some entries are commented due to duplicated IDs, not way to pick without having more context: which API call and which argument)
        //

        case 256: return "GL_DEPTH_BUFFER_BIT(0x100)";
        case 1024: return "GL_STENCIL_BUFFER_BIT(0x400)";
        case 16384: return "GL_COLOR_BUFFER_BIT(0x4000)";
        case 0: return "GL_ZERO(0x0)"; // "GL_POINTS(0x0)";
        case 1: return "GL_ONE(0x1)"; // "GL_LINES(0x1)";
        case 2: return "GL_LINE_LOOP(0x2)";
        case 3: return "GL_LINE_STRIP(0x3)";
        case 4: return "GL_TRIANGLES(0x4)";
        case 5: return "GL_TRIANGLE_STRIP(0x5)";
        case 6: return "GL_TRIANGLE_FAN(0x6)";
        case 7: return "GL_QUADS(0x7)";
        case 512: return "GL_NEVER(0x200)";
        case 513: return "GL_LESS(0x201)";
        case 514: return "GL_EQUAL(0x202)";
        case 515: return "GL_LEQUAL(0x203)";
        case 516: return "GL_GREATER(0x204)";
        case 517: return "GL_NOTEQUAL(0x205)";
        case 518: return "GL_GEQUAL(0x206)";
        case 519: return "GL_ALWAYS(0x207)";
        case 768: return "GL_SRC_COLOR(0x300)";
        case 769: return "GL_ONE_MINUS_SRC_COLOR(0x301)";
        case 770: return "GL_SRC_ALPHA(0x302)";
        case 771: return "GL_ONE_MINUS_SRC_ALPHA(0x303)";
        case 772: return "GL_DST_ALPHA(0x304)";
        case 773: return "GL_ONE_MINUS_DST_ALPHA(0x305)";
        case 774: return "GL_DST_COLOR(0x306)";
        case 775: return "GL_ONE_MINUS_DST_COLOR(0x307)";
        case 776: return "GL_SRC_ALPHA_SATURATE(0x308)";
        // case 1024: return "GL_FRONT_LEFT(0x400)";
        case 1025: return "GL_FRONT_RIGHT(0x401)";
        case 1026: return "GL_BACK_LEFT(0x402)";
        case 1027: return "GL_BACK_RIGHT(0x403)";
        case 1028: return "GL_FRONT(0x404)";
        case 1029: return "GL_BACK(0x405)";
        case 1030: return "GL_LEFT(0x406)";
        case 1031: return "GL_RIGHT(0x407)";
        case 1032: return "GL_FRONT_AND_BACK(0x408)";
        // case 1280: return "GL_INVALID_ENUM(0x500)";
        case 1281: return "GL_INVALID_VALUE(0x501)";
        case 1282: return "GL_INVALID_OPERATION(0x502)";
        case 1285: return "GL_OUT_OF_MEMORY(0x505)";
        case 2304: return "GL_CW(0x900)";
        case 2305: return "GL_CCW(0x901)";
        case 2833: return "GL_POINT_SIZE(0xb11)";
        case 2834: return "GL_POINT_SIZE_RANGE(0xb12)";
        case 2835: return "GL_POINT_SIZE_GRANULARITY(0xb13)";
        case 2848: return "GL_LINE_SMOOTH(0xb20)";
        case 2849: return "GL_LINE_WIDTH(0xb21)";
        case 2850: return "GL_LINE_WIDTH_RANGE(0xb22)";
        case 2851: return "GL_LINE_WIDTH_GRANULARITY(0xb23)";
        case 2880: return "GL_POLYGON_MODE(0xb40)";
        case 2881: return "GL_POLYGON_SMOOTH(0xb41)";
        case 2884: return "GL_CULL_FACE(0xb44)";
        case 2885: return "GL_CULL_FACE_MODE(0xb45)";
        case 2886: return "GL_FRONT_FACE(0xb46)";
        case 2928: return "GL_DEPTH_RANGE(0xb70)";
        case 2929: return "GL_DEPTH_TEST(0xb71)";
        case 2930: return "GL_DEPTH_WRITEMASK(0xb72)";
        case 2931: return "GL_DEPTH_CLEAR_VALUE(0xb73)";
        case 2932: return "GL_DEPTH_FUNC(0xb74)";
        case 2960: return "GL_STENCIL_TEST(0xb90)";
        case 2961: return "GL_STENCIL_CLEAR_VALUE(0xb91)";
        case 2962: return "GL_STENCIL_FUNC(0xb92)";
        case 2963: return "GL_STENCIL_VALUE_MASK(0xb93)";
        case 2964: return "GL_STENCIL_FAIL(0xb94)";
        case 2965: return "GL_STENCIL_PASS_DEPTH_FAIL(0xb95)";
        case 2966: return "GL_STENCIL_PASS_DEPTH_PASS(0xb96)";
        case 2967: return "GL_STENCIL_REF(0xb97)";
        case 2968: return "GL_STENCIL_WRITEMASK(0xb98)";
        case 2978: return "GL_VIEWPORT(0xba2)";
        case 3024: return "GL_DITHER(0xbd0)";
        case 3040: return "GL_BLEND_DST(0xbe0)";
        case 3041: return "GL_BLEND_SRC(0xbe1)";
        case 3042: return "GL_BLEND(0xbe2)";
        case 3056: return "GL_LOGIC_OP_MODE(0xbf0)";
        case 3058: return "GL_COLOR_LOGIC_OP(0xbf2)";
        case 3073: return "GL_DRAW_BUFFER(0xc01)";
        case 3074: return "GL_READ_BUFFER(0xc02)";
        case 3088: return "GL_SCISSOR_BOX(0xc10)";
        case 3089: return "GL_SCISSOR_TEST(0xc11)";
        case 3106: return "GL_COLOR_CLEAR_VALUE(0xc22)";
        case 3107: return "GL_COLOR_WRITEMASK(0xc23)";
        case 3122: return "GL_DOUBLEBUFFER(0xc32)";
        case 3123: return "GL_STEREO(0xc33)";
        case 3154: return "GL_LINE_SMOOTH_HINT(0xc52)";
        case 3155: return "GL_POLYGON_SMOOTH_HINT(0xc53)";
        case 3312: return "GL_UNPACK_SWAP_BYTES(0xcf0)";
        case 3313: return "GL_UNPACK_LSB_FIRST(0xcf1)";
        case 3314: return "GL_UNPACK_ROW_LENGTH(0xcf2)";
        case 3315: return "GL_UNPACK_SKIP_ROWS(0xcf3)";
        case 3316: return "GL_UNPACK_SKIP_PIXELS(0xcf4)";
        case 3317: return "GL_UNPACK_ALIGNMENT(0xcf5)";
        case 3328: return "GL_PACK_SWAP_BYTES(0xd00)";
        case 3329: return "GL_PACK_LSB_FIRST(0xd01)";
        case 3330: return "GL_PACK_ROW_LENGTH(0xd02)";
        case 3331: return "GL_PACK_SKIP_ROWS(0xd03)";
        case 3332: return "GL_PACK_SKIP_PIXELS(0xd04)";
        case 3333: return "GL_PACK_ALIGNMENT(0xd05)";
        case 3379: return "GL_MAX_TEXTURE_SIZE(0xd33)";
        case 3386: return "GL_MAX_VIEWPORT_DIMS(0xd3a)";
        case 3408: return "GL_SUBPIXEL_BITS(0xd50)";
        case 3552: return "GL_TEXTURE_1D(0xde0)";
        case 3553: return "GL_TEXTURE_2D(0xde1)";
        case 10752: return "GL_POLYGON_OFFSET_UNITS(0x2a00)";
        case 10753: return "GL_POLYGON_OFFSET_POINT(0x2a01)";
        case 10754: return "GL_POLYGON_OFFSET_LINE(0x2a02)";
        case 32823: return "GL_POLYGON_OFFSET_FILL(0x8037)";
        case 32824: return "GL_POLYGON_OFFSET_FACTOR(0x8038)";
        case 32872: return "GL_TEXTURE_BINDING_1D(0x8068)";
        case 32873: return "GL_TEXTURE_BINDING_2D(0x8069)";
        case 4096: return "GL_TEXTURE_WIDTH(0x1000)";
        case 4097: return "GL_TEXTURE_HEIGHT(0x1001)";
        case 4099: return "GL_TEXTURE_INTERNAL_FORMAT(0x1003)";
        case 4100: return "GL_TEXTURE_BORDER_COLOR(0x1004)";
        case 32860: return "GL_TEXTURE_RED_SIZE(0x805c)";
        case 32861: return "GL_TEXTURE_GREEN_SIZE(0x805d)";
        case 32862: return "GL_TEXTURE_BLUE_SIZE(0x805e)";
        case 32863: return "GL_TEXTURE_ALPHA_SIZE(0x805f)";
        case 4352: return "GL_DONT_CARE(0x1100)";
        case 4353: return "GL_FASTEST(0x1101)";
        case 4354: return "GL_NICEST(0x1102)";
        case 5120: return "GL_BYTE(0x1400)";
        case 5121: return "GL_UNSIGNED_BYTE(0x1401)";
        case 5122: return "GL_SHORT(0x1402)";
        case 5123: return "GL_UNSIGNED_SHORT(0x1403)";
        case 5124: return "GL_INT(0x1404)";
        case 5125: return "GL_UNSIGNED_INT(0x1405)";
        case 5126: return "GL_FLOAT(0x1406)";
        case 5130: return "GL_DOUBLE(0x140a)";
        case 1283: return "GL_STACK_OVERFLOW(0x503)";
        case 1284: return "GL_STACK_UNDERFLOW(0x504)";
        case 5376: return "GL_CLEAR(0x1500)";
        case 5377: return "GL_AND(0x1501)";
        case 5378: return "GL_AND_REVERSE(0x1502)";
        case 5379: return "GL_COPY(0x1503)";
        case 5380: return "GL_AND_INVERTED(0x1504)";
        case 5381: return "GL_NOOP(0x1505)";
        case 5382: return "GL_XOR(0x1506)";
        case 5383: return "GL_OR(0x1507)";
        case 5384: return "GL_NOR(0x1508)";
        case 5385: return "GL_EQUIV(0x1509)";
        case 5386: return "GL_INVERT(0x150a)";
        case 5387: return "GL_OR_REVERSE(0x150b)";
        case 5388: return "GL_COPY_INVERTED(0x150c)";
        case 5389: return "GL_OR_INVERTED(0x150d)";
        case 5390: return "GL_NAND(0x150e)";
        case 5391: return "GL_SET(0x150f)";
        case 5890: return "GL_TEXTURE(0x1702)";
        case 6144: return "GL_COLOR(0x1800)";
        case 6145: return "GL_DEPTH(0x1801)";
        case 6146: return "GL_STENCIL(0x1802)";
        case 6401: return "GL_STENCIL_INDEX(0x1901)";
        case 6402: return "GL_DEPTH_COMPONENT(0x1902)";
        case 6403: return "GL_RED(0x1903)";
        case 6404: return "GL_GREEN(0x1904)";
        case 6405: return "GL_BLUE(0x1905)";
        case 6406: return "GL_ALPHA(0x1906)";
        case 6407: return "GL_RGB(0x1907)";
        case 6408: return "GL_RGBA(0x1908)";
        case 6912: return "GL_POINT(0x1b00)";
        case 6913: return "GL_LINE(0x1b01)";
        case 6914: return "GL_FILL(0x1b02)";
        case 7680: return "GL_KEEP(0x1e00)";
        case 7681: return "GL_REPLACE(0x1e01)";
        case 7682: return "GL_INCR(0x1e02)";
        case 7683: return "GL_DECR(0x1e03)";
        case 7936: return "GL_VENDOR(0x1f00)";
        case 7937: return "GL_RENDERER(0x1f01)";
        case 7938: return "GL_VERSION(0x1f02)";
        case 7939: return "GL_EXTENSIONS(0x1f03)";
        case 9728: return "GL_NEAREST(0x2600)";
        case 9729: return "GL_LINEAR(0x2601)";
        case 9984: return "GL_NEAREST_MIPMAP_NEAREST(0x2700)";
        case 9985: return "GL_LINEAR_MIPMAP_NEAREST(0x2701)";
        case 9986: return "GL_NEAREST_MIPMAP_LINEAR(0x2702)";
        case 9987: return "GL_LINEAR_MIPMAP_LINEAR(0x2703)";
        case 10240: return "GL_TEXTURE_MAG_FILTER(0x2800)";
        case 10241: return "GL_TEXTURE_MIN_FILTER(0x2801)";
        case 10242: return "GL_TEXTURE_WRAP_S(0x2802)";
        case 10243: return "GL_TEXTURE_WRAP_T(0x2803)";
        case 32867: return "GL_PROXY_TEXTURE_1D(0x8063)";
        case 32868: return "GL_PROXY_TEXTURE_2D(0x8064)";
        case 10497: return "GL_REPEAT(0x2901)";
        case 10768: return "GL_R3_G3_B2(0x2a10)";
        case 32847: return "GL_RGB4(0x804f)";
        case 32848: return "GL_RGB5(0x8050)";
        case 32849: return "GL_RGB8(0x8051)";
        case 32850: return "GL_RGB10(0x8052)";
        case 32851: return "GL_RGB12(0x8053)";
        case 32852: return "GL_RGB16(0x8054)";
        case 32853: return "GL_RGBA2(0x8055)";
        case 32854: return "GL_RGBA4(0x8056)";
        case 32855: return "GL_RGB5_A1(0x8057)";
        case 32856: return "GL_RGBA8(0x8058)";
        case 32857: return "GL_RGB10_A2(0x8059)";
        case 32858: return "GL_RGBA12(0x805a)";
        case 32859: return "GL_RGBA16(0x805b)";
        case 32884: return "GL_VERTEX_ARRAY(0x8074)";
        case 32818: return "GL_UNSIGNED_BYTE_3_3_2(0x8032)";
        case 32819: return "GL_UNSIGNED_SHORT_4_4_4_4(0x8033)";
        case 32820: return "GL_UNSIGNED_SHORT_5_5_5_1(0x8034)";
        case 32821: return "GL_UNSIGNED_INT_8_8_8_8(0x8035)";
        case 32822: return "GL_UNSIGNED_INT_10_10_10_2(0x8036)";
        case 32874: return "GL_TEXTURE_BINDING_3D(0x806a)";
        case 32875: return "GL_PACK_SKIP_IMAGES(0x806b)";
        case 32876: return "GL_PACK_IMAGE_HEIGHT(0x806c)";
        case 32877: return "GL_UNPACK_SKIP_IMAGES(0x806d)";
        case 32878: return "GL_UNPACK_IMAGE_HEIGHT(0x806e)";
        case 32879: return "GL_TEXTURE_3D(0x806f)";
        case 32880: return "GL_PROXY_TEXTURE_3D(0x8070)";
        case 32881: return "GL_TEXTURE_DEPTH(0x8071)";
        case 32882: return "GL_TEXTURE_WRAP_R(0x8072)";
        case 32883: return "GL_MAX_3D_TEXTURE_SIZE(0x8073)";
        case 33634: return "GL_UNSIGNED_BYTE_2_3_3_REV(0x8362)";
        case 33635: return "GL_UNSIGNED_SHORT_5_6_5(0x8363)";
        case 33636: return "GL_UNSIGNED_SHORT_5_6_5_REV(0x8364)";
        case 33637: return "GL_UNSIGNED_SHORT_4_4_4_4_REV(0x8365)";
        case 33638: return "GL_UNSIGNED_SHORT_1_5_5_5_REV(0x8366)";
        case 33639: return "GL_UNSIGNED_INT_8_8_8_8_REV(0x8367)";
        case 33640: return "GL_UNSIGNED_INT_2_10_10_10_REV(0x8368)";
        case 32992: return "GL_BGR(0x80e0)";
        case 32993: return "GL_BGRA(0x80e1)";
        case 33000: return "GL_MAX_ELEMENTS_VERTICES(0x80e8)";
        case 33001: return "GL_MAX_ELEMENTS_INDICES(0x80e9)";
        case 33071: return "GL_CLAMP_TO_EDGE(0x812f)";
        case 33082: return "GL_TEXTURE_MIN_LOD(0x813a)";
        case 33083: return "GL_TEXTURE_MAX_LOD(0x813b)";
        case 33084: return "GL_TEXTURE_BASE_LEVEL(0x813c)";
        case 33085: return "GL_TEXTURE_MAX_LEVEL(0x813d)";
        // case 2834: return "GL_SMOOTH_POINT_SIZE_RANGE(0xb12)";
        // case 2835: return "GL_SMOOTH_POINT_SIZE_GRANULARITY(0xb13)";
        // case 2850: return "GL_SMOOTH_LINE_WIDTH_RANGE(0xb22)";
        // case 2851: return "GL_SMOOTH_LINE_WIDTH_GRANULARITY(0xb23)";
        case 33902: return "GL_ALIASED_LINE_WIDTH_RANGE(0x846e)";
        case 33984: return "GL_TEXTURE0(0x84c0)";
        case 33985: return "GL_TEXTURE1(0x84c1)";
        case 33986: return "GL_TEXTURE2(0x84c2)";
        case 33987: return "GL_TEXTURE3(0x84c3)";
        case 33988: return "GL_TEXTURE4(0x84c4)";
        case 33989: return "GL_TEXTURE5(0x84c5)";
        case 33990: return "GL_TEXTURE6(0x84c6)";
        case 33991: return "GL_TEXTURE7(0x84c7)";
        case 33992: return "GL_TEXTURE8(0x84c8)";
        case 33993: return "GL_TEXTURE9(0x84c9)";
        case 33994: return "GL_TEXTURE10(0x84ca)";
        case 33995: return "GL_TEXTURE11(0x84cb)";
        case 33996: return "GL_TEXTURE12(0x84cc)";
        case 33997: return "GL_TEXTURE13(0x84cd)";
        case 33998: return "GL_TEXTURE14(0x84ce)";
        case 33999: return "GL_TEXTURE15(0x84cf)";
        case 34000: return "GL_TEXTURE16(0x84d0)";
        case 34001: return "GL_TEXTURE17(0x84d1)";
        case 34002: return "GL_TEXTURE18(0x84d2)";
        case 34003: return "GL_TEXTURE19(0x84d3)";
        case 34004: return "GL_TEXTURE20(0x84d4)";
        case 34005: return "GL_TEXTURE21(0x84d5)";
        case 34006: return "GL_TEXTURE22(0x84d6)";
        case 34007: return "GL_TEXTURE23(0x84d7)";
        case 34008: return "GL_TEXTURE24(0x84d8)";
        case 34009: return "GL_TEXTURE25(0x84d9)";
        case 34010: return "GL_TEXTURE26(0x84da)";
        case 34011: return "GL_TEXTURE27(0x84db)";
        case 34012: return "GL_TEXTURE28(0x84dc)";
        case 34013: return "GL_TEXTURE29(0x84dd)";
        case 34014: return "GL_TEXTURE30(0x84de)";
        case 34015: return "GL_TEXTURE31(0x84df)";
        case 34016: return "GL_ACTIVE_TEXTURE(0x84e0)";
        case 32925: return "GL_MULTISAMPLE(0x809d)";
        case 32926: return "GL_SAMPLE_ALPHA_TO_COVERAGE(0x809e)";
        case 32927: return "GL_SAMPLE_ALPHA_TO_ONE(0x809f)";
        case 32928: return "GL_SAMPLE_COVERAGE(0x80a0)";
        case 32936: return "GL_SAMPLE_BUFFERS(0x80a8)";
        case 32937: return "GL_SAMPLES(0x80a9)";
        case 32938: return "GL_SAMPLE_COVERAGE_VALUE(0x80aa)";
        case 32939: return "GL_SAMPLE_COVERAGE_INVERT(0x80ab)";
        case 34067: return "GL_TEXTURE_CUBE_MAP(0x8513)";
        case 34068: return "GL_TEXTURE_BINDING_CUBE_MAP(0x8514)";
        case 34069: return "GL_TEXTURE_CUBE_MAP_POSITIVE_X(0x8515)";
        case 34070: return "GL_TEXTURE_CUBE_MAP_NEGATIVE_X(0x8516)";
        case 34071: return "GL_TEXTURE_CUBE_MAP_POSITIVE_Y(0x8517)";
        case 34072: return "GL_TEXTURE_CUBE_MAP_NEGATIVE_Y(0x8518)";
        case 34073: return "GL_TEXTURE_CUBE_MAP_POSITIVE_Z(0x8519)";
        case 34074: return "GL_TEXTURE_CUBE_MAP_NEGATIVE_Z(0x851a)";
        case 34075: return "GL_PROXY_TEXTURE_CUBE_MAP(0x851b)";
        case 34076: return "GL_MAX_CUBE_MAP_TEXTURE_SIZE(0x851c)";
        case 34029: return "GL_COMPRESSED_RGB(0x84ed)";
        case 34030: return "GL_COMPRESSED_RGBA(0x84ee)";
        case 34031: return "GL_TEXTURE_COMPRESSION_HINT(0x84ef)";
        case 34464: return "GL_TEXTURE_COMPRESSED_IMAGE_SIZE(0x86a0)";
        case 34465: return "GL_TEXTURE_COMPRESSED(0x86a1)";
        case 34466: return "GL_NUM_COMPRESSED_TEXTURE_FORMATS(0x86a2)";
        case 34467: return "GL_COMPRESSED_TEXTURE_FORMATS(0x86a3)";
        case 33069: return "GL_CLAMP_TO_BORDER(0x812d)";
        case 32968: return "GL_BLEND_DST_RGB(0x80c8)";
        case 32969: return "GL_BLEND_SRC_RGB(0x80c9)";
        case 32970: return "GL_BLEND_DST_ALPHA(0x80ca)";
        case 32971: return "GL_BLEND_SRC_ALPHA(0x80cb)";
        case 33064: return "GL_POINT_FADE_THRESHOLD_SIZE(0x8128)";
        case 33189: return "GL_DEPTH_COMPONENT16(0x81a5)";
        case 33190: return "GL_DEPTH_COMPONENT24(0x81a6)";
        case 33191: return "GL_DEPTH_COMPONENT32(0x81a7)";
        case 33648: return "GL_MIRRORED_REPEAT(0x8370)";
        case 34045: return "GL_MAX_TEXTURE_LOD_BIAS(0x84fd)";
        case 34049: return "GL_TEXTURE_LOD_BIAS(0x8501)";
        case 34055: return "GL_INCR_WRAP(0x8507)";
        case 34056: return "GL_DECR_WRAP(0x8508)";
        case 34890: return "GL_TEXTURE_DEPTH_SIZE(0x884a)";
        case 34892: return "GL_TEXTURE_COMPARE_MODE(0x884c)";
        case 34893: return "GL_TEXTURE_COMPARE_FUNC(0x884d)";
        case 32774: return "GL_FUNC_ADD(0x8006)";
        case 32778: return "GL_FUNC_SUBTRACT(0x800a)";
        case 32779: return "GL_FUNC_REVERSE_SUBTRACT(0x800b)";
        case 32775: return "GL_MIN(0x8007)";
        case 32776: return "GL_MAX(0x8008)";
        case 32769: return "GL_CONSTANT_COLOR(0x8001)";
        case 32770: return "GL_ONE_MINUS_CONSTANT_COLOR(0x8002)";
        case 32771: return "GL_CONSTANT_ALPHA(0x8003)";
        case 32772: return "GL_ONE_MINUS_CONSTANT_ALPHA(0x8004)";
        case 34660: return "GL_BUFFER_SIZE(0x8764)";
        case 34661: return "GL_BUFFER_USAGE(0x8765)";
        case 34916: return "GL_QUERY_COUNTER_BITS(0x8864)";
        case 34917: return "GL_CURRENT_QUERY(0x8865)";
        case 34918: return "GL_QUERY_RESULT(0x8866)";
        case 34919: return "GL_QUERY_RESULT_AVAILABLE(0x8867)";
        case 34962: return "GL_ARRAY_BUFFER(0x8892)";
        case 34963: return "GL_ELEMENT_ARRAY_BUFFER(0x8893)";
        case 34964: return "GL_ARRAY_BUFFER_BINDING(0x8894)";
        case 34965: return "GL_ELEMENT_ARRAY_BUFFER_BINDING(0x8895)";
        case 34975: return "GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING(0x889f)";
        case 35000: return "GL_READ_ONLY(0x88b8)";
        case 35001: return "GL_WRITE_ONLY(0x88b9)";
        case 35002: return "GL_READ_WRITE(0x88ba)";
        case 35003: return "GL_BUFFER_ACCESS(0x88bb)";
        case 35004: return "GL_BUFFER_MAPPED(0x88bc)";
        case 35005: return "GL_BUFFER_MAP_POINTER(0x88bd)";
        case 35040: return "GL_STREAM_DRAW(0x88e0)";
        case 35041: return "GL_STREAM_READ(0x88e1)";
        case 35042: return "GL_STREAM_COPY(0x88e2)";
        case 35044: return "GL_STATIC_DRAW(0x88e4)";
        case 35045: return "GL_STATIC_READ(0x88e5)";
        case 35046: return "GL_STATIC_COPY(0x88e6)";
        case 35048: return "GL_DYNAMIC_DRAW(0x88e8)";
        case 35049: return "GL_DYNAMIC_READ(0x88e9)";
        case 35050: return "GL_DYNAMIC_COPY(0x88ea)";
        case 35092: return "GL_SAMPLES_PASSED(0x8914)";
        case 34185: return "GL_SRC1_ALPHA(0x8589)";
        case 32777: return "GL_BLEND_EQUATION_RGB(0x8009)";
        case 34338: return "GL_VERTEX_ATTRIB_ARRAY_ENABLED(0x8622)";
        case 34339: return "GL_VERTEX_ATTRIB_ARRAY_SIZE(0x8623)";
        case 34340: return "GL_VERTEX_ATTRIB_ARRAY_STRIDE(0x8624)";
        case 34341: return "GL_VERTEX_ATTRIB_ARRAY_TYPE(0x8625)";
        case 34342: return "GL_CURRENT_VERTEX_ATTRIB(0x8626)";
        case 34370: return "GL_VERTEX_PROGRAM_POINT_SIZE(0x8642)";
        case 34373: return "GL_VERTEX_ATTRIB_ARRAY_POINTER(0x8645)";
        case 34816: return "GL_STENCIL_BACK_FUNC(0x8800)";
        case 34817: return "GL_STENCIL_BACK_FAIL(0x8801)";
        case 34818: return "GL_STENCIL_BACK_PASS_DEPTH_FAIL(0x8802)";
        case 34819: return "GL_STENCIL_BACK_PASS_DEPTH_PASS(0x8803)";
        case 34852: return "GL_MAX_DRAW_BUFFERS(0x8824)";
        case 34853: return "GL_DRAW_BUFFER0(0x8825)";
        case 34854: return "GL_DRAW_BUFFER1(0x8826)";
        case 34855: return "GL_DRAW_BUFFER2(0x8827)";
        case 34856: return "GL_DRAW_BUFFER3(0x8828)";
        case 34857: return "GL_DRAW_BUFFER4(0x8829)";
        case 34858: return "GL_DRAW_BUFFER5(0x882a)";
        case 34859: return "GL_DRAW_BUFFER6(0x882b)";
        case 34860: return "GL_DRAW_BUFFER7(0x882c)";
        case 34861: return "GL_DRAW_BUFFER8(0x882d)";
        case 34862: return "GL_DRAW_BUFFER9(0x882e)";
        case 34863: return "GL_DRAW_BUFFER10(0x882f)";
        case 34864: return "GL_DRAW_BUFFER11(0x8830)";
        case 34865: return "GL_DRAW_BUFFER12(0x8831)";
        case 34866: return "GL_DRAW_BUFFER13(0x8832)";
        case 34867: return "GL_DRAW_BUFFER14(0x8833)";
        case 34868: return "GL_DRAW_BUFFER15(0x8834)";
        case 34877: return "GL_BLEND_EQUATION_ALPHA(0x883d)";
        case 34921: return "GL_MAX_VERTEX_ATTRIBS(0x8869)";
        case 34922: return "GL_VERTEX_ATTRIB_ARRAY_NORMALIZED(0x886a)";
        case 34930: return "GL_MAX_TEXTURE_IMAGE_UNITS(0x8872)";
        case 35632: return "GL_FRAGMENT_SHADER(0x8b30)";
        case 35633: return "GL_VERTEX_SHADER(0x8b31)";
        case 35657: return "GL_MAX_FRAGMENT_UNIFORM_COMPONENTS(0x8b49)";
        case 35658: return "GL_MAX_VERTEX_UNIFORM_COMPONENTS(0x8b4a)";
        case 35659: return "GL_MAX_VARYING_FLOATS(0x8b4b)";
        case 35660: return "GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS(0x8b4c)";
        case 35661: return "GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS(0x8b4d)";
        case 35663: return "GL_SHADER_TYPE(0x8b4f)";
        case 35664: return "GL_FLOAT_VEC2(0x8b50)";
        case 35665: return "GL_FLOAT_VEC3(0x8b51)";
        case 35666: return "GL_FLOAT_VEC4(0x8b52)";
        case 35667: return "GL_INT_VEC2(0x8b53)";
        case 35668: return "GL_INT_VEC3(0x8b54)";
        case 35669: return "GL_INT_VEC4(0x8b55)";
        case 35670: return "GL_BOOL(0x8b56)";
        case 35671: return "GL_BOOL_VEC2(0x8b57)";
        case 35672: return "GL_BOOL_VEC3(0x8b58)";
        case 35673: return "GL_BOOL_VEC4(0x8b59)";
        case 35674: return "GL_FLOAT_MAT2(0x8b5a)";
        case 35675: return "GL_FLOAT_MAT3(0x8b5b)";
        case 35676: return "GL_FLOAT_MAT4(0x8b5c)";
        case 35677: return "GL_SAMPLER_1D(0x8b5d)";
        case 35678: return "GL_SAMPLER_2D(0x8b5e)";
        case 35679: return "GL_SAMPLER_3D(0x8b5f)";
        case 35680: return "GL_SAMPLER_CUBE(0x8b60)";
        case 35681: return "GL_SAMPLER_1D_SHADOW(0x8b61)";
        case 35682: return "GL_SAMPLER_2D_SHADOW(0x8b62)";
        case 35712: return "GL_DELETE_STATUS(0x8b80)";
        case 35713: return "GL_COMPILE_STATUS(0x8b81)";
        case 35714: return "GL_LINK_STATUS(0x8b82)";
        case 35715: return "GL_VALIDATE_STATUS(0x8b83)";
        case 35716: return "GL_INFO_LOG_LENGTH(0x8b84)";
        case 35717: return "GL_ATTACHED_SHADERS(0x8b85)";
        case 35718: return "GL_ACTIVE_UNIFORMS(0x8b86)";
        case 35719: return "GL_ACTIVE_UNIFORM_MAX_LENGTH(0x8b87)";
        case 35720: return "GL_SHADER_SOURCE_LENGTH(0x8b88)";
        case 35721: return "GL_ACTIVE_ATTRIBUTES(0x8b89)";
        case 35722: return "GL_ACTIVE_ATTRIBUTE_MAX_LENGTH(0x8b8a)";
        case 35723: return "GL_FRAGMENT_SHADER_DERIVATIVE_HINT(0x8b8b)";
        case 35724: return "GL_SHADING_LANGUAGE_VERSION(0x8b8c)";
        case 35725: return "GL_CURRENT_PROGRAM(0x8b8d)";
        case 36000: return "GL_POINT_SPRITE_COORD_ORIGIN(0x8ca0)";
        case 36001: return "GL_LOWER_LEFT(0x8ca1)";
        case 36002: return "GL_UPPER_LEFT(0x8ca2)";
        case 36003: return "GL_STENCIL_BACK_REF(0x8ca3)";
        case 36004: return "GL_STENCIL_BACK_VALUE_MASK(0x8ca4)";
        case 36005: return "GL_STENCIL_BACK_WRITEMASK(0x8ca5)";
        case 35051: return "GL_PIXEL_PACK_BUFFER(0x88eb)";
        case 35052: return "GL_PIXEL_UNPACK_BUFFER(0x88ec)";
        case 35053: return "GL_PIXEL_PACK_BUFFER_BINDING(0x88ed)";
        case 35055: return "GL_PIXEL_UNPACK_BUFFER_BINDING(0x88ef)";
        case 35904: return "GL_SRGB(0x8c40)";
        case 35905: return "GL_SRGB8(0x8c41)";
        case 35906: return "GL_SRGB_ALPHA(0x8c42)";
        case 35907: return "GL_SRGB8_ALPHA8(0x8c43)";
        case 35912: return "GL_COMPRESSED_SRGB(0x8c48)";
        case 35913: return "GL_COMPRESSED_SRGB_ALPHA(0x8c49)";
        case 34894: return "GL_COMPARE_REF_TO_TEXTURE(0x884e)";
        case 12288: return "GL_CLIP_DISTANCE0(0x3000)";
        case 12289: return "GL_CLIP_DISTANCE1(0x3001)";
        case 12290: return "GL_CLIP_DISTANCE2(0x3002)";
        case 12291: return "GL_CLIP_DISTANCE3(0x3003)";
        case 12292: return "GL_CLIP_DISTANCE4(0x3004)";
        case 12293: return "GL_CLIP_DISTANCE5(0x3005)";
        case 12294: return "GL_CLIP_DISTANCE6(0x3006)";
        case 12295: return "GL_CLIP_DISTANCE7(0x3007)";
        case 3378: return "GL_MAX_CLIP_DISTANCES(0xd32)";
        case 33307: return "GL_MAJOR_VERSION(0x821b)";
        case 33308: return "GL_MINOR_VERSION(0x821c)";
        case 33309: return "GL_NUM_EXTENSIONS(0x821d)";
        case 33310: return "GL_CONTEXT_FLAGS(0x821e)";
        case 33317: return "GL_COMPRESSED_RED(0x8225)";
        case 33318: return "GL_COMPRESSED_RG(0x8226)";
        case 34836: return "GL_RGBA32F(0x8814)";
        case 34837: return "GL_RGB32F(0x8815)";
        case 34842: return "GL_RGBA16F(0x881a)";
        case 34843: return "GL_RGB16F(0x881b)";
        case 35069: return "GL_VERTEX_ATTRIB_ARRAY_INTEGER(0x88fd)";
        case 35071: return "GL_MAX_ARRAY_TEXTURE_LAYERS(0x88ff)";
        case 35076: return "GL_MIN_PROGRAM_TEXEL_OFFSET(0x8904)";
        case 35077: return "GL_MAX_PROGRAM_TEXEL_OFFSET(0x8905)";
        case 35100: return "GL_CLAMP_READ_COLOR(0x891c)";
        case 35101: return "GL_FIXED_ONLY(0x891d)";
        // case 35659: return "GL_MAX_VARYING_COMPONENTS(0x8b4b)";
        case 35864: return "GL_TEXTURE_1D_ARRAY(0x8c18)";
        case 35865: return "GL_PROXY_TEXTURE_1D_ARRAY(0x8c19)";
        case 35866: return "GL_TEXTURE_2D_ARRAY(0x8c1a)";
        case 35867: return "GL_PROXY_TEXTURE_2D_ARRAY(0x8c1b)";
        case 35868: return "GL_TEXTURE_BINDING_1D_ARRAY(0x8c1c)";
        case 35869: return "GL_TEXTURE_BINDING_2D_ARRAY(0x8c1d)";
        case 35898: return "GL_R11F_G11F_B10F(0x8c3a)";
        case 35899: return "GL_UNSIGNED_INT_10F_11F_11F_REV(0x8c3b)";
        case 35901: return "GL_RGB9_E5(0x8c3d)";
        case 35902: return "GL_UNSIGNED_INT_5_9_9_9_REV(0x8c3e)";
        case 35903: return "GL_TEXTURE_SHARED_SIZE(0x8c3f)";
        case 35958: return "GL_TRANSFORM_FEEDBACK_VARYING_MAX_LENGTH(0x8c76)";
        case 35967: return "GL_TRANSFORM_FEEDBACK_BUFFER_MODE(0x8c7f)";
        case 35968: return "GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS(0x8c80)";
        case 35971: return "GL_TRANSFORM_FEEDBACK_VARYINGS(0x8c83)";
        case 35972: return "GL_TRANSFORM_FEEDBACK_BUFFER_START(0x8c84)";
        case 35973: return "GL_TRANSFORM_FEEDBACK_BUFFER_SIZE(0x8c85)";
        case 35975: return "GL_PRIMITIVES_GENERATED(0x8c87)";
        case 35976: return "GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN(0x8c88)";
        case 35977: return "GL_RASTERIZER_DISCARD(0x8c89)";
        case 35978: return "GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS(0x8c8a)";
        case 35979: return "GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS(0x8c8b)";
        case 35980: return "GL_INTERLEAVED_ATTRIBS(0x8c8c)";
        case 35981: return "GL_SEPARATE_ATTRIBS(0x8c8d)";
        case 35982: return "GL_TRANSFORM_FEEDBACK_BUFFER(0x8c8e)";
        case 35983: return "GL_TRANSFORM_FEEDBACK_BUFFER_BINDING(0x8c8f)";
        case 36208: return "GL_RGBA32UI(0x8d70)";
        case 36209: return "GL_RGB32UI(0x8d71)";
        case 36214: return "GL_RGBA16UI(0x8d76)";
        case 36215: return "GL_RGB16UI(0x8d77)";
        case 36220: return "GL_RGBA8UI(0x8d7c)";
        case 36221: return "GL_RGB8UI(0x8d7d)";
        case 36226: return "GL_RGBA32I(0x8d82)";
        case 36227: return "GL_RGB32I(0x8d83)";
        case 36232: return "GL_RGBA16I(0x8d88)";
        case 36233: return "GL_RGB16I(0x8d89)";
        case 36238: return "GL_RGBA8I(0x8d8e)";
        case 36239: return "GL_RGB8I(0x8d8f)";
        case 36244: return "GL_RED_INTEGER(0x8d94)";
        case 36245: return "GL_GREEN_INTEGER(0x8d95)";
        case 36246: return "GL_BLUE_INTEGER(0x8d96)";
        case 36248: return "GL_RGB_INTEGER(0x8d98)";
        case 36249: return "GL_RGBA_INTEGER(0x8d99)";
        case 36250: return "GL_BGR_INTEGER(0x8d9a)";
        case 36251: return "GL_BGRA_INTEGER(0x8d9b)";
        case 36288: return "GL_SAMPLER_1D_ARRAY(0x8dc0)";
        case 36289: return "GL_SAMPLER_2D_ARRAY(0x8dc1)";
        case 36291: return "GL_SAMPLER_1D_ARRAY_SHADOW(0x8dc3)";
        case 36292: return "GL_SAMPLER_2D_ARRAY_SHADOW(0x8dc4)";
        case 36293: return "GL_SAMPLER_CUBE_SHADOW(0x8dc5)";
        case 36294: return "GL_UNSIGNED_INT_VEC2(0x8dc6)";
        case 36295: return "GL_UNSIGNED_INT_VEC3(0x8dc7)";
        case 36296: return "GL_UNSIGNED_INT_VEC4(0x8dc8)";
        case 36297: return "GL_INT_SAMPLER_1D(0x8dc9)";
        case 36298: return "GL_INT_SAMPLER_2D(0x8dca)";
        case 36299: return "GL_INT_SAMPLER_3D(0x8dcb)";
        case 36300: return "GL_INT_SAMPLER_CUBE(0x8dcc)";
        case 36302: return "GL_INT_SAMPLER_1D_ARRAY(0x8dce)";
        case 36303: return "GL_INT_SAMPLER_2D_ARRAY(0x8dcf)";
        case 36305: return "GL_UNSIGNED_INT_SAMPLER_1D(0x8dd1)";
        case 36306: return "GL_UNSIGNED_INT_SAMPLER_2D(0x8dd2)";
        case 36307: return "GL_UNSIGNED_INT_SAMPLER_3D(0x8dd3)";
        case 36308: return "GL_UNSIGNED_INT_SAMPLER_CUBE(0x8dd4)";
        case 36310: return "GL_UNSIGNED_INT_SAMPLER_1D_ARRAY(0x8dd6)";
        case 36311: return "GL_UNSIGNED_INT_SAMPLER_2D_ARRAY(0x8dd7)";
        case 36371: return "GL_QUERY_WAIT(0x8e13)";
        case 36372: return "GL_QUERY_NO_WAIT(0x8e14)";
        case 36373: return "GL_QUERY_BY_REGION_WAIT(0x8e15)";
        case 36374: return "GL_QUERY_BY_REGION_NO_WAIT(0x8e16)";
        case 37151: return "GL_BUFFER_ACCESS_FLAGS(0x911f)";
        case 37152: return "GL_BUFFER_MAP_LENGTH(0x9120)";
        case 37153: return "GL_BUFFER_MAP_OFFSET(0x9121)";
        case 36012: return "GL_DEPTH_COMPONENT32F(0x8cac)";
        case 36013: return "GL_DEPTH32F_STENCIL8(0x8cad)";
        case 36269: return "GL_FLOAT_32_UNSIGNED_INT_24_8_REV(0x8dad)";
        case 1286: return "GL_INVALID_FRAMEBUFFER_OPERATION(0x506)";
        case 33296: return "GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING(0x8210)";
        case 33297: return "GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE(0x8211)";
        case 33298: return "GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE(0x8212)";
        case 33299: return "GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE(0x8213)";
        case 33300: return "GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE(0x8214)";
        case 33301: return "GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE(0x8215)";
        case 33302: return "GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE(0x8216)";
        case 33303: return "GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE(0x8217)";
        case 33304: return "GL_FRAMEBUFFER_DEFAULT(0x8218)";
        case 33305: return "GL_FRAMEBUFFER_UNDEFINED(0x8219)";
        case 33306: return "GL_DEPTH_STENCIL_ATTACHMENT(0x821a)";
        case 34024: return "GL_MAX_RENDERBUFFER_SIZE(0x84e8)";
        case 34041: return "GL_DEPTH_STENCIL(0x84f9)";
        case 34042: return "GL_UNSIGNED_INT_24_8(0x84fa)";
        case 35056: return "GL_DEPTH24_STENCIL8(0x88f0)";
        case 35057: return "GL_TEXTURE_STENCIL_SIZE(0x88f1)";
        case 35856: return "GL_TEXTURE_RED_TYPE(0x8c10)";
        case 35857: return "GL_TEXTURE_GREEN_TYPE(0x8c11)";
        case 35858: return "GL_TEXTURE_BLUE_TYPE(0x8c12)";
        case 35859: return "GL_TEXTURE_ALPHA_TYPE(0x8c13)";
        case 35862: return "GL_TEXTURE_DEPTH_TYPE(0x8c16)";
        case 35863: return "GL_UNSIGNED_NORMALIZED(0x8c17)";
        // case 36006: return "GL_FRAMEBUFFER_BINDING(0x8ca6)";
        case 36006: return "GL_DRAW_FRAMEBUFFER_BINDING(0x8ca6)";
        case 36007: return "GL_RENDERBUFFER_BINDING(0x8ca7)";
        case 36008: return "GL_READ_FRAMEBUFFER(0x8ca8)";
        case 36009: return "GL_DRAW_FRAMEBUFFER(0x8ca9)";
        case 36010: return "GL_READ_FRAMEBUFFER_BINDING(0x8caa)";
        case 36011: return "GL_RENDERBUFFER_SAMPLES(0x8cab)";
        case 36048: return "GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE(0x8cd0)";
        case 36049: return "GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME(0x8cd1)";
        case 36050: return "GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL(0x8cd2)";
        case 36051: return "GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE(0x8cd3)";
        case 36052: return "GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER(0x8cd4)";
        case 36053: return "GL_FRAMEBUFFER_COMPLETE(0x8cd5)";
        case 36054: return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT(0x8cd6)";
        case 36055: return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT(0x8cd7)";
        case 36059: return "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER(0x8cdb)";
        case 36060: return "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER(0x8cdc)";
        case 36061: return "GL_FRAMEBUFFER_UNSUPPORTED(0x8cdd)";
        case 36063: return "GL_MAX_COLOR_ATTACHMENTS(0x8cdf)";
        case 36064: return "GL_COLOR_ATTACHMENT0(0x8ce0)";
        case 36065: return "GL_COLOR_ATTACHMENT1(0x8ce1)";
        case 36066: return "GL_COLOR_ATTACHMENT2(0x8ce2)";
        case 36067: return "GL_COLOR_ATTACHMENT3(0x8ce3)";
        case 36068: return "GL_COLOR_ATTACHMENT4(0x8ce4)";
        case 36069: return "GL_COLOR_ATTACHMENT5(0x8ce5)";
        case 36070: return "GL_COLOR_ATTACHMENT6(0x8ce6)";
        case 36071: return "GL_COLOR_ATTACHMENT7(0x8ce7)";
        case 36072: return "GL_COLOR_ATTACHMENT8(0x8ce8)";
        case 36073: return "GL_COLOR_ATTACHMENT9(0x8ce9)";
        case 36074: return "GL_COLOR_ATTACHMENT10(0x8cea)";
        case 36075: return "GL_COLOR_ATTACHMENT11(0x8ceb)";
        case 36076: return "GL_COLOR_ATTACHMENT12(0x8cec)";
        case 36077: return "GL_COLOR_ATTACHMENT13(0x8ced)";
        case 36078: return "GL_COLOR_ATTACHMENT14(0x8cee)";
        case 36079: return "GL_COLOR_ATTACHMENT15(0x8cef)";
        case 36080: return "GL_COLOR_ATTACHMENT16(0x8cf0)";
        case 36081: return "GL_COLOR_ATTACHMENT17(0x8cf1)";
        case 36082: return "GL_COLOR_ATTACHMENT18(0x8cf2)";
        case 36083: return "GL_COLOR_ATTACHMENT19(0x8cf3)";
        case 36084: return "GL_COLOR_ATTACHMENT20(0x8cf4)";
        case 36085: return "GL_COLOR_ATTACHMENT21(0x8cf5)";
        case 36086: return "GL_COLOR_ATTACHMENT22(0x8cf6)";
        case 36087: return "GL_COLOR_ATTACHMENT23(0x8cf7)";
        case 36088: return "GL_COLOR_ATTACHMENT24(0x8cf8)";
        case 36089: return "GL_COLOR_ATTACHMENT25(0x8cf9)";
        case 36090: return "GL_COLOR_ATTACHMENT26(0x8cfa)";
        case 36091: return "GL_COLOR_ATTACHMENT27(0x8cfb)";
        case 36092: return "GL_COLOR_ATTACHMENT28(0x8cfc)";
        case 36093: return "GL_COLOR_ATTACHMENT29(0x8cfd)";
        case 36094: return "GL_COLOR_ATTACHMENT30(0x8cfe)";
        case 36095: return "GL_COLOR_ATTACHMENT31(0x8cff)";
        case 36096: return "GL_DEPTH_ATTACHMENT(0x8d00)";
        case 36128: return "GL_STENCIL_ATTACHMENT(0x8d20)";
        case 36160: return "GL_FRAMEBUFFER(0x8d40)";
        case 36161: return "GL_RENDERBUFFER(0x8d41)";
        case 36162: return "GL_RENDERBUFFER_WIDTH(0x8d42)";
        case 36163: return "GL_RENDERBUFFER_HEIGHT(0x8d43)";
        case 36164: return "GL_RENDERBUFFER_INTERNAL_FORMAT(0x8d44)";
        case 36166: return "GL_STENCIL_INDEX1(0x8d46)";
        case 36167: return "GL_STENCIL_INDEX4(0x8d47)";
        case 36168: return "GL_STENCIL_INDEX8(0x8d48)";
        case 36169: return "GL_STENCIL_INDEX16(0x8d49)";
        case 36176: return "GL_RENDERBUFFER_RED_SIZE(0x8d50)";
        case 36177: return "GL_RENDERBUFFER_GREEN_SIZE(0x8d51)";
        case 36178: return "GL_RENDERBUFFER_BLUE_SIZE(0x8d52)";
        case 36179: return "GL_RENDERBUFFER_ALPHA_SIZE(0x8d53)";
        case 36180: return "GL_RENDERBUFFER_DEPTH_SIZE(0x8d54)";
        case 36181: return "GL_RENDERBUFFER_STENCIL_SIZE(0x8d55)";
        case 36182: return "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE(0x8d56)";
        case 36183: return "GL_MAX_SAMPLES(0x8d57)";
        case 36281: return "GL_FRAMEBUFFER_SRGB(0x8db9)";
        case 5131: return "GL_HALF_FLOAT(0x140b)";
        // case 1: return "GL_MAP_READ_BIT(0x1)";
        // case 2: return "GL_MAP_WRITE_BIT(0x2)";
        // case 4: return "GL_MAP_INVALIDATE_RANGE_BIT(0x4)";
        case 8: return "GL_MAP_INVALIDATE_BUFFER_BIT(0x8)";
        case 16: return "GL_MAP_FLUSH_EXPLICIT_BIT(0x10)";
        case 32: return "GL_MAP_UNSYNCHRONIZED_BIT(0x20)";
        case 36283: return "GL_COMPRESSED_RED_RGTC1(0x8dbb)";
        case 36284: return "GL_COMPRESSED_SIGNED_RED_RGTC1(0x8dbc)";
        case 36285: return "GL_COMPRESSED_RG_RGTC2(0x8dbd)";
        case 36286: return "GL_COMPRESSED_SIGNED_RG_RGTC2(0x8dbe)";
        case 33319: return "GL_RG(0x8227)";
        case 33320: return "GL_RG_INTEGER(0x8228)";
        case 33321: return "GL_R8(0x8229)";
        case 33322: return "GL_R16(0x822a)";
        case 33323: return "GL_RG8(0x822b)";
        case 33324: return "GL_RG16(0x822c)";
        case 33325: return "GL_R16F(0x822d)";
        case 33326: return "GL_R32F(0x822e)";
        case 33327: return "GL_RG16F(0x822f)";
        case 33328: return "GL_RG32F(0x8230)";
        case 33329: return "GL_R8I(0x8231)";
        case 33330: return "GL_R8UI(0x8232)";
        case 33331: return "GL_R16I(0x8233)";
        case 33332: return "GL_R16UI(0x8234)";
        case 33333: return "GL_R32I(0x8235)";
        case 33334: return "GL_R32UI(0x8236)";
        case 33335: return "GL_RG8I(0x8237)";
        case 33336: return "GL_RG8UI(0x8238)";
        case 33337: return "GL_RG16I(0x8239)";
        case 33338: return "GL_RG16UI(0x823a)";
        case 33339: return "GL_RG32I(0x823b)";
        case 33340: return "GL_RG32UI(0x823c)";
        case 34229: return "GL_VERTEX_ARRAY_BINDING(0x85b5)";
        case 35683: return "GL_SAMPLER_2D_RECT(0x8b63)";
        case 35684: return "GL_SAMPLER_2D_RECT_SHADOW(0x8b64)";
        case 36290: return "GL_SAMPLER_BUFFER(0x8dc2)";
        case 36301: return "GL_INT_SAMPLER_2D_RECT(0x8dcd)";
        case 36304: return "GL_INT_SAMPLER_BUFFER(0x8dd0)";
        case 36309: return "GL_UNSIGNED_INT_SAMPLER_2D_RECT(0x8dd5)";
        case 36312: return "GL_UNSIGNED_INT_SAMPLER_BUFFER(0x8dd8)";
        case 35882: return "GL_TEXTURE_BUFFER(0x8c2a)";
        case 35883: return "GL_MAX_TEXTURE_BUFFER_SIZE(0x8c2b)";
        case 35884: return "GL_TEXTURE_BINDING_BUFFER(0x8c2c)";
        case 35885: return "GL_TEXTURE_BUFFER_DATA_STORE_BINDING(0x8c2d)";
        case 34037: return "GL_TEXTURE_RECTANGLE(0x84f5)";
        case 34038: return "GL_TEXTURE_BINDING_RECTANGLE(0x84f6)";
        case 34039: return "GL_PROXY_TEXTURE_RECTANGLE(0x84f7)";
        case 34040: return "GL_MAX_RECTANGLE_TEXTURE_SIZE(0x84f8)";
        case 36756: return "GL_R8_SNORM(0x8f94)";
        case 36757: return "GL_RG8_SNORM(0x8f95)";
        case 36758: return "GL_RGB8_SNORM(0x8f96)";
        case 36759: return "GL_RGBA8_SNORM(0x8f97)";
        case 36760: return "GL_R16_SNORM(0x8f98)";
        case 36761: return "GL_RG16_SNORM(0x8f99)";
        case 36762: return "GL_RGB16_SNORM(0x8f9a)";
        case 36763: return "GL_RGBA16_SNORM(0x8f9b)";
        case 36764: return "GL_SIGNED_NORMALIZED(0x8f9c)";
        case 36765: return "GL_PRIMITIVE_RESTART(0x8f9d)";
        case 36766: return "GL_PRIMITIVE_RESTART_INDEX(0x8f9e)";
        case 36662: return "GL_COPY_READ_BUFFER(0x8f36)";
        case 36663: return "GL_COPY_WRITE_BUFFER(0x8f37)";
        case 35345: return "GL_UNIFORM_BUFFER(0x8a11)";
        case 35368: return "GL_UNIFORM_BUFFER_BINDING(0x8a28)";
        case 35369: return "GL_UNIFORM_BUFFER_START(0x8a29)";
        case 35370: return "GL_UNIFORM_BUFFER_SIZE(0x8a2a)";
        case 35371: return "GL_MAX_VERTEX_UNIFORM_BLOCKS(0x8a2b)";
        case 35372: return "GL_MAX_GEOMETRY_UNIFORM_BLOCKS(0x8a2c)";
        case 35373: return "GL_MAX_FRAGMENT_UNIFORM_BLOCKS(0x8a2d)";
        case 35374: return "GL_MAX_COMBINED_UNIFORM_BLOCKS(0x8a2e)";
        case 35375: return "GL_MAX_UNIFORM_BUFFER_BINDINGS(0x8a2f)";
        case 35376: return "GL_MAX_UNIFORM_BLOCK_SIZE(0x8a30)";
        case 35377: return "GL_MAX_COMBINED_VERTEX_UNIFORM_COMPONENTS(0x8a31)";
        case 35378: return "GL_MAX_COMBINED_GEOMETRY_UNIFORM_COMPONENTS(0x8a32)";
        case 35379: return "GL_MAX_COMBINED_FRAGMENT_UNIFORM_COMPONENTS(0x8a33)";
        case 35380: return "GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT(0x8a34)";
        case 35381: return "GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH(0x8a35)";
        case 35382: return "GL_ACTIVE_UNIFORM_BLOCKS(0x8a36)";
        case 35383: return "GL_UNIFORM_TYPE(0x8a37)";
        case 35384: return "GL_UNIFORM_SIZE(0x8a38)";
        case 35385: return "GL_UNIFORM_NAME_LENGTH(0x8a39)";
        case 35386: return "GL_UNIFORM_BLOCK_INDEX(0x8a3a)";
        case 35387: return "GL_UNIFORM_OFFSET(0x8a3b)";
        case 35388: return "GL_UNIFORM_ARRAY_STRIDE(0x8a3c)";
        case 35389: return "GL_UNIFORM_MATRIX_STRIDE(0x8a3d)";
        case 35390: return "GL_UNIFORM_IS_ROW_MAJOR(0x8a3e)";
        case 35391: return "GL_UNIFORM_BLOCK_BINDING(0x8a3f)";
        case 35392: return "GL_UNIFORM_BLOCK_DATA_SIZE(0x8a40)";
        case 35393: return "GL_UNIFORM_BLOCK_NAME_LENGTH(0x8a41)";
        case 35394: return "GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS(0x8a42)";
        case 35395: return "GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES(0x8a43)";
        case 35396: return "GL_UNIFORM_BLOCK_REFERENCED_BY_VERTEX_SHADER(0x8a44)";
        case 35397: return "GL_UNIFORM_BLOCK_REFERENCED_BY_GEOMETRY_SHADER(0x8a45)";
        case 35398: return "GL_UNIFORM_BLOCK_REFERENCED_BY_FRAGMENT_SHADER(0x8a46)";
        // case 1: return "GL_CONTEXT_CORE_PROFILE_BIT(0x1)";
        // case 2: return "GL_CONTEXT_COMPATIBILITY_PROFILE_BIT(0x2)";
        case 10: return "GL_LINES_ADJACENCY(0xa)";
        case 11: return "GL_LINE_STRIP_ADJACENCY(0xb)";
        case 12: return "GL_TRIANGLES_ADJACENCY(0xc)";
        case 13: return "GL_TRIANGLE_STRIP_ADJACENCY(0xd)";
        // case 34370: return "GL_PROGRAM_POINT_SIZE(0x8642)";
        case 35881: return "GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS(0x8c29)";
        case 36263: return "GL_FRAMEBUFFER_ATTACHMENT_LAYERED(0x8da7)";
        case 36264: return "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS(0x8da8)";
        case 36313: return "GL_GEOMETRY_SHADER(0x8dd9)";
        case 35094: return "GL_GEOMETRY_VERTICES_OUT(0x8916)";
        case 35095: return "GL_GEOMETRY_INPUT_TYPE(0x8917)";
        case 35096: return "GL_GEOMETRY_OUTPUT_TYPE(0x8918)";
        case 36319: return "GL_MAX_GEOMETRY_UNIFORM_COMPONENTS(0x8ddf)";
        case 36320: return "GL_MAX_GEOMETRY_OUTPUT_VERTICES(0x8de0)";
        case 36321: return "GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS(0x8de1)";
        case 37154: return "GL_MAX_VERTEX_OUTPUT_COMPONENTS(0x9122)";
        case 37155: return "GL_MAX_GEOMETRY_INPUT_COMPONENTS(0x9123)";
        case 37156: return "GL_MAX_GEOMETRY_OUTPUT_COMPONENTS(0x9124)";
        case 37157: return "GL_MAX_FRAGMENT_INPUT_COMPONENTS(0x9125)";
        case 37158: return "GL_CONTEXT_PROFILE_MASK(0x9126)";
        case 34383: return "GL_DEPTH_CLAMP(0x864f)";
        case 36428: return "GL_QUADS_FOLLOW_PROVOKING_VERTEX_CONVENTION(0x8e4c)";
        case 36429: return "GL_FIRST_VERTEX_CONVENTION(0x8e4d)";
        case 36430: return "GL_LAST_VERTEX_CONVENTION(0x8e4e)";
        case 36431: return "GL_PROVOKING_VERTEX(0x8e4f)";
        case 34895: return "GL_TEXTURE_CUBE_MAP_SEAMLESS(0x884f)";
        case 37137: return "GL_MAX_SERVER_WAIT_TIMEOUT(0x9111)";
        case 37138: return "GL_OBJECT_TYPE(0x9112)";
        case 37139: return "GL_SYNC_CONDITION(0x9113)";
        case 37140: return "GL_SYNC_STATUS(0x9114)";
        case 37141: return "GL_SYNC_FLAGS(0x9115)";
        case 37142: return "GL_SYNC_FENCE(0x9116)";
        case 37143: return "GL_SYNC_GPU_COMMANDS_COMPLETE(0x9117)";
        case 37144: return "GL_UNSIGNALED(0x9118)";
        case 37145: return "GL_SIGNALED(0x9119)";
        case 37146: return "GL_ALREADY_SIGNALED(0x911a)";
        case 37147: return "GL_TIMEOUT_EXPIRED(0x911b)";
        case 37148: return "GL_CONDITION_SATISFIED(0x911c)";
        case 37149: return "GL_WAIT_FAILED(0x911d)";
        // case 1: return "GL_SYNC_FLUSH_COMMANDS_BIT(0x1)";
        case 36432: return "GL_SAMPLE_POSITION(0x8e50)";
        case 36433: return "GL_SAMPLE_MASK(0x8e51)";
        case 36434: return "GL_SAMPLE_MASK_VALUE(0x8e52)";
        case 36441: return "GL_MAX_SAMPLE_MASK_WORDS(0x8e59)";
        case 37120: return "GL_TEXTURE_2D_MULTISAMPLE(0x9100)";
        case 37121: return "GL_PROXY_TEXTURE_2D_MULTISAMPLE(0x9101)";
        case 37122: return "GL_TEXTURE_2D_MULTISAMPLE_ARRAY(0x9102)";
        case 37123: return "GL_PROXY_TEXTURE_2D_MULTISAMPLE_ARRAY(0x9103)";
        case 37124: return "GL_TEXTURE_BINDING_2D_MULTISAMPLE(0x9104)";
        case 37125: return "GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY(0x9105)";
        case 37126: return "GL_TEXTURE_SAMPLES(0x9106)";
        case 37127: return "GL_TEXTURE_FIXED_SAMPLE_LOCATIONS(0x9107)";
        case 37128: return "GL_SAMPLER_2D_MULTISAMPLE(0x9108)";
        case 37129: return "GL_INT_SAMPLER_2D_MULTISAMPLE(0x9109)";
        case 37130: return "GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE(0x910a)";
        case 37131: return "GL_SAMPLER_2D_MULTISAMPLE_ARRAY(0x910b)";
        case 37132: return "GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY(0x910c)";
        case 37133: return "GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY(0x910d)";
        case 37134: return "GL_MAX_COLOR_TEXTURE_SAMPLES(0x910e)";
        case 37135: return "GL_MAX_DEPTH_TEXTURE_SAMPLES(0x910f)";
        case 37136: return "GL_MAX_INTEGER_SAMPLES(0x9110)";
        case 35070: return "GL_VERTEX_ATTRIB_ARRAY_DIVISOR(0x88fe)";
        case 35065: return "GL_SRC1_COLOR(0x88f9)";
        case 35066: return "GL_ONE_MINUS_SRC1_COLOR(0x88fa)";
        case 35067: return "GL_ONE_MINUS_SRC1_ALPHA(0x88fb)";
        case 35068: return "GL_MAX_DUAL_SOURCE_DRAW_BUFFERS(0x88fc)";
        case 35887: return "GL_ANY_SAMPLES_PASSED(0x8c2f)";
        case 35097: return "GL_SAMPLER_BINDING(0x8919)";
        case 36975: return "GL_RGB10_A2UI(0x906f)";
        case 36418: return "GL_TEXTURE_SWIZZLE_R(0x8e42)";
        case 36419: return "GL_TEXTURE_SWIZZLE_G(0x8e43)";
        case 36420: return "GL_TEXTURE_SWIZZLE_B(0x8e44)";
        case 36421: return "GL_TEXTURE_SWIZZLE_A(0x8e45)";
        case 36422: return "GL_TEXTURE_SWIZZLE_RGBA(0x8e46)";
        case 35007: return "GL_TIME_ELAPSED(0x88bf)";
        case 36392: return "GL_TIMESTAMP(0x8e28)";
        case 36255: return "GL_INT_2_10_10_10_REV(0x8d9f)";
        case 35894: return "GL_SAMPLE_SHADING(0x8c36)";
        case 35895: return "GL_MIN_SAMPLE_SHADING_VALUE(0x8c37)";
        case 36446: return "GL_MIN_PROGRAM_TEXTURE_GATHER_OFFSET(0x8e5e)";
        case 36447: return "GL_MAX_PROGRAM_TEXTURE_GATHER_OFFSET(0x8e5f)";
        case 36873: return "GL_TEXTURE_CUBE_MAP_ARRAY(0x9009)";
        case 36874: return "GL_TEXTURE_BINDING_CUBE_MAP_ARRAY(0x900a)";
        case 36875: return "GL_PROXY_TEXTURE_CUBE_MAP_ARRAY(0x900b)";
        case 36876: return "GL_SAMPLER_CUBE_MAP_ARRAY(0x900c)";
        case 36877: return "GL_SAMPLER_CUBE_MAP_ARRAY_SHADOW(0x900d)";
        case 36878: return "GL_INT_SAMPLER_CUBE_MAP_ARRAY(0x900e)";
        case 36879: return "GL_UNSIGNED_INT_SAMPLER_CUBE_MAP_ARRAY(0x900f)";
        case 36671: return "GL_DRAW_INDIRECT_BUFFER(0x8f3f)";
        case 36675: return "GL_DRAW_INDIRECT_BUFFER_BINDING(0x8f43)";
        case 34943: return "GL_GEOMETRY_SHADER_INVOCATIONS(0x887f)";
        case 36442: return "GL_MAX_GEOMETRY_SHADER_INVOCATIONS(0x8e5a)";
        case 36443: return "GL_MIN_FRAGMENT_INTERPOLATION_OFFSET(0x8e5b)";
        case 36444: return "GL_MAX_FRAGMENT_INTERPOLATION_OFFSET(0x8e5c)";
        case 36445: return "GL_FRAGMENT_INTERPOLATION_OFFSET_BITS(0x8e5d)";
        case 36465: return "GL_MAX_VERTEX_STREAMS(0x8e71)";
        case 36860: return "GL_DOUBLE_VEC2(0x8ffc)";
        case 36861: return "GL_DOUBLE_VEC3(0x8ffd)";
        case 36862: return "GL_DOUBLE_VEC4(0x8ffe)";
        case 36678: return "GL_DOUBLE_MAT2(0x8f46)";
        case 36679: return "GL_DOUBLE_MAT3(0x8f47)";
        case 36680: return "GL_DOUBLE_MAT4(0x8f48)";
        case 36325: return "GL_ACTIVE_SUBROUTINES(0x8de5)";
        case 36326: return "GL_ACTIVE_SUBROUTINE_UNIFORMS(0x8de6)";
        case 36423: return "GL_ACTIVE_SUBROUTINE_UNIFORM_LOCATIONS(0x8e47)";
        case 36424: return "GL_ACTIVE_SUBROUTINE_MAX_LENGTH(0x8e48)";
        case 36425: return "GL_ACTIVE_SUBROUTINE_UNIFORM_MAX_LENGTH(0x8e49)";
        case 36327: return "GL_MAX_SUBROUTINES(0x8de7)";
        case 36328: return "GL_MAX_SUBROUTINE_UNIFORM_LOCATIONS(0x8de8)";
        case 36426: return "GL_NUM_COMPATIBLE_SUBROUTINES(0x8e4a)";
        case 36427: return "GL_COMPATIBLE_SUBROUTINES(0x8e4b)";
        case 14: return "GL_PATCHES(0xe)";
        case 36466: return "GL_PATCH_VERTICES(0x8e72)";
        case 36467: return "GL_PATCH_DEFAULT_INNER_LEVEL(0x8e73)";
        case 36468: return "GL_PATCH_DEFAULT_OUTER_LEVEL(0x8e74)";
        case 36469: return "GL_TESS_CONTROL_OUTPUT_VERTICES(0x8e75)";
        case 36470: return "GL_TESS_GEN_MODE(0x8e76)";
        case 36471: return "GL_TESS_GEN_SPACING(0x8e77)";
        case 36472: return "GL_TESS_GEN_VERTEX_ORDER(0x8e78)";
        case 36473: return "GL_TESS_GEN_POINT_MODE(0x8e79)";
        case 36474: return "GL_ISOLINES(0x8e7a)";
        case 36475: return "GL_FRACTIONAL_ODD(0x8e7b)";
        case 36476: return "GL_FRACTIONAL_EVEN(0x8e7c)";
        case 36477: return "GL_MAX_PATCH_VERTICES(0x8e7d)";
        case 36478: return "GL_MAX_TESS_GEN_LEVEL(0x8e7e)";
        case 36479: return "GL_MAX_TESS_CONTROL_UNIFORM_COMPONENTS(0x8e7f)";
        case 36480: return "GL_MAX_TESS_EVALUATION_UNIFORM_COMPONENTS(0x8e80)";
        case 36481: return "GL_MAX_TESS_CONTROL_TEXTURE_IMAGE_UNITS(0x8e81)";
        case 36482: return "GL_MAX_TESS_EVALUATION_TEXTURE_IMAGE_UNITS(0x8e82)";
        case 36483: return "GL_MAX_TESS_CONTROL_OUTPUT_COMPONENTS(0x8e83)";
        case 36484: return "GL_MAX_TESS_PATCH_COMPONENTS(0x8e84)";
        case 36485: return "GL_MAX_TESS_CONTROL_TOTAL_OUTPUT_COMPONENTS(0x8e85)";
        case 36486: return "GL_MAX_TESS_EVALUATION_OUTPUT_COMPONENTS(0x8e86)";
        case 36489: return "GL_MAX_TESS_CONTROL_UNIFORM_BLOCKS(0x8e89)";
        case 36490: return "GL_MAX_TESS_EVALUATION_UNIFORM_BLOCKS(0x8e8a)";
        case 34924: return "GL_MAX_TESS_CONTROL_INPUT_COMPONENTS(0x886c)";
        case 34925: return "GL_MAX_TESS_EVALUATION_INPUT_COMPONENTS(0x886d)";
        case 36382: return "GL_MAX_COMBINED_TESS_CONTROL_UNIFORM_COMPONENTS(0x8e1e)";
        case 36383: return "GL_MAX_COMBINED_TESS_EVALUATION_UNIFORM_COMPONENTS(0x8e1f)";
        case 34032: return "GL_UNIFORM_BLOCK_REFERENCED_BY_TESS_CONTROL_SHADER(0x84f0)";
        case 34033: return "GL_UNIFORM_BLOCK_REFERENCED_BY_TESS_EVALUATION_SHADER(0x84f1)";
        case 36487: return "GL_TESS_EVALUATION_SHADER(0x8e87)";
        case 36488: return "GL_TESS_CONTROL_SHADER(0x8e88)";
        case 36386: return "GL_TRANSFORM_FEEDBACK(0x8e22)";
        case 36387: return "GL_TRANSFORM_FEEDBACK_BUFFER_PAUSED(0x8e23)";
        case 36388: return "GL_TRANSFORM_FEEDBACK_BUFFER_ACTIVE(0x8e24)";
        case 36389: return "GL_TRANSFORM_FEEDBACK_BINDING(0x8e25)";
        case 36464: return "GL_MAX_TRANSFORM_FEEDBACK_BUFFERS(0x8e70)";
        case 5132: return "GL_FIXED(0x140c)";
        case 35738: return "GL_IMPLEMENTATION_COLOR_READ_TYPE(0x8b9a)";
        case 35739: return "GL_IMPLEMENTATION_COLOR_READ_FORMAT(0x8b9b)";
        case 36336: return "GL_LOW_FLOAT(0x8df0)";
        case 36337: return "GL_MEDIUM_FLOAT(0x8df1)";
        case 36338: return "GL_HIGH_FLOAT(0x8df2)";
        case 36339: return "GL_LOW_INT(0x8df3)";
        case 36340: return "GL_MEDIUM_INT(0x8df4)";
        case 36341: return "GL_HIGH_INT(0x8df5)";
        case 36346: return "GL_SHADER_COMPILER(0x8dfa)";
        case 36344: return "GL_SHADER_BINARY_FORMATS(0x8df8)";
        case 36345: return "GL_NUM_SHADER_BINARY_FORMATS(0x8df9)";
        case 36347: return "GL_MAX_VERTEX_UNIFORM_VECTORS(0x8dfb)";
        case 36348: return "GL_MAX_VARYING_VECTORS(0x8dfc)";
        case 36349: return "GL_MAX_FRAGMENT_UNIFORM_VECTORS(0x8dfd)";
        case 36194: return "GL_RGB565(0x8d62)";
        case 33367: return "GL_PROGRAM_BINARY_RETRIEVABLE_HINT(0x8257)";
        case 34625: return "GL_PROGRAM_BINARY_LENGTH(0x8741)";
        case 34814: return "GL_NUM_PROGRAM_BINARY_FORMATS(0x87fe)";
        case 34815: return "GL_PROGRAM_BINARY_FORMATS(0x87ff)";
        // case 1: return "GL_VERTEX_SHADER_BIT(0x1)";
        // case 2: return "GL_FRAGMENT_SHADER_BIT(0x2)";
        // case 4: return "GL_GEOMETRY_SHADER_BIT(0x4)";
        // case 8: return "GL_TESS_CONTROL_SHADER_BIT(0x8)";
        // case 16: return "GL_TESS_EVALUATION_SHADER_BIT(0x10)";
        case 33368: return "GL_PROGRAM_SEPARABLE(0x8258)";
        case 33369: return "GL_ACTIVE_PROGRAM(0x8259)";
        case 33370: return "GL_PROGRAM_PIPELINE_BINDING(0x825a)";
        case 33371: return "GL_MAX_VIEWPORTS(0x825b)";
        case 33372: return "GL_VIEWPORT_SUBPIXEL_BITS(0x825c)";
        case 33373: return "GL_VIEWPORT_BOUNDS_RANGE(0x825d)";
        case 33374: return "GL_LAYER_PROVOKING_VERTEX(0x825e)";
        case 33375: return "GL_VIEWPORT_INDEX_PROVOKING_VERTEX(0x825f)";
        case 33376: return "GL_UNDEFINED_VERTEX(0x8260)";
        // case 36662: return "GL_COPY_READ_BUFFER_BINDING(0x8f36)";
        // case 36663: return "GL_COPY_WRITE_BUFFER_BINDING(0x8f37)";
        // case 36388: return "GL_TRANSFORM_FEEDBACK_ACTIVE(0x8e24)";
        // case 36387: return "GL_TRANSFORM_FEEDBACK_PAUSED(0x8e23)";
        case 37159: return "GL_UNPACK_COMPRESSED_BLOCK_WIDTH(0x9127)";
        case 37160: return "GL_UNPACK_COMPRESSED_BLOCK_HEIGHT(0x9128)";
        case 37161: return "GL_UNPACK_COMPRESSED_BLOCK_DEPTH(0x9129)";
        case 37162: return "GL_UNPACK_COMPRESSED_BLOCK_SIZE(0x912a)";
        case 37163: return "GL_PACK_COMPRESSED_BLOCK_WIDTH(0x912b)";
        case 37164: return "GL_PACK_COMPRESSED_BLOCK_HEIGHT(0x912c)";
        case 37165: return "GL_PACK_COMPRESSED_BLOCK_DEPTH(0x912d)";
        case 37166: return "GL_PACK_COMPRESSED_BLOCK_SIZE(0x912e)";
        case 37760: return "GL_NUM_SAMPLE_COUNTS(0x9380)";
        case 37052: return "GL_MIN_MAP_BUFFER_ALIGNMENT(0x90bc)";
        case 37568: return "GL_ATOMIC_COUNTER_BUFFER(0x92c0)";
        case 37569: return "GL_ATOMIC_COUNTER_BUFFER_BINDING(0x92c1)";
        case 37570: return "GL_ATOMIC_COUNTER_BUFFER_START(0x92c2)";
        case 37571: return "GL_ATOMIC_COUNTER_BUFFER_SIZE(0x92c3)";
        case 37572: return "GL_ATOMIC_COUNTER_BUFFER_DATA_SIZE(0x92c4)";
        case 37573: return "GL_ATOMIC_COUNTER_BUFFER_ACTIVE_ATOMIC_COUNTERS(0x92c5)";
        case 37574: return "GL_ATOMIC_COUNTER_BUFFER_ACTIVE_ATOMIC_COUNTER_INDICES(0x92c6)";
        case 37575: return "GL_ATOMIC_COUNTER_BUFFER_REFERENCED_BY_VERTEX_SHADER(0x92c7)";
        case 37576: return "GL_ATOMIC_COUNTER_BUFFER_REFERENCED_BY_TESS_CONTROL_SHADER(0x92c8)";
        case 37577: return "GL_ATOMIC_COUNTER_BUFFER_REFERENCED_BY_TESS_EVALUATION_SHADER(0x92c9)";
        case 37578: return "GL_ATOMIC_COUNTER_BUFFER_REFERENCED_BY_GEOMETRY_SHADER(0x92ca)";
        case 37579: return "GL_ATOMIC_COUNTER_BUFFER_REFERENCED_BY_FRAGMENT_SHADER(0x92cb)";
        case 37580: return "GL_MAX_VERTEX_ATOMIC_COUNTER_BUFFERS(0x92cc)";
        case 37581: return "GL_MAX_TESS_CONTROL_ATOMIC_COUNTER_BUFFERS(0x92cd)";
        case 37582: return "GL_MAX_TESS_EVALUATION_ATOMIC_COUNTER_BUFFERS(0x92ce)";
        case 37583: return "GL_MAX_GEOMETRY_ATOMIC_COUNTER_BUFFERS(0x92cf)";
        case 37584: return "GL_MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS(0x92d0)";
        case 37585: return "GL_MAX_COMBINED_ATOMIC_COUNTER_BUFFERS(0x92d1)";
        case 37586: return "GL_MAX_VERTEX_ATOMIC_COUNTERS(0x92d2)";
        case 37587: return "GL_MAX_TESS_CONTROL_ATOMIC_COUNTERS(0x92d3)";
        case 37588: return "GL_MAX_TESS_EVALUATION_ATOMIC_COUNTERS(0x92d4)";
        case 37589: return "GL_MAX_GEOMETRY_ATOMIC_COUNTERS(0x92d5)";
        case 37590: return "GL_MAX_FRAGMENT_ATOMIC_COUNTERS(0x92d6)";
        case 37591: return "GL_MAX_COMBINED_ATOMIC_COUNTERS(0x92d7)";
        case 37592: return "GL_MAX_ATOMIC_COUNTER_BUFFER_SIZE(0x92d8)";
        case 37596: return "GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS(0x92dc)";
        case 37593: return "GL_ACTIVE_ATOMIC_COUNTER_BUFFERS(0x92d9)";
        case 37594: return "GL_UNIFORM_ATOMIC_COUNTER_BUFFER_INDEX(0x92da)";
        case 37595: return "GL_UNSIGNED_INT_ATOMIC_COUNTER(0x92db)";
        // case 1: return "GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT(0x1)";
        // case 2: return "GL_ELEMENT_ARRAY_BARRIER_BIT(0x2)";
        // case 4: return "GL_UNIFORM_BARRIER_BIT(0x4)";
        // case 8: return "GL_TEXTURE_FETCH_BARRIER_BIT(0x8)";
        // case 32: return "GL_SHADER_IMAGE_ACCESS_BARRIER_BIT(0x20)";
        case 64: return "GL_COMMAND_BARRIER_BIT(0x40)";
        case 128: return "GL_PIXEL_BUFFER_BARRIER_BIT(0x80)";
        // case 256: return "GL_TEXTURE_UPDATE_BARRIER_BIT(0x100)";
        // case 512: return "GL_BUFFER_UPDATE_BARRIER_BIT(0x200)";
        // case 1024: return "GL_FRAMEBUFFER_BARRIER_BIT(0x400)";
        case 2048: return "GL_TRANSFORM_FEEDBACK_BARRIER_BIT(0x800)";
        // case 4096: return "GL_ATOMIC_COUNTER_BARRIER_BIT(0x1000)";
        case 36664: return "GL_MAX_IMAGE_UNITS(0x8f38)";
        case 36665: return "GL_MAX_COMBINED_IMAGE_UNITS_AND_FRAGMENT_OUTPUTS(0x8f39)";
        case 36666: return "GL_IMAGE_BINDING_NAME(0x8f3a)";
        case 36667: return "GL_IMAGE_BINDING_LEVEL(0x8f3b)";
        case 36668: return "GL_IMAGE_BINDING_LAYERED(0x8f3c)";
        case 36669: return "GL_IMAGE_BINDING_LAYER(0x8f3d)";
        case 36670: return "GL_IMAGE_BINDING_ACCESS(0x8f3e)";
        case 36940: return "GL_IMAGE_1D(0x904c)";
        case 36941: return "GL_IMAGE_2D(0x904d)";
        case 36942: return "GL_IMAGE_3D(0x904e)";
        case 36943: return "GL_IMAGE_2D_RECT(0x904f)";
        case 36944: return "GL_IMAGE_CUBE(0x9050)";
        case 36945: return "GL_IMAGE_BUFFER(0x9051)";
        case 36946: return "GL_IMAGE_1D_ARRAY(0x9052)";
        case 36947: return "GL_IMAGE_2D_ARRAY(0x9053)";
        case 36948: return "GL_IMAGE_CUBE_MAP_ARRAY(0x9054)";
        case 36949: return "GL_IMAGE_2D_MULTISAMPLE(0x9055)";
        case 36950: return "GL_IMAGE_2D_MULTISAMPLE_ARRAY(0x9056)";
        case 36951: return "GL_INT_IMAGE_1D(0x9057)";
        case 36952: return "GL_INT_IMAGE_2D(0x9058)";
        case 36953: return "GL_INT_IMAGE_3D(0x9059)";
        case 36954: return "GL_INT_IMAGE_2D_RECT(0x905a)";
        case 36955: return "GL_INT_IMAGE_CUBE(0x905b)";
        case 36956: return "GL_INT_IMAGE_BUFFER(0x905c)";
        case 36957: return "GL_INT_IMAGE_1D_ARRAY(0x905d)";
        case 36958: return "GL_INT_IMAGE_2D_ARRAY(0x905e)";
        case 36959: return "GL_INT_IMAGE_CUBE_MAP_ARRAY(0x905f)";
        case 36960: return "GL_INT_IMAGE_2D_MULTISAMPLE(0x9060)";
        case 36961: return "GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY(0x9061)";
        case 36962: return "GL_UNSIGNED_INT_IMAGE_1D(0x9062)";
        case 36963: return "GL_UNSIGNED_INT_IMAGE_2D(0x9063)";
        case 36964: return "GL_UNSIGNED_INT_IMAGE_3D(0x9064)";
        case 36965: return "GL_UNSIGNED_INT_IMAGE_2D_RECT(0x9065)";
        case 36966: return "GL_UNSIGNED_INT_IMAGE_CUBE(0x9066)";
        case 36967: return "GL_UNSIGNED_INT_IMAGE_BUFFER(0x9067)";
        case 36968: return "GL_UNSIGNED_INT_IMAGE_1D_ARRAY(0x9068)";
        case 36969: return "GL_UNSIGNED_INT_IMAGE_2D_ARRAY(0x9069)";
        case 36970: return "GL_UNSIGNED_INT_IMAGE_CUBE_MAP_ARRAY(0x906a)";
        case 36971: return "GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE(0x906b)";
        case 36972: return "GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY(0x906c)";
        case 36973: return "GL_MAX_IMAGE_SAMPLES(0x906d)";
        case 36974: return "GL_IMAGE_BINDING_FORMAT(0x906e)";
        case 37063: return "GL_IMAGE_FORMAT_COMPATIBILITY_TYPE(0x90c7)";
        case 37064: return "GL_IMAGE_FORMAT_COMPATIBILITY_BY_SIZE(0x90c8)";
        case 37065: return "GL_IMAGE_FORMAT_COMPATIBILITY_BY_CLASS(0x90c9)";
        case 37066: return "GL_MAX_VERTEX_IMAGE_UNIFORMS(0x90ca)";
        case 37067: return "GL_MAX_TESS_CONTROL_IMAGE_UNIFORMS(0x90cb)";
        case 37068: return "GL_MAX_TESS_EVALUATION_IMAGE_UNIFORMS(0x90cc)";
        case 37069: return "GL_MAX_GEOMETRY_IMAGE_UNIFORMS(0x90cd)";
        case 37070: return "GL_MAX_FRAGMENT_IMAGE_UNIFORMS(0x90ce)";
        case 37071: return "GL_MAX_COMBINED_IMAGE_UNIFORMS(0x90cf)";
        case 36492: return "GL_COMPRESSED_RGBA_BPTC_UNORM(0x8e8c)";
        case 36493: return "GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM(0x8e8d)";
        case 36494: return "GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT(0x8e8e)";
        case 36495: return "GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT(0x8e8f)";
        case 37167: return "GL_TEXTURE_IMMUTABLE_FORMAT(0x912f)";
        case 33513: return "GL_NUM_SHADING_LANGUAGE_VERSIONS(0x82e9)";
        case 34638: return "GL_VERTEX_ATTRIB_ARRAY_LONG(0x874e)";
        case 37492: return "GL_COMPRESSED_RGB8_ETC2(0x9274)";
        case 37493: return "GL_COMPRESSED_SRGB8_ETC2(0x9275)";
        case 37494: return "GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2(0x9276)";
        case 37495: return "GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2(0x9277)";
        case 37496: return "GL_COMPRESSED_RGBA8_ETC2_EAC(0x9278)";
        case 37497: return "GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC(0x9279)";
        case 37488: return "GL_COMPRESSED_R11_EAC(0x9270)";
        case 37489: return "GL_COMPRESSED_SIGNED_R11_EAC(0x9271)";
        case 37490: return "GL_COMPRESSED_RG11_EAC(0x9272)";
        case 37491: return "GL_COMPRESSED_SIGNED_RG11_EAC(0x9273)";
        case 36201: return "GL_PRIMITIVE_RESTART_FIXED_INDEX(0x8d69)";
        case 36202: return "GL_ANY_SAMPLES_PASSED_CONSERVATIVE(0x8d6a)";
        case 36203: return "GL_MAX_ELEMENT_INDEX(0x8d6b)";
        case 37305: return "GL_COMPUTE_SHADER(0x91b9)";
        case 37307: return "GL_MAX_COMPUTE_UNIFORM_BLOCKS(0x91bb)";
        case 37308: return "GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS(0x91bc)";
        case 37309: return "GL_MAX_COMPUTE_IMAGE_UNIFORMS(0x91bd)";
        case 33378: return "GL_MAX_COMPUTE_SHARED_MEMORY_SIZE(0x8262)";
        case 33379: return "GL_MAX_COMPUTE_UNIFORM_COMPONENTS(0x8263)";
        case 33380: return "GL_MAX_COMPUTE_ATOMIC_COUNTER_BUFFERS(0x8264)";
        case 33381: return "GL_MAX_COMPUTE_ATOMIC_COUNTERS(0x8265)";
        case 33382: return "GL_MAX_COMBINED_COMPUTE_UNIFORM_COMPONENTS(0x8266)";
        case 37099: return "GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS(0x90eb)";
        case 37310: return "GL_MAX_COMPUTE_WORK_GROUP_COUNT(0x91be)";
        case 37311: return "GL_MAX_COMPUTE_WORK_GROUP_SIZE(0x91bf)";
        case 33383: return "GL_COMPUTE_WORK_GROUP_SIZE(0x8267)";
        case 37100: return "GL_UNIFORM_BLOCK_REFERENCED_BY_COMPUTE_SHADER(0x90ec)";
        case 37101: return "GL_ATOMIC_COUNTER_BUFFER_REFERENCED_BY_COMPUTE_SHADER(0x90ed)";
        case 37102: return "GL_DISPATCH_INDIRECT_BUFFER(0x90ee)";
        case 37103: return "GL_DISPATCH_INDIRECT_BUFFER_BINDING(0x90ef)";
        // case 32: return "GL_COMPUTE_SHADER_BIT(0x20)";
        case 33346: return "GL_DEBUG_OUTPUT_SYNCHRONOUS(0x8242)";
        case 33347: return "GL_DEBUG_NEXT_LOGGED_MESSAGE_LENGTH(0x8243)";
        case 33348: return "GL_DEBUG_CALLBACK_FUNCTION(0x8244)";
        case 33349: return "GL_DEBUG_CALLBACK_USER_PARAM(0x8245)";
        case 33350: return "GL_DEBUG_SOURCE_API(0x8246)";
        case 33351: return "GL_DEBUG_SOURCE_WINDOW_SYSTEM(0x8247)";
        case 33352: return "GL_DEBUG_SOURCE_SHADER_COMPILER(0x8248)";
        case 33353: return "GL_DEBUG_SOURCE_THIRD_PARTY(0x8249)";
        case 33354: return "GL_DEBUG_SOURCE_APPLICATION(0x824a)";
        case 33355: return "GL_DEBUG_SOURCE_OTHER(0x824b)";
        case 33356: return "GL_DEBUG_TYPE_ERROR(0x824c)";
        case 33357: return "GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR(0x824d)";
        case 33358: return "GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR(0x824e)";
        case 33359: return "GL_DEBUG_TYPE_PORTABILITY(0x824f)";
        case 33360: return "GL_DEBUG_TYPE_PERFORMANCE(0x8250)";
        case 33361: return "GL_DEBUG_TYPE_OTHER(0x8251)";
        case 37187: return "GL_MAX_DEBUG_MESSAGE_LENGTH(0x9143)";
        case 37188: return "GL_MAX_DEBUG_LOGGED_MESSAGES(0x9144)";
        case 37189: return "GL_DEBUG_LOGGED_MESSAGES(0x9145)";
        case 37190: return "GL_DEBUG_SEVERITY_HIGH(0x9146)";
        case 37191: return "GL_DEBUG_SEVERITY_MEDIUM(0x9147)";
        case 37192: return "GL_DEBUG_SEVERITY_LOW(0x9148)";
        case 33384: return "GL_DEBUG_TYPE_MARKER(0x8268)";
        case 33385: return "GL_DEBUG_TYPE_PUSH_GROUP(0x8269)";
        case 33386: return "GL_DEBUG_TYPE_POP_GROUP(0x826a)";
        case 33387: return "GL_DEBUG_SEVERITY_NOTIFICATION(0x826b)";
        case 33388: return "GL_MAX_DEBUG_GROUP_STACK_DEPTH(0x826c)";
        case 33389: return "GL_DEBUG_GROUP_STACK_DEPTH(0x826d)";
        case 33504: return "GL_BUFFER(0x82e0)";
        case 33505: return "GL_SHADER(0x82e1)";
        case 33506: return "GL_PROGRAM(0x82e2)";
        case 33507: return "GL_QUERY(0x82e3)";
        case 33508: return "GL_PROGRAM_PIPELINE(0x82e4)";
        case 33510: return "GL_SAMPLER(0x82e6)";
        case 33512: return "GL_MAX_LABEL_LENGTH(0x82e8)";
        case 37600: return "GL_DEBUG_OUTPUT(0x92e0)";
        // case 2: return "GL_CONTEXT_FLAG_DEBUG_BIT(0x2)";
        case 33390: return "GL_MAX_UNIFORM_LOCATIONS(0x826e)";
        case 37648: return "GL_FRAMEBUFFER_DEFAULT_WIDTH(0x9310)";
        case 37649: return "GL_FRAMEBUFFER_DEFAULT_HEIGHT(0x9311)";
        case 37650: return "GL_FRAMEBUFFER_DEFAULT_LAYERS(0x9312)";
        case 37651: return "GL_FRAMEBUFFER_DEFAULT_SAMPLES(0x9313)";
        case 37652: return "GL_FRAMEBUFFER_DEFAULT_FIXED_SAMPLE_LOCATIONS(0x9314)";
        case 37653: return "GL_MAX_FRAMEBUFFER_WIDTH(0x9315)";
        case 37654: return "GL_MAX_FRAMEBUFFER_HEIGHT(0x9316)";
        case 37655: return "GL_MAX_FRAMEBUFFER_LAYERS(0x9317)";
        case 37656: return "GL_MAX_FRAMEBUFFER_SAMPLES(0x9318)";
        case 33391: return "GL_INTERNALFORMAT_SUPPORTED(0x826f)";
        case 33392: return "GL_INTERNALFORMAT_PREFERRED(0x8270)";
        case 33393: return "GL_INTERNALFORMAT_RED_SIZE(0x8271)";
        case 33394: return "GL_INTERNALFORMAT_GREEN_SIZE(0x8272)";
        case 33395: return "GL_INTERNALFORMAT_BLUE_SIZE(0x8273)";
        case 33396: return "GL_INTERNALFORMAT_ALPHA_SIZE(0x8274)";
        case 33397: return "GL_INTERNALFORMAT_DEPTH_SIZE(0x8275)";
        case 33398: return "GL_INTERNALFORMAT_STENCIL_SIZE(0x8276)";
        case 33399: return "GL_INTERNALFORMAT_SHARED_SIZE(0x8277)";
        case 33400: return "GL_INTERNALFORMAT_RED_TYPE(0x8278)";
        case 33401: return "GL_INTERNALFORMAT_GREEN_TYPE(0x8279)";
        case 33402: return "GL_INTERNALFORMAT_BLUE_TYPE(0x827a)";
        case 33403: return "GL_INTERNALFORMAT_ALPHA_TYPE(0x827b)";
        case 33404: return "GL_INTERNALFORMAT_DEPTH_TYPE(0x827c)";
        case 33405: return "GL_INTERNALFORMAT_STENCIL_TYPE(0x827d)";
        case 33406: return "GL_MAX_WIDTH(0x827e)";
        case 33407: return "GL_MAX_HEIGHT(0x827f)";
        case 33408: return "GL_MAX_DEPTH(0x8280)";
        case 33409: return "GL_MAX_LAYERS(0x8281)";
        case 33410: return "GL_MAX_COMBINED_DIMENSIONS(0x8282)";
        case 33411: return "GL_COLOR_COMPONENTS(0x8283)";
        case 33412: return "GL_DEPTH_COMPONENTS(0x8284)";
        case 33413: return "GL_STENCIL_COMPONENTS(0x8285)";
        case 33414: return "GL_COLOR_RENDERABLE(0x8286)";
        case 33415: return "GL_DEPTH_RENDERABLE(0x8287)";
        case 33416: return "GL_STENCIL_RENDERABLE(0x8288)";
        case 33417: return "GL_FRAMEBUFFER_RENDERABLE(0x8289)";
        case 33418: return "GL_FRAMEBUFFER_RENDERABLE_LAYERED(0x828a)";
        case 33419: return "GL_FRAMEBUFFER_BLEND(0x828b)";
        case 33420: return "GL_READ_PIXELS(0x828c)";
        case 33421: return "GL_READ_PIXELS_FORMAT(0x828d)";
        case 33422: return "GL_READ_PIXELS_TYPE(0x828e)";
        case 33423: return "GL_TEXTURE_IMAGE_FORMAT(0x828f)";
        case 33424: return "GL_TEXTURE_IMAGE_TYPE(0x8290)";
        case 33425: return "GL_GET_TEXTURE_IMAGE_FORMAT(0x8291)";
        case 33426: return "GL_GET_TEXTURE_IMAGE_TYPE(0x8292)";
        case 33427: return "GL_MIPMAP(0x8293)";
        case 33428: return "GL_MANUAL_GENERATE_MIPMAP(0x8294)";
        case 33429: return "GL_AUTO_GENERATE_MIPMAP(0x8295)";
        case 33430: return "GL_COLOR_ENCODING(0x8296)";
        case 33431: return "GL_SRGB_READ(0x8297)";
        case 33432: return "GL_SRGB_WRITE(0x8298)";
        case 33434: return "GL_FILTER(0x829a)";
        case 33435: return "GL_VERTEX_TEXTURE(0x829b)";
        case 33436: return "GL_TESS_CONTROL_TEXTURE(0x829c)";
        case 33437: return "GL_TESS_EVALUATION_TEXTURE(0x829d)";
        case 33438: return "GL_GEOMETRY_TEXTURE(0x829e)";
        case 33439: return "GL_FRAGMENT_TEXTURE(0x829f)";
        case 33440: return "GL_COMPUTE_TEXTURE(0x82a0)";
        case 33441: return "GL_TEXTURE_SHADOW(0x82a1)";
        case 33442: return "GL_TEXTURE_GATHER(0x82a2)";
        case 33443: return "GL_TEXTURE_GATHER_SHADOW(0x82a3)";
        case 33444: return "GL_SHADER_IMAGE_LOAD(0x82a4)";
        case 33445: return "GL_SHADER_IMAGE_STORE(0x82a5)";
        case 33446: return "GL_SHADER_IMAGE_ATOMIC(0x82a6)";
        case 33447: return "GL_IMAGE_TEXEL_SIZE(0x82a7)";
        case 33448: return "GL_IMAGE_COMPATIBILITY_CLASS(0x82a8)";
        case 33449: return "GL_IMAGE_PIXEL_FORMAT(0x82a9)";
        case 33450: return "GL_IMAGE_PIXEL_TYPE(0x82aa)";
        case 33452: return "GL_SIMULTANEOUS_TEXTURE_AND_DEPTH_TEST(0x82ac)";
        case 33453: return "GL_SIMULTANEOUS_TEXTURE_AND_STENCIL_TEST(0x82ad)";
        case 33454: return "GL_SIMULTANEOUS_TEXTURE_AND_DEPTH_WRITE(0x82ae)";
        case 33455: return "GL_SIMULTANEOUS_TEXTURE_AND_STENCIL_WRITE(0x82af)";
        case 33457: return "GL_TEXTURE_COMPRESSED_BLOCK_WIDTH(0x82b1)";
        case 33458: return "GL_TEXTURE_COMPRESSED_BLOCK_HEIGHT(0x82b2)";
        case 33459: return "GL_TEXTURE_COMPRESSED_BLOCK_SIZE(0x82b3)";
        case 33460: return "GL_CLEAR_BUFFER(0x82b4)";
        case 33461: return "GL_TEXTURE_VIEW(0x82b5)";
        case 33462: return "GL_VIEW_COMPATIBILITY_CLASS(0x82b6)";
        case 33463: return "GL_FULL_SUPPORT(0x82b7)";
        case 33464: return "GL_CAVEAT_SUPPORT(0x82b8)";
        case 33465: return "GL_IMAGE_CLASS_4_X_32(0x82b9)";
        case 33466: return "GL_IMAGE_CLASS_2_X_32(0x82ba)";
        case 33467: return "GL_IMAGE_CLASS_1_X_32(0x82bb)";
        case 33468: return "GL_IMAGE_CLASS_4_X_16(0x82bc)";
        case 33469: return "GL_IMAGE_CLASS_2_X_16(0x82bd)";
        case 33470: return "GL_IMAGE_CLASS_1_X_16(0x82be)";
        case 33471: return "GL_IMAGE_CLASS_4_X_8(0x82bf)";
        case 33472: return "GL_IMAGE_CLASS_2_X_8(0x82c0)";
        case 33473: return "GL_IMAGE_CLASS_1_X_8(0x82c1)";
        case 33474: return "GL_IMAGE_CLASS_11_11_10(0x82c2)";
        case 33475: return "GL_IMAGE_CLASS_10_10_10_2(0x82c3)";
        case 33476: return "GL_VIEW_CLASS_128_BITS(0x82c4)";
        case 33477: return "GL_VIEW_CLASS_96_BITS(0x82c5)";
        case 33478: return "GL_VIEW_CLASS_64_BITS(0x82c6)";
        case 33479: return "GL_VIEW_CLASS_48_BITS(0x82c7)";
        case 33480: return "GL_VIEW_CLASS_32_BITS(0x82c8)";
        case 33481: return "GL_VIEW_CLASS_24_BITS(0x82c9)";
        case 33482: return "GL_VIEW_CLASS_16_BITS(0x82ca)";
        case 33483: return "GL_VIEW_CLASS_8_BITS(0x82cb)";
        case 33484: return "GL_VIEW_CLASS_S3TC_DXT1_RGB(0x82cc)";
        case 33485: return "GL_VIEW_CLASS_S3TC_DXT1_RGBA(0x82cd)";
        case 33486: return "GL_VIEW_CLASS_S3TC_DXT3_RGBA(0x82ce)";
        case 33487: return "GL_VIEW_CLASS_S3TC_DXT5_RGBA(0x82cf)";
        case 33488: return "GL_VIEW_CLASS_RGTC1_RED(0x82d0)";
        case 33489: return "GL_VIEW_CLASS_RGTC2_RG(0x82d1)";
        case 33490: return "GL_VIEW_CLASS_BPTC_UNORM(0x82d2)";
        case 33491: return "GL_VIEW_CLASS_BPTC_FLOAT(0x82d3)";
        case 37601: return "GL_UNIFORM(0x92e1)";
        case 37602: return "GL_UNIFORM_BLOCK(0x92e2)";
        case 37603: return "GL_PROGRAM_INPUT(0x92e3)";
        case 37604: return "GL_PROGRAM_OUTPUT(0x92e4)";
        case 37605: return "GL_BUFFER_VARIABLE(0x92e5)";
        case 37606: return "GL_SHADER_STORAGE_BLOCK(0x92e6)";
        case 37608: return "GL_VERTEX_SUBROUTINE(0x92e8)";
        case 37609: return "GL_TESS_CONTROL_SUBROUTINE(0x92e9)";
        case 37610: return "GL_TESS_EVALUATION_SUBROUTINE(0x92ea)";
        case 37611: return "GL_GEOMETRY_SUBROUTINE(0x92eb)";
        case 37612: return "GL_FRAGMENT_SUBROUTINE(0x92ec)";
        case 37613: return "GL_COMPUTE_SUBROUTINE(0x92ed)";
        case 37614: return "GL_VERTEX_SUBROUTINE_UNIFORM(0x92ee)";
        case 37615: return "GL_TESS_CONTROL_SUBROUTINE_UNIFORM(0x92ef)";
        case 37616: return "GL_TESS_EVALUATION_SUBROUTINE_UNIFORM(0x92f0)";
        case 37617: return "GL_GEOMETRY_SUBROUTINE_UNIFORM(0x92f1)";
        case 37618: return "GL_FRAGMENT_SUBROUTINE_UNIFORM(0x92f2)";
        case 37619: return "GL_COMPUTE_SUBROUTINE_UNIFORM(0x92f3)";
        case 37620: return "GL_TRANSFORM_FEEDBACK_VARYING(0x92f4)";
        case 37621: return "GL_ACTIVE_RESOURCES(0x92f5)";
        case 37622: return "GL_MAX_NAME_LENGTH(0x92f6)";
        case 37623: return "GL_MAX_NUM_ACTIVE_VARIABLES(0x92f7)";
        case 37624: return "GL_MAX_NUM_COMPATIBLE_SUBROUTINES(0x92f8)";
        case 37625: return "GL_NAME_LENGTH(0x92f9)";
        case 37626: return "GL_TYPE(0x92fa)";
        case 37627: return "GL_ARRAY_SIZE(0x92fb)";
        case 37628: return "GL_OFFSET(0x92fc)";
        case 37629: return "GL_BLOCK_INDEX(0x92fd)";
        case 37630: return "GL_ARRAY_STRIDE(0x92fe)";
        case 37631: return "GL_MATRIX_STRIDE(0x92ff)";
        case 37632: return "GL_IS_ROW_MAJOR(0x9300)";
        case 37633: return "GL_ATOMIC_COUNTER_BUFFER_INDEX(0x9301)";
        case 37634: return "GL_BUFFER_BINDING(0x9302)";
        case 37635: return "GL_BUFFER_DATA_SIZE(0x9303)";
        case 37636: return "GL_NUM_ACTIVE_VARIABLES(0x9304)";
        case 37637: return "GL_ACTIVE_VARIABLES(0x9305)";
        case 37638: return "GL_REFERENCED_BY_VERTEX_SHADER(0x9306)";
        case 37639: return "GL_REFERENCED_BY_TESS_CONTROL_SHADER(0x9307)";
        case 37640: return "GL_REFERENCED_BY_TESS_EVALUATION_SHADER(0x9308)";
        case 37641: return "GL_REFERENCED_BY_GEOMETRY_SHADER(0x9309)";
        case 37642: return "GL_REFERENCED_BY_FRAGMENT_SHADER(0x930a)";
        case 37643: return "GL_REFERENCED_BY_COMPUTE_SHADER(0x930b)";
        case 37644: return "GL_TOP_LEVEL_ARRAY_SIZE(0x930c)";
        case 37645: return "GL_TOP_LEVEL_ARRAY_STRIDE(0x930d)";
        case 37646: return "GL_LOCATION(0x930e)";
        case 37647: return "GL_LOCATION_INDEX(0x930f)";
        case 37607: return "GL_IS_PER_PATCH(0x92e7)";
        case 37074: return "GL_SHADER_STORAGE_BUFFER(0x90d2)";
        case 37075: return "GL_SHADER_STORAGE_BUFFER_BINDING(0x90d3)";
        case 37076: return "GL_SHADER_STORAGE_BUFFER_START(0x90d4)";
        case 37077: return "GL_SHADER_STORAGE_BUFFER_SIZE(0x90d5)";
        case 37078: return "GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS(0x90d6)";
        case 37079: return "GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS(0x90d7)";
        case 37080: return "GL_MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS(0x90d8)";
        case 37081: return "GL_MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS(0x90d9)";
        case 37082: return "GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS(0x90da)";
        case 37083: return "GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS(0x90db)";
        case 37084: return "GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS(0x90dc)";
        case 37085: return "GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS(0x90dd)";
        case 37086: return "GL_MAX_SHADER_STORAGE_BLOCK_SIZE(0x90de)";
        case 37087: return "GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT(0x90df)";
        case 8192: return "GL_SHADER_STORAGE_BARRIER_BIT(0x2000)";
        // case 36665: return "GL_MAX_COMBINED_SHADER_OUTPUT_RESOURCES(0x8f39)";
        case 37098: return "GL_DEPTH_STENCIL_TEXTURE_MODE(0x90ea)";
        case 37277: return "GL_TEXTURE_BUFFER_OFFSET(0x919d)";
        case 37278: return "GL_TEXTURE_BUFFER_SIZE(0x919e)";
        case 37279: return "GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT(0x919f)";
        case 33499: return "GL_TEXTURE_VIEW_MIN_LEVEL(0x82db)";
        case 33500: return "GL_TEXTURE_VIEW_NUM_LEVELS(0x82dc)";
        case 33501: return "GL_TEXTURE_VIEW_MIN_LAYER(0x82dd)";
        case 33502: return "GL_TEXTURE_VIEW_NUM_LAYERS(0x82de)";
        case 33503: return "GL_TEXTURE_IMMUTABLE_LEVELS(0x82df)";
        case 33492: return "GL_VERTEX_ATTRIB_BINDING(0x82d4)";
        case 33493: return "GL_VERTEX_ATTRIB_RELATIVE_OFFSET(0x82d5)";
        case 33494: return "GL_VERTEX_BINDING_DIVISOR(0x82d6)";
        case 33495: return "GL_VERTEX_BINDING_OFFSET(0x82d7)";
        case 33496: return "GL_VERTEX_BINDING_STRIDE(0x82d8)";
        case 33497: return "GL_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET(0x82d9)";
        case 33498: return "GL_MAX_VERTEX_ATTRIB_BINDINGS(0x82da)";
        case 36687: return "GL_VERTEX_BINDING_BUFFER(0x8f4f)";
        case 33509: return "GL_MAX_VERTEX_ATTRIB_STRIDE(0x82e5)";
        case 33313: return "GL_PRIMITIVE_RESTART_FOR_PATCHES_SUPPORTED(0x8221)";
        // case 35882: return "GL_TEXTURE_BUFFER_BINDING(0x8c2a)";
        // case 64: return "GL_MAP_PERSISTENT_BIT(0x40)";
        // case 128: return "GL_MAP_COHERENT_BIT(0x80)";
        // case 256: return "GL_DYNAMIC_STORAGE_BIT(0x100)";
        // case 512: return "GL_CLIENT_STORAGE_BIT(0x200)";
        // case 16384: return "GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT(0x4000)";
        case 33311: return "GL_BUFFER_IMMUTABLE_STORAGE(0x821f)";
        case 33312: return "GL_BUFFER_STORAGE_FLAGS(0x8220)";
        case 37733: return "GL_CLEAR_TEXTURE(0x9365)";
        case 37706: return "GL_LOCATION_COMPONENT(0x934a)";
        case 37707: return "GL_TRANSFORM_FEEDBACK_BUFFER_INDEX(0x934b)";
        case 37708: return "GL_TRANSFORM_FEEDBACK_BUFFER_STRIDE(0x934c)";
        case 37266: return "GL_QUERY_BUFFER(0x9192)";
        case 32768: return "GL_QUERY_BUFFER_BARRIER_BIT(0x8000)";
        case 37267: return "GL_QUERY_BUFFER_BINDING(0x9193)";
        case 37268: return "GL_QUERY_RESULT_NO_WAIT(0x9194)";
        case 34627: return "GL_MIRROR_CLAMP_TO_EDGE(0x8743)";
        case 1287: return "GL_CONTEXT_LOST(0x507)";
        case 37726: return "GL_NEGATIVE_ONE_TO_ONE(0x935e)";
        case 37727: return "GL_ZERO_TO_ONE(0x935f)";
        case 37724: return "GL_CLIP_ORIGIN(0x935c)";
        case 37725: return "GL_CLIP_DEPTH_MODE(0x935d)";
        case 36375: return "GL_QUERY_WAIT_INVERTED(0x8e17)";
        case 36376: return "GL_QUERY_NO_WAIT_INVERTED(0x8e18)";
        case 36377: return "GL_QUERY_BY_REGION_WAIT_INVERTED(0x8e19)";
        case 36378: return "GL_QUERY_BY_REGION_NO_WAIT_INVERTED(0x8e1a)";
        case 33529: return "GL_MAX_CULL_DISTANCES(0x82f9)";
        case 33530: return "GL_MAX_COMBINED_CLIP_AND_CULL_DISTANCES(0x82fa)";
        case 4102: return "GL_TEXTURE_TARGET(0x1006)";
        case 33514: return "GL_QUERY_TARGET(0x82ea)";
        case 33363: return "GL_GUILTY_CONTEXT_RESET(0x8253)";
        case 33364: return "GL_INNOCENT_CONTEXT_RESET(0x8254)";
        case 33365: return "GL_UNKNOWN_CONTEXT_RESET(0x8255)";
        case 33366: return "GL_RESET_NOTIFICATION_STRATEGY(0x8256)";
        case 33362: return "GL_LOSE_CONTEXT_ON_RESET(0x8252)";
        case 33377: return "GL_NO_RESET_NOTIFICATION(0x8261)";
        case 33531: return "GL_CONTEXT_RELEASE_BEHAVIOR(0x82fb)";
        case 33532: return "GL_CONTEXT_RELEASE_BEHAVIOR_FLUSH(0x82fc)";
        case 5135: return "GL_UNSIGNED_INT64_ARB(0x140f)";
        case 33344: return "GL_SYNC_CL_EVENT_ARB(0x8240)";
        case 33345: return "GL_SYNC_CL_EVENT_COMPLETE_ARB(0x8241)";
        case 37700: return "GL_MAX_COMPUTE_VARIABLE_GROUP_INVOCATIONS_ARB(0x9344)";
        // case 37099: return "GL_MAX_COMPUTE_FIXED_GROUP_INVOCATIONS_ARB(0x90eb)";
        case 37701: return "GL_MAX_COMPUTE_VARIABLE_GROUP_SIZE_ARB(0x9345)";
        // case 37311: return "GL_MAX_COMPUTE_FIXED_GROUP_SIZE_ARB(0x91bf)";
        // case 33346: return "GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB(0x8242)";
        // case 33347: return "GL_DEBUG_NEXT_LOGGED_MESSAGE_LENGTH_ARB(0x8243)";
        // case 33348: return "GL_DEBUG_CALLBACK_FUNCTION_ARB(0x8244)";
        // case 33349: return "GL_DEBUG_CALLBACK_USER_PARAM_ARB(0x8245)";
        // case 33350: return "GL_DEBUG_SOURCE_API_ARB(0x8246)";
        // case 33351: return "GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB(0x8247)";
        // case 33352: return "GL_DEBUG_SOURCE_SHADER_COMPILER_ARB(0x8248)";
        // case 33353: return "GL_DEBUG_SOURCE_THIRD_PARTY_ARB(0x8249)";
        // case 33354: return "GL_DEBUG_SOURCE_APPLICATION_ARB(0x824a)";
        // case 33355: return "GL_DEBUG_SOURCE_OTHER_ARB(0x824b)";
        // case 33356: return "GL_DEBUG_TYPE_ERROR_ARB(0x824c)";
        // case 33357: return "GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB(0x824d)";
        // case 33358: return "GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB(0x824e)";
        // case 33359: return "GL_DEBUG_TYPE_PORTABILITY_ARB(0x824f)";
        // case 33360: return "GL_DEBUG_TYPE_PERFORMANCE_ARB(0x8250)";
        // case 33361: return "GL_DEBUG_TYPE_OTHER_ARB(0x8251)";
        // case 37187: return "GL_MAX_DEBUG_MESSAGE_LENGTH_ARB(0x9143)";
        // case 37188: return "GL_MAX_DEBUG_LOGGED_MESSAGES_ARB(0x9144)";
        // case 37189: return "GL_DEBUG_LOGGED_MESSAGES_ARB(0x9145)";
        // case 37190: return "GL_DEBUG_SEVERITY_HIGH_ARB(0x9146)";
        // case 37191: return "GL_DEBUG_SEVERITY_MEDIUM_ARB(0x9147)";
        // case 37192: return "GL_DEBUG_SEVERITY_LOW_ARB(0x9148)";
        case 32773: return "GL_BLEND_COLOR(0x8005)";
        // case 32777: return "GL_BLEND_EQUATION(0x8009)";
        case 33006: return "GL_PARAMETER_BUFFER_ARB(0x80ee)";
        case 33007: return "GL_PARAMETER_BUFFER_BINDING_ARB(0x80ef)";
        case 33433: return "GL_SRGB_DECODE_ARB(0x8299)";
        case 33518: return "GL_VERTICES_SUBMITTED_ARB(0x82ee)";
        case 33519: return "GL_PRIMITIVES_SUBMITTED_ARB(0x82ef)";
        case 33520: return "GL_VERTEX_SHADER_INVOCATIONS_ARB(0x82f0)";
        case 33521: return "GL_TESS_CONTROL_SHADER_PATCHES_ARB(0x82f1)";
        case 33522: return "GL_TESS_EVALUATION_SHADER_INVOCATIONS_ARB(0x82f2)";
        case 33523: return "GL_GEOMETRY_SHADER_PRIMITIVES_EMITTED_ARB(0x82f3)";
        case 33524: return "GL_FRAGMENT_SHADER_INVOCATIONS_ARB(0x82f4)";
        case 33525: return "GL_COMPUTE_SHADER_INVOCATIONS_ARB(0x82f5)";
        case 33526: return "GL_CLIPPING_INPUT_PRIMITIVES_ARB(0x82f6)";
        case 33527: return "GL_CLIPPING_OUTPUT_PRIMITIVES_ARB(0x82f7)";
        // case 4: return "GL_CONTEXT_FLAG_ROBUST_ACCESS_BIT_ARB(0x4)";
        // case 33362: return "GL_LOSE_CONTEXT_ON_RESET_ARB(0x8252)";
        // case 33363: return "GL_GUILTY_CONTEXT_RESET_ARB(0x8253)";
        // case 33364: return "GL_INNOCENT_CONTEXT_RESET_ARB(0x8254)";
        // case 33365: return "GL_UNKNOWN_CONTEXT_RESET_ARB(0x8255)";
        // case 33366: return "GL_RESET_NOTIFICATION_STRATEGY_ARB(0x8256)";
        // case 33377: return "GL_NO_RESET_NOTIFICATION_ARB(0x8261)";
        // case 35894: return "GL_SAMPLE_SHADING_ARB(0x8c36)";
        // case 35895: return "GL_MIN_SAMPLE_SHADING_VALUE_ARB(0x8c37)";
        case 36270: return "GL_SHADER_INCLUDE_ARB(0x8dae)";
        case 36329: return "GL_NAMED_STRING_LENGTH_ARB(0x8de9)";
        case 36330: return "GL_NAMED_STRING_TYPE_ARB(0x8dea)";
        // case 1024: return "GL_SPARSE_STORAGE_BIT_ARB(0x400)";
        case 33528: return "GL_SPARSE_BUFFER_PAGE_SIZE_ARB(0x82f8)";
        case 37286: return "GL_TEXTURE_SPARSE_ARB(0x91a6)";
        case 37287: return "GL_VIRTUAL_PAGE_SIZE_INDEX_ARB(0x91a7)";
        case 37290: return "GL_NUM_SPARSE_LEVELS_ARB(0x91aa)";
        case 37288: return "GL_NUM_VIRTUAL_PAGE_SIZES_ARB(0x91a8)";
        case 37269: return "GL_VIRTUAL_PAGE_SIZE_X_ARB(0x9195)";
        case 37270: return "GL_VIRTUAL_PAGE_SIZE_Y_ARB(0x9196)";
        case 37271: return "GL_VIRTUAL_PAGE_SIZE_Z_ARB(0x9197)";
        case 37272: return "GL_MAX_SPARSE_TEXTURE_SIZE_ARB(0x9198)";
        case 37273: return "GL_MAX_SPARSE_3D_TEXTURE_SIZE_ARB(0x9199)";
        case 37274: return "GL_MAX_SPARSE_ARRAY_TEXTURE_LAYERS_ARB(0x919a)";
        case 37289: return "GL_SPARSE_TEXTURE_FULL_ARRAY_CUBE_MIPMAPS_ARB(0x91a9)";
        // case 36492: return "GL_COMPRESSED_RGBA_BPTC_UNORM_ARB(0x8e8c)";
        // case 36493: return "GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB(0x8e8d)";
        // case 36494: return "GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB(0x8e8e)";
        // case 36495: return "GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB(0x8e8f)";
        // case 36873: return "GL_TEXTURE_CUBE_MAP_ARRAY_ARB(0x9009)";
        // case 36874: return "GL_TEXTURE_BINDING_CUBE_MAP_ARRAY_ARB(0x900a)";
        // case 36875: return "GL_PROXY_TEXTURE_CUBE_MAP_ARRAY_ARB(0x900b)";
        // case 36876: return "GL_SAMPLER_CUBE_MAP_ARRAY_ARB(0x900c)";
        // case 36877: return "GL_SAMPLER_CUBE_MAP_ARRAY_SHADOW_ARB(0x900d)";
        // case 36878: return "GL_INT_SAMPLER_CUBE_MAP_ARRAY_ARB(0x900e)";
        // case 36879: return "GL_UNSIGNED_INT_SAMPLER_CUBE_MAP_ARRAY_ARB(0x900f)";
        // case 36446: return "GL_MIN_PROGRAM_TEXTURE_GATHER_OFFSET_ARB(0x8e5e)";
        // case 36447: return "GL_MAX_PROGRAM_TEXTURE_GATHER_OFFSET_ARB(0x8e5f)";
        case 36767: return "GL_MAX_PROGRAM_TEXTURE_GATHER_COMPONENTS_ARB(0x8f9f)";
        case 33516: return "GL_TRANSFORM_FEEDBACK_OVERFLOW_ARB(0x82ec)";
        case 33517: return "GL_TRANSFORM_FEEDBACK_STREAM_OVERFLOW_ARB(0x82ed)";
        // case 8: return "GL_CONTEXT_FLAG_NO_ERROR_BIT_KHR(0x8)";
        case 37107: return "GL_CONTEXT_ROBUST_ACCESS(0x90f3)";
        // case 256: return "GL_ACCUM(0x100)";
        case 257: return "GL_LOAD(0x101)";
        case 258: return "GL_RETURN(0x102)";
        case 259: return "GL_MULT(0x103)";
        case 260: return "GL_ADD(0x104)";
        // case 1: return "GL_CURRENT_BIT(0x1)";
        // case 2: return "GL_POINT_BIT(0x2)";
        // case 4: return "GL_LINE_BIT(0x4)";
        // case 8: return "GL_POLYGON_BIT(0x8)";
        // case 16: return "GL_POLYGON_STIPPLE_BIT(0x10)";
        // case 32: return "GL_PIXEL_MODE_BIT(0x20)";
        // case 64: return "GL_LIGHTING_BIT(0x40)";
        // case 128: return "GL_FOG_BIT(0x80)";
        // case 512: return "GL_ACCUM_BUFFER_BIT(0x200)";
        // case 2048: return "GL_VIEWPORT_BIT(0x800)";
        // case 4096: return "GL_TRANSFORM_BIT(0x1000)";
        // case 8192: return "GL_ENABLE_BIT(0x2000)";
        // case 32768: return "GL_HINT_BIT(0x8000)";
        case 65536: return "GL_EVAL_BIT(0x10000)";
        case 131072: return "GL_LIST_BIT(0x20000)";
        case 262144: return "GL_TEXTURE_BIT(0x40000)";
        case 524288: return "GL_SCISSOR_BIT(0x80000)";
        case 1048575: return "GL_ALL_ATTRIB_BITS(0xfffff)";
        // case 8: return "GL_QUAD_STRIP(0x8)";
        case 9: return "GL_POLYGON(0x9)";
        // case 12288: return "GL_CLIP_PLANE0(0x3000)";
        // case 12289: return "GL_CLIP_PLANE1(0x3001)";
        // case 12290: return "GL_CLIP_PLANE2(0x3002)";
        // case 12291: return "GL_CLIP_PLANE3(0x3003)";
        // case 12292: return "GL_CLIP_PLANE4(0x3004)";
        // case 12293: return "GL_CLIP_PLANE5(0x3005)";
        case 5127: return "GL_2_BYTES(0x1407)";
        case 5128: return "GL_3_BYTES(0x1408)";
        case 5129: return "GL_4_BYTES(0x1409)";
        case 1033: return "GL_AUX0(0x409)";
        case 1034: return "GL_AUX1(0x40a)";
        case 1035: return "GL_AUX2(0x40b)";
        case 1036: return "GL_AUX3(0x40c)";
        case 1536: return "GL_2D(0x600)";
        case 1537: return "GL_3D(0x601)";
        case 1538: return "GL_3D_COLOR(0x602)";
        case 1539: return "GL_3D_COLOR_TEXTURE(0x603)";
        case 1540: return "GL_4D_COLOR_TEXTURE(0x604)";
        case 1792: return "GL_PASS_THROUGH_TOKEN(0x700)";
        case 1793: return "GL_POINT_TOKEN(0x701)";
        case 1794: return "GL_LINE_TOKEN(0x702)";
        case 1795: return "GL_POLYGON_TOKEN(0x703)";
        case 1796: return "GL_BITMAP_TOKEN(0x704)";
        case 1797: return "GL_DRAW_PIXEL_TOKEN(0x705)";
        case 1798: return "GL_COPY_PIXEL_TOKEN(0x706)";
        case 1799: return "GL_LINE_RESET_TOKEN(0x707)";
        // case 2048: return "GL_EXP(0x800)";
        case 2049: return "GL_EXP2(0x801)";
        case 2560: return "GL_COEFF(0xa00)";
        case 2561: return "GL_ORDER(0xa01)";
        case 2562: return "GL_DOMAIN(0xa02)";
        case 2816: return "GL_CURRENT_COLOR(0xb00)";
        case 2817: return "GL_CURRENT_INDEX(0xb01)";
        case 2818: return "GL_CURRENT_NORMAL(0xb02)";
        case 2819: return "GL_CURRENT_TEXTURE_COORDS(0xb03)";
        case 2820: return "GL_CURRENT_RASTER_COLOR(0xb04)";
        case 2821: return "GL_CURRENT_RASTER_INDEX(0xb05)";
        case 2822: return "GL_CURRENT_RASTER_TEXTURE_COORDS(0xb06)";
        case 2823: return "GL_CURRENT_RASTER_POSITION(0xb07)";
        case 2824: return "GL_CURRENT_RASTER_POSITION_VALID(0xb08)";
        case 2825: return "GL_CURRENT_RASTER_DISTANCE(0xb09)";
        case 2832: return "GL_POINT_SMOOTH(0xb10)";
        case 2852: return "GL_LINE_STIPPLE(0xb24)";
        case 2853: return "GL_LINE_STIPPLE_PATTERN(0xb25)";
        case 2854: return "GL_LINE_STIPPLE_REPEAT(0xb26)";
        case 2864: return "GL_LIST_MODE(0xb30)";
        case 2865: return "GL_MAX_LIST_NESTING(0xb31)";
        case 2866: return "GL_LIST_BASE(0xb32)";
        case 2867: return "GL_LIST_INDEX(0xb33)";
        case 2882: return "GL_POLYGON_STIPPLE(0xb42)";
        case 2883: return "GL_EDGE_FLAG(0xb43)";
        case 2896: return "GL_LIGHTING(0xb50)";
        case 2897: return "GL_LIGHT_MODEL_LOCAL_VIEWER(0xb51)";
        case 2898: return "GL_LIGHT_MODEL_TWO_SIDE(0xb52)";
        case 2899: return "GL_LIGHT_MODEL_AMBIENT(0xb53)";
        case 2900: return "GL_SHADE_MODEL(0xb54)";
        case 2901: return "GL_COLOR_MATERIAL_FACE(0xb55)";
        case 2902: return "GL_COLOR_MATERIAL_PARAMETER(0xb56)";
        case 2903: return "GL_COLOR_MATERIAL(0xb57)";
        case 2912: return "GL_FOG(0xb60)";
        case 2913: return "GL_FOG_INDEX(0xb61)";
        case 2914: return "GL_FOG_DENSITY(0xb62)";
        case 2915: return "GL_FOG_START(0xb63)";
        case 2916: return "GL_FOG_END(0xb64)";
        case 2917: return "GL_FOG_MODE(0xb65)";
        case 2918: return "GL_FOG_COLOR(0xb66)";
        case 2944: return "GL_ACCUM_CLEAR_VALUE(0xb80)";
        case 2976: return "GL_MATRIX_MODE(0xba0)";
        case 2977: return "GL_NORMALIZE(0xba1)";
        case 2979: return "GL_MODELVIEW_STACK_DEPTH(0xba3)";
        case 2980: return "GL_PROJECTION_STACK_DEPTH(0xba4)";
        case 2981: return "GL_TEXTURE_STACK_DEPTH(0xba5)";
        case 2982: return "GL_MODELVIEW_MATRIX(0xba6)";
        case 2983: return "GL_PROJECTION_MATRIX(0xba7)";
        case 2984: return "GL_TEXTURE_MATRIX(0xba8)";
        case 2992: return "GL_ATTRIB_STACK_DEPTH(0xbb0)";
        case 2993: return "GL_CLIENT_ATTRIB_STACK_DEPTH(0xbb1)";
        case 3008: return "GL_ALPHA_TEST(0xbc0)";
        case 3009: return "GL_ALPHA_TEST_FUNC(0xbc1)";
        case 3010: return "GL_ALPHA_TEST_REF(0xbc2)";
        case 3057: return "GL_INDEX_LOGIC_OP(0xbf1)";
        case 3072: return "GL_AUX_BUFFERS(0xc00)";
        case 3104: return "GL_INDEX_CLEAR_VALUE(0xc20)";
        case 3105: return "GL_INDEX_WRITEMASK(0xc21)";
        case 3120: return "GL_INDEX_MODE(0xc30)";
        case 3121: return "GL_RGBA_MODE(0xc31)";
        case 3136: return "GL_RENDER_MODE(0xc40)";
        case 3152: return "GL_PERSPECTIVE_CORRECTION_HINT(0xc50)";
        case 3153: return "GL_POINT_SMOOTH_HINT(0xc51)";
        case 3156: return "GL_FOG_HINT(0xc54)";
        case 3168: return "GL_TEXTURE_GEN_S(0xc60)";
        case 3169: return "GL_TEXTURE_GEN_T(0xc61)";
        case 3170: return "GL_TEXTURE_GEN_R(0xc62)";
        case 3171: return "GL_TEXTURE_GEN_Q(0xc63)";
        case 3184: return "GL_PIXEL_MAP_I_TO_I(0xc70)";
        case 3185: return "GL_PIXEL_MAP_S_TO_S(0xc71)";
        case 3186: return "GL_PIXEL_MAP_I_TO_R(0xc72)";
        case 3187: return "GL_PIXEL_MAP_I_TO_G(0xc73)";
        case 3188: return "GL_PIXEL_MAP_I_TO_B(0xc74)";
        case 3189: return "GL_PIXEL_MAP_I_TO_A(0xc75)";
        case 3190: return "GL_PIXEL_MAP_R_TO_R(0xc76)";
        case 3191: return "GL_PIXEL_MAP_G_TO_G(0xc77)";
        case 3192: return "GL_PIXEL_MAP_B_TO_B(0xc78)";
        case 3193: return "GL_PIXEL_MAP_A_TO_A(0xc79)";
        case 3248: return "GL_PIXEL_MAP_I_TO_I_SIZE(0xcb0)";
        case 3249: return "GL_PIXEL_MAP_S_TO_S_SIZE(0xcb1)";
        case 3250: return "GL_PIXEL_MAP_I_TO_R_SIZE(0xcb2)";
        case 3251: return "GL_PIXEL_MAP_I_TO_G_SIZE(0xcb3)";
        case 3252: return "GL_PIXEL_MAP_I_TO_B_SIZE(0xcb4)";
        case 3253: return "GL_PIXEL_MAP_I_TO_A_SIZE(0xcb5)";
        case 3254: return "GL_PIXEL_MAP_R_TO_R_SIZE(0xcb6)";
        case 3255: return "GL_PIXEL_MAP_G_TO_G_SIZE(0xcb7)";
        case 3256: return "GL_PIXEL_MAP_B_TO_B_SIZE(0xcb8)";
        case 3257: return "GL_PIXEL_MAP_A_TO_A_SIZE(0xcb9)";
        case 3344: return "GL_MAP_COLOR(0xd10)";
        case 3345: return "GL_MAP_STENCIL(0xd11)";
        case 3346: return "GL_INDEX_SHIFT(0xd12)";
        case 3347: return "GL_INDEX_OFFSET(0xd13)";
        case 3348: return "GL_RED_SCALE(0xd14)";
        case 3349: return "GL_RED_BIAS(0xd15)";
        case 3350: return "GL_ZOOM_X(0xd16)";
        case 3351: return "GL_ZOOM_Y(0xd17)";
        case 3352: return "GL_GREEN_SCALE(0xd18)";
        case 3353: return "GL_GREEN_BIAS(0xd19)";
        case 3354: return "GL_BLUE_SCALE(0xd1a)";
        case 3355: return "GL_BLUE_BIAS(0xd1b)";
        case 3356: return "GL_ALPHA_SCALE(0xd1c)";
        case 3357: return "GL_ALPHA_BIAS(0xd1d)";
        case 3358: return "GL_DEPTH_SCALE(0xd1e)";
        case 3359: return "GL_DEPTH_BIAS(0xd1f)";
        case 3376: return "GL_MAX_EVAL_ORDER(0xd30)";
        case 3377: return "GL_MAX_LIGHTS(0xd31)";
        // case 3378: return "GL_MAX_CLIP_PLANES(0xd32)";
        case 3380: return "GL_MAX_PIXEL_MAP_TABLE(0xd34)";
        case 3381: return "GL_MAX_ATTRIB_STACK_DEPTH(0xd35)";
        case 3382: return "GL_MAX_MODELVIEW_STACK_DEPTH(0xd36)";
        case 3383: return "GL_MAX_NAME_STACK_DEPTH(0xd37)";
        case 3384: return "GL_MAX_PROJECTION_STACK_DEPTH(0xd38)";
        case 3385: return "GL_MAX_TEXTURE_STACK_DEPTH(0xd39)";
        case 3387: return "GL_MAX_CLIENT_ATTRIB_STACK_DEPTH(0xd3b)";
        case 3409: return "GL_INDEX_BITS(0xd51)";
        case 3410: return "GL_RED_BITS(0xd52)";
        case 3411: return "GL_GREEN_BITS(0xd53)";
        case 3412: return "GL_BLUE_BITS(0xd54)";
        case 3413: return "GL_ALPHA_BITS(0xd55)";
        case 3414: return "GL_DEPTH_BITS(0xd56)";
        case 3415: return "GL_STENCIL_BITS(0xd57)";
        case 3416: return "GL_ACCUM_RED_BITS(0xd58)";
        case 3417: return "GL_ACCUM_GREEN_BITS(0xd59)";
        case 3418: return "GL_ACCUM_BLUE_BITS(0xd5a)";
        case 3419: return "GL_ACCUM_ALPHA_BITS(0xd5b)";
        case 3440: return "GL_NAME_STACK_DEPTH(0xd70)";
        case 3456: return "GL_AUTO_NORMAL(0xd80)";
        case 3472: return "GL_MAP1_COLOR_4(0xd90)";
        case 3473: return "GL_MAP1_INDEX(0xd91)";
        case 3474: return "GL_MAP1_NORMAL(0xd92)";
        case 3475: return "GL_MAP1_TEXTURE_COORD_1(0xd93)";
        case 3476: return "GL_MAP1_TEXTURE_COORD_2(0xd94)";
        case 3477: return "GL_MAP1_TEXTURE_COORD_3(0xd95)";
        case 3478: return "GL_MAP1_TEXTURE_COORD_4(0xd96)";
        case 3479: return "GL_MAP1_VERTEX_3(0xd97)";
        case 3480: return "GL_MAP1_VERTEX_4(0xd98)";
        case 3504: return "GL_MAP2_COLOR_4(0xdb0)";
        case 3505: return "GL_MAP2_INDEX(0xdb1)";
        case 3506: return "GL_MAP2_NORMAL(0xdb2)";
        case 3507: return "GL_MAP2_TEXTURE_COORD_1(0xdb3)";
        case 3508: return "GL_MAP2_TEXTURE_COORD_2(0xdb4)";
        case 3509: return "GL_MAP2_TEXTURE_COORD_3(0xdb5)";
        case 3510: return "GL_MAP2_TEXTURE_COORD_4(0xdb6)";
        case 3511: return "GL_MAP2_VERTEX_3(0xdb7)";
        case 3512: return "GL_MAP2_VERTEX_4(0xdb8)";
        case 3536: return "GL_MAP1_GRID_DOMAIN(0xdd0)";
        case 3537: return "GL_MAP1_GRID_SEGMENTS(0xdd1)";
        case 3538: return "GL_MAP2_GRID_DOMAIN(0xdd2)";
        case 3539: return "GL_MAP2_GRID_SEGMENTS(0xdd3)";
        case 3568: return "GL_FEEDBACK_BUFFER_POINTER(0xdf0)";
        case 3569: return "GL_FEEDBACK_BUFFER_SIZE(0xdf1)";
        case 3570: return "GL_FEEDBACK_BUFFER_TYPE(0xdf2)";
        case 3571: return "GL_SELECTION_BUFFER_POINTER(0xdf3)";
        case 3572: return "GL_SELECTION_BUFFER_SIZE(0xdf4)";
        case 4101: return "GL_TEXTURE_BORDER(0x1005)";
        // case 16384: return "GL_LIGHT0(0x4000)";
        case 16385: return "GL_LIGHT1(0x4001)";
        case 16386: return "GL_LIGHT2(0x4002)";
        case 16387: return "GL_LIGHT3(0x4003)";
        case 16388: return "GL_LIGHT4(0x4004)";
        case 16389: return "GL_LIGHT5(0x4005)";
        case 16390: return "GL_LIGHT6(0x4006)";
        case 16391: return "GL_LIGHT7(0x4007)";
        case 4608: return "GL_AMBIENT(0x1200)";
        case 4609: return "GL_DIFFUSE(0x1201)";
        case 4610: return "GL_SPECULAR(0x1202)";
        case 4611: return "GL_POSITION(0x1203)";
        case 4612: return "GL_SPOT_DIRECTION(0x1204)";
        case 4613: return "GL_SPOT_EXPONENT(0x1205)";
        case 4614: return "GL_SPOT_CUTOFF(0x1206)";
        case 4615: return "GL_CONSTANT_ATTENUATION(0x1207)";
        case 4616: return "GL_LINEAR_ATTENUATION(0x1208)";
        case 4617: return "GL_QUADRATIC_ATTENUATION(0x1209)";
        case 4864: return "GL_COMPILE(0x1300)";
        case 4865: return "GL_COMPILE_AND_EXECUTE(0x1301)";
        case 5632: return "GL_EMISSION(0x1600)";
        case 5633: return "GL_SHININESS(0x1601)";
        case 5634: return "GL_AMBIENT_AND_DIFFUSE(0x1602)";
        case 5635: return "GL_COLOR_INDEXES(0x1603)";
        case 5888: return "GL_MODELVIEW(0x1700)";
        case 5889: return "GL_PROJECTION(0x1701)";
        case 6400: return "GL_COLOR_INDEX(0x1900)";
        case 6409: return "GL_LUMINANCE(0x1909)";
        case 6410: return "GL_LUMINANCE_ALPHA(0x190a)";
        case 6656: return "GL_BITMAP(0x1a00)";
        case 7168: return "GL_RENDER(0x1c00)";
        case 7169: return "GL_FEEDBACK(0x1c01)";
        case 7170: return "GL_SELECT(0x1c02)";
        case 7424: return "GL_FLAT(0x1d00)";
        case 7425: return "GL_SMOOTH(0x1d01)";
        // case 8192: return "GL_S(0x2000)";
        case 8193: return "GL_T(0x2001)";
        case 8194: return "GL_R(0x2002)";
        case 8195: return "GL_Q(0x2003)";
        case 8448: return "GL_MODULATE(0x2100)";
        case 8449: return "GL_DECAL(0x2101)";
        case 8704: return "GL_TEXTURE_ENV_MODE(0x2200)";
        case 8705: return "GL_TEXTURE_ENV_COLOR(0x2201)";
        case 8960: return "GL_TEXTURE_ENV(0x2300)";
        case 9216: return "GL_EYE_LINEAR(0x2400)";
        case 9217: return "GL_OBJECT_LINEAR(0x2401)";
        case 9218: return "GL_SPHERE_MAP(0x2402)";
        case 9472: return "GL_TEXTURE_GEN_MODE(0x2500)";
        case 9473: return "GL_OBJECT_PLANE(0x2501)";
        case 9474: return "GL_EYE_PLANE(0x2502)";
        case 10496: return "GL_CLAMP(0x2900)";
        // case 1: return "GL_CLIENT_PIXEL_STORE_BIT(0x1)";
        // case 2: return "GL_CLIENT_VERTEX_ARRAY_BIT(0x2)";
        case 32827: return "GL_ALPHA4(0x803b)";
        case 32828: return "GL_ALPHA8(0x803c)";
        case 32829: return "GL_ALPHA12(0x803d)";
        case 32830: return "GL_ALPHA16(0x803e)";
        case 32831: return "GL_LUMINANCE4(0x803f)";
        case 32832: return "GL_LUMINANCE8(0x8040)";
        case 32833: return "GL_LUMINANCE12(0x8041)";
        case 32834: return "GL_LUMINANCE16(0x8042)";
        case 32835: return "GL_LUMINANCE4_ALPHA4(0x8043)";
        case 32836: return "GL_LUMINANCE6_ALPHA2(0x8044)";
        case 32837: return "GL_LUMINANCE8_ALPHA8(0x8045)";
        case 32838: return "GL_LUMINANCE12_ALPHA4(0x8046)";
        case 32839: return "GL_LUMINANCE12_ALPHA12(0x8047)";
        case 32840: return "GL_LUMINANCE16_ALPHA16(0x8048)";
        case 32841: return "GL_INTENSITY(0x8049)";
        case 32842: return "GL_INTENSITY4(0x804a)";
        case 32843: return "GL_INTENSITY8(0x804b)";
        case 32844: return "GL_INTENSITY12(0x804c)";
        case 32845: return "GL_INTENSITY16(0x804d)";
        case 32864: return "GL_TEXTURE_LUMINANCE_SIZE(0x8060)";
        case 32865: return "GL_TEXTURE_INTENSITY_SIZE(0x8061)";
        case 32870: return "GL_TEXTURE_PRIORITY(0x8066)";
        case 32871: return "GL_TEXTURE_RESIDENT(0x8067)";
        case 32885: return "GL_NORMAL_ARRAY(0x8075)";
        case 32886: return "GL_COLOR_ARRAY(0x8076)";
        case 32887: return "GL_INDEX_ARRAY(0x8077)";
        case 32888: return "GL_TEXTURE_COORD_ARRAY(0x8078)";
        case 32889: return "GL_EDGE_FLAG_ARRAY(0x8079)";
        case 32890: return "GL_VERTEX_ARRAY_SIZE(0x807a)";
        case 32891: return "GL_VERTEX_ARRAY_TYPE(0x807b)";
        case 32892: return "GL_VERTEX_ARRAY_STRIDE(0x807c)";
        case 32894: return "GL_NORMAL_ARRAY_TYPE(0x807e)";
        case 32895: return "GL_NORMAL_ARRAY_STRIDE(0x807f)";
        case 32897: return "GL_COLOR_ARRAY_SIZE(0x8081)";
        case 32898: return "GL_COLOR_ARRAY_TYPE(0x8082)";
        case 32899: return "GL_COLOR_ARRAY_STRIDE(0x8083)";
        case 32901: return "GL_INDEX_ARRAY_TYPE(0x8085)";
        case 32902: return "GL_INDEX_ARRAY_STRIDE(0x8086)";
        case 32904: return "GL_TEXTURE_COORD_ARRAY_SIZE(0x8088)";
        case 32905: return "GL_TEXTURE_COORD_ARRAY_TYPE(0x8089)";
        case 32906: return "GL_TEXTURE_COORD_ARRAY_STRIDE(0x808a)";
        case 32908: return "GL_EDGE_FLAG_ARRAY_STRIDE(0x808c)";
        case 32910: return "GL_VERTEX_ARRAY_POINTER(0x808e)";
        case 32911: return "GL_NORMAL_ARRAY_POINTER(0x808f)";
        case 32912: return "GL_COLOR_ARRAY_POINTER(0x8090)";
        case 32913: return "GL_INDEX_ARRAY_POINTER(0x8091)";
        case 32914: return "GL_TEXTURE_COORD_ARRAY_POINTER(0x8092)";
        case 32915: return "GL_EDGE_FLAG_ARRAY_POINTER(0x8093)";
        case 10784: return "GL_V2F(0x2a20)";
        case 10785: return "GL_V3F(0x2a21)";
        case 10786: return "GL_C4UB_V2F(0x2a22)";
        case 10787: return "GL_C4UB_V3F(0x2a23)";
        case 10788: return "GL_C3F_V3F(0x2a24)";
        case 10789: return "GL_N3F_V3F(0x2a25)";
        case 10790: return "GL_C4F_N3F_V3F(0x2a26)";
        case 10791: return "GL_T2F_V3F(0x2a27)";
        case 10792: return "GL_T4F_V4F(0x2a28)";
        case 10793: return "GL_T2F_C4UB_V3F(0x2a29)";
        case 10794: return "GL_T2F_C3F_V3F(0x2a2a)";
        case 10795: return "GL_T2F_N3F_V3F(0x2a2b)";
        case 10796: return "GL_T2F_C4F_N3F_V3F(0x2a2c)";
        case 10797: return "GL_T4F_C4F_N3F_V4F(0x2a2d)";
        // case 32884: return "GL_VERTEX_ARRAY_EXT(0x8074)";
        // case 32885: return "GL_NORMAL_ARRAY_EXT(0x8075)";
        // case 32886: return "GL_COLOR_ARRAY_EXT(0x8076)";
        // case 32887: return "GL_INDEX_ARRAY_EXT(0x8077)";
        // case 32888: return "GL_TEXTURE_COORD_ARRAY_EXT(0x8078)";
        // case 32889: return "GL_EDGE_FLAG_ARRAY_EXT(0x8079)";
        // case 32890: return "GL_VERTEX_ARRAY_SIZE_EXT(0x807a)";
        // case 32891: return "GL_VERTEX_ARRAY_TYPE_EXT(0x807b)";
        // case 32892: return "GL_VERTEX_ARRAY_STRIDE_EXT(0x807c)";
        case 32893: return "GL_VERTEX_ARRAY_COUNT_EXT(0x807d)";
        // case 32894: return "GL_NORMAL_ARRAY_TYPE_EXT(0x807e)";
        // case 32895: return "GL_NORMAL_ARRAY_STRIDE_EXT(0x807f)";
        case 32896: return "GL_NORMAL_ARRAY_COUNT_EXT(0x8080)";
        // case 32897: return "GL_COLOR_ARRAY_SIZE_EXT(0x8081)";
        // case 32898: return "GL_COLOR_ARRAY_TYPE_EXT(0x8082)";
        // case 32899: return "GL_COLOR_ARRAY_STRIDE_EXT(0x8083)";
        case 32900: return "GL_COLOR_ARRAY_COUNT_EXT(0x8084)";
        // case 32901: return "GL_INDEX_ARRAY_TYPE_EXT(0x8085)";
        // case 32902: return "GL_INDEX_ARRAY_STRIDE_EXT(0x8086)";
        case 32903: return "GL_INDEX_ARRAY_COUNT_EXT(0x8087)";
        // case 32904: return "GL_TEXTURE_COORD_ARRAY_SIZE_EXT(0x8088)";
        // case 32905: return "GL_TEXTURE_COORD_ARRAY_TYPE_EXT(0x8089)";
        // case 32906: return "GL_TEXTURE_COORD_ARRAY_STRIDE_EXT(0x808a)";
        case 32907: return "GL_TEXTURE_COORD_ARRAY_COUNT_EXT(0x808b)";
        // case 32908: return "GL_EDGE_FLAG_ARRAY_STRIDE_EXT(0x808c)";
        case 32909: return "GL_EDGE_FLAG_ARRAY_COUNT_EXT(0x808d)";
        // case 32910: return "GL_VERTEX_ARRAY_POINTER_EXT(0x808e)";
        // case 32911: return "GL_NORMAL_ARRAY_POINTER_EXT(0x808f)";
        // case 32912: return "GL_COLOR_ARRAY_POINTER_EXT(0x8090)";
        // case 32913: return "GL_INDEX_ARRAY_POINTER_EXT(0x8091)";
        // case 32914: return "GL_TEXTURE_COORD_ARRAY_POINTER_EXT(0x8092)";
        // case 32915: return "GL_EDGE_FLAG_ARRAY_POINTER_EXT(0x8093)";
        // case 32992: return "GL_BGR_EXT(0x80e0)";
        // case 32993: return "GL_BGRA_EXT(0x80e1)";
        case 32984: return "GL_COLOR_TABLE_FORMAT_EXT(0x80d8)";
        case 32985: return "GL_COLOR_TABLE_WIDTH_EXT(0x80d9)";
        case 32986: return "GL_COLOR_TABLE_RED_SIZE_EXT(0x80da)";
        case 32987: return "GL_COLOR_TABLE_GREEN_SIZE_EXT(0x80db)";
        case 32988: return "GL_COLOR_TABLE_BLUE_SIZE_EXT(0x80dc)";
        case 32989: return "GL_COLOR_TABLE_ALPHA_SIZE_EXT(0x80dd)";
        case 32990: return "GL_COLOR_TABLE_LUMINANCE_SIZE_EXT(0x80de)";
        case 32991: return "GL_COLOR_TABLE_INTENSITY_SIZE_EXT(0x80df)";
        case 32994: return "GL_COLOR_INDEX1_EXT(0x80e2)";
        case 32995: return "GL_COLOR_INDEX2_EXT(0x80e3)";
        case 32996: return "GL_COLOR_INDEX4_EXT(0x80e4)";
        case 32997: return "GL_COLOR_INDEX8_EXT(0x80e5)";
        case 32998: return "GL_COLOR_INDEX12_EXT(0x80e6)";
        case 32999: return "GL_COLOR_INDEX16_EXT(0x80e7)";
        // case 33000: return "GL_MAX_ELEMENTS_VERTICES_WIN(0x80e8)";
        // case 33001: return "GL_MAX_ELEMENTS_INDICES_WIN(0x80e9)";
        case 33002: return "GL_PHONG_WIN(0x80ea)";
        case 33003: return "GL_PHONG_HINT_WIN(0x80eb)";
        case 33004: return "GL_FOG_SPECULAR_TEXTURE_WIN(0x80ec)";
    #endif
        default: return fallback;
    } // clang-format on
}

} // namespace snap::rhi::backend::opengl
