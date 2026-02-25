//
//  Instance.cpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 6/29/22.
//

#include "Instance.hpp"

#include "snap/rhi/common/Throw.h"
#include <vector>

#if SNAP_RHI_GL21
namespace {
void initTexFormatProperties(snap::rhi::backend::opengl::Features& features, const std::string& glExtensionsList) {
    bool ARB_texture_rg = snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "ARB_texture_rg");
    if (ARB_texture_rg) {
        { // snap::rhi::PixelFormat::R8Unorm
            features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R8Unorm)] = {
                snap::rhi::backend::opengl::SizedInternalFormat::R8,
                snap::rhi::backend::opengl::FormatGroup::R,
                snap::rhi::backend::opengl::FormatDataType::UNSIGNED_BYTE};

            auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R8Unorm)];
            formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
            formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
        }

        { // snap::rhi::PixelFormat::R8G8Unorm
            features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R8G8Unorm)] = {
                snap::rhi::backend::opengl::SizedInternalFormat::RG8,
                snap::rhi::backend::opengl::FormatGroup::RG,
                snap::rhi::backend::opengl::FormatDataType::UNSIGNED_BYTE};

            auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R8G8Unorm)];
            formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
            formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
        }
    }

    { // snap::rhi::PixelFormat::R8G8B8Unorm
        features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R8G8B8Unorm)] = {
            snap::rhi::backend::opengl::SizedInternalFormat::RGB8,
            snap::rhi::backend::opengl::FormatGroup::RGB,
            snap::rhi::backend::opengl::FormatDataType::UNSIGNED_BYTE};

        auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R8G8B8Unorm)];
        formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
        formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
    }

    { // snap::rhi::PixelFormat::R8G8B8A8Unorm
        features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R8G8B8A8Unorm)] = {
            snap::rhi::backend::opengl::SizedInternalFormat::RGBA8,
            snap::rhi::backend::opengl::FormatGroup::RGBA,
            snap::rhi::backend::opengl::FormatDataType::UNSIGNED_BYTE};

        auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R8G8B8A8Unorm)];
        formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
        formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
    }

    if (ARB_texture_rg) {
        { // snap::rhi::PixelFormat::R16Unorm
            features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R16Unorm)] = {
                snap::rhi::backend::opengl::SizedInternalFormat::R16,
                snap::rhi::backend::opengl::FormatGroup::R,
                snap::rhi::backend::opengl::FormatDataType::UNSIGNED_SHORT};

            auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R16Unorm)];
            formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
            formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
        }

        { // snap::rhi::PixelFormat::R16G16Unorm
            features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R16G16Unorm)] = {
                snap::rhi::backend::opengl::SizedInternalFormat::RG16,
                snap::rhi::backend::opengl::FormatGroup::RG,
                snap::rhi::backend::opengl::FormatDataType::UNSIGNED_SHORT};

            auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R16G16Unorm)];
            formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
            formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
        }
    }

    { // snap::rhi::PixelFormat::R16G16B16A16Unorm
        features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R16G16B16A16Unorm)] = {
            snap::rhi::backend::opengl::SizedInternalFormat::RGBA16,
            snap::rhi::backend::opengl::FormatGroup::RGBA,
            snap::rhi::backend::opengl::FormatDataType::UNSIGNED_SHORT};

        auto& formatInfo =
            features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R16G16B16A16Unorm)];
        formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
        formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
    }

    bool EXT_texture_integer =
        snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "EXT_texture_integer");
    if (EXT_texture_integer) {
        if (ARB_texture_rg) {
            { // snap::rhi::PixelFormat::R8Uint
                features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R8Uint)] = {
                    snap::rhi::backend::opengl::SizedInternalFormat::R8UI,
                    snap::rhi::backend::opengl::FormatGroup::R_INTEGER,
                    snap::rhi::backend::opengl::FormatDataType::UNSIGNED_BYTE};

                auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R8Uint)];
                formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
                formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestOnly;
            }

            { // snap::rhi::PixelFormat::R8G8Uint
                features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R8G8Uint)] = {
                    snap::rhi::backend::opengl::SizedInternalFormat::RG8UI,
                    snap::rhi::backend::opengl::FormatGroup::RG_INTEGER,
                    snap::rhi::backend::opengl::FormatDataType::UNSIGNED_BYTE};

                auto& formatInfo =
                    features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R8G8Uint)];
                formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
                formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestOnly;
            }
        }

        { // snap::rhi::PixelFormat::R8G8B8A8Uint
            features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R8G8B8A8Uint)] = {
                snap::rhi::backend::opengl::SizedInternalFormat::RGBA8UI,
                snap::rhi::backend::opengl::FormatGroup::RGBA_INTEGER,
                snap::rhi::backend::opengl::FormatDataType::UNSIGNED_BYTE};

            auto& formatInfo =
                features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R8G8B8A8Uint)];
            formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
            formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestOnly;
        }

        if (ARB_texture_rg) {
            { // snap::rhi::PixelFormat::R16Uint
                features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R16Uint)] = {
                    snap::rhi::backend::opengl::SizedInternalFormat::R16UI,
                    snap::rhi::backend::opengl::FormatGroup::R_INTEGER,
                    snap::rhi::backend::opengl::FormatDataType::UNSIGNED_SHORT};

                auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R16Uint)];
                formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
                formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestOnly;
            }

            { // snap::rhi::PixelFormat::R16G16Uint
                features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R16G16Uint)] = {
                    snap::rhi::backend::opengl::SizedInternalFormat::RG16UI,
                    snap::rhi::backend::opengl::FormatGroup::RG_INTEGER,
                    snap::rhi::backend::opengl::FormatDataType::UNSIGNED_SHORT};

                auto& formatInfo =
                    features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R16G16Uint)];
                formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
                formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestOnly;
            }
        }

        { // snap::rhi::PixelFormat::R16G16B16A16Uint
            features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R16G16B16A16Uint)] = {
                snap::rhi::backend::opengl::SizedInternalFormat::RGBA16UI,
                snap::rhi::backend::opengl::FormatGroup::RGBA_INTEGER,
                snap::rhi::backend::opengl::FormatDataType::UNSIGNED_SHORT};

            auto& formatInfo =
                features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R16G16B16A16Uint)];
            formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
            formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestOnly;
        }

        if (ARB_texture_rg) {
            { // snap::rhi::PixelFormat::R32Uint
                features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R32Uint)] = {
                    snap::rhi::backend::opengl::SizedInternalFormat::R32UI,
                    snap::rhi::backend::opengl::FormatGroup::R_INTEGER,
                    snap::rhi::backend::opengl::FormatDataType::UNSIGNED_INT};

                auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R32Uint)];
                formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
                formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestOnly;
            }

            { // snap::rhi::PixelFormat::R32G32Uint
                features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R32G32Uint)] = {
                    snap::rhi::backend::opengl::SizedInternalFormat::RG32UI,
                    snap::rhi::backend::opengl::FormatGroup::RG_INTEGER,
                    snap::rhi::backend::opengl::FormatDataType::UNSIGNED_INT};

                auto& formatInfo =
                    features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R32G32Uint)];
                formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
                formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestOnly;
            }
        }

        { // snap::rhi::PixelFormat::R32G32B32A32Uint
            features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R32G32B32A32Uint)] = {
                snap::rhi::backend::opengl::SizedInternalFormat::RGBA32UI,
                snap::rhi::backend::opengl::FormatGroup::RGBA_INTEGER,
                snap::rhi::backend::opengl::FormatDataType::UNSIGNED_INT};

            auto& formatInfo =
                features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R32G32B32A32Uint)];
            formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
            formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestOnly;
        }

        if (ARB_texture_rg) {
            { // snap::rhi::PixelFormat::R8Sint
                features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R8Sint)] = {
                    snap::rhi::backend::opengl::SizedInternalFormat::R8I,
                    snap::rhi::backend::opengl::FormatGroup::R_INTEGER,
                    snap::rhi::backend::opengl::FormatDataType::BYTE};

                auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R8Sint)];
                formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
                formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestOnly;
            }

            { // snap::rhi::PixelFormat::R8G8Sint
                features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R8G8Sint)] = {
                    snap::rhi::backend::opengl::SizedInternalFormat::RG8I,
                    snap::rhi::backend::opengl::FormatGroup::RG_INTEGER,
                    snap::rhi::backend::opengl::FormatDataType::BYTE};

                auto& formatInfo =
                    features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R8G8Sint)];
                formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
                formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestOnly;
            }
        }

        { // snap::rhi::PixelFormat::R8G8B8A8Sint
            features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R8G8B8A8Sint)] = {
                snap::rhi::backend::opengl::SizedInternalFormat::RGBA8I,
                snap::rhi::backend::opengl::FormatGroup::RGBA_INTEGER,
                snap::rhi::backend::opengl::FormatDataType::BYTE};

            auto& formatInfo =
                features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R8G8B8A8Sint)];
            formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
            formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestOnly;
        }

        if (ARB_texture_rg) {
            { // snap::rhi::PixelFormat::R16Sint
                features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R16Sint)] = {
                    snap::rhi::backend::opengl::SizedInternalFormat::R16I,
                    snap::rhi::backend::opengl::FormatGroup::R_INTEGER,
                    snap::rhi::backend::opengl::FormatDataType::SHORT};

                auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R16Sint)];
                formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
                formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestOnly;
            }

            { // snap::rhi::PixelFormat::R16G16Sint
                features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R16G16Sint)] = {
                    snap::rhi::backend::opengl::SizedInternalFormat::RG16I,
                    snap::rhi::backend::opengl::FormatGroup::RG_INTEGER,
                    snap::rhi::backend::opengl::FormatDataType::SHORT};

                auto& formatInfo =
                    features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R16G16Sint)];
                formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
                formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestOnly;
            }
        }

        { // snap::rhi::PixelFormat::R16G16B16A16Sint
            features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R16G16B16A16Sint)] = {
                snap::rhi::backend::opengl::SizedInternalFormat::RGBA16I,
                snap::rhi::backend::opengl::FormatGroup::RGBA_INTEGER,
                snap::rhi::backend::opengl::FormatDataType::SHORT};

            auto& formatInfo =
                features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R16G16B16A16Sint)];
            formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
            formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestOnly;
        }

        if (ARB_texture_rg) {
            { // snap::rhi::PixelFormat::R32Sint
                features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R32Sint)] = {
                    snap::rhi::backend::opengl::SizedInternalFormat::R32I,
                    snap::rhi::backend::opengl::FormatGroup::R_INTEGER,
                    snap::rhi::backend::opengl::FormatDataType::INT};

                auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R32Sint)];
                formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
                formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestOnly;
            }

            { // snap::rhi::PixelFormat::R32G32Sint
                features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R32G32Sint)] = {
                    snap::rhi::backend::opengl::SizedInternalFormat::RG32I,
                    snap::rhi::backend::opengl::FormatGroup::RG_INTEGER,
                    snap::rhi::backend::opengl::FormatDataType::INT};

                auto& formatInfo =
                    features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R32G32Sint)];
                formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
                formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestOnly;
            }
        }

        { // snap::rhi::PixelFormat::R32G32B32A32Sint
            features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R32G32B32A32Sint)] = {
                snap::rhi::backend::opengl::SizedInternalFormat::RGBA32I,
                snap::rhi::backend::opengl::FormatGroup::RGBA_INTEGER,
                snap::rhi::backend::opengl::FormatDataType::INT};

            auto& formatInfo =
                features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R32G32B32A32Sint)];
            formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
            formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestOnly;
        }
    }

    bool ARB_texture_float = snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "ARB_texture_float");
    if (ARB_texture_float) {
        if (ARB_texture_rg) {
            { // snap::rhi::PixelFormat::R16Float
                features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R16Float)] = {
                    snap::rhi::backend::opengl::SizedInternalFormat::R16F,
                    snap::rhi::backend::opengl::FormatGroup::R,
                    snap::rhi::backend::opengl::FormatDataType::HALF_FLOAT};

                auto& formatInfo =
                    features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R16Float)];
                formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
                formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
            }

            { // snap::rhi::PixelFormat::R16G16Float
                features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R16G16Float)] = {
                    snap::rhi::backend::opengl::SizedInternalFormat::RG16F,
                    snap::rhi::backend::opengl::FormatGroup::RG,
                    snap::rhi::backend::opengl::FormatDataType::HALF_FLOAT};

                auto& formatInfo =
                    features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R16G16Float)];
                formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
                formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
            }
        }

        { // snap::rhi::PixelFormat::R16G16B16A16Float
            features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R16G16B16A16Float)] = {
                snap::rhi::backend::opengl::SizedInternalFormat::RGBA16F,
                snap::rhi::backend::opengl::FormatGroup::RGBA,
                snap::rhi::backend::opengl::FormatDataType::HALF_FLOAT};

            auto& formatInfo =
                features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R16G16B16A16Float)];
            formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
            formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
        }

        if (ARB_texture_rg) {
            { // snap::rhi::PixelFormat::R32Float
                features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R32Float)] = {
                    snap::rhi::backend::opengl::SizedInternalFormat::R32F,
                    snap::rhi::backend::opengl::FormatGroup::R,
                    snap::rhi::backend::opengl::FormatDataType::FLOAT};

                auto& formatInfo =
                    features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R32Float)];
                formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
                formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestOnly;
            }

            { // snap::rhi::PixelFormat::R32G32Float
                features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R32G32Float)] = {
                    snap::rhi::backend::opengl::SizedInternalFormat::RG32F,
                    snap::rhi::backend::opengl::FormatGroup::RG,
                    snap::rhi::backend::opengl::FormatDataType::FLOAT};

                auto& formatInfo =
                    features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R32G32Float)];
                formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
                formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestOnly;
            }
        }

        { // snap::rhi::PixelFormat::R32G32B32A32Float
            features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R32G32B32A32Float)] = {
                snap::rhi::backend::opengl::SizedInternalFormat::RGBA32F,
                snap::rhi::backend::opengl::FormatGroup::RGBA,
                snap::rhi::backend::opengl::FormatDataType::FLOAT};

            auto& formatInfo =
                features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R32G32B32A32Float)];
            formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
            formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestOnly;
        }
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

    bool EXT_bgra = snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "GL_EXT_bgra");
    if (EXT_bgra) {
        {
            features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::B8G8R8A8Unorm)] = {
                snap::rhi::backend::opengl::SizedInternalFormat::DEPRECATED_RGBA,
                snap::rhi::backend::opengl::FormatGroup::BGRA,
                snap::rhi::backend::opengl::FormatDataType::UNSIGNED_BYTE};

            auto& formatInfo =
                features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::B8G8R8A8Unorm)];
            formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
            formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
        }
    }

    bool EXT_texture_sRGB = snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "EXT_texture_sRGB");
    if (EXT_texture_sRGB) {
        { // snap::rhi::PixelFormat::R8G8B8A8Srgb
            features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R8G8B8A8Srgb)] = {
                snap::rhi::backend::opengl::SizedInternalFormat::SRGB8_ALPHA8,
                snap::rhi::backend::opengl::FormatGroup::RGBA,
                snap::rhi::backend::opengl::FormatDataType::UNSIGNED_BYTE};

            auto& formatInfo =
                features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R8G8B8A8Srgb)];
            formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
            formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
        }
    }

    { // snap::rhi::PixelFormat::R10G10B10A2Unorm
        features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R10G10B10A2Unorm)] = {
            snap::rhi::backend::opengl::SizedInternalFormat::RGB10_A2,
            snap::rhi::backend::opengl::FormatGroup::RGBA,
            snap::rhi::backend::opengl::FormatDataType::UNSIGNED_INT_2_10_10_10_REV};

        auto& formatInfo =
            features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R10G10B10A2Unorm)];
        formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
        formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
    }

    bool EXT_packed_float = snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "EXT_packed_float");
    if (EXT_packed_float) {
        { // snap::rhi::PixelFormat::R11G11B10Float
            features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R11G11B10Float)] = {
                snap::rhi::backend::opengl::SizedInternalFormat::R11F_G11F_B10F,
                snap::rhi::backend::opengl::FormatGroup::RGB,
                snap::rhi::backend::opengl::FormatDataType::UNSIGNED_INT_10F_11F_11F_REV};

            auto& formatInfo =
                features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R11G11B10Float)];
            formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
            formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
        }
    }

    { // snap::rhi::PixelFormat::Depth16Unorm
        features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::Depth16Unorm)] = {
            snap::rhi::backend::opengl::SizedInternalFormat::DEPRECATED_DEPTH,
            snap::rhi::backend::opengl::FormatGroup::DEPTH,
            snap::rhi::backend::opengl::FormatDataType::UNSIGNED_SHORT};

        auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::Depth16Unorm)];
        formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::Renderable;
        formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::None;
    }

    { // snap::rhi::PixelFormat::DepthFloat
        features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::DepthFloat)] = {
            snap::rhi::backend::opengl::SizedInternalFormat::DEPRECATED_DEPTH,
            snap::rhi::backend::opengl::FormatGroup::DEPTH,
            snap::rhi::backend::opengl::FormatDataType::FLOAT};

        auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::DepthFloat)];
        formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::Renderable;
        formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::None;
    }

    { // snap::rhi::PixelFormat::DepthStencil
        features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::DepthStencil)] = {
            snap::rhi::backend::opengl::SizedInternalFormat::DEPTH24_STENCIL8,
            snap::rhi::backend::opengl::FormatGroup::DEPTH_STENCIL,
            snap::rhi::backend::opengl::FormatDataType::UNSIGNED_INT_24_8};

        auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::DepthStencil)];
        formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::Renderable;
        formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::None;
    }

    snap::rhi::backend::opengl::addCompressedFormat(features);
}

