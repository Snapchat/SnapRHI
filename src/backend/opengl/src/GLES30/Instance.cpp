#include "Instance.hpp"

#include "snap/rhi/Exception.h"
#include "snap/rhi/backend/common/Logging.hpp"
#include "snap/rhi/backend/opengl/FramebufferDescription.h"
#include "snap/rhi/backend/opengl/GPUInfo.h"
#include "snap/rhi/common/Throw.h"

#if SNAP_RHI_PLATFORM_WEBASSEMBLY()
#include <emscripten.h>
#include <emscripten/bind.h>
#include <webgl/webgl2.h>
#endif

#if SNAP_RHI_GLES30
namespace {
inline void addCompressedFormat(snap::rhi::backend::opengl::Features& features,
                                const snap::rhi::PixelFormat format,
                                const snap::rhi::backend::opengl::SizedInternalFormat internalFormat,
                                const snap::rhi::backend::opengl::FormatGroup formatGroup,
                                const snap::rhi::backend::opengl::FormatDataType formatDataType =
                                    snap::rhi::backend::opengl::FormatDataType::UNSIGNED_BYTE) noexcept {
    features.textureFormat[static_cast<uint32_t>(format)] = {internalFormat, formatGroup, formatDataType};

    auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(format)];
    formatInfo.features =
        snap::rhi::backend::opengl::FormatFeatures::Uploadable | snap::rhi::backend::opengl::FormatFeatures::Copyable;
    formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
}

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
        formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::Uploadable |
                              snap::rhi::backend::opengl::FormatFeatures::Copyable;
        formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
    }

    { // snap::rhi::PixelFormat::R16G16Float
        features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R16G16Float)] = {
            snap::rhi::backend::opengl::SizedInternalFormat::RG16F,
            snap::rhi::backend::opengl::FormatGroup::RG,
            snap::rhi::backend::opengl::FormatDataType::HALF_FLOAT};

        auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R16G16Float)];
        formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::Uploadable |
                              snap::rhi::backend::opengl::FormatFeatures::Copyable;
        formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
    }

    { // snap::rhi::PixelFormat::R16G16B16A16Float
        features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R16G16B16A16Float)] = {
            snap::rhi::backend::opengl::SizedInternalFormat::RGBA16F,
            snap::rhi::backend::opengl::FormatGroup::RGBA,
            snap::rhi::backend::opengl::FormatDataType::HALF_FLOAT};

        auto& formatInfo =
            features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R16G16B16A16Float)];
        formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::Uploadable |
                              snap::rhi::backend::opengl::FormatFeatures::Copyable;
        formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
    }

    bool OES_texture_float_linear =
        snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "OES_texture_float_linear");
    snap::rhi::backend::opengl::FormatFilteringType float32FilteringType =
        OES_texture_float_linear ? snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear :
                                   snap::rhi::backend::opengl::FormatFilteringType::NearestOnly;

    { // snap::rhi::PixelFormat::R32Float
        features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R32Float)] = {
            snap::rhi::backend::opengl::SizedInternalFormat::R32F,
            snap::rhi::backend::opengl::FormatGroup::R,
            snap::rhi::backend::opengl::FormatDataType::FLOAT};

        auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R32Float)];
        formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::Uploadable |
                              snap::rhi::backend::opengl::FormatFeatures::Copyable;
        formatInfo.formatFilteringType = float32FilteringType;
    }

    { // snap::rhi::PixelFormat::R32G32Float
        features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R32G32Float)] = {
            snap::rhi::backend::opengl::SizedInternalFormat::RG32F,
            snap::rhi::backend::opengl::FormatGroup::RG,
            snap::rhi::backend::opengl::FormatDataType::FLOAT};

        auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R32G32Float)];
        formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::Uploadable |
                              snap::rhi::backend::opengl::FormatFeatures::Copyable;
        formatInfo.formatFilteringType = float32FilteringType;
    }

    { // snap::rhi::PixelFormat::R32G32B32A32Float
        features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::R32G32B32A32Float)] = {
            snap::rhi::backend::opengl::SizedInternalFormat::RGBA32F,
            snap::rhi::backend::opengl::FormatGroup::RGBA,
            snap::rhi::backend::opengl::FormatDataType::FLOAT};

        auto& formatInfo =
            features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R32G32B32A32Float)];
        formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::Uploadable |
                              snap::rhi::backend::opengl::FormatFeatures::Copyable;
        formatInfo.formatFilteringType = float32FilteringType;
    }

    { // snap::rhi::PixelFormat::Grayscale
#if SNAP_RHI_PLATFORM_WEBASSEMBLY()
        features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::Grayscale)] = {
            // GL_R8 format with swizzle.
            snap::rhi::backend::opengl::SizedInternalFormat::DEPRECATED_LUMINANCE,
            snap::rhi::backend::opengl::FormatGroup::DEPRECATED_LUMINANCE,
            snap::rhi::backend::opengl::FormatDataType::UNSIGNED_BYTE};
