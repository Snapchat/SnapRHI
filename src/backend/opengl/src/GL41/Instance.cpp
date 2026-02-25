//
//  Instance.cpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 6/30/22.
//

#include "Instance.hpp"

#include "snap/rhi/common/Throw.h"

#if SNAP_RHI_GL41
namespace {
void initTexFormatProperties(snap::rhi::backend::opengl::Features& features, const std::string& glExtensionsList) {
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

    { // snap::rhi::PixelFormat::R8Snorm
        features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R8Snorm)] = {
            snap::rhi::backend::opengl::SizedInternalFormat::R8_SNORM,
            snap::rhi::backend::opengl::FormatGroup::R,
            snap::rhi::backend::opengl::FormatDataType::BYTE};

        auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R8Snorm)];
        formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::Uploadable |
                              snap::rhi::backend::opengl::FormatFeatures::Copyable;
        formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
    }

    { // snap::rhi::PixelFormat::R8G8Snorm
        features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R8G8Snorm)] = {
            snap::rhi::backend::opengl::SizedInternalFormat::RG8_SNORM,
            snap::rhi::backend::opengl::FormatGroup::RG,
            snap::rhi::backend::opengl::FormatDataType::BYTE};

        auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R8G8Snorm)];
        formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::Uploadable |
                              snap::rhi::backend::opengl::FormatFeatures::Copyable;
        formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
    }

    { // snap::rhi::PixelFormat::R8G8B8A8Snorm
        features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R8G8B8A8Snorm)] = {
            snap::rhi::backend::opengl::SizedInternalFormat::RGBA8_SNORM,
            snap::rhi::backend::opengl::FormatGroup::RGBA,
            snap::rhi::backend::opengl::FormatDataType::BYTE};

        auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R8G8B8A8Snorm)];
        formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::Uploadable |
                              snap::rhi::backend::opengl::FormatFeatures::Copyable;
        formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
    }

    { // snap::rhi::PixelFormat::R16Snorm
        features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R16Snorm)] = {
            snap::rhi::backend::opengl::SizedInternalFormat::R16_SNORM,
            snap::rhi::backend::opengl::FormatGroup::R,
            snap::rhi::backend::opengl::FormatDataType::SHORT};

        auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R16Snorm)];
        formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::Uploadable |
                              snap::rhi::backend::opengl::FormatFeatures::Readable |
                              snap::rhi::backend::opengl::FormatFeatures::Copyable;
        formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
    }

    { // snap::rhi::PixelFormat::R16G16Snorm
        features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R16G16Snorm)] = {
            snap::rhi::backend::opengl::SizedInternalFormat::RG16_SNORM,
            snap::rhi::backend::opengl::FormatGroup::RG,
            snap::rhi::backend::opengl::FormatDataType::SHORT};

        auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R16G16Snorm)];
        formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::Uploadable |
                              snap::rhi::backend::opengl::FormatFeatures::Readable |
                              snap::rhi::backend::opengl::FormatFeatures::Copyable;
        formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
    }

    { // snap::rhi::PixelFormat::R16G16B16A16Snorm
        features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R16G16B16A16Snorm)] = {
            snap::rhi::backend::opengl::SizedInternalFormat::RGBA16_SNORM,
            snap::rhi::backend::opengl::FormatGroup::RGBA,
            snap::rhi::backend::opengl::FormatDataType::SHORT};

        auto& formatInfo =
            features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R16G16B16A16Snorm)];
        formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::Uploadable |
                              snap::rhi::backend::opengl::FormatFeatures::Readable |
                              snap::rhi::backend::opengl::FormatFeatures::Copyable;
        formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
    }

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

        auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R8G8Uint)];
        formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
        formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestOnly;
    }

    { // snap::rhi::PixelFormat::R8G8B8A8Uint
        features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R8G8B8A8Uint)] = {
            snap::rhi::backend::opengl::SizedInternalFormat::RGBA8UI,
            snap::rhi::backend::opengl::FormatGroup::RGBA_INTEGER,
            snap::rhi::backend::opengl::FormatDataType::UNSIGNED_BYTE};

        auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R8G8B8A8Uint)];
        formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
        formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestOnly;
    }

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

        auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R16G16Uint)];
        formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
        formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestOnly;
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

        auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R32G32Uint)];
        formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
        formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestOnly;
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

        auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R8G8Sint)];
        formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
        formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestOnly;
    }

    { // snap::rhi::PixelFormat::R8G8B8A8Sint
        features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R8G8B8A8Sint)] = {
            snap::rhi::backend::opengl::SizedInternalFormat::RGBA8I,
            snap::rhi::backend::opengl::FormatGroup::RGBA_INTEGER,
            snap::rhi::backend::opengl::FormatDataType::BYTE};

        auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R8G8B8A8Sint)];
        formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
        formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestOnly;
    }

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

        auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R16G16Sint)];
        formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
        formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestOnly;
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

        auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R32G32Sint)];
        formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
        formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestOnly;
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

    { // snap::rhi::PixelFormat::R16Float
        features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R16Float)] = {
            snap::rhi::backend::opengl::SizedInternalFormat::R16F,
            snap::rhi::backend::opengl::FormatGroup::R,
            snap::rhi::backend::opengl::FormatDataType::HALF_FLOAT};

        auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R16Float)];
        formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
        formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
    }

    { // snap::rhi::PixelFormat::R16G16Float
        features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R16G16Float)] = {
            snap::rhi::backend::opengl::SizedInternalFormat::RG16F,
            snap::rhi::backend::opengl::FormatGroup::RG,
            snap::rhi::backend::opengl::FormatDataType::HALF_FLOAT};

        auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R16G16Float)];
        formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
        formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
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

    { // snap::rhi::PixelFormat::R32Float
        features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R32Float)] = {
            snap::rhi::backend::opengl::SizedInternalFormat::R32F,
            snap::rhi::backend::opengl::FormatGroup::R,
            snap::rhi::backend::opengl::FormatDataType::FLOAT};

        auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R32Float)];
        formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
        formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestOnly;
    }

    { // snap::rhi::PixelFormat::R32G32Float
        features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R32G32Float)] = {
            snap::rhi::backend::opengl::SizedInternalFormat::RG32F,
            snap::rhi::backend::opengl::FormatGroup::RG,
            snap::rhi::backend::opengl::FormatDataType::FLOAT};

        auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R32G32Float)];
        formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
        formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestOnly;
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

    { // snap::rhi::PixelFormat::Grayscale
        features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::Grayscale)] = {
            // GL_R8 format with swizzle.
            snap::rhi::backend::opengl::SizedInternalFormat::R8,
            snap::rhi::backend::opengl::FormatGroup::R,
            snap::rhi::backend::opengl::FormatDataType::UNSIGNED_BYTE};

        auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::Grayscale)];
        formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::Uploadable;
        formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
    }

    bool EXT_bgra = snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "GL_EXT_bgra");
    if (EXT_bgra) {
        {
            features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::B8G8R8A8Unorm)] = {
                snap::rhi::backend::opengl::SizedInternalFormat::RGBA8,
                snap::rhi::backend::opengl::FormatGroup::BGRA,
                snap::rhi::backend::opengl::FormatDataType::UNSIGNED_BYTE};

            auto& formatInfo =
                features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::B8G8R8A8Unorm)];
            formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
            formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
        }
    }

    { // snap::rhi::PixelFormat::R8G8B8A8Srgb
        features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R8G8B8A8Srgb)] = {
            snap::rhi::backend::opengl::SizedInternalFormat::SRGB8_ALPHA8,
            snap::rhi::backend::opengl::FormatGroup::RGBA,
            snap::rhi::backend::opengl::FormatDataType::UNSIGNED_BYTE};

        auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R8G8B8A8Srgb)];
        formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
        formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
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

    { // snap::rhi::PixelFormat::R10G10B10A2Uint
        features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R10G10B10A2Uint)] = {
            snap::rhi::backend::opengl::SizedInternalFormat::RGB10_A2UI,
            snap::rhi::backend::opengl::FormatGroup::RGBA_INTEGER,
            snap::rhi::backend::opengl::FormatDataType::UNSIGNED_INT_2_10_10_10_REV};

        auto& formatInfo =
            features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R10G10B10A2Unorm)];
        formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
        formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestOnly;
    }

    { // snap::rhi::PixelFormat::R11G11B10Float
        features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R11G11B10Float)] = {
            snap::rhi::backend::opengl::SizedInternalFormat::R11F_G11F_B10F,
            snap::rhi::backend::opengl::FormatGroup::RGB,
            snap::rhi::backend::opengl::FormatDataType::UNSIGNED_INT_10F_11F_11F_REV};

        auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R11G11B10Float)];
        formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::Uploadable |
                              snap::rhi::backend::opengl::FormatFeatures::Copyable;
        formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
    }

    { // snap::rhi::PixelFormat::Depth16Unorm
        features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::Depth16Unorm)] = {
            snap::rhi::backend::opengl::SizedInternalFormat::DEPTH16,
            snap::rhi::backend::opengl::FormatGroup::DEPTH,
            snap::rhi::backend::opengl::FormatDataType::UNSIGNED_SHORT};

        auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::Depth16Unorm)];
        formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::Renderable;
        formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
    }

    { // snap::rhi::PixelFormat::DepthFloat
        features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::DepthFloat)] = {
            snap::rhi::backend::opengl::SizedInternalFormat::DEPTH32F,
            snap::rhi::backend::opengl::FormatGroup::DEPTH,
            snap::rhi::backend::opengl::FormatDataType::FLOAT};

        auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::DepthFloat)];
        formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::Renderable;
        formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
    }

    { // snap::rhi::PixelFormat::DepthStencil
        features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::DepthStencil)] = {
            snap::rhi::backend::opengl::SizedInternalFormat::DEPTH24_STENCIL8,
            snap::rhi::backend::opengl::FormatGroup::DEPTH_STENCIL,
            snap::rhi::backend::opengl::FormatDataType::UNSIGNED_INT_24_8};

        auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::DepthStencil)];
        formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::Renderable;
        formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
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

