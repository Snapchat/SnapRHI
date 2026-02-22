//
//  Instance.cpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 6/30/22.
//

#include "Instance.hpp"

#include "snap/rhi/backend/common/Logging.hpp"
#include "snap/rhi/common/Throw.h"
#include <algorithm>
#include <string_view>
#include <vector>

#if SNAP_RHI_GLES20
namespace {
void initTexFormatProperties(snap::rhi::backend::opengl::Features& features,
                             gl::APIVersion realApiVersion,
                             const std::string& glExtensionsList) {
    { // snap::rhi::PixelFormat::R8G8B8Unorm
        features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R8G8B8Unorm)] = {
            snap::rhi::backend::opengl::SizedInternalFormat::DEPRECATED_RGB,
            snap::rhi::backend::opengl::FormatGroup::RGB,
            snap::rhi::backend::opengl::FormatDataType::UNSIGNED_BYTE};

        auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R8G8B8Unorm)];
        formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
        formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
    }

    { // snap::rhi::PixelFormat::R8G8B8A8Unorm
        features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R8G8B8A8Unorm)] = {
            snap::rhi::backend::opengl::SizedInternalFormat::DEPRECATED_RGBA,
            snap::rhi::backend::opengl::FormatGroup::RGBA,
            snap::rhi::backend::opengl::FormatDataType::UNSIGNED_BYTE};

        auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R8G8B8A8Unorm)];
        formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
        formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
    }

    { // snap::rhi::PixelFormat::Grayscale
        features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::Grayscale)] = {
            snap::rhi::backend::opengl::SizedInternalFormat::DEPRECATED_LUMINANCE,
            snap::rhi::backend::opengl::FormatGroup::DEPRECATED_LUMINANCE,
            snap::rhi::backend::opengl::FormatDataType::UNSIGNED_BYTE};

        auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::Grayscale)];
        formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::Uploadable;
        formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
    }

    if (realApiVersion >= gl::APIVersion::GLES30) {
        { // snap::rhi::PixelFormat::R8Unorm
            features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R8Unorm)] = {
                snap::rhi::backend::opengl::SizedInternalFormat::DEPRECATED_R,
                snap::rhi::backend::opengl::FormatGroup::R,
                snap::rhi::backend::opengl::FormatDataType::UNSIGNED_BYTE}; // {GL_RED_EXT, GL_RED_EXT,
                                                                            // GL_UNSIGNED_BYTE}; GL_EXT_texture_rg

            auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R8Unorm)];
            formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
            formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
        }

        { // snap::rhi::PixelFormat::R8G8Unorm
#if SNAP_RHI_PLATFORM_WEBASSEMBLY()
            features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R8G8Unorm)] = {
                snap::rhi::backend::opengl::SizedInternalFormat::DEPRECATED_LUMINANCE_ALPHA,
                snap::rhi::backend::opengl::FormatGroup::DEPRECATED_LUMINANCE_ALPHA,
                snap::rhi::backend::opengl::FormatDataType::UNSIGNED_BYTE};
#else  // SNAP_RHI_PLATFORM_WEBASSEMBLY()
            features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R8G8Unorm)] = {
                snap::rhi::backend::opengl::SizedInternalFormat::DEPRECATED_RG,
                snap::rhi::backend::opengl::FormatGroup::RG,
                snap::rhi::backend::opengl::FormatDataType::UNSIGNED_BYTE}; // {GL_RG_EXT, GL_RG_EXT, GL_UNSIGNED_BYTE};
                                                                            // GL_EXT_texture_rg