void initRenderbufferFormatProperties(snap::rhi::backend::opengl::Features& features) {
    for (auto format = 0u; format < static_cast<uint32_t>(snap::rhi::PixelFormat::Count); ++format) {
        const auto& formatInfo = features.textureFormatOpInfo[format];
        if ((formatInfo.features & snap::rhi::backend::opengl::FormatFeatures::Renderable) !=
            snap::rhi::backend::opengl::FormatFeatures::None) {
            const auto& textureFormat = features.textureFormat[format];
            features.renderbufferFormat[format] = {textureFormat.internalFormat};
        }
    }
}
} // unnamed namespace

namespace snap::rhi::backend::opengl21 {
snap::rhi::backend::opengl::Features Instance::buildFeatures() {
    snap::rhi::backend::opengl::Features features{};

    std::string glExtensionsList;
    // OpenGL 2.1 and below use glGetString(GL_EXTENSIONS) to get the list of extensions.
    // OpenGL 3.0 and above use glGetStringi(GL_EXTENSIONS, i) to get the list of extensions.
    auto version = snap::rhi::backend::opengl::computeVersion(reinterpret_cast<const char*>(glGetString(GL_VERSION)));
    if (version <= gl::APIVersion::GL21) {
        auto glExtensions = (const char*)glGetString(GL_EXTENSIONS);
        if (glExtensions && glGetError() == GL_NO_ERROR) {
            glExtensionsList = glExtensions;
        }
    } else {
        GLint glExtensionsCount = 0;
        glGetIntegerv(GL_NUM_EXTENSIONS, &glExtensionsCount);
        if (glGetError() == GL_NO_ERROR) {
            for (int i = 0; i < glExtensionsCount; ++i) {
                auto extension = (const char*)glGetStringi(GL_EXTENSIONS, i);
                if (extension && glGetError() == GL_NO_ERROR) {
                    glExtensionsList += extension;
                    if (i < glExtensionsCount - 1) {
                        glExtensionsList += " ";
                    }
                }
            }
        }
    }

    features.isS3TCCompressionFormatFamilySupported =
        snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "EXT_texture_compression_s3tc") &&
        snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "EXT_texture_sRGB");

    features.isASTCCompressionFormatFamilySupported =
        snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "GL_KHR_texture_compression_astc_ldr");

    initTexFormatProperties(features, glExtensionsList);
    initRenderbufferFormatProperties(features);

    features.apiVersion = gl::APIVersion::GL21;
    features.isTextureParameterSupported = true;
    features.isFragmentPrimitiveIDSupported = false;
    features.isPBOSupported = false;
    features.isUInt32IndexSupported = true;
    features.isBlitFramebufferSupported = false;
    features.isCopyBufferSubDataSupported = false;
    features.isSamplerSupported = false;
    features.isVAOSupported = false;
    features.isDifferentBlendSettingsSupported = false;
    features.isMRTSupported = true;
    features.isStorageTextureSupported = false;
    features.isNativeUBOSupported = false;
    features.isSSBOSupported = false;
    features.isDepthStencilAttachmentSupported = true;
    features.isOVRDepthStencilSupported = false;
    features.isRasterizationDisableSupported = false;
    features.isMinMaxBlendModeSupported = true;
    features.isFBORenderToNonZeroMipSupported = true;
    features.isClampToBorderSupported = true;
    features.isTexMinMaxLodSupported = true;
    features.isTexBaseMaxLevelSupported = true;
    features.isMapUnmapAvailable =
        false; // https://registry.khronos.org/OpenGL-Refpages/gl4/html/glMapBufferRange.xhtml
    features.isNPOTGenMipmapSupported = true;
    features.isTexSwizzleSupported = false;
    features.isAlphaToCoverageSupported = true;
    features.isNPOTWrapModeSupported = true;
    features.isDepthCompareFuncSupported = false;
    features.isTexture3DSupported = true;
    features.isTextureArraySupported = true;
    features.isInstancingSupported = true;
    features.isDepthCubemapSupported = false;
    features.isPrimitiveRestartIndexSupported = false;
    features.isFramebufferFetchSupported = false;
    features.isIntConstantRateSupported = false;
    features.isMultisampleSupported = true;
    features.isDiscardFramebufferSupported = false;
    features.isFenceSupported = true;
    features.isSemaphoreSupported = true;
    features.isCustomTexturePackingSupported = true;
    features.isProgramBinarySupported = false;
    features.isCopyImageSubDataSupported = false;
    features.isOcclusionQuerySupported = true;
    features.isDisjointQuerySupported = false;
    features.isTimerQuerySupported = false;

    // OpenGL 2.1 supported all vertext attribute formats except HalfFloat.
    features.isVtxAttribSupported.fill(true);
    features.isVtxAttribSupported[static_cast<uint32_t>(snap::rhi::VertexAttributeFormat::HalfFloat2)] = false;
    features.isVtxAttribSupported[static_cast<uint32_t>(snap::rhi::VertexAttributeFormat::HalfFloat3)] = false;
    features.isVtxAttribSupported[static_cast<uint32_t>(snap::rhi::VertexAttributeFormat::HalfFloat4)] = false;

    features.maxOVRViews = 0;
    features.maxArrayTextureLayers = 1;
    features.maxTextureDimension3D = 1;
    features.maxClientWaitTimeout = 1e9;

    GLint maxFramebufferColorAttachmentCount = 0;
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxFramebufferColorAttachmentCount);
    assert(maxFramebufferColorAttachmentCount >= 1);

    features.maxFramebufferColorAttachmentCount = std::max(1, maxFramebufferColorAttachmentCount);

    GLint maxDrawBuffers = 0;
    glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);
    assert(maxDrawBuffers >= 1);

    features.maxDrawBuffers = std::max(1, maxDrawBuffers);

    GLint maxClipDistances = 0;
    glGetIntegerv(GL_MAX_CLIP_DISTANCES, &maxClipDistances);

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
            glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &maxUniformComponents);
            assert(maxUniformComponents >= 0);

            features.maxVertexUniformComponents = static_cast<uint32_t>(maxUniformComponents);
        }

        {
            glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &maxUniformComponents);
            assert(maxUniformComponents >= 0);

            features.maxFragmentUniformComponents = static_cast<uint32_t>(maxUniformComponents);
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

    // https://en.wikipedia.org/wiki/OpenGL_Shading_Language
    features.shaderVersionHeader = "#version 120\n";

    return features;
}

