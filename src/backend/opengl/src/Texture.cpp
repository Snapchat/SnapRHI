#include "snap/rhi/backend/opengl/Texture.hpp"
#include "snap/rhi/backend/common/Logging.hpp"
#include "snap/rhi/backend/opengl/Device.hpp"
#include "snap/rhi/backend/opengl/ErrorCheckGuard.hpp"
#include "snap/rhi/backend/opengl/Profile.hpp"
#include "snap/rhi/backend/opengl/TextureInterop.h"
#include "snap/rhi/backend/opengl/Utils.hpp"

#include <cassert>
#include <cmath>
#include <snap/rhi/common/Scope.h>

#if SNAP_RHI_PLATFORM_WEBASSEMBLY()
#include <emscripten/bind.h>
#endif

namespace {
snap::rhi::backend::opengl::TextureTarget convertToGLTextureTarget(const snap::rhi::TextureType textureType) {
    switch (textureType) {
        case snap::rhi::TextureType::Texture2D:
            return snap::rhi::backend::opengl::TextureTarget::Texture2D;

        case snap::rhi::TextureType::Texture3D:
            return snap::rhi::backend::opengl::TextureTarget::Texture3D; // GL_OES_texture_3D

        case snap::rhi::TextureType::Texture2DArray:
            return snap::rhi::backend::opengl::TextureTarget::Texture2DArray; // GL_EXT_texture_array

        case snap::rhi::TextureType::TextureCubemap:
            return snap::rhi::backend::opengl::TextureTarget::TextureCubeMap;

        default:
            snap::rhi::common::throwException("Unkown texture type");
    }

    return snap::rhi::backend::opengl::TextureTarget::Detached;
}

void initializeTextureParameter(const snap::rhi::backend::opengl::Profile& gl,
                                const snap::rhi::backend::opengl::TextureTarget target,
                                const snap::rhi::TextureCreateInfo& textureCreateInfo,
                                const bool isImmutable) {
    gl.texParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl.texParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    gl.texParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl.texParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    if (target == snap::rhi::backend::opengl::TextureTarget::Texture3D) {
        gl.texParameteri(target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }

    /**
     * https://www.khronos.org/opengl/wiki/Common_Mistakes#Creating_a_complete_texture
     *
     * The texture won't work because it is incomplete. The default GL_TEXTURE_MIN_FILTER state is
     *GL_NEAREST_MIPMAP_LINEAR. And because OpenGL defines the default GL_TEXTURE_MAX_LEVEL to be 1000, OpenGL will
     *expect there to be mipmap levels defined. Since you have only defined a single mipmap level, OpenGL will consider
     *the texture incomplete until the GL_TEXTURE_MAX_LEVEL is properly set, or the GL_TEXTURE_MIN_FILTER parameter is
     *set to not use mipmaps.
     **/
    if (!isImmutable && gl.getFeatures().isTexBaseMaxLevelSupported) {
        gl.texParameteri(target, GL_TEXTURE_BASE_LEVEL, 0);
        gl.texParameteri(target, GL_TEXTURE_MAX_LEVEL, textureCreateInfo.mipLevels - 1);
    }
}

void initGrayscaleTextureFormat(const snap::rhi::backend::opengl::Profile& gl,
                                const snap::rhi::TextureCreateInfo& textureCreateInfo,
                                const snap::rhi::backend::opengl::Features& features,
                                const snap::rhi::backend::opengl::SizedInternalFormat internalFormat) {
    if (textureCreateInfo.format == snap::rhi::PixelFormat::Grayscale) {
        if (internalFormat == snap::rhi::backend::opengl::SizedInternalFormat::R8) {
            SNAP_RHI_VALIDATE(
                gl.getDevice()->getValidationLayer(),
                features.isTexSwizzleSupported,
                snap::rhi::ReportLevel::CriticalError,
                snap::rhi::ValidationTag::TextureOp,
                "[initGrayscaleTextureFormat] Invalid grayscale format config, \"isTexSwizzleSupported\" for "
                "SizedInternalFormat::R8 is false");

            gl.texParameteri(snap::rhi::backend::opengl::TextureTarget::Texture2D, GL_TEXTURE_SWIZZLE_R, GL_RED);
            gl.texParameteri(snap::rhi::backend::opengl::TextureTarget::Texture2D, GL_TEXTURE_SWIZZLE_G, GL_RED);
            gl.texParameteri(snap::rhi::backend::opengl::TextureTarget::Texture2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
            gl.texParameteri(snap::rhi::backend::opengl::TextureTarget::Texture2D, GL_TEXTURE_SWIZZLE_A, GL_ONE);
        } else {
            SNAP_RHI_VALIDATE(gl.getDevice()->getValidationLayer(),
                              internalFormat == snap::rhi::backend::opengl::SizedInternalFormat::DEPRECATED_LUMINANCE,
                              snap::rhi::ReportLevel::CriticalError,
                              snap::rhi::ValidationTag::TextureOp,
                              "[initGrayscaleTextureFormat] Invalid grayscale physical format: %d",
                              internalFormat);
        }
    }
}

void initializeTextureStorage(const snap::rhi::backend::opengl::Profile& gl,
                              const snap::rhi::TextureCreateInfo& textureCreateInfo,
                              snap::rhi::backend::opengl::TextureTarget textureTarget,
                              snap::rhi::backend::opengl::TextureId textureId,
                              const snap::rhi::backend::opengl::SizedInternalFormat internalFormat) {
    const auto& features = gl.getFeatures();

    SNAP_RHI_VALIDATE(gl.getDevice()->getValidationLayer(),
                      internalFormat != snap::rhi::backend::opengl::SizedInternalFormat::UNKNOWN,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::TextureOp,
                      "[initializeTexture] Invalid texture pixel format: %d",
                      textureCreateInfo.format);

    uint32_t sampleCount = static_cast<uint32_t>(textureCreateInfo.sampleCount);
    uint32_t w = textureCreateInfo.size.width;
    uint32_t h = textureCreateInfo.size.height;
    uint32_t d = textureCreateInfo.size.depth;
    uint32_t arraySize = textureCreateInfo.size.depth;

    switch (textureCreateInfo.textureType) {
        case snap::rhi::TextureType::Texture2D: {
            if (sampleCount > 1) {
                SNAP_RHI_VALIDATE(gl.getDevice()->getValidationLayer(),
                                  textureCreateInfo.mipLevels == 1,
                                  snap::rhi::ReportLevel::Error,
                                  snap::rhi::ValidationTag::TextureOp,
                                  "[initializeTexture] multisample texture supported only 1 mip level");
                gl.texStorage2DMultisample(textureTarget,
                                           sampleCount,
                                           internalFormat,
                                           static_cast<GLsizei>(w),
                                           static_cast<GLsizei>(h),
                                           GL_TRUE);
            } else {
                gl.texStorage2D(textureTarget,
                                textureCreateInfo.mipLevels,
                                internalFormat,
                                static_cast<GLsizei>(w),
                                static_cast<GLsizei>(h));
            }
        } break;

        case snap::rhi::TextureType::Texture2DArray: {
            if (sampleCount > 1) {
                SNAP_RHI_VALIDATE(gl.getDevice()->getValidationLayer(),
                                  textureCreateInfo.mipLevels == 1,
                                  snap::rhi::ReportLevel::Error,
                                  snap::rhi::ValidationTag::TextureOp,
                                  "[initializeTexture] multisample texture supported only 1 mip level");
                gl.texStorage3DMultisample(textureTarget,
                                           sampleCount,
                                           internalFormat,
                                           static_cast<GLsizei>(w),
                                           static_cast<GLsizei>(h),
                                           static_cast<GLsizei>(arraySize),
                                           GL_TRUE);
            } else {
                gl.texStorage3D(textureTarget,
                                textureCreateInfo.mipLevels,
                                internalFormat,
                                static_cast<GLsizei>(w),
                                static_cast<GLsizei>(h),
                                static_cast<GLsizei>(arraySize));
            }
        } break;

        case snap::rhi::TextureType::Texture3D:
            gl.texStorage3D(textureTarget,
                            textureCreateInfo.mipLevels,
                            internalFormat,
                            static_cast<GLsizei>(w),
                            static_cast<GLsizei>(h),
                            static_cast<GLsizei>(d));
            break;

        case snap::rhi::TextureType::TextureCubemap:
            gl.texStorage2D(snap::rhi::backend::opengl::TextureTarget::TextureCubeMap,
                            textureCreateInfo.mipLevels,
                            internalFormat,
                            static_cast<GLsizei>(w),
                            static_cast<GLsizei>(h));
            break;

        default:
            snap::rhi::common::throwException("Unkown texture type");
    }

    initGrayscaleTextureFormat(gl, textureCreateInfo, features, internalFormat);
    initializeTextureParameter(gl, textureTarget, textureCreateInfo, true);
}

snap::rhi::backend::opengl::SizedInternalFormat initializeTextureImage(
    const snap::rhi::backend::opengl::Profile& gl,
    const snap::rhi::TextureCreateInfo& textureCreateInfo,
    snap::rhi::backend::opengl::TextureTarget textureTarget,
    snap::rhi::backend::opengl::TextureId textureId,
    const snap::rhi::backend::opengl::SizedInternalFormat internalFormat) {
    const auto& features = gl.getFeatures();
    const auto& formatInfo = gl.getTextureFormat(textureCreateInfo.format);
    assert(formatInfo.internalFormat == internalFormat);

    SNAP_RHI_VALIDATE(gl.getDevice()->getValidationLayer(),
                      formatInfo.dataType != snap::rhi::backend::opengl::FormatDataType::UNKNOWN_OR_COMPRESSED,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::TextureOp,
                      "[initializeTexture] Invalid texture pixel format: %d",
                      textureCreateInfo.format);

    uint32_t w = textureCreateInfo.size.width;
    uint32_t h = textureCreateInfo.size.height;
    uint32_t d = textureCreateInfo.size.depth;
    uint32_t arraySize = textureCreateInfo.size.depth;

    for (uint32_t i = 0; i < textureCreateInfo.mipLevels; ++i) {
        switch (textureCreateInfo.textureType) {
            case snap::rhi::TextureType::Texture2D: {
                if (snap::rhi::isCompressedFormat(textureCreateInfo.format)) {
                    gl.compressedTexImage2D(textureTarget,
                                            i,
                                            internalFormat,
                                            static_cast<GLsizei>(w),
                                            static_cast<GLsizei>(h),
                                            0,
                                            snap::rhi::bytesPerSlice(w, h, textureCreateInfo.format),
                                            nullptr);
                } else {
                    gl.texImage2D(textureTarget,
                                  i,
                                  internalFormat,
                                  static_cast<GLsizei>(w),
                                  static_cast<GLsizei>(h),
                                  0,
                                  formatInfo.format,
                                  formatInfo.dataType,
                                  nullptr);
                }
            } break;

            case snap::rhi::TextureType::Texture2DArray: {
                if (snap::rhi::isCompressedFormat(textureCreateInfo.format)) {
                    gl.compressedTexImage3D(textureTarget,
                                            i,
                                            internalFormat,
                                            static_cast<GLsizei>(w),
                                            static_cast<GLsizei>(h),
                                            static_cast<GLsizei>(arraySize),
                                            0,
                                            snap::rhi::bytesPerSlice(w, h, textureCreateInfo.format) * arraySize,
                                            nullptr);
                } else {
                    gl.texImage3D(textureTarget,
                                  i,
                                  internalFormat,
                                  static_cast<GLsizei>(w),
                                  static_cast<GLsizei>(h),
                                  static_cast<GLsizei>(arraySize),
                                  0,
                                  formatInfo.format,
                                  formatInfo.dataType,
                                  nullptr);
                }
            } break;

            case snap::rhi::TextureType::Texture3D: {
                if (snap::rhi::isCompressedFormat(textureCreateInfo.format)) {
                    gl.compressedTexImage3D(textureTarget,
                                            i,
                                            internalFormat,
                                            static_cast<GLsizei>(w),
                                            static_cast<GLsizei>(h),
                                            static_cast<GLsizei>(d),
                                            0,
                                            snap::rhi::bytesPerSlice(w, h, textureCreateInfo.format) * d,
                                            nullptr);
                } else {
                    gl.texImage3D(textureTarget,
                                  i,
                                  internalFormat,
                                  static_cast<GLsizei>(w),
                                  static_cast<GLsizei>(h),
                                  static_cast<GLsizei>(d),
                                  0,
                                  formatInfo.format,
                                  formatInfo.dataType,
                                  nullptr);
                }
            } break;

            case snap::rhi::TextureType::TextureCubemap: {
                assert(d == 1);

                for (uint32_t face = 0; face < 6; ++face) {
                    if (snap::rhi::isCompressedFormat(textureCreateInfo.format)) {
                        gl.compressedTexImage2D(snap::rhi::backend::opengl::getCubeMapSideTextureTarget(face),
                                                i,
                                                internalFormat,
                                                static_cast<GLsizei>(w),
                                                static_cast<GLsizei>(h),
                                                0,
                                                snap::rhi::bytesPerSlice(w, h, textureCreateInfo.format),
                                                nullptr);
                    } else {
                        gl.texImage2D(snap::rhi::backend::opengl::getCubeMapSideTextureTarget(face),
                                      i,
                                      internalFormat,
                                      static_cast<GLsizei>(w),
                                      static_cast<GLsizei>(h),
                                      0,
                                      formatInfo.format,
                                      formatInfo.dataType,
                                      nullptr);
                    }
                }
            } break;

            default:
                snap::rhi::common::throwException("Unkown texture type");
        }

        w = std::max(1u, w >> 1);
        h = std::max(1u, h >> 1);
        d = std::max(1u, d >> 1);
    }

    initGrayscaleTextureFormat(gl, textureCreateInfo, features, internalFormat);
    initializeTextureParameter(gl, textureTarget, textureCreateInfo, false);

    return formatInfo.internalFormat;
}

void initializeRenderbuffer(const snap::rhi::backend::opengl::Profile& gl,
                            const snap::rhi::TextureCreateInfo& textureCreateInfo,
                            const snap::rhi::backend::opengl::TextureTarget target,
                            const snap::rhi::backend::opengl::TextureId textureId,
                            const snap::rhi::backend::opengl::SizedInternalFormat internalFormat) {
    SNAP_RHI_VALIDATE(gl.getDevice()->getValidationLayer(),
                      internalFormat != snap::rhi::backend::opengl::SizedInternalFormat::UNKNOWN,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::TextureOp,
                      "[initializeTexture] Invalid texture pixel format: %d",
                      textureCreateInfo.format);

    gl.bindRenderbuffer(target, textureId);
    if (textureCreateInfo.sampleCount == snap::rhi::SampleCount::Count1) {
        gl.renderbufferStorage(target,
                               internalFormat,
                               static_cast<GLsizei>(textureCreateInfo.size.width),
                               static_cast<GLsizei>(textureCreateInfo.size.height));
    } else {
        gl.renderbufferStorageMultisample(target,
                                          static_cast<GLsizei>(textureCreateInfo.sampleCount),
                                          internalFormat,
                                          static_cast<GLsizei>(textureCreateInfo.size.width),
                                          static_cast<GLsizei>(textureCreateInfo.size.height));
    }

    gl.bindRenderbuffer(target, snap::rhi::backend::opengl::TextureId::Null);
}

void uploadCompressedTexSubImage(const snap::rhi::backend::opengl::Profile& gl,
                                 const snap::rhi::TextureCreateInfo& info,
                                 const snap::rhi::backend::opengl::TextureTarget target,
                                 const snap::rhi::backend::opengl::CompositeFormat& formatInfo,
                                 const snap::rhi::Offset3D& offset,
                                 const snap::rhi::Extent3D& size,
                                 const uint32_t mipmapLevel,
                                 const uint8_t* pixels,
                                 const uint32_t srcBytesPerRow,
                                 const uint64_t srcBytesPerSlice) {
    auto* glDevice = snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Device>(gl.getDevice());
    const snap::rhi::backend::common::ValidationLayer& validationLayer = glDevice->getValidationLayer();

    const uint32_t dstBytesPerRow = snap::rhi::bytesPerRow(size.width, size.height, info.format);
    const uint64_t dstBytesPerSlice = snap::rhi::bytesPerSlice(size.width, size.height, info.format);

    const uint32_t bytesPerRow = srcBytesPerRow ? srcBytesPerRow : dstBytesPerRow;
    const uint64_t bytesPerSlice =
        srcBytesPerSlice ? srcBytesPerSlice : snap::rhi::bytesPerSliceWithRow(bytesPerRow, size.height, info.format);

    SNAP_RHI_VALIDATE(validationLayer,
                      snap::rhi::isCompressedFormat(info.format),
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::CreateOp,
                      "[uploadCompressedTexSubImage] texture format {%d} isn't compressed format",
                      static_cast<uint32_t>(info.format));

    SNAP_RHI_VALIDATE(validationLayer,
                      (bytesPerRow == dstBytesPerRow) && (bytesPerSlice == dstBytesPerSlice),
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::CreateOp,
                      "[uploadCompressedTexSubImage] (bytesPerRow{%d} == dstBytesPerRow{%d}) && "
                      "(bytesPerSlice{%d} == dstBytesPerSlice{%d})",
                      static_cast<uint32_t>(bytesPerRow),
                      static_cast<uint32_t>(dstBytesPerRow),
                      static_cast<uint32_t>(bytesPerSlice),
                      static_cast<uint32_t>(dstBytesPerSlice));

    gl.pixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    gl.pixelStorei(GL_UNPACK_IMAGE_HEIGHT, 0);

    switch (info.textureType) {
        case snap::rhi::TextureType::Texture2D: {
            assert(offset.z == 0 && size.depth == 1 && target == snap::rhi::backend::opengl::TextureTarget::Texture2D);
            gl.compressedTexSubImage2D(target,
                                       static_cast<GLint>(mipmapLevel),
                                       offset.x,
                                       offset.y,
                                       size.width,
                                       size.height,
                                       formatInfo.internalFormat,
                                       bytesPerSlice,
                                       pixels);
        } break;

        case snap::rhi::TextureType::Texture2DArray: {
            assert(target == snap::rhi::backend::opengl::TextureTarget::Texture2DArray);

            gl.compressedTexSubImage3D(target,
                                       static_cast<GLint>(mipmapLevel),
                                       offset.x,
                                       offset.y,
                                       offset.z,
                                       size.width,
                                       size.height,
                                       size.depth,
                                       formatInfo.internalFormat,
                                       bytesPerSlice * size.depth,
                                       pixels);
        } break;

        case snap::rhi::TextureType::Texture3D: {
            assert(target == snap::rhi::backend::opengl::TextureTarget::Texture3D);

            gl.compressedTexSubImage3D(target,
                                       static_cast<GLint>(mipmapLevel),
                                       offset.x,
                                       offset.y,
                                       offset.z,
                                       size.width,
                                       size.height,
                                       size.depth,
                                       formatInfo.internalFormat,
                                       bytesPerSlice * size.depth,
                                       pixels);
        } break;

        case snap::rhi::TextureType::TextureCubemap: {
            assert(offset.z < 6 && offset.z >= 0 && size.depth > 0 && size.depth <= 6 &&
                   target == snap::rhi::backend::opengl::TextureTarget::TextureCubeMap);

            const uint8_t* slicePixels = pixels;
            for (uint32_t z = 0; z < size.depth; ++z) {
                gl.compressedTexSubImage2D(snap::rhi::backend::opengl::getCubeMapSideTextureTarget(offset.z + z),
                                           static_cast<GLint>(mipmapLevel),
                                           offset.x,
                                           offset.y,
                                           size.width,
                                           size.height,
                                           formatInfo.internalFormat,
                                           bytesPerSlice,
                                           slicePixels);
                slicePixels += bytesPerSlice;
            }

        } break;

        default:
            snap::rhi::common::throwException("Unkown texture type");
    }
}

template<typename Pixels>
void uploadTexSubImage(const snap::rhi::backend::opengl::Profile& gl,
                       const snap::rhi::TextureCreateInfo& info,
                       const snap::rhi::backend::opengl::TextureTarget target,
                       const snap::rhi::backend::opengl::CompositeFormat& formatInfo,
                       const snap::rhi::Offset3D& offset,
                       const snap::rhi::Extent3D& size,
                       const uint32_t mipmapLevel,
                       const Pixels pixels,
                       const uint32_t srcBytesPerRow,
                       const uint32_t srcBytesPerSlice) {
    auto* glDevice = snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Device>(gl.getDevice());
    const snap::rhi::backend::common::ValidationLayer& validationLayer = glDevice->getValidationLayer();

    SNAP_RHI_VALIDATE(validationLayer,
                      !snap::rhi::isCompressedFormat(info.format),
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::CreateOp,
                      "[uploadTexSubImage] texture format {%d} is compressed format",
                      static_cast<uint32_t>(info.format));

    uint32_t bytesPerRow = 0;
    uint64_t bytesPerSlice = 0;

    uint32_t unpackRowLength = 0;
    uint32_t unpackImageHeight = 0;

    snap::rhi::backend::opengl::computeUnpackImageSize(size.width,
                                                       size.height,
                                                       info.format,
                                                       srcBytesPerRow,
                                                       srcBytesPerSlice,
                                                       bytesPerRow,
                                                       bytesPerSlice,
                                                       unpackRowLength,
                                                       unpackImageHeight);

    gl.pixelStorei(GL_UNPACK_ROW_LENGTH, size.width == unpackRowLength ? 0 : unpackRowLength);
    gl.pixelStorei(GL_UNPACK_IMAGE_HEIGHT, size.height == unpackImageHeight ? 0 : unpackImageHeight);

    switch (info.textureType) {
        case snap::rhi::TextureType::Texture2D: {
            assert(offset.z == 0 && size.depth == 1 &&
                   (target == snap::rhi::backend::opengl::TextureTarget::Texture2D ||
                    target == snap::rhi::backend::opengl::TextureTarget::TextureRectangle));
            gl.texSubImage2D(target,
                             static_cast<GLint>(mipmapLevel),
                             offset.x,
                             offset.y,
                             size.width,
                             size.height,
                             formatInfo.format,
                             formatInfo.dataType,
                             pixels);
        } break;

        case snap::rhi::TextureType::Texture2DArray: {
            assert(target == snap::rhi::backend::opengl::TextureTarget::Texture2DArray);
            gl.texSubImage3D(target,
                             static_cast<GLint>(mipmapLevel),
                             offset.x,
                             offset.y,
                             offset.z,
                             size.width,
                             size.height,
                             size.depth,
                             formatInfo.format,
                             formatInfo.dataType,
                             pixels);
        } break;

        case snap::rhi::TextureType::Texture3D: {
            assert(target == snap::rhi::backend::opengl::TextureTarget::Texture3D);
            gl.texSubImage3D(target,
                             static_cast<GLint>(mipmapLevel),
                             offset.x,
                             offset.y,
                             offset.z,
                             size.width,
                             size.height,
                             size.depth,
                             formatInfo.format,
                             formatInfo.dataType,
                             pixels);
        } break;

        case snap::rhi::TextureType::TextureCubemap: {
#if SNAP_RHI_PLATFORM_WEBASSEMBLY()
            if constexpr (std::is_same_v<std::decay_t<Pixels>, emscripten::val>) {
                snap::rhi::common::throwException(
                    "Texture Cubemap uploadTexSubImage currently not available for WebGL for raw val objects");
            } else {
                assert(offset.z < 6 && offset.z >= 0 && size.depth > 0 && size.depth <= 6 &&
                       target == snap::rhi::backend::opengl::TextureTarget::TextureCubeMap);

                const uint8_t* slicePixels = pixels;
                for (uint32_t z = 0; z < size.depth; ++z) {
                    gl.texSubImage2D(snap::rhi::backend::opengl::getCubeMapSideTextureTarget(offset.z + z),
                                     static_cast<GLint>(mipmapLevel),
                                     offset.x,
                                     offset.y,
                                     size.width,
                                     size.height,
                                     formatInfo.format,
                                     formatInfo.dataType,
                                     slicePixels);
                    slicePixels += bytesPerSlice;
                }
            }
#else
            assert(offset.z < 6 && offset.z >= 0 && size.depth > 0 && size.depth <= 6 &&
                   target == snap::rhi::backend::opengl::TextureTarget::TextureCubeMap);

            const uint8_t* slicePixels = pixels;
            for (uint32_t z = 0; z < size.depth; ++z) {
                gl.texSubImage2D(snap::rhi::backend::opengl::getCubeMapSideTextureTarget(offset.z + z),
                                 static_cast<GLint>(mipmapLevel),
                                 offset.x,
                                 offset.y,
                                 size.width,
                                 size.height,
                                 formatInfo.format,
                                 formatInfo.dataType,
                                 slicePixels);
                slicePixels += bytesPerSlice;
            }
#endif
        } break;

        default:
            snap::rhi::common::throwException("Unkown texture type");
    }
}

snap::rhi::backend::opengl::TextureTarget getTextureTarget(const snap::rhi::TextureCreateInfo& info) {
    snap::rhi::backend::opengl::TextureTarget target = snap::rhi::backend::opengl::TextureTarget::Detached;

    if (((info.textureUsage &
          ~(snap::rhi::TextureUsage::ColorAttachment | snap::rhi::TextureUsage::DepthStencilAttachment |
            snap::rhi::TextureUsage::TransferSrc)) == snap::rhi::TextureUsage::None) &&
        info.textureType == snap::rhi::TextureType::Texture2D && info.mipLevels == 1) {
        target = snap::rhi::backend::opengl::TextureTarget::Renderbuffer;
    } else {
        target = convertToGLTextureTarget(info.textureType);
    }

    return target;
}

snap::rhi::backend::opengl::SizedInternalFormat getSizedInternalFormatFromInfo(
    const snap::rhi::TextureCreateInfo& info, const snap::rhi::backend::opengl::Profile& gl) {
    snap::rhi::backend::opengl::SizedInternalFormat internalFormat =
        snap::rhi::backend::opengl::SizedInternalFormat::UNKNOWN;

    if (((info.textureUsage &
          ~(snap::rhi::TextureUsage::ColorAttachment | snap::rhi::TextureUsage::DepthStencilAttachment |
            snap::rhi::TextureUsage::TransferSrc)) == snap::rhi::TextureUsage::None) &&
        info.textureType == snap::rhi::TextureType::Texture2D && info.mipLevels == 1) {
        const auto& formatInfo = gl.getRenderbufferFormat(info.format);
        internalFormat = formatInfo.internalFormat;
    } else {
        const auto& formatInfo = gl.getTextureFormat(info.format);
        internalFormat = formatInfo.internalFormat;
    }

    return internalFormat;
}
} // unnamed namespace

namespace snap::rhi::backend::opengl {
Texture::Texture(snap::rhi::backend::opengl::Device* device, const TextureCreateInfo& info, TextureUUID textureUUID)
    : snap::rhi::Texture(device, info),
      gl(device->getOpenGL()),
      validationLayer(device->getValidationLayer()),
      textureUUID(textureUUID) {
    target = getTextureTarget(info);
    internalFormat = getSizedInternalFormatFromInfo(info, gl);

    if (!device->areResourcesLazyAllocationsEnabled()) {
        tryAllocate(nullptr);
    }

    SNAP_RHI_REPORT(validationLayer,
                    snap::rhi::ReportLevel::Debug,
                    snap::rhi::ValidationTag::CreateOp,
                    "[snap::rhi::backend::opengl::Texture] Texture created");
}

Texture::Texture(snap::rhi::backend::opengl::Device* device,
                 const std::shared_ptr<snap::rhi::TextureInterop>& textureInterop,
                 TextureUUID textureUUID)
    : snap::rhi::Texture(device, textureInterop),
      gl(device->getOpenGL()),
      validationLayer(device->getValidationLayer()),
      textureUUID(textureUUID) {
    auto* glTextureInterop = snap::rhi::backend::common::smart_dynamic_cast<snap::rhi::backend::opengl::TextureInterop>(
        textureInterop.get());

    target = glTextureInterop->getOpenGLTextureTarget(gl);
    internalFormat = getSizedInternalFormatFromInfo(info, gl);

    if (!device->areResourcesLazyAllocationsEnabled()) {
        tryAllocate(nullptr);
    }

    SNAP_RHI_REPORT(validationLayer,
                    snap::rhi::ReportLevel::Debug,
                    snap::rhi::ValidationTag::CreateOp,
                    "[snap::rhi::backend::opengl::Texture] Texture created");
}

Texture::Texture(snap::rhi::backend::opengl::Device* device,
                 const TextureCreateInfo& info,
                 const snap::rhi::backend::opengl::TextureId textureID,
                 const snap::rhi::backend::opengl::TextureTarget target,
                 const bool isTextureOwner,
                 TextureUUID textureUUID)
    : snap::rhi::Texture(device, info),
      gl(device->getOpenGL()),
      validationLayer(device->getValidationLayer()),
      texture(textureID),
      target(target),
      isTextureOwner(isTextureOwner),
      textureUUID(textureUUID) {
    SNAP_RHI_VALIDATE(validationLayer,
                      textureID != snap::rhi::backend::opengl::TextureId::Null,
                      snap::rhi::ReportLevel::CriticalError,
                      snap::rhi::ValidationTag::CreateOp,
                      "[snap::rhi::backend::opengl::Texture] textureID is null");

    SNAP_RHI_REPORT(validationLayer,
                    snap::rhi::ReportLevel::Debug,
                    snap::rhi::ValidationTag::CreateOp,
                    "[snap::rhi::backend::opengl::Texture] Texture created");
}

Texture::~Texture() {
    try {
        auto* glDevice = snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Device>(device);
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::DestroyOp,
                        "[snap::rhi::backend::opengl::~Texture] start destruction");
        SNAP_RHI_VALIDATE(validationLayer,
                          glDevice->getCurrentDeviceContext(),
                          snap::rhi::ReportLevel::Warning,
                          snap::rhi::ValidationTag::DestroyOp,
                          "[snap::rhi::backend::opengl::~Texture] DeviceContext isn't attached to thread");
        SNAP_RHI_VALIDATE(validationLayer,
                          glDevice->isNativeContextAttached(),
                          snap::rhi::ReportLevel::CriticalError,
                          snap::rhi::ValidationTag::DestroyOp,
                          "[snap::rhi::backend::opengl::~Texture] GLES context isn't attached to thread");
        destroyGLTexture();
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::DestroyOp,
                        "[snap::rhi::backend::opengl::~Texture] end of destruction");
    } catch (const snap::rhi::common::Exception& e) {
        SNAP_RHI_LOGE("[snap::rhi::backend::opengl::Texture::~Texture] Caught: %s, (possible resource leak).",
                      e.what());
    } catch (...) {
        SNAP_RHI_LOGE(
            "[snap::rhi::backend::opengl::Texture::~Texture] Caught unexpected error (possible resource leak).");
    }
}

void Texture::tryAllocate(DeviceContext* dc) {
    SNAP_RHI_VALIDATE(validationLayer,
                      glObject.isValid(),
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::CreateOp,
                      "[snap::rhi::backend::opengl::Texture][tryAllocate] Texture{width: %d, height: %d, depth: %d, "
                      "format: %d} is invalid!",
                      info.size.width,
                      info.size.height,
                      info.size.depth,
                      info.format);

    if (texture != TextureId::Null) {
        return;
    }

    auto* glDevice = snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Device>(device);
    ErrorCheckGuard glErrorCheckGuard(glDevice,
                                      static_cast<bool>(validationLayer.getValidationTags() & ValidationTag::TextureOp),
                                      [this]() { destroyGLTexture(); });

    if (textureInterop) {
        auto* glTextureInterop =
            snap::rhi::backend::common::smart_dynamic_cast<snap::rhi::backend::opengl::TextureInterop>(
                textureInterop.get());
        texture = static_cast<TextureId>(glTextureInterop->getOpenGLTexture(gl));

        return;
    }

    if (((info.textureUsage &
          ~(snap::rhi::TextureUsage::ColorAttachment | snap::rhi::TextureUsage::DepthStencilAttachment |
            snap::rhi::TextureUsage::TransferSrc)) == snap::rhi::TextureUsage::None) &&
        info.textureType == snap::rhi::TextureType::Texture2D && info.mipLevels == 1) {
        gl.genRenderbuffers(1, &texture);
        initializeRenderbuffer(gl, info, target, texture, internalFormat);

        return;
    }

    {
        gl.genTextures(1, &texture);

        GLuint restoreTexture = GL_NONE;
        if (dc) {
            const auto& cache = dc->getGLStateCache();
            restoreTexture = cache.getBindTextureOrNone(static_cast<GLenum>(target));
        }

        gl.bindTexture(static_cast<snap::rhi::backend::opengl::TextureTarget>(target),
                       static_cast<snap::rhi::backend::opengl::TextureId>(texture),
                       dc);
        SNAP_RHI_ON_SCOPE_EXIT {
            gl.bindTexture(static_cast<snap::rhi::backend::opengl::TextureTarget>(target),
                           static_cast<snap::rhi::backend::opengl::TextureId>(restoreTexture),
                           dc);
        };

        if ((info.textureUsage & snap::rhi::TextureUsage::Storage) != snap::rhi::TextureUsage::None) {
            initializeTextureStorage(gl, info, target, texture, internalFormat);
        } else {
            initializeTextureImage(gl, info, target, texture, internalFormat);
        }
    }
}

void Texture::destroyGLTexture() {
    if (!textureInterop && texture != TextureId::Null && isTextureOwner) {
        if (target == TextureTarget::Renderbuffer) {
            gl.deleteRenderbuffers(1, &texture);
        } else {
            gl.deleteTextures(1, &texture);
        }
    }

    texture = TextureId::Null;
    glObject.markInvalid();
}

TextureId Texture::getTextureID(DeviceContext* dc) const {
    const_cast<Texture*>(this)->tryAllocate(dc);
    return texture;
}

TextureTarget Texture::getTarget() const noexcept {
    return target;
}

snap::rhi::backend::opengl::SizedInternalFormat Texture::getInternalFormat() const noexcept {
    return internalFormat;
}

void Texture::upload(const Offset3D& offset,
                     const Extent3D& size,
                     const uint32_t mipmapLevel,
                     const uint8_t* pixels,
                     const uint32_t unpackRowLength,
                     const uint32_t unpackImageHeight,
                     DeviceContext* dc) {
    tryAllocate(dc);

    if (target == TextureTarget::Renderbuffer) {
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Error,
                        snap::rhi::ValidationTag::TextureOp,
                        "[Texture::upload] Invalid texture usage");
    }