#endif // // SNAP_RHI_PLATFORM_WEBASSEMBLY()
            auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R8G8Unorm)];
            formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
            formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
        }

        {
            features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R32G32B32A32Float)] = {
                snap::rhi::backend::opengl::SizedInternalFormat::RGBA32F,
                snap::rhi::backend::opengl::FormatGroup::RGBA,
                snap::rhi::backend::opengl::FormatDataType::FLOAT};

            auto& formatInfo =
                features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R32G32B32A32Float)];
            formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::Uploadable |
                                  snap::rhi::backend::opengl::FormatFeatures::Copyable;
            formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestOnly;
        }

        {
            features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R32Float)] = {
                snap::rhi::backend::opengl::SizedInternalFormat::R32F,
                snap::rhi::backend::opengl::FormatGroup::R,
                snap::rhi::backend::opengl::FormatDataType::FLOAT};

            auto& formatInfoR = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R32Float)];
            formatInfoR.features = snap::rhi::backend::opengl::FormatFeatures::Uploadable;
            formatInfoR.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestOnly;
        }

        {
            features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R32G32Float)] = {
                snap::rhi::backend::opengl::SizedInternalFormat::RG32F,
                snap::rhi::backend::opengl::FormatGroup::RG,
                snap::rhi::backend::opengl::FormatDataType::FLOAT};

            auto& formatInfoRG =
                features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R32G32Float)];
            formatInfoRG.features = snap::rhi::backend::opengl::FormatFeatures::Uploadable;
            formatInfoRG.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestOnly;
        }

        { // snap::rhi::PixelFormat::DepthStencil
            features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::DepthStencil)] = {
                snap::rhi::backend::opengl::SizedInternalFormat::DEPTH24_STENCIL8,
                snap::rhi::backend::opengl::FormatGroup::DEPTH_STENCIL,
                snap::rhi::backend::opengl::FormatDataType::UNSIGNED_INT_24_8};

            auto& formatInfo =
                features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::DepthStencil)];
            formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::Renderable;
            formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::None;
        }

        { // snap::rhi::PixelFormat::Depth16Unorm
            features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::Depth16Unorm)] = {
                snap::rhi::backend::opengl::SizedInternalFormat::DEPTH16,
                snap::rhi::backend::opengl::FormatGroup::DEPTH,
                snap::rhi::backend::opengl::FormatDataType::UNSIGNED_SHORT};

            auto& formatInfo =
                features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::Depth16Unorm)];
            formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::Renderable;
            formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::None;
        }

        { // snap::rhi::PixelFormat::DepthFloat
            features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::DepthFloat)] = {
                snap::rhi::backend::opengl::SizedInternalFormat::DEPTH32F,
                snap::rhi::backend::opengl::FormatGroup::DEPTH,
                snap::rhi::backend::opengl::FormatDataType::FLOAT};

            auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::DepthFloat)];
            formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::Renderable;
            formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::None;
        }
    } else {
        bool isRGFormatSupported =
            snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "GL_EXT_texture_rg");
        bool isDepthStencilSupported =
            snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "GL_OES_packed_depth_stencil");
        bool isDepthTextureSupported =
            snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "GL_OES_depth_texture");
        bool isTextureFloatSupported =
            snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "GL_OES_texture_float");
        bool isTextureHalfFloatSupported =
            snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "GL_OES_texture_half_float");

        if (isRGFormatSupported) {
            { // snap::rhi::PixelFormat::R8Unorm
                features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R8Unorm)] = {
                    snap::rhi::backend::opengl::SizedInternalFormat::DEPRECATED_R,
                    snap::rhi::backend::opengl::FormatGroup::R,
                    snap::rhi::backend::opengl::FormatDataType::UNSIGNED_BYTE}; // {GL_RED_EXT, GL_RED_EXT,
                                                                                // GL_UNSIGNED_BYTE}; GL_EXT_texture_rg

                auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R8Unorm)];
                formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
                formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
            }

            { // snap::rhi::PixelFormat::R8G8Unorm
#if SNAP_RHI_PLATFORM_WEBASSEMBLY()
                features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R8G8Unorm)] = {
                    snap::rhi::backend::opengl::SizedInternalFormat::DEPRECATED_LUMINANCE_ALPHA,
                    snap::rhi::backend::opengl::FormatGroup::DEPRECATED_LUMINANCE_ALPHA,
                    snap::rhi::backend::opengl::FormatDataType::UNSIGNED_BYTE};
#else  // SNAP_RHI_PLATFORM_WEBASSEMBLY()
                features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R8G8Unorm)] = {
                    snap::rhi::backend::opengl::SizedInternalFormat::DEPRECATED_RG,
                    snap::rhi::backend::opengl::FormatGroup::RG,
                    snap::rhi::backend::opengl::FormatDataType::UNSIGNED_BYTE}; // {GL_RG_EXT, GL_RG_EXT,
                                                                                // GL_UNSIGNED_BYTE}; GL_EXT_texture_rg