/**
 * @brief Maps a range of the buffer object's data store into the client's address space.
 *
 * @details
 * This function is intended to emulate the behavior of `glMapBufferRange` (OpenGL 3.0+).
 * However, due to the constraints of the macOS Legacy Profile (`NSOpenGLProfileVersionLegacy`),
 * the native `glMapBufferRange` entry point is not available.
 *
 * Platform Specific Implementation Details (macOS Legacy):
 *
 * Since the legacy profile is strictly capped at OpenGL 2.1, this implementation falls back
 * to using `glMapBuffer` to map the entire buffer, rather than a specific range.
 *
 * - Range access: The returned pointer is offset manually by the implementation to match
 * the requested `offset`.
 *
 * @param target The target buffer object (e.g., `GL_ARRAY_BUFFER`).
 * @param offset The starting offset within the buffer of the range to be mapped.
 * @param length The length of the range to be mapped.
 * @param access A bitfield containing access flags (e.g., `GL_MAP_WRITE_BIT`).
 * Note: Specific flags may be ignored or emulated in Legacy contexts.
 *
 * @return A pointer to the mapped range, or NULL if the operation failed.
 *
 * @see https://developer.apple.com/documentation/appkit/nsopenglprofileversionlegacy
 * (Confirmation that Legacy Profile is pre-OpenGL 3.0)
 * @see https://registry.khronos.org/OpenGL/extensions/ARB/ARB_map_buffer_range.txt
 * (Specification showing glMapBufferRange requires OpenGL 3.0 / ARB extension)
 * @see
 * https://developer.apple.com/library/archive/documentation/GraphicsImaging/Conceptual/OpenGL-MacProgGuide/UpdatinganApplicationtoSupportOpenGL3/UpdatinganApplicationtoSupportOpenGL3.html
 * (Apple Migration Guide listing map_buffer_range as a Core Profile feature)
 */