    SNAP_RHI_VALIDATE(validationLayer,
                      mipmapLevel < info.mipLevels,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::TextureOp,
                      "[Texture::upload] mipmapLevel{%d} greater than texture may upload{%d}.",
                      mipmapLevel,
                      info.mipLevels);

    const auto& formatInfo = gl.getTextureFormat(info.format);
    assert(formatInfo.dataType != snap::rhi::backend::opengl::FormatDataType::UNKNOWN_OR_COMPRESSED);
    const TextureTarget target = getTarget();

    gl.bindTexture(target, getTextureID(dc), dc);
    if (snap::rhi::isCompressedFormat(info.format)) {
        uploadCompressedTexSubImage(
            gl, info, target, formatInfo, offset, size, mipmapLevel, pixels, unpackRowLength, unpackImageHeight);
    } else {
        uploadTexSubImage(
            gl, info, target, formatInfo, offset, size, mipmapLevel, pixels, unpackRowLength, unpackImageHeight);
    }
    gl.bindTexture(target, snap::rhi::backend::opengl::TextureId::Null, dc);

    /**
     * Driver may reallocate texture memory after uploading
     * In order to prevent issues with FBO pool, we have to regenerate texture UUID.
     **/
    updateTextureUUID();
}

#if SNAP_RHI_PLATFORM_WEBASSEMBLY()
void Texture::upload(const Offset3D& offset,
                     const Extent3D& size,
                     const uint32_t mipmapLevel,
                     const emscripten::val& pixels,
                     const uint32_t unpackRowLength,
                     const uint32_t unpackImageHeight,
                     DeviceContext* dc) {
    tryAllocate(dc);

    if (target == TextureTarget::Renderbuffer) {
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Error,
                        snap::rhi::ValidationTag::TextureOp,
                        "[Texture::upload] Invalid texture usage");
    }

    SNAP_RHI_VALIDATE(validationLayer,
                      mipmapLevel < info.mipLevels,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::TextureOp,
                      "[Texture::upload] mipmapLevel{%d} greater than texture may upload{%d}.",
                      mipmapLevel,
                      info.mipLevels);

    const auto& formatInfo = gl.getTextureFormat(info.format);
    assert(formatInfo.dataType != snap::rhi::backend::opengl::FormatDataType::UNKNOWN_OR_COMPRESSED);
    const TextureTarget target = getTarget();

    gl.bindTexture(target, getTextureID(dc), dc);
    if (snap::rhi::isCompressedFormat(info.format)) {
        snap::rhi::common::throwException("Attempting to upload compressed sub texture via raw JS val");
    }
    uploadTexSubImage(
        gl, info, target, formatInfo, offset, size, mipmapLevel, pixels, unpackRowLength, unpackImageHeight);
    gl.bindTexture(target, snap::rhi::backend::opengl::TextureId::Null, dc);

    updateTextureUUID();
}
#endif

void Texture::updateTextureUUID() const {
    auto* glDevice = snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Device>(device);
    textureUUID = glDevice->acquireTextureUUID();
}

TextureUUID Texture::getTextureUUID() const {
    return textureUUID;
}

void Texture::setDebugLabel(std::string_view label) {
#if SNAP_RHI_ENABLE_DEBUG_LABELS
    ObjectDebugMarkers::setDebugLabel(label);
    if (texture != TextureId::Null) {
        gl.objectLabel(GL_TEXTURE, static_cast<GLuint>(texture), label);
    }
#endif
}
} // namespace snap::rhi::backend::opengl