#endif // SNAP_RHI_PLATFORM_WEBASSEMBLY()
                auto& formatInfo =
                    features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R8G8Unorm)];
                formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
                formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
            }
        }

        if (isDepthStencilSupported) {
            { // snap::rhi::PixelFormat::DepthStencil
                features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::DepthStencil)] = {
                    snap::rhi::backend::opengl::SizedInternalFormat::DEPRECATED_DEPTH_STENCIL,
                    snap::rhi::backend::opengl::FormatGroup::DEPTH_STENCIL,
                    snap::rhi::backend::opengl::FormatDataType::UNSIGNED_INT_24_8};

                auto& formatInfo =
                    features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::DepthStencil)];
                formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::Renderable;
                formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::None;
            }
        }

        if (isDepthTextureSupported) {
            { // snap::rhi::PixelFormat::Depth16Unorm
                features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::Depth16Unorm)] = {
                    snap::rhi::backend::opengl::SizedInternalFormat::DEPRECATED_DEPTH,
                    snap::rhi::backend::opengl::FormatGroup::DEPTH,
                    snap::rhi::backend::opengl::FormatDataType::UNSIGNED_SHORT};

                auto& formatInfo =
                    features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::Depth16Unorm)];
                formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::Renderable;
                formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::None;
            }

            { // snap::rhi::PixelFormat::DepthFloat
                features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::DepthFloat)] = {
                    snap::rhi::backend::opengl::SizedInternalFormat::DEPRECATED_DEPTH,
                    snap::rhi::backend::opengl::FormatGroup::DEPTH,
                    snap::rhi::backend::opengl::FormatDataType::UNSIGNED_INT};

                auto& formatInfo =
                    features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::DepthFloat)];
                formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::Renderable;
                formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::None;
            }
        }

        if (isTextureFloatSupported) {
            features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R32G32B32A32Float)] = {
                snap::rhi::backend::opengl::SizedInternalFormat::DEPRECATED_RGBA,
                snap::rhi::backend::opengl::FormatGroup::RGBA,
                snap::rhi::backend::opengl::FormatDataType::FLOAT};

            auto& formatInfo =
                features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R32G32B32A32Float)];
            formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::Uploadable |
                                  snap::rhi::backend::opengl::FormatFeatures::Copyable;
            formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestOnly;

            if (isRGFormatSupported) { // snap::rhi::PixelFormat::R32Float
                features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R32Float)] = {
                    snap::rhi::backend::opengl::SizedInternalFormat::DEPRECATED_R,
                    snap::rhi::backend::opengl::FormatGroup::R,
                    snap::rhi::backend::opengl::FormatDataType::FLOAT};

                auto& formatInfoR =
                    features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R32Float)];
                formatInfoR.features = snap::rhi::backend::opengl::FormatFeatures::Uploadable;
                formatInfoR.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestOnly;

                features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R32G32Float)] = {
                    snap::rhi::backend::opengl::SizedInternalFormat::DEPRECATED_RG,
                    snap::rhi::backend::opengl::FormatGroup::RG,
                    snap::rhi::backend::opengl::FormatDataType::FLOAT};

                auto& formatInfoRG =
                    features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R32G32Float)];
                formatInfoRG.features = snap::rhi::backend::opengl::FormatFeatures::Uploadable;
                formatInfoRG.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestOnly;

                // Only the NEAREST and NEAREST_MIPMAP_NEAREST minification filters are supported.
                // Only the NEAREST magnification filter is supported
            }
        }

        if (isRGFormatSupported && isTextureHalfFloatSupported) {
            // snap::rhi::PixelFormat::R16Float
            // note: the internal format of the texture is 16-bit floats but pixel data passed in through
            // glTexSubImage2D is still expected to be with regular 32-bit GL_FLOAT
            features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R16Float)] = {
                snap::rhi::backend::opengl::SizedInternalFormat::DEPRECATED_R,
                snap::rhi::backend::opengl::FormatGroup::R,
                snap::rhi::backend::opengl::FormatDataType::FLOAT};

            auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R16Float)];
            formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::Uploadable |
                                  snap::rhi::backend::opengl::FormatFeatures::Copyable;
            formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestOnly;

            features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R16G16Float)] = {
                snap::rhi::backend::opengl::SizedInternalFormat::DEPRECATED_RG,
                snap::rhi::backend::opengl::FormatGroup::RG,
                snap::rhi::backend::opengl::FormatDataType::FLOAT};

            auto& formatInfoRG =
                features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R16G16Float)];
            formatInfoRG.features = snap::rhi::backend::opengl::FormatFeatures::Uploadable;
            formatInfoRG.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestOnly;
        }
    }

    bool EXT_texture_format_BGRA8888 =
        snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "GL_EXT_texture_format_BGRA8888");
    bool APPLE_texture_format_BGRA8888 =
        snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "GL_APPLE_texture_format_BGRA8888");
    if (EXT_texture_format_BGRA8888 || APPLE_texture_format_BGRA8888) {
        {
            features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::B8G8R8A8Unorm)] = {
                snap::rhi::backend::opengl::SizedInternalFormat::DEPRECATED_BGRA,
                snap::rhi::backend::opengl::FormatGroup::BGRA,
                snap::rhi::backend::opengl::FormatDataType::UNSIGNED_BYTE};

            auto& formatInfo =
                features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::B8G8R8A8Unorm)];
            formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
            formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
        }
    }

    snap::rhi::backend::opengl::addCompressedFormat(features);

    // snap::rhi::PixelFormat::ETC_R8G8B8_Unorm
    if (snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "GL_OES_compressed_ETC1_RGB8_texture") ||
        snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "WEBGL_compressed_texture_etc1")) {
        features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::ETC_R8G8B8_Unorm)] = {
            snap::rhi::backend::opengl::SizedInternalFormat::COMPRESSED_RGB8_ETC1,
            snap::rhi::backend::opengl::FormatGroup::RGBA,
            snap::rhi::backend::opengl::FormatDataType::UNSIGNED_BYTE};

        auto& formatInfo =
            features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::ETC_R8G8B8_Unorm)];
        formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::Uploadable |
                              snap::rhi::backend::opengl::FormatFeatures::Copyable;
        formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
    }
}

void initRenderbufferFormatProperties(snap::rhi::backend::opengl::Features& features,
                                      gl::APIVersion realApiVersion,
                                      const std::string& glExtensionsList) {
    features.renderbufferFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::Depth16Unorm)] = {
        snap::rhi::backend::opengl::SizedInternalFormat::DEPTH16};

    if (realApiVersion >= gl::APIVersion::GLES30) {
        features.renderbufferFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R8G8B8A8Unorm)] = {
            snap::rhi::backend::opengl::SizedInternalFormat::RGBA8};
        features.renderbufferFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R8G8B8Unorm)] = {
            snap::rhi::backend::opengl::SizedInternalFormat::RGB8};
        features.renderbufferFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R8Unorm)] = {
            snap::rhi::backend::opengl::SizedInternalFormat::R8}; // GL_R8_EXT; GL_EXT_texture_rg
        features.renderbufferFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R8G8Unorm)] = {
            snap::rhi::backend::opengl::SizedInternalFormat::RG8}; // GL_RG8_EXT; GL_EXT_texture_rg
        features.renderbufferFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::DepthFloat)] = {
            snap::rhi::backend::opengl::SizedInternalFormat::DEPTH24};
        features.renderbufferFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::DepthFloat)] = {
            snap::rhi::backend::opengl::SizedInternalFormat::DEPTH32F};
        features.renderbufferFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::DepthStencil)] = {
            snap::rhi::backend::opengl::SizedInternalFormat::DEPTH24_STENCIL8};
    } else {
        bool isRenderBufferRGBA8Supported =
            snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "GL_OES_rgb8_rgba8");
        bool isRGFormatSupported =
            snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "GL_EXT_texture_rg");
        bool isDepth24Supported = snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "GL_OES_depth24");
        bool isDepth32Supported = snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "GL_OES_depth32");
        bool isDepthStencilSupported =
            snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "GL_OES_packed_depth_stencil");

        if (isRenderBufferRGBA8Supported) {
            features.renderbufferFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R8G8B8A8Unorm)] = {
                snap::rhi::backend::opengl::SizedInternalFormat::RGBA8};
            features.renderbufferFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R8G8B8Unorm)] = {
                snap::rhi::backend::opengl::SizedInternalFormat::RGB8};
        } else {
            features.renderbufferFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R8G8B8A8Unorm)] = {
                snap::rhi::backend::opengl::SizedInternalFormat::RGBA4};
            features.renderbufferFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R8G8B8Unorm)] = {
                snap::rhi::backend::opengl::SizedInternalFormat::RGB565};
        }

        if (isRGFormatSupported) {
            features.renderbufferFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R8Unorm)] = {
                snap::rhi::backend::opengl::SizedInternalFormat::R8}; // GL_R8_EXT; GL_EXT_texture_rg
            features.renderbufferFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R8G8Unorm)] = {
                snap::rhi::backend::opengl::SizedInternalFormat::RG8}; // GL_RG8_EXT; GL_EXT_texture_rg
        }

        if (isDepth24Supported) {
            features.renderbufferFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::DepthFloat)] = {
                snap::rhi::backend::opengl::SizedInternalFormat::DEPTH24};
        }

        if (isDepth32Supported) {
            features.renderbufferFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::DepthFloat)] = {
                snap::rhi::backend::opengl::SizedInternalFormat::DEPTH32};
        }

        if (isDepthStencilSupported) {
            features.renderbufferFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::DepthStencil)] = {
                snap::rhi::backend::opengl::SizedInternalFormat::DEPTH24_STENCIL8};
        }
    }
}