namespace snap::rhi::backend::opengl41 {
snap::rhi::backend::opengl::Features Instance::buildFeatures() {
    snap::rhi::backend::opengl::Features features = opengl21::Instance::buildFeatures();

    std::string glExtensionsList;
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

    initTexFormatProperties(features, glExtensionsList);
    initRenderbufferFormatProperties(features);

    features.isFragmentPrimitiveIDSupported = true;
    features.apiVersion = gl::APIVersion::GL41;
    features.isPBOSupported = true;
    features.isBlitFramebufferSupported = true;
    features.isCopyBufferSubDataSupported = true;
    features.isSamplerSupported = true;
    features.isVAOSupported = true;
    features.isDifferentBlendSettingsSupported = true;
    features.isMRTSupported = true;
    features.isStorageTextureSupported = true;
    features.isNativeUBOSupported = true;
    features.isSSBOSupported = false;
    features.isDepthStencilAttachmentSupported = true;
    features.isOVRDepthStencilSupported = false;
    features.isRasterizationDisableSupported = true;
    features.isMinMaxBlendModeSupported = true;
    features.isFBORenderToNonZeroMipSupported = true;
    features.isClampToBorderSupported = false;
    features.isTexMinMaxLodSupported = true;
    features.isTexBaseMaxLevelSupported = true;
    features.isMapUnmapAvailable = true;
    features.isNPOTGenMipmapSupported = true;
    features.isTexSwizzleSupported = true;
    features.isNPOTWrapModeSupported = true;
    features.isDepthCompareFuncSupported = true;
    features.isTexture3DSupported = true;
    features.isTextureArraySupported = true;
    features.isInstancingSupported = true;
    features.isDepthCubemapSupported = true;
    features.isMultisampleSupported = true;
    features.isCustomTexturePackingSupported = true;
    features.isOcclusionQuerySupported = true;
    /**
     * GL_DISJOINT_EXT is not a thing in GL non-ES
     * It was introduced in https://registry.khronos.org/OpenGL/extensions/EXT/EXT_disjoint_timer_query.txt
     * and "written against the OpenGL ES 2.0 specification"
     */
    features.isDisjointQuerySupported = false;
    /**
     * supported when GL Version is 3.2 and greater
     * https://registry.khronos.org/OpenGL-Refpages/gl4/html/glQueryCounter.xhtml
     */
    features.isTimerQuerySupported = true;

    /**
     * https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glEnable.xhtml
     * GL_PRIMITIVE_RESTART_FIXED_INDEX is available only if the GL version is 4.3 or greater.
     * mb we should use glPrimitiveRestartIndex?
     */
    features.isPrimitiveRestartIndexSupported = false;
    features.isFramebufferFetchSupported = false;
    features.isIntConstantRateSupported = true;
    features.isDiscardFramebufferSupported = false;
    features.isFenceSupported = true;
    features.isSemaphoreSupported = true;

    {
        GLint numFormats = 0;
        glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &numFormats);

        if (numFormats > 0) {
            features.isProgramBinarySupported = true;
        } else {
            features.isProgramBinarySupported = false;
        }
    }