#else  // SNAP_RHI_PLATFORM_WEBASSEMBLY()
        features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::Grayscale)] = {
            // GL_R8 format with swizzle.
            snap::rhi::backend::opengl::SizedInternalFormat::R8,
            snap::rhi::backend::opengl::FormatGroup::R,
            snap::rhi::backend::opengl::FormatDataType::UNSIGNED_BYTE};
#endif // SNAP_RHI_PLATFORM_WEBASSEMBLY()

        auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::Grayscale)];
        formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::Uploadable;
        formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
    }

    bool EXT_texture_format_BGRA8888 =
        snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "GL_EXT_texture_format_BGRA8888");
    bool APPLE_texture_format_BGRA8888 =
        snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "GL_APPLE_texture_format_BGRA8888");
    if (EXT_texture_format_BGRA8888 || APPLE_texture_format_BGRA8888) {
        {
            features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::B8G8R8A8Unorm)] = {
                snap::rhi::backend::opengl::SizedInternalFormat::BGRA8,
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
        formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
    }

    bool EXT_render_snorm = snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "EXT_render_snorm");
#if !SNAP_RHI_OS_IOS()
    bool EXT_texture_norm16 = snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "EXT_texture_norm16");
    if (EXT_texture_norm16) {
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

        if (EXT_render_snorm) {
            for (auto format : {snap::rhi::PixelFormat::R16Snorm,
                                snap::rhi::PixelFormat::R16G16Snorm,
                                snap::rhi::PixelFormat::R16G16B16A16Snorm}) {
                auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(format)];
                formatInfo.features |= snap::rhi::backend::opengl::FormatFeatures::Renderable;
            }
        }
    }
#endif // !SNAP_RHI_OS_IOS()

    if (EXT_render_snorm) {
        for (auto format : {snap::rhi::PixelFormat::R8Snorm,
                            snap::rhi::PixelFormat::R8G8Snorm,
                            snap::rhi::PixelFormat::R8G8B8A8Snorm}) {
            auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(format)];
            formatInfo.features |= snap::rhi::backend::opengl::FormatFeatures::Renderable;
        }
    }

    bool EXT_color_buffer_float =
        snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "EXT_color_buffer_float");
    if (EXT_color_buffer_float) {
        for (auto format : {snap::rhi::PixelFormat::R32Float,
                            snap::rhi::PixelFormat::R32G32Float,
                            snap::rhi::PixelFormat::R32G32B32A32Float}) {
            auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(format)];
            formatInfo.features |= snap::rhi::backend::opengl::FormatFeatures::Renderable;
        }
    }

    bool EXT_color_buffer_half_float =
        snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "EXT_color_buffer_half_float");
    if (EXT_color_buffer_half_float || EXT_color_buffer_float) {
        for (auto format : {snap::rhi::PixelFormat::R16Float,
                            snap::rhi::PixelFormat::R16G16Float,
                            snap::rhi::PixelFormat::R16G16B16A16Float}) {
            auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(format)];
            formatInfo.features |= snap::rhi::backend::opengl::FormatFeatures::Renderable;
        }
    }

    bool APPLE_color_buffer_packed_float =
        snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "APPLE_color_buffer_packed_float");
    if (APPLE_color_buffer_packed_float || EXT_color_buffer_float) {
        auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R11G11B10Float)];
        formatInfo.features |= snap::rhi::backend::opengl::FormatFeatures::Renderable;
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
#if SNAP_RHI_PLATFORM_WEBASSEMBLY()
    if (!snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "WEBGL_compressed_texture_etc")) {
        return;
    }