std::vector<std::string> split(const std::string& str, const std::string& delim) {
    std::vector<std::string> result;
    size_t start = 0;

    for (size_t found = str.find_first_of(delim); found != std::string::npos; found = str.find_first_of(delim, start)) {
        result.emplace_back(str.begin() + start, str.begin() + found);
        start = found + 1;
    }
    if (start != str.size()) {
        result.emplace_back(str.begin() + start, str.end());
    }
    return result;
}
} // unnamed namespace

namespace snap::rhi::backend::opengl::es20 {
snap::rhi::backend::opengl::Features Instance::buildFeatures(gl::APIVersion realApiVersion) {
    snap::rhi::backend::opengl::Features features{};

    std::string glExtensionsList = (char*)glGetString(GL_EXTENSIONS);
    std::string eglExtensionsList = getEGLExt();

    {
        auto eglExtensions = split(eglExtensionsList, " \n");
        std::sort(eglExtensions.begin(), eglExtensions.end());
        SNAP_RHI_LOGI("SnapRHI OpenGL EGL extensions:");
        for (auto&& ext : eglExtensions) {
            SNAP_RHI_LOGI("\t%s", ext.c_str());
        }

        auto glExtensions = split(glExtensionsList, " ");
        std::sort(glExtensions.begin(), glExtensions.end());
        SNAP_RHI_LOGI("SnapRHI OpenGL ES extensions:");
        for (auto&& ext : glExtensions) {
            SNAP_RHI_LOGI("\t%s", ext.c_str());
        }
    }

    const bool isGLES30Supported = realApiVersion >= gl::APIVersion::GLES30;

    // ASTC is native for OpenGL ES 3.2 and above. Before that it is available via extension.
    features.isASTCCompressionFormatFamilySupported =
        realApiVersion >= gl::APIVersion::GLES32 ||
        snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "GL_KHR_texture_compression_astc_ldr");

    initTexFormatProperties(features, realApiVersion, glExtensionsList);
    initRenderbufferFormatProperties(features, realApiVersion, glExtensionsList);

    // GL_OES_element_index_uint
    // GL_OES_get_program_binary

    // https://docs.gl/es2/glFramebufferTexture2D: GLES20, level must be 0 unless the extension is supported.
    // https://docs.gl/es3/glFramebufferTexture2D: GLES30, non-0 level is always supported.

    features.isCustomTexturePackingSupported = false;
    features.isTextureParameterSupported = false;
    features.isFragmentPrimitiveIDSupported = false;
    features.apiVersion = gl::APIVersion::GLES20;
    features.isPBOSupported = false;
    features.isUInt32IndexSupported = false;
    features.isBlitFramebufferSupported = false;
    features.isCopyBufferSubDataSupported = false;
    features.isSamplerSupported = false;
    features.isVAOSupported = false;
    features.isDifferentBlendSettingsSupported = false;
    features.isMRTSupported = false;
    features.isStorageTextureSupported = false;
    features.isNativeUBOSupported = false;
    features.isSSBOSupported = false;
    features.isCopyImageSubDataSupported = false;
    /**
     * isAlphaToCoverageSupported supports GLES20,
     */
    features.isAlphaToCoverageSupported = false;