    features.isVtxAttribSupported.fill(true);

    features.maxOVRViews = 0;

    GLint maxArrayTextureLayers = 0;
    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &maxArrayTextureLayers);
    features.maxArrayTextureLayers = static_cast<uint32_t>(maxArrayTextureLayers);

    {
        // https://registry.khronos.org/OpenGL-Refpages/gl4/html/glGet.xhtml
        // The value must be at least 64.
        constexpr uint32_t minTexture3DSize = 64;

        GLint textureSize = 0;
        glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &textureSize);

        assert(static_cast<uint32_t>(textureSize) >= minTexture3DSize);
        features.maxTextureDimension3D = std::max(minTexture3DSize, static_cast<uint32_t>(textureSize));
    }

    /**
     * https://www.khronos.org/registry/OpenGL-Refpages/es3.0/html/glGet.xhtml
     * GL_MAX_COLOR_ATTACHMENTS
     * params returns one value, the maximum number of color attachment points in a framebuffer object.
     * The value must be at least 4. See glFramebufferRenderbuffer .
     */
    GLint maxFramebufferColorAttachmentCount = 0;
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxFramebufferColorAttachmentCount);
    features.maxFramebufferColorAttachmentCount = std::max(1, maxFramebufferColorAttachmentCount);

    GLint maxClipDistances = 0;
    glGetIntegerv(GL_MAX_CLIP_DISTANCES, &maxClipDistances);
    features.maxClipDistances = maxClipDistances;
    features.maxCullDistances = 0;
    features.maxCombinedClipAndCullDistances = maxClipDistances;

    // https://en.wikipedia.org/wiki/OpenGL_Shading_Language
    features.shaderVersionHeader = "#version 410 core\n";

    return features;
}