#endif
    addCompressedFormat(features,
                        snap::rhi::PixelFormat::ETC2_R8G8B8_Unorm,
                        snap::rhi::backend::opengl::SizedInternalFormat::COMPRESSED_RGB8_ETC2,
                        snap::rhi::backend::opengl::FormatGroup::RGB);

    addCompressedFormat(features,
                        snap::rhi::PixelFormat::ETC2_R8G8B8A1_Unorm,
                        snap::rhi::backend::opengl::SizedInternalFormat::COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2,
                        snap::rhi::backend::opengl::FormatGroup::RGBA);

    addCompressedFormat(features,
                        snap::rhi::PixelFormat::ETC2_R8G8B8A8_Unorm,
                        snap::rhi::backend::opengl::SizedInternalFormat::COMPRESSED_RGBA8_ETC2_EAC,
                        snap::rhi::backend::opengl::FormatGroup::RGBA);

    addCompressedFormat(features,
                        snap::rhi::PixelFormat::ETC2_R8G8B8_sRGB,
                        snap::rhi::backend::opengl::SizedInternalFormat::COMPRESSED_SRGB8_ETC2,
                        snap::rhi::backend::opengl::FormatGroup::RGB);

    addCompressedFormat(features,
                        snap::rhi::PixelFormat::ETC2_R8G8B8A1_sRGB,
                        snap::rhi::backend::opengl::SizedInternalFormat::COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2,
                        snap::rhi::backend::opengl::FormatGroup::RGBA);

    addCompressedFormat(features,
                        snap::rhi::PixelFormat::ETC2_R8G8B8A8_sRGB,
                        snap::rhi::backend::opengl::SizedInternalFormat::COMPRESSED_SRGB8_ALPHA8_ETC2_EAC,
                        snap::rhi::backend::opengl::FormatGroup::RGBA);
}

void initRenderbufferFormatProperties(snap::rhi::backend::opengl::Features& features,
                                      const std::string& glExtensionsList) {
    for (auto format = 0u; format < static_cast<uint32_t>(snap::rhi::PixelFormat::Count); ++format) {
        const auto& formatInfo = features.textureFormatOpInfo[format];
        if ((formatInfo.features & snap::rhi::backend::opengl::FormatFeatures::Renderable) !=
            snap::rhi::backend::opengl::FormatFeatures::None) {
            const auto& textureFormat = features.textureFormat[format];
            features.renderbufferFormat[format] = {textureFormat.internalFormat};
        }
    }
}

bool isMapUnmapAvailable() {
// https://registry.khronos.org/webgl/specs/latest/2.0/#5.14 MapBufferRange/UnmapBuffer unsupported for WebGL
#if SNAP_RHI_PLATFORM_WEBASSEMBLY()
    return false;
#else  // SNAP_RHI_PLATFORM_WEBASSEMBLY()
    GLuint buffer = GL_NONE;
    GLsizeiptr size = 128;

    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_STATIC_DRAW);
    GLvoid* data = glMapBufferRange(GL_ARRAY_BUFFER, 0, size, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);
    glUnmapBuffer(GL_ARRAY_BUFFER);
    glDeleteBuffers(1, &buffer);

    return snap::rhi::backend::opengl::checkOpenGLErrors() && data != nullptr;
#endif // SNAP_RHI_PLATFORM_WEBASSEMBLY()
}

#if SNAP_RHI_GL_ES && SNAP_RHI_OS_LINUX_BASED()

struct DrawBufferGuard {
    std::vector<GLint> drawBuffers{};

    DrawBufferGuard() {
        GLint drawBufferCount = 0;
        glGetIntegerv(GL_MAX_DRAW_BUFFERS, &drawBufferCount);

        drawBuffers.resize(drawBufferCount);
        for (GLint i = 0; i < drawBufferCount; ++i) {
            glGetIntegerv(GL_DRAW_BUFFER0 + i, drawBuffers.data() + i);
        }
    }