    // GL_DEPTH_STENCIL_ATTACHMENT is not supported by OES_packed_depth_stencil, only the GL_DEPTH_STENCIL format.
    features.isDepthStencilAttachmentSupported = isGLES30Supported;
    features.isOVRDepthStencilSupported = false;
    features.isRasterizationDisableSupported = false;
    features.isMinMaxBlendModeSupported =
        snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "GL_EXT_blend_minmax");
    features.isFBORenderToNonZeroMipSupported = isGLES30Supported || snap::rhi::backend::opengl::isExtensionSupported(
                                                                         glExtensionsList, "GL_OES_fbo_render_mipmap");
    features.isClampToBorderSupported =
        snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "GL_NV_texture_border_clamp") ||
        snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "GL_OES_texture_border_clamp") ||
        snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "GL_EXT_texture_border_clamp");
    features.isTexMinMaxLodSupported = false;
    features.isTexBaseMaxLevelSupported = false;
    features.isMapUnmapAvailable =
        false; // snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "GL_OES_mapbuffer");
    features.isNPOTGenMipmapSupported = false;
    features.isTexSwizzleSupported = false;
    features.isNPOTWrapModeSupported =
        isGLES30Supported || snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "GL_OES_texture_npot");
    features.isDepthCompareFuncSupported = false;
    features.isTexture3DSupported = false;    // GL_OES_texture_3D
    features.isTextureArraySupported = false; // GL_EXT_texture_array
    features.isInstancingSupported = false;
    features.isDepthCubemapSupported =
        snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "GL_OES_depth_texture_cube_map");
    features.isPrimitiveRestartIndexSupported = false;

    // GL_EXT_occlusion_query_boolean with doc
    // https://registry.khronos.org/OpenGL/extensions/EXT/EXT_occlusion_query_boolean.txt
    features.isOcclusionQuerySupported =
        snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "GL_EXT_occlusion_query_boolean");
    // EXT_disjoint_timer_query with doc https://registry.khronos.org/OpenGL/extensions/EXT/EXT_disjoint_timer_query.txt
    features.isDisjointQuerySupported =
        snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "GL_EXT_disjoint_timer_query");
    // comes with disjoint queries
    // this is notably not the case in non-ES GL
    features.isTimerQuerySupported = features.isDisjointQuerySupported;

    features.isANDROIDNativeFenceSyncSupported =
        snap::rhi::backend::opengl::isExtensionSupported(eglExtensionsList, "EGL_ANDROID_native_fence_sync");

    // GL_EXT_shader_framebuffer_fetch_non_coherent
    features.isFramebufferFetchSupported =
        snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "GL_EXT_shader_framebuffer_fetch") ||
        snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "GL_ARM_shader_framebuffer_fetch");
    features.isIntConstantRateSupported = false;
    //    features.isMultisampleSupported =   snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList,
    //    "GL_EXT_multisampled_render_to_texture") ||
    //                                        snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList,
    //                                        "GL_APPLE_framebuffer_multisample") ||
    //                                        snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList,
    //                                        "GL_EXT_framebuffer_multisample");
    features.isMultisampleSupported = false;
    features.isDiscardFramebufferSupported =
        snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "GL_EXT_discard_framebuffer");
    features.isFenceSupported =
        snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "GL_APPLE_sync") ||
        snap::rhi::backend::opengl::isExtensionSupported(eglExtensionsList, "EGL_KHR_fence_sync") ||
        snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "GL_OES_EGL_sync") ||
        snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "VG_KHR_EGL_sync");
    features.isSemaphoreSupported =
        snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "GL_APPLE_sync") ||
        snap::rhi::backend::opengl::isExtensionSupported(eglExtensionsList, "EGL_KHR_wait_sync");

    features.isProgramBinarySupported =
        snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "GL_OES_get_program_binary");
    if (features.isProgramBinarySupported) {
        GLint numFormats = 0;
        glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &numFormats);

        if (numFormats > 0) {
            features.isProgramBinarySupported = true;
        } else {
            features.isProgramBinarySupported = false;
        }
    }

    // OpenGL ES 2.0 supported all vertext attribute formats except HalfFloat.
    features.isVtxAttribSupported.fill(true);
    features.isVtxAttribSupported[static_cast<uint32_t>(snap::rhi::VertexAttributeFormat::HalfFloat2)] = false;
    features.isVtxAttribSupported[static_cast<uint32_t>(snap::rhi::VertexAttributeFormat::HalfFloat3)] = false;
    features.isVtxAttribSupported[static_cast<uint32_t>(snap::rhi::VertexAttributeFormat::HalfFloat4)] = false;

    features.maxOVRViews = 0;
    features.maxArrayTextureLayers = 1;
    features.maxTextureDimension3D = 1;

    features.maxFramebufferColorAttachmentCount = 1;
    features.maxDrawBuffers = 1;

    GLint maxClipDistances = 0;
    if (snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "GL_APPLE_clip_distance")) {
        glGetIntegerv(GL_MAX_CLIP_DISTANCES, &maxClipDistances);
    }
    features.maxClipDistances = maxClipDistances;
    features.maxCullDistances = 0;
    features.maxCombinedClipAndCullDistances = maxClipDistances;

    GLint maxVertexAttribs = 0;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxVertexAttribs);
    assert(maxVertexAttribs >= 8);

    features.maxVertexAttribs = static_cast<uint32_t>(maxVertexAttribs);

    GLint maxCombinedTextureImageUnits = 0;
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxCombinedTextureImageUnits);
    assert(maxCombinedTextureImageUnits >= 8);

    features.maxCombinedTextureImageUnits = static_cast<uint32_t>(maxCombinedTextureImageUnits);

    {
        GLint maxUniformComponents = 0;

        {
            glGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS, &maxUniformComponents);
            assert(maxUniformComponents >= 0);

            /**
             * The maximum number of four-element floating-point, integer, or boolean vectors
             * that can be held in uniform variable storage for a vertex shader.
             *
             * The value must be at least 128.
             * **/
            features.maxVertexUniformComponents = static_cast<uint32_t>(maxUniformComponents) << 2;
        }

        {
            glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_VECTORS, &maxUniformComponents);
            assert(maxUniformComponents >= 0);

            /**
             * The maximum number of four-element floating-point, integer, or boolean vectors
             * that can be held in uniform variable storage for a fragment shader.
             *
             * The value must be at least 16
             * */
            features.maxFragmentUniformComponents = static_cast<uint32_t>(maxUniformComponents) << 2;
        }
    }

    features.maxComputeWorkGroupInvocations = 0;

    features.maxComputeWorkGroupCount[0] = 0;
    features.maxComputeWorkGroupCount[1] = 0;
    features.maxComputeWorkGroupCount[2] = 0;

    features.maxComputeWorkGroupSize[0] = 0;
    features.maxComputeWorkGroupSize[1] = 0;
    features.maxComputeWorkGroupSize[2] = 0;

    // GL_EXT_texture_filter_anisotropic - anisotropic texture filtering
    // https://registry.khronos.org/OpenGL/extensions/EXT/EXT_texture_filter_anisotropic.txt
    features.isAnisotropicFilteringSupported =
        snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "GL_EXT_texture_filter_anisotropic");
    if (features.isAnisotropicFilteringSupported) {
        GLfloat maxAnisotropy = 1.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
        features.maxAnisotropy = maxAnisotropy;
    }

    features.shaderVersionHeader = "#version 100\n";

    return features;
}