void* Instance::mapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access) {
    GLenum mapBufferAccess = 0;
    if (access & GL_MAP_READ_BIT && access & GL_MAP_WRITE_BIT) {
        mapBufferAccess = GL_READ_WRITE;
    } else if (access & GL_MAP_READ_BIT) {
        mapBufferAccess = GL_READ_ONLY;
    } else if (access & GL_MAP_WRITE_BIT) {
        mapBufferAccess = GL_WRITE_ONLY;
    } else {
        snap::rhi::common::throwException("[GL21] Invalid access flags for mapBufferRange");
    }
    auto mappedPtr = static_cast<std::byte*>(glMapBuffer(target, mapBufferAccess));
    if (!mappedPtr) {
        return nullptr;
    }
    auto result = mappedPtr + offset;
    return result;
}

void Instance::flushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length) {
    glFlushMappedBufferRange(target, offset, length);
}

GLboolean Instance::unmapBuffer(GLenum target) {
    auto result = glUnmapBuffer(target);
    return result;
}

void Instance::texImage3D(snap::rhi::backend::opengl::TextureTarget target,
                          GLint level,
                          snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                          GLsizei width,
                          GLsizei height,
                          GLsizei depth,
                          GLint border,
                          snap::rhi::backend::opengl::FormatGroup format,
                          snap::rhi::backend::opengl::FormatDataType type,
                          const void* data) {
    glTexImage3D(static_cast<GLenum>(target),
                 level,
                 static_cast<GLenum>(internalformat),
                 width,
                 height,
                 depth,
                 border,
                 static_cast<GLenum>(format),
                 static_cast<GLenum>(type),
                 data);
}