void Instance::getInternalformativ(snap::rhi::backend::opengl::TextureTarget target,
                                   snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                                   GLenum pname,
                                   GLsizei bufSize,
                                   GLint* params) {
    // OpenGL 4.1 doesn't supported glGetInternalformativ
    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glGetInternalformat.xhtml
    if (pname == GL_NUM_SAMPLE_COUNTS && bufSize) {
        *params = 1;
    }

    if (pname == GL_SAMPLES && bufSize) {
        glGetIntegerv(GL_MAX_SAMPLES, params);
    }
}

void Instance::getActiveUniformBlockName(
    GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei* length, GLchar* uniformBlockName) {
    glGetActiveUniformBlockName(program, uniformBlockIndex, bufSize, length, uniformBlockName);
}

GLuint Instance::getUniformBlockIndex(GLuint program, const GLchar* uniformBlockName) {
    return glGetUniformBlockIndex(program, uniformBlockName);
}

void Instance::getActiveUniformBlockiv(GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint* params) {
    glGetActiveUniformBlockiv(program, uniformBlockIndex, pname, params);
}

void Instance::getActiveUniformsiv(
    GLuint program, GLsizei uniformCount, const GLuint* uniformIndices, GLenum pname, GLint* params) {
    glGetActiveUniformsiv(program, uniformCount, uniformIndices, pname, params);
}

void Instance::uniformBlockBinding(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding) {
    glUniformBlockBinding(program, uniformBlockIndex, uniformBlockBinding);
}

void Instance::uniform1uiv(GLint location, GLsizei count, const GLuint* value) {
    glUniform1uiv(location, count, value);
}

void Instance::uniform2uiv(GLint location, GLsizei count, const GLuint* value) {
    glUniform2uiv(location, count, value);
}

void Instance::uniform3uiv(GLint location, GLsizei count, const GLuint* value) {
    glUniform3uiv(location, count, value);
}

void Instance::uniform4uiv(GLint location, GLsizei count, const GLuint* value) {
    glUniform4uiv(location, count, value);
}

void Instance::texStorage2D(snap::rhi::backend::opengl::TextureTarget target,
                            GLsizei levels,
                            snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                            GLsizei width,
                            GLsizei height) {
    glTexStorage2D(static_cast<GLenum>(target), levels, static_cast<GLenum>(internalformat), width, height);
}

void Instance::texStorage3D(snap::rhi::backend::opengl::TextureTarget target,
                            GLsizei levels,
                            snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                            GLsizei width,
                            GLsizei height,
                            GLsizei depth) {
    glTexStorage3D(static_cast<GLenum>(target), levels, static_cast<GLenum>(internalformat), width, height, depth);
}

void Instance::genVertexArrays(GLsizei n, GLuint* arrays) {
    glGenVertexArrays(n, arrays);
}

void Instance::bindVertexArray(GLuint array) {
    glBindVertexArray(array);
}

void Instance::deleteVertexArrays(GLsizei n, const GLuint* arrays) {
    glDeleteVertexArrays(n, arrays);
}

GLboolean Instance::isSampler(GLuint id) {
    return glIsSampler(id);
}

void Instance::samplerParameteri(GLuint sampler, GLenum pname, GLint param) {
    glSamplerParameteri(sampler, pname, param);
}