    ~DrawBufferGuard() {
        glDrawBuffers(drawBuffers.size(), reinterpret_cast<const GLenum*>(drawBuffers.data()));
    }
};

struct ReadBufferGuard {
    GLint readBuffer = GL_NONE;

    ReadBufferGuard() {
        glGetIntegerv(GL_READ_BUFFER, &readBuffer);
    }

    ~ReadBufferGuard() {
        glReadBuffer(readBuffer);
    }
};

bool supportsDepthStencilAttachmentsOVR() {
    constexpr uint32_t width = 16, height = 16, layers = 2;

    constexpr GLenum fbTarget = static_cast<GLenum>(snap::rhi::backend::opengl::FramebufferTarget::DrawFramebuffer);
    constexpr GLenum colorFmt = static_cast<GLenum>(snap::rhi::backend::opengl::SizedInternalFormat::RGBA8);
    constexpr GLenum depthStencilFmt =
        static_cast<GLenum>(snap::rhi::backend::opengl::SizedInternalFormat::DEPTH24_STENCIL8);

    snap::rhi::backend::opengl::FramebufferAttachment colorViewOVR = {}, depthStencilViewOVR = {};

    glGenTextures(1, reinterpret_cast<GLuint*>(&colorViewOVR.texId));
    glBindTexture(GL_TEXTURE_2D_ARRAY, static_cast<GLuint>(colorViewOVR.texId));
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, colorFmt, width, height, layers);

    glGenTextures(1, reinterpret_cast<GLuint*>(&depthStencilViewOVR.texId));
    glBindTexture(GL_TEXTURE_2D_ARRAY, static_cast<GLuint>(depthStencilViewOVR.texId));
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, depthStencilFmt, width, height, layers);

    colorViewOVR.target = snap::rhi::backend::opengl::TextureTarget::Texture2DArray;
    colorViewOVR.size.width = width;
    colorViewOVR.size.height = height;
    colorViewOVR.sampleCount = snap::rhi::SampleCount::Count1;

    depthStencilViewOVR.target = snap::rhi::backend::opengl::TextureTarget::Texture2DArray;
    depthStencilViewOVR.size.width = width;
    depthStencilViewOVR.size.height = height;
    depthStencilViewOVR.sampleCount = snap::rhi::SampleCount::Count1;

    GLuint fbId = 0;
    glGenFramebuffers(1, &fbId);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbId);
    glFramebufferTextureMultiviewOVR(fbTarget,
                                     GL_COLOR_ATTACHMENT0,
                                     static_cast<GLuint>(colorViewOVR.texId),
                                     colorViewOVR.level,
                                     colorViewOVR.firstLayer,
                                     layers);
    glFramebufferTextureMultiviewOVR(fbTarget,
                                     GL_DEPTH_STENCIL_ATTACHMENT,
                                     static_cast<GLuint>(depthStencilViewOVR.texId),
                                     depthStencilViewOVR.level,
                                     depthStencilViewOVR.firstLayer,
                                     layers);

    bool isComplete = false;
    {
        DrawBufferGuard drawBufferGuard{};
        ReadBufferGuard readBufferGuard{};

        GLenum drawBuffers[] = {GL_COLOR_ATTACHMENT0};
        glDrawBuffers(1, drawBuffers);
        glReadBuffer(GL_NONE);

        snap::rhi::backend::opengl::FramebufferStatus status =
            static_cast<snap::rhi::backend::opengl::FramebufferStatus>(glCheckFramebufferStatus(fbTarget));
        assert(status == snap::rhi::backend::opengl::FramebufferStatus::Complete ||
               status == snap::rhi::backend::opengl::FramebufferStatus::IncompleteViewTargetsOVR);
        isComplete = status == snap::rhi::backend::opengl::FramebufferStatus::Complete;
    }

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &fbId);
    glDeleteTextures(1, reinterpret_cast<GLuint*>(&colorViewOVR.texId));
    glDeleteTextures(1, reinterpret_cast<GLuint*>(&depthStencilViewOVR.texId));

    return isComplete;
}
#endif // SNAP_RHI_GL_ES && SNAP_RHI_OS_LINUX_BASED()