void Instance::texSubImage3D(snap::rhi::backend::opengl::TextureTarget target,
                             GLint level,
                             GLint xoffset,
                             GLint yoffset,
                             GLint zoffset,
                             GLsizei width,
                             GLsizei height,
                             GLsizei depth,
                             snap::rhi::backend::opengl::FormatGroup format,
                             snap::rhi::backend::opengl::FormatDataType type,
                             const void* pixels) {
    glTexSubImage3D(static_cast<GLenum>(target),
                    level,
                    xoffset,
                    yoffset,
                    zoffset,
                    width,
                    height,
                    depth,
                    static_cast<GLenum>(format),
                    static_cast<GLenum>(type),
                    pixels);
}

void Instance::getInternalformativ(snap::rhi::backend::opengl::TextureTarget target,
                                   snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                                   GLenum pname,
                                   GLsizei bufSize,
                                   GLint* params) {
    // OpenGL 2.1 doesn't supported glGetInternalformativ
    // https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetInternalformativ.xhtml
    if (pname == GL_NUM_SAMPLE_COUNTS && bufSize) {
        *params = 1;
    }

    if (pname == GL_SAMPLES && bufSize) {
        glGetIntegerv(GL_MAX_SAMPLES, params);
    }
}

void Instance::renderbufferStorageMultisample(snap::rhi::backend::opengl::TextureTarget target,
                                              GLsizei samples,
                                              snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                                              GLsizei width,
                                              GLsizei height) {
    glRenderbufferStorageMultisample(
        static_cast<GLenum>(target), samples, static_cast<GLenum>(internalformat), width, height);
}