void Instance::samplerParameterfv(GLuint sampler, GLenum pname, const GLfloat* params) {
    glSamplerParameterfv(sampler, pname, params);
}

void Instance::samplerParameterf(GLuint sampler, GLenum pname, GLfloat param) {
    glSamplerParameterf(sampler, pname, param);
}

void Instance::genSamplers(GLsizei n, GLuint* samplers) {
    glGenSamplers(n, samplers);
}

void Instance::deleteSamplers(GLsizei n, const GLuint* samplers) {
    glDeleteSamplers(n, samplers);
}

void Instance::bindSampler(GLuint unit, GLuint sampler) {
    glBindSampler(unit, sampler);
}

void Instance::framebufferTextureLayer(snap::rhi::backend::opengl::FramebufferTarget target,
                                       snap::rhi::backend::opengl::FramebufferAttachmentTarget attachment,
                                       snap::rhi::backend::opengl::TextureId texture,
                                       int32_t level,
                                       int32_t layer) {
    glFramebufferTextureLayer(
        static_cast<GLenum>(target), static_cast<GLenum>(attachment), static_cast<GLuint>(texture), level, layer);
}

void Instance::clearBufferiv(GLenum buffer, GLint drawBuffer, const GLint* value) {
    glClearBufferiv(buffer, drawBuffer, value);
}

void Instance::clearBufferuiv(GLenum buffer, GLint drawBuffer, const GLuint* value) {
    glClearBufferuiv(buffer, drawBuffer, value);
}

void Instance::clearBufferfv(GLenum buffer, GLint drawBuffer, const GLfloat* value) {
    glClearBufferfv(buffer, drawBuffer, value);
}

void Instance::clearBufferfi(GLenum buffer, GLint drawBuffer, GLfloat depth, GLint stencil) {
    glClearBufferfi(buffer, drawBuffer, depth, stencil);
}

void Instance::bindBufferRange(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size) {
    glBindBufferRange(target, index, buffer, offset, size);
}

void Instance::vertexAttribI4iv(GLuint index, const GLint* v) {
    glVertexAttribI4iv(index, v);
}

void Instance::vertexAttribI4uiv(GLuint index, const GLuint* v) {
    glVertexAttribI4uiv(index, v);
}

void* Instance::mapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access) {
    auto result = glMapBufferRange(target, offset, length, access);
    return result;
}

void Instance::copyBufferSubData(
    GLenum readtarget, GLenum writetarget, GLintptr readoffset, GLintptr writeoffset, GLsizeiptr size) {
    glCopyBufferSubData(readtarget, writetarget, readoffset, writeoffset, size);
}

void Instance::colorMaski(GLuint buf, GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
    glColorMaski(buf, red, green, blue, alpha);
}

void Instance::enablei(GLenum cap, GLuint index) {
    glEnablei(cap, index);
}

void Instance::disablei(GLenum cap, GLuint index) {
    glDisablei(cap, index);
}

void Instance::blendEquationSeparatei(GLuint buf, GLenum modeRGB, GLenum modeAlpha) {
    glBlendEquationSeparatei(buf, modeRGB, modeAlpha);
}

void Instance::blendFuncSeparatei(GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) {
    glBlendFuncSeparatei(buf, srcRGB, dstRGB, srcAlpha, dstAlpha);
}

void Instance::hint(GLenum target, GLenum mode) {
    if (target == GL_GENERATE_MIPMAP_HINT) {
        return;
    }

    glHint(target, mode);
}

void Instance::programParameteri(GLuint program, GLenum pname, GLint value) {
    glProgramParameteri(program, pname, value);
}

void Instance::programBinary(GLuint program, GLenum binaryFormat, const void* binary, GLsizei length) {
    glProgramBinary(program, binaryFormat, binary, length);
}

void Instance::getProgramBinary(GLuint program, GLsizei bufsize, GLsizei* length, GLenum* binaryFormat, void* binary) {
    glGetProgramBinary(program, bufsize, length, binaryFormat, binary);
}

void Instance::queryCounter(GLuint id, GLenum target) {
    glQueryCounter(id, target);
}

void Instance::getQueryObjecti64v(GLuint id, GLenum pname, GLint64* params) {
    glGetQueryObjecti64v(id, pname, params);
}

void Instance::getQueryObjectui64v(GLuint id, GLenum pname, GLuint64* params) {
    glGetQueryObjectui64v(id, pname, params);
}

} // namespace snap::rhi::backend::opengl41
#endif // SNAP_RHI_GL41