void initClipCullDistanceExt(const std::string& glExtensionsList, snap::rhi::backend::opengl::Features& features) {
    GLint maxClipDistances = 0;
    GLint maxCullDistances = 0;
    GLint maxCombinedDistances = 0;
    if (snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "GL_EXT_clip_cull_distance")) {
        glGetIntegerv(GL_MAX_CLIP_DISTANCES, &maxClipDistances);
        glGetIntegerv(GL_MAX_CULL_DISTANCES, &maxCullDistances);
        glGetIntegerv(GL_MAX_COMBINED_CLIP_AND_CULL_DISTANCES, &maxCombinedDistances);
    } else if (snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "GL_APPLE_clip_distance")) {
        glGetIntegerv(GL_MAX_CLIP_DISTANCES, &maxClipDistances);
        maxCullDistances = 0;
        maxCombinedDistances = maxClipDistances;
    }

    /**
     * We found an OpenGL error in glGetIntegerv with GL_MAX_CLIP_DISTANCES on SM-S901B, SM-S906B, SM-S908B devices.
     */
    if (!snap::rhi::backend::opengl::checkOpenGLErrors()) {
        maxClipDistances = 0;
        maxCullDistances = 0;
        maxCombinedDistances = 0;
    }

    features.maxClipDistances = maxClipDistances;
    features.maxCullDistances = maxCullDistances;
    features.maxCombinedClipAndCullDistances = maxCombinedDistances;
}

void initOVRExt(const std::string& glExtensionsList,
                const GLint maxArrayTextureLayers,
                snap::rhi::backend::opengl::Features& features) {
    features.maxOVRViews = 0;
    features.isOVRDepthStencilSupported = false;
    features.isOVRMultiviewSupported = false;
    features.isOVRMultiviewMultisampledSupported = false;
#if SNAP_RHI_GL_ES && SNAP_RHI_OS_LINUX_BASED()
    {
        // We consider OVR supported if all three extensions are present and function addresses were found.
        features.isOVRMultiviewSupported = nullptr != glFramebufferTextureMultiviewOVR;
        features.isOVRMultiviewSupported &= nullptr != glFramebufferTextureMultisampleMultiviewOVR;
        features.isOVRMultiviewSupported &=
            snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "GL_OVR_multiview");
        features.isOVRMultiviewSupported &=
            snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "GL_OVR_multiview2");
        features.isOVRMultiviewSupported &= snap::rhi::backend::opengl::isExtensionSupported(
            glExtensionsList, "GL_OVR_multiview_multisampled_render_to_texture");

        // An extra sanity check to make sure that we can allocate enough texture storage for OVR.
        features.isOVRMultiviewSupported &= maxArrayTextureLayers >= 2;

        // An extra sanity check to make sure that the GPU driver is OVR-capable.
        if (features.isOVRMultiviewSupported) {
            GLint maxOVRViews = 0;
            glGetIntegerv(GL_MAX_VIEWS_OVR, &maxOVRViews);
            features.maxOVRViews = maxOVRViews;
            features.isOVRMultiviewSupported &= maxOVRViews >= 2;
        }

        features.isOVRMultiviewMultisampledSupported = features.isOVRMultiviewSupported;

        //     Google Pixel (maybe other Adrenos) supports only depth OVR attachments.
        //     Otherwise, FRAMEBUFFER_INCOMPLETE_VIEW_TARGETS_OVR is generated.
        //     I've double-checked via glGetFramebufferAttachmentParameteriv that
        //     DEPTH_ATTACHMENT and STENCIL_ATTACHMENT are not bound to DRAW_FRAMEBUFFER.
        features.isOVRDepthStencilSupported = features.isOVRMultiviewSupported && supportsDepthStencilAttachmentsOVR();
    }

    if (!snap::rhi::backend::opengl::checkOpenGLErrors()) {
        features.maxOVRViews = 0;
        features.isOVRMultiviewSupported = false;
        features.isOVRMultiviewMultisampledSupported = false;
        features.isOVRDepthStencilSupported = false;
    }