void Instance::drawBuffers(GLsizei n, const snap::rhi::backend::opengl::FramebufferAttachmentTarget* bufs) {
    glDrawBuffers(n, reinterpret_cast<const GLenum*>(bufs));
}

void Instance::discardFramebuffer(snap::rhi::backend::opengl::FramebufferTarget target,
                                  GLsizei numAttachments,
                                  const snap::rhi::backend::opengl::FramebufferAttachmentTarget* attachments) {
    // Do nothing
}

void Instance::readBuffer(snap::rhi::backend::opengl::FramebufferAttachmentTarget mode) {
    glReadBuffer(static_cast<GLenum>(mode));
}

SNAP_RHI_GLsync Instance::fenceSync(GLenum condition, GLbitfield flags) {
    GLsync result = glFenceSync(condition, flags);
    return reinterpret_cast<SNAP_RHI_GLsync>(result);
}

void Instance::waitSync(SNAP_RHI_GLsync sync, GLbitfield flags, GLuint64 timeout) {
    glWaitSync(reinterpret_cast<GLsync>(sync), flags, timeout);
}

GLenum Instance::clientWaitSync(SNAP_RHI_GLsync sync, GLbitfield flags, GLuint64 timeout) {
    return glClientWaitSync(reinterpret_cast<GLsync>(sync), flags, timeout);
}

void Instance::deleteSync(SNAP_RHI_GLsync sync) {
    glDeleteSync(reinterpret_cast<GLsync>(sync));
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
    glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
}

void Instance::drawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei primcount) {
    glDrawArraysInstanced(mode, first, count, primcount);
}

void Instance::drawElementsInstanced(
    GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instancecount) {
    glDrawElementsInstanced(mode, count, type, indices, instancecount);
}

void Instance::vertexAttribDivisor(GLuint index, GLuint divisor) {
    glVertexAttribDivisor(index, divisor);
}

void Instance::clearBufferfv(GLenum buffer, GLint drawBuffer, const GLfloat* value) {
    GLint maxFramebufferColorAttachmentCount = 0;
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxFramebufferColorAttachmentCount);
    std::vector<GLint> activeBuffers(maxFramebufferColorAttachmentCount);

    for (GLint i = 0; i < maxFramebufferColorAttachmentCount; ++i) {
        glGetIntegerv(GL_DRAW_BUFFER0 + i, &activeBuffers[i]);
    }

    glDrawBuffer(GL_COLOR_ATTACHMENT0 + drawBuffer);
    glClearColor(value[0], value[1], value[2], value[3]);
    glClear(GL_COLOR_BUFFER_BIT);

    glDrawBuffers(static_cast<GLsizei>(maxFramebufferColorAttachmentCount),
                  reinterpret_cast<const GLenum*>(activeBuffers.data()));
}

void Instance::getActiveUniformBlockName(
    GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei* length, GLchar* uniformBlockName) {
    snap::rhi::common::throwException("[GL21] getActiveUniformBlockName doesn't supported");
}

GLuint Instance::getUniformBlockIndex(GLuint program, const GLchar* uniformBlockName) {
    snap::rhi::common::throwException("[GL21] getUniformBlockIndex doesn't supported");
}

void Instance::getActiveUniformBlockiv(GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint* params) {
    snap::rhi::common::throwException("[GL21] getActiveUniformBlockiv doesn't supported");
}

void Instance::getActiveUniformsiv(
    GLuint program, GLsizei uniformCount, const GLuint* uniformIndices, GLenum pname, GLint* params) {
    snap::rhi::common::throwException("[GL21] getActiveUniformsiv doesn't supported");
}

void Instance::uniformBlockBinding(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding) {
    snap::rhi::common::throwException("[GL21] uniformBlockBinding doesn't supported");
}