void Instance::getInternalformativ(snap::rhi::backend::opengl::TextureTarget target,
                                   snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                                   GLenum pname,
                                   GLsizei bufSize,
                                   GLint* params) {
    // OpenGL ES 2.0 doesn't supported glGetInternalformativ
    // https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetInternalformativ.xhtml
    if (pname == GL_NUM_SAMPLE_COUNTS && bufSize) {
        *params = 1;
    }

    if (pname == GL_SAMPLES && bufSize) {
        *params = 0;
        // glGetIntegerv(GL_MAX_SAMPLES, params);
    }
}

void Instance::drawBuffers(GLsizei n, const snap::rhi::backend::opengl::FramebufferAttachmentTarget* bufs) {
    // OpenGL ES 2.0 doesn't supported MRT
}

void Instance::discardFramebuffer(snap::rhi::backend::opengl::FramebufferTarget target,
                                  GLsizei numAttachments,
                                  const snap::rhi::backend::opengl::FramebufferAttachmentTarget* attachments) {
#if SNAP_RHI_PLATFORM_WEBASSEMBLY()
    snap::rhi::common::throwException("discardFramebuffer unsupported for WebGL");
#else
    if (target != snap::rhi::backend::opengl::FramebufferTarget::Framebuffer) {
        // https://www.khronos.org/registry/OpenGL/extensions/EXT/EXT_discard_framebuffer.txt
        snap::rhi::common::throwException(std::string("[glDiscardFramebufferEXT] not supported, ignoring discard") +
                                          snap::rhi::backend::opengl::toString(target).data());
    }

    glDiscardFramebufferEXT(static_cast<GLenum>(target), numAttachments, reinterpret_cast<const GLenum*>(attachments));
#endif
}

void Instance::readBuffer(snap::rhi::backend::opengl::FramebufferAttachmentTarget mode) {
    // OpenGL ES 2.0 doesn't supported MRT
}

void Instance::blitFramebuffer(GLint srcX0,
                               GLint srcY0,
                               GLint srcX1,
                               GLint srcY1,
                               GLint dstX0,
                               GLint dstY0,
                               GLint dstX1,
                               GLint dstY1,
                               GLbitfield mask,
                               GLenum filter) {
    snap::rhi::common::throwException("OpenGL ES 2.0 doesn't supported blitFramebuffer");
}

void Instance::drawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei primcount) {
    snap::rhi::common::throwException("OpenGL ES 2.0 doesn't supported instancing");
}

void Instance::drawElementsInstanced(
    GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instancecount) {
    snap::rhi::common::throwException("OpenGL ES 2.0 doesn't supported instancing");
}

void Instance::vertexAttribDivisor(GLuint index, GLuint divisor) {
    if (divisor != 0) {
        snap::rhi::common::throwException("OpenGL ES 2.0 doesn't supported instancing");
    }
}

void Instance::getActiveUniformBlockName(
    GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei* length, GLchar* uniformBlockName) {
    snap::rhi::common::throwException("[GLES20] getActiveUniformBlockName doesn't supported");
}

GLuint Instance::getUniformBlockIndex(GLuint program, const GLchar* uniformBlockName) {
    snap::rhi::common::throwException("[GLES20] getUniformBlockIndex doesn't supported");
}

void Instance::getActiveUniformBlockiv(GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint* params) {
    snap::rhi::common::throwException("[GLES20] getActiveUniformBlockiv doesn't supported");
}

void Instance::getActiveUniformsiv(
    GLuint program, GLsizei uniformCount, const GLuint* uniformIndices, GLenum pname, GLint* params) {
    snap::rhi::common::throwException("[GLES20] getActiveUniformsiv doesn't supported");
}

void Instance::uniformBlockBinding(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding) {
    snap::rhi::common::throwException("[GLES20] uniformBlockBinding doesn't supported");
}

void Instance::uniform1uiv(GLint location, GLsizei count, const GLuint* value) {
    snap::rhi::common::throwException("[GLES20] uniform1uiv isn't supported");
}

void Instance::uniform2uiv(GLint location, GLsizei count, const GLuint* value) {
    snap::rhi::common::throwException("[GLES20] uniform2uiv isn't supported");
}

void Instance::uniform3uiv(GLint location, GLsizei count, const GLuint* value) {
    snap::rhi::common::throwException("[GLES20] uniform3uiv isn't supported");
}

void Instance::uniform4uiv(GLint location, GLsizei count, const GLuint* value) {
    snap::rhi::common::throwException("[GLES20] uniform4uiv isn't supported");
}

void Instance::getProgramInterfaceiv(GLuint program, GLenum programInterface, GLenum pname, GLint* params) {
    snap::rhi::common::throwException("[GLES20] getProgramInterfaceiv doesn't supported");
}

void Instance::bindImageTexture(
    GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format) {
    snap::rhi::common::throwException("[GLES20] bindImageTexture doesn't supported");
}