#endif // SNAP_RHI_GL_ES && SNAP_RHI_OS_LINUX_BASED()
}
} // unnamed namespace

namespace snap::rhi::backend::opengl::es30 {
snap::rhi::backend::opengl::Features Instance::buildFeatures(gl::APIVersion realApiVersion) {
    snap::rhi::backend::opengl::Features features = es20::Instance::buildFeatures(realApiVersion);

    std::string_view rendererName = reinterpret_cast<const GLchar*>(glGetString(GL_RENDERER));
    snap::rhi::backend::opengl::GPUModel gpuModel = snap::rhi::backend::opengl::getGPUModel(rendererName);

    std::string glExtensionsList = (char*)glGetString(GL_EXTENSIONS);
    initTexFormatProperties(features, glExtensionsList);
    initRenderbufferFormatProperties(features, glExtensionsList);

    features.apiVersion = gl::APIVersion::GLES30;
    features.isCustomTexturePackingSupported = true;
    features.isTextureParameterSupported = false;
    features.isFragmentPrimitiveIDSupported = false;
    features.isBlitFramebufferSupported = true;
    features.isCopyBufferSubDataSupported = true;
    features.isSamplerSupported = true;
    features.isVAOSupported = true;
    features.isDifferentBlendSettingsSupported = false;
    features.isMRTSupported = true;
    features.isStorageTextureSupported = true;
    features.isNativeUBOSupported = true;
    features.isPBOSupported = true;
    features.isUInt32IndexSupported = true;
    features.isSSBOSupported = false;
    features.isDepthStencilAttachmentSupported = true;
    features.isRasterizationDisableSupported = true;
    features.isMinMaxBlendModeSupported = true;
    features.isFBORenderToNonZeroMipSupported = true;
    features.isClampToBorderSupported =
        snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "GL_NV_texture_border_clamp") ||
        snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "GL_OES_texture_border_clamp") ||
        snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "GL_EXT_texture_border_clamp");
    features.isTexMinMaxLodSupported = true;
    features.isTexBaseMaxLevelSupported = true;
    features.isMapUnmapAvailable = isMapUnmapAvailable();
    features.isNPOTGenMipmapSupported = true;
    // https://registry.khronos.org/webgl/specs/latest/2.0/#5.19 texture swizzle not supported for WebGL
    features.isTexSwizzleSupported = !SNAP_RHI_PLATFORM_WEBASSEMBLY();
    features.isNPOTWrapModeSupported = true;
    features.isDepthCompareFuncSupported = true;
    features.isTexture3DSupported = true;
    features.isTextureArraySupported = true;
    features.isInstancingSupported = true;
    features.isDepthCubemapSupported = true;
    features.isMultisampleSupported = true;
    features.isPrimitiveRestartIndexSupported = true;
    features.isFramebufferFetchSupported =
        snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "GL_EXT_shader_framebuffer_fetch") ||
        snap::rhi::backend::opengl::isExtensionSupported(glExtensionsList, "GL_ARM_shader_framebuffer_fetch");
    features.isIntConstantRateSupported = true;
    // https://registry.khronos.org/OpenGL-Refpages/es3.0/html/glInvalidateFramebuffer.xhtml
    // Always available in ES3.0+
    features.isDiscardFramebufferSupported = true;
    features.isFenceSupported = true;
    features.isSemaphoreSupported = true;

    {
        features.isAlphaToCoverageSupported = (gpuModel != snap::rhi::backend::opengl::GPUModel::PowerVR_Rogue_G6430) &&
                                              (gpuModel != snap::rhi::backend::opengl::GPUModel::MaliT628) &&
                                              (gpuModel != snap::rhi::backend::opengl::GPUModel::MaliT720);
    }
    {
        GLint numFormats = 0;
        glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &numFormats);

        if (numFormats > 0) {
            features.isProgramBinarySupported = true;
        } else {
            features.isProgramBinarySupported = false;
        }
    }

    {
#if SNAP_RHI_PLATFORM_WEBASSEMBLY()
#ifndef GL_MAX_CLIENT_WAIT_TIMEOUT_WEBGL
#define GL_MAX_CLIENT_WAIT_TIMEOUT_WEBGL 0x9247
#endif
        GLint maxTimeout = 0;
        glGetIntegerv(GL_MAX_CLIENT_WAIT_TIMEOUT_WEBGL, &maxTimeout);
        features.maxClientWaitTimeout = static_cast<GLuint64>(maxTimeout);
#else
        features.maxClientWaitTimeout = 1e9;
#endif
    }

    features.isVtxAttribSupported.fill(true);

    GLint maxArrayTextureLayers = 0;
    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &maxArrayTextureLayers);
    features.maxArrayTextureLayers = static_cast<uint32_t>(maxArrayTextureLayers);

    {
        // https://registry.khronos.org/OpenGL-Refpages/es3.0/html/glGet.xhtml
        // The value must be at least 256.
        constexpr uint32_t minTexture3DSize = 256;

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

    GLint maxDrawBuffers = 0;
    glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);
    assert(maxDrawBuffers >= 1);

    features.maxDrawBuffers = std::max(1, maxDrawBuffers);

    initOVRExt(glExtensionsList, maxArrayTextureLayers, features);
    initClipCullDistanceExt(glExtensionsList, features);

    // https://en.wikipedia.org/wiki/OpenGL_Shading_Language
    features.shaderVersionHeader = "#version 300 es\n";

    return features;
}