void Instance::uniform1uiv(GLint location, GLsizei count, const GLuint* value) {
    snap::rhi::common::throwException("[GL21] uniform1uiv isn't supported");
}

void Instance::uniform2uiv(GLint location, GLsizei count, const GLuint* value) {
    snap::rhi::common::throwException("[GL21] uniform2uiv isn't supported");
}

void Instance::uniform3uiv(GLint location, GLsizei count, const GLuint* value) {
    snap::rhi::common::throwException("[GL21] uniform3uiv isn't supported");
}

void Instance::uniform4uiv(GLint location, GLsizei count, const GLuint* value) {
    snap::rhi::common::throwException("[GL21] uniform4uiv isn't supported");
}

void Instance::getProgramInterfaceiv(GLuint program, GLenum programInterface, GLenum pname, GLint* params) {
    snap::rhi::common::throwException("[GL21] getProgramInterfaceiv doesn't supported");
}

void Instance::bindImageTexture(
    GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format) {
    snap::rhi::common::throwException("[GL21] bindImageTexture doesn't supported");
}

void Instance::getProgramResourceiv(GLuint program,
                                    GLenum programInterface,
                                    GLuint index,
                                    GLsizei propCount,
                                    const GLenum* props,
                                    GLsizei bufSize,
                                    GLsizei* length,
                                    GLint* params) {
    snap::rhi::common::throwException("[GL21] bindImageTexture doesn't supported");
}

void Instance::getProgramResourceName(
    GLuint program, GLenum programInterface, GLuint index, GLsizei bufSize, GLsizei* length, char* name) {
    snap::rhi::common::throwException("[GL21] getProgramResourceName doesn't supported");
}

GLuint Instance::getProgramResourceIndex(GLuint program, GLenum programInterface, const char* name) {
    snap::rhi::common::throwException("[GL21] getProgramResourceIndex doesn't supported");
}

void Instance::memoryBarrierByRegion(GLbitfield barriers) {
    snap::rhi::common::throwException("[GL21] memoryBarrierByRegion doesn't supported");
}

void Instance::texStorage2D(snap::rhi::backend::opengl::TextureTarget target,
                            GLsizei levels,
                            snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                            GLsizei width,
                            GLsizei height) {
    snap::rhi::common::throwException("[GL21] texStorage2D doesn't supported");
}

void Instance::texStorage3D(snap::rhi::backend::opengl::TextureTarget target,
                            GLsizei levels,
                            snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                            GLsizei width,
                            GLsizei height,
                            GLsizei depth) {
    snap::rhi::common::throwException("[GL21] texStorage3D doesn't supported");
}

void Instance::texStorage2DMultisample(snap::rhi::backend::opengl::TextureTarget target,
                                       GLsizei samples,
                                       snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                                       GLsizei width,
                                       GLsizei height,
                                       GLboolean fixedsamplelocations) {
    snap::rhi::common::throwException("[GL21] texStorage2DMultisample doesn't supported");
}

void Instance::texStorage3DMultisample(snap::rhi::backend::opengl::TextureTarget target,
                                       GLsizei samples,
                                       snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                                       GLsizei width,
                                       GLsizei height,
                                       GLsizei depth,
                                       GLboolean fixedsamplelocations) {
    snap::rhi::common::throwException("[GL21] texStorage3DMultisample doesn't supported");
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
    snap::rhi::common::throwException("[GL21] copyImageSubData doesn't supported");
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
    snap::rhi::common::throwException("[GL21] colorMaski doesn't supported");
}

void Instance::enablei(GLenum cap, GLuint index) {
    snap::rhi::common::throwException("[GL21] enablei doesn't supported");
}

void Instance::disablei(GLenum cap, GLuint index) {
    snap::rhi::common::throwException("[GL21] disablei doesn't supported");
}

void Instance::blendEquationSeparatei(GLuint buf, GLenum modeRGB, GLenum modeAlpha) {
    snap::rhi::common::throwException("[GL21] blendEquationSeparatei doesn't supported");
}

void Instance::blendFuncSeparatei(GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) {
    snap::rhi::common::throwException("[GL21] blendFuncSeparatei doesn't supported");
}

GLboolean Instance::isSampler(GLuint id) {
    snap::rhi::common::throwException("[GL21] isSampler doesn't supported");
}

void Instance::samplerParameteri(GLuint sampler, GLenum pname, GLint param) {
    snap::rhi::common::throwException("[GL21] samplerParameteri doesn't supported");
}

void Instance::samplerParameterfv(GLuint sampler, GLenum pname, const GLfloat* params) {
    snap::rhi::common::throwException("[GL21] samplerParameterfv doesn't supported");
}

void Instance::samplerParameterf(GLuint sampler, GLenum pname, GLfloat param) {
    snap::rhi::common::throwException("[GL21] samplerParameterf doesn't supported");
}

void Instance::genSamplers(GLsizei n, GLuint* samplers) {
    snap::rhi::common::throwException("[GL21] genSamplers doesn't supported");
}

void Instance::deleteSamplers(GLsizei n, const GLuint* samplers) {
    snap::rhi::common::throwException("[GL21] deleteSamplers doesn't supported");
}

void Instance::bindSampler(GLuint unit, GLuint sampler) {
    snap::rhi::common::throwException("[GL21] bindSampler doesn't supported");
}