void Instance::getProgramResourceiv(GLuint program,
                                    GLenum programInterface,
                                    GLuint index,
                                    GLsizei propCount,
                                    const GLenum* props,
                                    GLsizei bufSize,
                                    GLsizei* length,
                                    GLint* params) {
    snap::rhi::common::throwException("[GLES20] bindImageTexture doesn't supported");
}

void Instance::getProgramResourceName(
    GLuint program, GLenum programInterface, GLuint index, GLsizei bufSize, GLsizei* length, char* name) {
    snap::rhi::common::throwException("[GLES20] getProgramResourceName doesn't supported");
}

GLuint Instance::getProgramResourceIndex(GLuint program, GLenum programInterface, const char* name) {
    snap::rhi::common::throwException("[GLES20] getProgramResourceIndex doesn't supported");
}

void Instance::memoryBarrierByRegion(GLbitfield barriers) {
    snap::rhi::common::throwException("[GLES20] memoryBarrierByRegion doesn't supported");
}

void Instance::flushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length) {
    snap::rhi::common::throwException("[GLES20] flushMappedBufferRange doesn't supported");
}

void Instance::texStorage2D(snap::rhi::backend::opengl::TextureTarget target,
                            GLsizei levels,
                            snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                            GLsizei width,
                            GLsizei height) {
    snap::rhi::common::throwException("[GLES20] texStorage2D doesn't supported");
}

void Instance::texStorage3D(snap::rhi::backend::opengl::TextureTarget target,
                            GLsizei levels,
                            snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                            GLsizei width,
                            GLsizei height,
                            GLsizei depth) {
    snap::rhi::common::throwException("[GLES20] texStorage3D doesn't supported");
}

void Instance::texStorage2DMultisample(snap::rhi::backend::opengl::TextureTarget target,
                                       GLsizei samples,
                                       snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                                       GLsizei width,
                                       GLsizei height,
                                       GLboolean fixedsamplelocations) {
    snap::rhi::common::throwException("[GLES20] texStorage2DMultisample doesn't supported");
}

void Instance::texStorage3DMultisample(snap::rhi::backend::opengl::TextureTarget target,
                                       GLsizei samples,
                                       snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                                       GLsizei width,
                                       GLsizei height,
                                       GLsizei depth,
                                       GLboolean fixedsamplelocations) {
    snap::rhi::common::throwException("[GLES20] texStorage3DMultisample doesn't supported");
}

void Instance::copyImageSubData(GLuint srcName,
                                GLenum srcTarget,
                                GLint srcLevel,
                                GLint srcX,
                                GLint srcY,
                                GLint srcZ,
                                GLuint dstName,
                                GLenum dstTarget,
                                GLint dstLevel,
                                GLint dstX,
                                GLint dstY,
                                GLint dstZ,
                                GLsizei srcWidth,
                                GLsizei srcHeight,
                                GLsizei srcDepth) {
    snap::rhi::common::throwException("[GLES20] copyImageSubData doesn't supported");
}

void Instance::genVertexArrays(GLsizei n, GLuint* arrays) {
    for (GLsizei i = 0; i < n; ++i) {
        arrays[i] = 0;
    }
}

void Instance::bindVertexArray(GLuint array) {
    assert(array == 0);
}

void Instance::deleteVertexArrays(GLsizei n, const GLuint* arrays) {}

void Instance::colorMaski(GLuint buf, GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
    snap::rhi::common::throwException("[GLES20] colorMaski doesn't supported");
}

void Instance::enablei(GLenum cap, GLuint index) {
    snap::rhi::common::throwException("[GLES20] enablei doesn't supported");
}

void Instance::disablei(GLenum cap, GLuint index) {
    snap::rhi::common::throwException("[GLES20] disablei doesn't supported");
}

void Instance::blendEquationSeparatei(GLuint buf, GLenum modeRGB, GLenum modeAlpha) {
    snap::rhi::common::throwException("[GLES20] blendEquationSeparatei doesn't supported");
}

void Instance::blendFuncSeparatei(GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) {
    snap::rhi::common::throwException("[GLES20] blendFuncSeparatei doesn't supported");
}

GLboolean Instance::isSampler(GLuint id) {
    snap::rhi::common::throwException("[GLES20] isSampler doesn't supported");
}

void Instance::samplerParameteri(GLuint sampler, GLenum pname, GLint param) {
    snap::rhi::common::throwException("[GLES20] samplerParameteri doesn't supported");
}

void Instance::samplerParameterfv(GLuint sampler, GLenum pname, const GLfloat* params) {
    snap::rhi::common::throwException("[GLES20] samplerParameterfv doesn't supported");
}

void Instance::samplerParameterf(GLuint sampler, GLenum pname, GLfloat param) {
    snap::rhi::common::throwException("[GLES20] samplerParameterf doesn't supported");
}

void Instance::genSamplers(GLsizei n, GLuint* samplers) {
    snap::rhi::common::throwException("[GLES20] genSamplers doesn't supported");
}

void Instance::deleteSamplers(GLsizei n, const GLuint* samplers) {
    snap::rhi::common::throwException("[GLES20] deleteSamplers doesn't supported");
}

void Instance::bindSampler(GLuint unit, GLuint sampler) {
    snap::rhi::common::throwException("[GLES20] bindSampler doesn't supported");
}

void Instance::framebufferTextureLayer(snap::rhi::backend::opengl::FramebufferTarget target,
                                       snap::rhi::backend::opengl::FramebufferAttachmentTarget attachment,
                                       snap::rhi::backend::opengl::TextureId texture,
                                       int32_t level,
                                       int32_t layer) {
    snap::rhi::common::throwException("[GLES20] framebufferTextureLayer doesn't supported");
}