void Instance::copyBufferSubData(
    GLenum readtarget, GLenum writetarget, GLintptr readoffset, GLintptr writeoffset, GLsizeiptr size) {
    glCopyBufferSubData(readtarget, writetarget, readoffset, writeoffset, size);
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

void Instance::genVertexArrays(GLsizei n, GLuint* arrays) {
    glGenVertexArrays(n, arrays);
}

void Instance::bindVertexArray(GLuint array) {
    glBindVertexArray(array);
}

void Instance::deleteVertexArrays(GLsizei n, const GLuint* arrays) {
    glDeleteVertexArrays(n, arrays);
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

void Instance::flushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length) {
    glFlushMappedBufferRange(target, offset, length);
}

void* Instance::mapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access) {
#if SNAP_RHI_PLATFORM_WEBASSEMBLY()
    return nullptr;
#else  // SNAP_RHI_PLATFORM_WEBASSEMBLY()
    auto result = glMapBufferRange(target, offset, length, access);
    return result;
#endif // SNAP_RHI_PLATFORM_WEBASSEMBLY()
}

GLboolean Instance::unmapBuffer(GLenum target) {
#if SNAP_RHI_PLATFORM_WEBASSEMBLY()
    return GL_FALSE;
#else  // SNAP_RHI_PLATFORM_WEBASSEMBLY()
    auto result = glUnmapBuffer(target);
    return result;
#endif // SNAP_RHI_PLATFORM_WEBASSEMBLY()
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

void Instance::vertexAttribI4iv(GLuint index, const GLint* v) {
    glVertexAttribI4iv(index, v);
}

void Instance::vertexAttribI4uiv(GLuint index, const GLuint* v) {
    glVertexAttribI4uiv(index, v);
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

void Instance::renderbufferStorageMultisample(snap::rhi::backend::opengl::TextureTarget target,
                                              GLsizei samples,
                                              snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                                              GLsizei width,
                                              GLsizei height) {
    glRenderbufferStorageMultisample(
        static_cast<GLenum>(target), samples, static_cast<GLenum>(internalformat), width, height);
}

void Instance::framebufferTextureLayer(snap::rhi::backend::opengl::FramebufferTarget target,
                                       snap::rhi::backend::opengl::FramebufferAttachmentTarget attachment,
                                       snap::rhi::backend::opengl::TextureId texture,
                                       int32_t level,
                                       int32_t layer) {
    glFramebufferTextureLayer(
        static_cast<GLenum>(target), static_cast<GLenum>(attachment), static_cast<GLuint>(texture), level, layer);
}

void Instance::framebufferTextureMultiviewOVR(snap::rhi::backend::opengl::FramebufferTarget target,
                                              snap::rhi::backend::opengl::FramebufferAttachmentTarget attachment,
                                              snap::rhi::backend::opengl::TextureId texture,
                                              int32_t level,
                                              int32_t baseViewIndex,
                                              int32_t numViews) {
#if SNAP_RHI_GL_ES && SNAP_RHI_OS_LINUX_BASED()
    glFramebufferTextureMultiviewOVR(static_cast<GLenum>(target),
                                     static_cast<GLenum>(attachment),
                                     static_cast<GLuint>(texture),
                                     level,
                                     baseViewIndex,
                                     numViews);
#else
    snap::rhi::common::throwException<UnsupportedOperationException>("[GLES30] OVR doesn't supported");
#endif
}

void Instance::framebufferTextureMultisampleMultiviewOVR(
    snap::rhi::backend::opengl::FramebufferTarget target,
    snap::rhi::backend::opengl::FramebufferAttachmentTarget attachment,
    snap::rhi::backend::opengl::TextureId texture,
    int32_t level,
    int32_t samples,
    int32_t baseViewIndex,
    int32_t numViews) {
#if SNAP_RHI_GL_ES && SNAP_RHI_OS_LINUX_BASED()
    glFramebufferTextureMultisampleMultiviewOVR(static_cast<GLenum>(target),
                                                static_cast<GLenum>(attachment),
                                                static_cast<GLuint>(texture),
                                                level,
                                                samples,
                                                baseViewIndex,
                                                numViews);
#else
    snap::rhi::common::throwException<UnsupportedOperationException>("[GLES30] OVR doesn't supported");
#endif
}

void Instance::drawBuffers(GLsizei n, const snap::rhi::backend::opengl::FramebufferAttachmentTarget* bufs) {
    glDrawBuffers(n, reinterpret_cast<const GLenum*>(bufs));
}

void Instance::discardFramebuffer(snap::rhi::backend::opengl::FramebufferTarget target,
                                  GLsizei numAttachments,
                                  const snap::rhi::backend::opengl::FramebufferAttachmentTarget* attachments) {
    glInvalidateFramebuffer(static_cast<GLenum>(target), numAttachments, reinterpret_cast<const GLenum*>(attachments));
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
    GLenum status;
#if SNAP_RHI_PLATFORM_WEBASSEMBLY()
    status = emscripten_glClientWaitSync(reinterpret_cast<GLsync>(sync), flags, 0, 0);
#else
    status = glClientWaitSync(reinterpret_cast<GLsync>(sync), flags, timeout);
#endif
    return status;
}

void Instance::deleteSync(SNAP_RHI_GLsync sync) {
    glDeleteSync(reinterpret_cast<GLsync>(sync));
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

void Instance::samplerParameterf(GLuint sampler, GLenum pname, GLfloat param) {
    glSamplerParameterf(sampler, pname, param);
}

GLboolean Instance::isSampler(GLuint id) {
    return glIsSampler(id);
}

void Instance::getInternalformativ(snap::rhi::backend::opengl::TextureTarget target,
                                   snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                                   GLenum pname,
                                   GLsizei bufSize,
                                   GLint* params) {
    // There is crash on Pixel 3a and S21 Ultra
    // glGetInternalformativ(static_cast<GLenum>(target), static_cast<GLenum>(internalformat), pname, bufSize, params);
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
    GLuint result = glGetUniformBlockIndex(program, uniformBlockName);
    return result;
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

void Instance::samplerParameterfv(GLuint sampler, GLenum pname, const GLfloat* params) {
    glSamplerParameterfv(sampler, pname, params);
}

void Instance::samplerParameteri(GLuint sampler, GLenum pname, GLint param) {
    glSamplerParameteri(sampler, pname, param);
}

void Instance::getTexParameteriv(GLenum target, GLenum pname, GLint* params) {
    glGetTexParameteriv(target, pname, params);
}

void Instance::pixelStorei(GLenum pname, GLint param) {
    glPixelStorei(pname, param);
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

void Instance::getQueryObjectuiv(GLuint id, GLenum pname, GLuint* params) {
    glGetQueryObjectuiv(id, pname, params);
}

} // namespace snap::rhi::backend::opengl::es30
#endif // SNAP_RHI_GLES30