void Instance::framebufferTextureLayer(snap::rhi::backend::opengl::FramebufferTarget target,
                                       snap::rhi::backend::opengl::FramebufferAttachmentTarget attachment,
                                       snap::rhi::backend::opengl::TextureId texture,
                                       int32_t level,
                                       int32_t layer) {
    snap::rhi::common::throwException("[GL21] framebufferTextureLayer doesn't supported");
}

void Instance::framebufferTextureMultiviewOVR(snap::rhi::backend::opengl::FramebufferTarget target,
                                              snap::rhi::backend::opengl::FramebufferAttachmentTarget attachment,
                                              snap::rhi::backend::opengl::TextureId texture,
                                              int32_t level,
                                              int32_t baseViewIndex,
                                              int32_t numViews) {
    snap::rhi::common::throwException("[GL21] framebufferTextureMultiviewOVR doesn't supported");
}

void Instance::framebufferTextureMultisampleMultiviewOVR(
    snap::rhi::backend::opengl::FramebufferTarget target,
    snap::rhi::backend::opengl::FramebufferAttachmentTarget attachment,
    snap::rhi::backend::opengl::TextureId texture,
    int32_t level,
    int32_t samples,
    int32_t baseViewIndex,
    int32_t numViews) {
    snap::rhi::common::throwException("[GL21] framebufferTextureMultisampleMultiviewOVR doesn't supported");
}

void Instance::clearBufferiv(GLenum buffer, GLint drawBuffer, const GLint* value) {
    snap::rhi::common::throwException("[GL21] clearBufferiv doesn't supported");
}

void Instance::clearBufferuiv(GLenum buffer, GLint drawBuffer, const GLuint* value) {
    snap::rhi::common::throwException("[GL21] clearBufferuiv doesn't supported");
}

void Instance::clearBufferfi(GLenum buffer, GLint drawBuffer, GLfloat depth, GLint stencil) {
    snap::rhi::common::throwException("[GL21] clearBufferfi doesn't supported");
}

void Instance::bindBufferRange(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size) {
    snap::rhi::common::throwException("[GL21] bindBufferRange doesn't supported");
}

void Instance::vertexAttribI4iv(GLuint index, const GLint* v) {
    snap::rhi::common::throwException("[GL21] vertexAttribI4iv doesn't supported");
}

void Instance::vertexAttribI4uiv(GLuint index, const GLuint* v) {
    snap::rhi::common::throwException("[GL21] vertexAttribI4uiv doesn't supported");
}

void Instance::copyBufferSubData(
    GLenum readtarget, GLenum writetarget, GLintptr readoffset, GLintptr writeoffset, GLsizeiptr size) {
    snap::rhi::common::throwException("[GL21] copyBufferSubData doesn't supported");
}

void Instance::dispatchCompute(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z) {
    snap::rhi::common::throwException("[GL21] dispatchCompute doesn't supported");
}

void Instance::hint(GLenum target, GLenum mode) {
    glHint(target, mode);
}

void Instance::getTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint* params) {
    glGetTexLevelParameteriv(target, level, pname, params);
}

void Instance::getTexParameteriv(GLenum target, GLenum pname, GLint* params) {
    glGetTexParameteriv(target, pname, params);
}

void Instance::pixelStorei(GLenum pname, GLint param) {
    glPixelStorei(pname, param);
}

void Instance::programParameteri(GLuint program, GLenum pname, GLint value) {
    snap::rhi::common::throwException("[GL21] programParameteri doesn't supported");
}

void Instance::programBinary(GLuint program, GLenum binaryFormat, const void* binary, GLsizei length) {
    snap::rhi::common::throwException("[GL21] programBinary doesn't supported");
}

void Instance::getProgramBinary(GLuint program, GLsizei bufsize, GLsizei* length, GLenum* binaryFormat, void* binary) {
    snap::rhi::common::throwException("[GL21] getProgramBinary doesn't supported");
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
    glCopyTexSubImage3D(target, level, xoffset, yoffset, zoffset, x, y, width, height);
}

void Instance::genQueries(GLsizei n, GLuint* ids) {
    glGenQueries(n, ids);
}

void Instance::deleteQueries(GLsizei n, const GLuint* ids) {
    glDeleteQueries(n, ids);
}

GLboolean Instance::isQuery(GLuint id) {
    return glIsQuery(id);
}

void Instance::beginQuery(GLenum target, GLuint id) {
    glBeginQuery(target, id);
}

void Instance::endQuery(GLenum target) {
    glEndQuery(target);
}

void Instance::getQueryiv(GLenum target, GLenum pname, GLint* params) {
    glGetQueryiv(target, pname, params);
}

void Instance::getQueryObjectiv(GLuint id, GLenum pname, GLint* params) {
    glGetQueryObjectiv(id, pname, params);
}

void Instance::getQueryObjectuiv(GLuint id, GLenum pname, GLuint* params) {
    glGetQueryObjectuiv(id, pname, params);
}

void Instance::queryCounter(GLuint id, GLenum target) {
    snap::rhi::common::throwException("[GL21] queryCounter isn't supported");
}

void Instance::getQueryObjecti64v(GLuint id, GLenum pname, GLint64* params) {
    snap::rhi::common::throwException("[GL21] getQueryObjecti64v isn't supported");
}

void Instance::getQueryObjectui64v(GLuint id, GLenum pname, GLuint64* params) {
    snap::rhi::common::throwException("[GL21] getQueryObjectui64v isn't supported");
}

void Instance::getInteger64v(GLenum pname, GLint64* data) {
    snap::rhi::common::throwException("[GL21+] getInteger64v isn't supported");
}

} // namespace snap::rhi::backend::opengl21
#endif // SNAP_RHI_GL21