void Instance::framebufferTextureMultiviewOVR(snap::rhi::backend::opengl::FramebufferTarget target,
                                              snap::rhi::backend::opengl::FramebufferAttachmentTarget attachment,
                                              snap::rhi::backend::opengl::TextureId texture,
                                              int32_t level,
                                              int32_t baseViewIndex,
                                              int32_t numViews) {
    snap::rhi::common::throwException("[GLES20] framebufferTextureMultiviewOVR doesn't supported");
}

void Instance::framebufferTextureMultisampleMultiviewOVR(
    snap::rhi::backend::opengl::FramebufferTarget target,
    snap::rhi::backend::opengl::FramebufferAttachmentTarget attachment,
    snap::rhi::backend::opengl::TextureId texture,
    int32_t level,
    int32_t samples,
    int32_t baseViewIndex,
    int32_t numViews) {
    snap::rhi::common::throwException("[GLES20] framebufferTextureMultisampleMultiviewOVR doesn't supported");
}

void Instance::clearBufferiv(GLenum buffer, GLint drawBuffer, const GLint* value) {
    snap::rhi::common::throwException("[GLES20] clearBufferiv doesn't supported");
}

void Instance::clearBufferuiv(GLenum buffer, GLint drawBuffer, const GLuint* value) {
    snap::rhi::common::throwException("[GLES20] clearBufferuiv doesn't supported");
}

void Instance::clearBufferfv(GLenum buffer, GLint drawBuffer, const GLfloat* value) {
    snap::rhi::common::throwException("[GLES20] clearBufferfv doesn't supported");
}

void Instance::clearBufferfi(GLenum buffer, GLint drawBuffer, GLfloat depth, GLint stencil) {
    snap::rhi::common::throwException("[GLES20] clearBufferfi doesn't supported");
}

void Instance::bindBufferRange(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size) {
    snap::rhi::common::throwException("[GLES20] bindBufferRange doesn't supported");
}

void Instance::vertexAttribI4iv(GLuint index, const GLint* v) {
    snap::rhi::common::throwException("[GLES20] vertexAttribI4iv doesn't supported");
}

void Instance::vertexAttribI4uiv(GLuint index, const GLuint* v) {
    snap::rhi::common::throwException("[GLES20] vertexAttribI4uiv doesn't supported");
}

void Instance::copyBufferSubData(
    GLenum readtarget, GLenum writetarget, GLintptr readoffset, GLintptr writeoffset, GLsizeiptr size) {
    snap::rhi::common::throwException("[GLES20] copyBufferSubData doesn't supported");
}

void Instance::dispatchCompute(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z) {
    snap::rhi::common::throwException("[GLES20] dispatchCompute doesn't supported");
}

void Instance::hint(GLenum target, GLenum mode) {
    glHint(target, mode);
}

void Instance::getTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint* params) {
    snap::rhi::common::throwException("[GLES20] glGetTexLevelParameteriv doesn't supported");
}

void Instance::getTexParameteriv(GLenum target, GLenum pname, GLint* params) {
    if (pname == GL_TEXTURE_IMMUTABLE_FORMAT) {
        *params = GL_FALSE;
        return;
    }

    snap::rhi::common::throwException("[GLES20] getTexParameteriv doesn't supported");
}

void Instance::pixelStorei(GLenum pname, GLint param) {
    if (pname == GL_PACK_ROW_LENGTH || pname == GL_UNPACK_ROW_LENGTH || pname == GL_UNPACK_IMAGE_HEIGHT) {
        return;
    }

    glPixelStorei(pname, param);
}

void Instance::programParameteri(GLuint program, GLenum pname, GLint value) {
    // Do nothing since OpenGL ES 2.0 with OES_get_program_binary extension doesn't support it
}

void Instance::programBinary(GLuint program, GLenum binaryFormat, const void* binary, GLsizei length) {
#if SNAP_RHI_OS_ANDROID()
    assert(glProgramBinaryOES);
    glProgramBinaryOES(program, binaryFormat, binary, length);
#endif // SNAP_RHI_OS_ANDROID()
}

void Instance::getProgramBinary(GLuint program, GLsizei bufsize, GLsizei* length, GLenum* binaryFormat, void* binary) {
#if SNAP_RHI_OS_ANDROID()
    assert(glGetProgramBinaryOES);
    glGetProgramBinaryOES(program, bufsize, length, binaryFormat, binary);
#endif // SNAP_RHI_OS_ANDROID()
}

void Instance::copyTexSubImage3D(GLenum target,
                                 GLint level,
                                 GLint xoffset,
                                 GLint yoffset,
                                 GLint zoffset,
                                 GLint x,
                                 GLint y,
                                 GLsizei width,
                                 GLsizei height) {
    snap::rhi::common::throwException("[GLES20] copyTexSubImage3D not supported");
}

void Instance::genQueries(GLsizei n, GLuint* ids) {
    glGenQueriesEXT(n, ids);
}

void Instance::deleteQueries(GLsizei n, const GLuint* ids) {
    glDeleteQueriesEXT(n, ids);
}

GLboolean Instance::isQuery(GLuint id) {
    return glIsQueryEXT(id);
}

void Instance::beginQuery(GLenum target, GLuint id) {
    glBeginQueryEXT(target, id);
}

void Instance::endQuery(GLenum target) {
    glEndQueryEXT(target);
}

void Instance::getQueryiv(GLenum target, GLenum pname, GLint* params) {
    glGetQueryivEXT(target, pname, params);
}

void Instance::getQueryObjectuiv(GLuint id, GLenum pname, GLuint* params) {
    glGetQueryObjectuivEXT(id, pname, params);
}

} // namespace snap::rhi::backend::opengl::es20
#endif // SNAP_RHI_GLES20
