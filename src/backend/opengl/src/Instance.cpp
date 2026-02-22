#include "snap/rhi/backend/opengl/Instance.h"

#include "snap/rhi/backend/common/Logging.hpp"

#include <unordered_set>
#include <vector>

namespace {
inline void addFormat(snap::rhi::backend::opengl::Features& features,
                      const std::unordered_set<GLint>& supportedCompressedFormats,
                      const snap::rhi::PixelFormat format,
                      const snap::rhi::backend::opengl::SizedInternalFormat internalFormat,
                      const snap::rhi::backend::opengl::FormatGroup formatGroup,
                      const snap::rhi::backend::opengl::FormatDataType formatDataType =
                          snap::rhi::backend::opengl::FormatDataType::UNSIGNED_BYTE) noexcept {
    if (supportedCompressedFormats.count(static_cast<GLint>(internalFormat)) == 0) {
        return;
    }

    features.textureFormat[static_cast<uint32_t>(format)] = {internalFormat, formatGroup, formatDataType};

    auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(format)];
    formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::Uploadable;
    formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
}
} // unnamed namespace

namespace snap::rhi::backend::opengl {
void addCompressedFormat(Features& features) {
    GLint numCompressedFormats;
    glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &numCompressedFormats);

    std::vector<GLint> compressedFormats(numCompressedFormats);
    glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, compressedFormats.data());

    std::unordered_set<GLint> supportedCompressedFormats;
    for (const auto format : compressedFormats) {
        supportedCompressedFormats.insert(format);
    }

    addFormat(features,
              supportedCompressedFormats,
              snap::rhi::PixelFormat::ETC_R8G8B8_Unorm,
              snap::rhi::backend::opengl::SizedInternalFormat::COMPRESSED_RGB8_ETC1,
              snap::rhi::backend::opengl::FormatGroup::RGB);

    addFormat(features,
              supportedCompressedFormats,
              snap::rhi::PixelFormat::ETC2_R8G8B8_Unorm,
              snap::rhi::backend::opengl::SizedInternalFormat::COMPRESSED_RGB8_ETC2,
              snap::rhi::backend::opengl::FormatGroup::RGB);
    addFormat(features,
              supportedCompressedFormats,
              snap::rhi::PixelFormat::ETC2_R8G8B8_sRGB,
              snap::rhi::backend::opengl::SizedInternalFormat::COMPRESSED_SRGB8_ETC2,
              snap::rhi::backend::opengl::FormatGroup::RGB);

    addFormat(features,
              supportedCompressedFormats,
              snap::rhi::PixelFormat::ETC2_R8G8B8A1_Unorm,
              snap::rhi::backend::opengl::SizedInternalFormat::COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2,
              snap::rhi::backend::opengl::FormatGroup::RGBA);
    addFormat(features,
              supportedCompressedFormats,
              snap::rhi::PixelFormat::ETC2_R8G8B8A1_sRGB,
              snap::rhi::backend::opengl::SizedInternalFormat::COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2,
              snap::rhi::backend::opengl::FormatGroup::RGBA);

    addFormat(features,
              supportedCompressedFormats,
              snap::rhi::PixelFormat::ETC2_R8G8B8A8_Unorm,
              snap::rhi::backend::opengl::SizedInternalFormat::COMPRESSED_RGBA8_ETC2_EAC,
              snap::rhi::backend::opengl::FormatGroup::RGBA);
    addFormat(features,
              supportedCompressedFormats,
              snap::rhi::PixelFormat::ETC2_R8G8B8A8_sRGB,
              snap::rhi::backend::opengl::SizedInternalFormat::COMPRESSED_SRGB8_ALPHA8_ETC2_EAC,
              snap::rhi::backend::opengl::FormatGroup::RGBA);

    if (features.isS3TCCompressionFormatFamilySupported) { // EXT_texture_sRGB + EXT_texture_compression_s3tc
        {                                                  // snap::rhi::PixelFormat::BC3_sRGBA
            features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::BC3_sRGBA)] = {
                snap::rhi::backend::opengl::SizedInternalFormat::COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT,
                snap::rhi::backend::opengl::FormatGroup::RGBA,
                snap::rhi::backend::opengl::FormatDataType::UNSIGNED_BYTE};

            auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::BC3_sRGBA)];
            formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
            formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
        }
        { // snap::rhi::PixelFormat::BC3_RGBA
            features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::BC3_RGBA)] = {
                snap::rhi::backend::opengl::SizedInternalFormat::COMPRESSED_RGBA_S3TC_DXT5_EXT,
                snap::rhi::backend::opengl::FormatGroup::RGBA,
                snap::rhi::backend::opengl::FormatDataType::UNSIGNED_BYTE};

            auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::BC3_RGBA)];
            formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
            formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
        }
    }
    if (features.isBPTCCompressionFormatFamilySupported) {
        { // snap::rhi::PixelFormat::BC7_sRGBA
            features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::BC7_sRGBA)] = {
                snap::rhi::backend::opengl::SizedInternalFormat::COMPRESSED_SRGB_ALPHA_BPTC_UNORM,
                snap::rhi::backend::opengl::FormatGroup::RGBA,
                snap::rhi::backend::opengl::FormatDataType::UNSIGNED_BYTE};

            auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::BC7_sRGBA)];
            formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
            formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
        }
        { // snap::rhi::PixelFormat::BC7_RGBA
            features.textureFormat[static_cast<uint32_t>(snap::rhi::PixelFormat::BC7_RGBA)] = {
                snap::rhi::backend::opengl::SizedInternalFormat::COMPRESSED_RGBA_BPTC_UNORM,
                snap::rhi::backend::opengl::FormatGroup::RGBA,
                snap::rhi::backend::opengl::FormatDataType::UNSIGNED_BYTE};

            auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::BC7_RGBA)];
            formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
            formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
        }
    }

    if (features.isASTCCompressionFormatFamilySupported) { // GL_KHR_texture_compression_astc_ldr
        {                                                  // snap::rhi::PixelFormat::ASTC_4x4_Unorm
            addFormat(features,
                      supportedCompressedFormats,
                      snap::rhi::PixelFormat::ASTC_4x4_Unorm,
                      snap::rhi::backend::opengl::SizedInternalFormat::COMPRESSED_RGBA_ASTC_4x4,
                      snap::rhi::backend::opengl::FormatGroup::RGBA);
        }
        { // snap::rhi::PixelFormat::ASTC_4x4_sRGB
            addFormat(features,
                      supportedCompressedFormats,
                      snap::rhi::PixelFormat::ASTC_4x4_sRGB,
                      snap::rhi::backend::opengl::SizedInternalFormat::COMPRESSED_SRGB8_ALPHA8_ASTC_4x4,
                      snap::rhi::backend::opengl::FormatGroup::RGBA);
        }
    }
}

bool checkOpenGLErrors() {
    std::string message = getGLErrorsString();
    if (message.empty()) {
        return true;
    }

    SNAP_RHI_LOGE("[checkOpenGLErrors]: OpenGL errors: %s\n;", message.c_str());
    return false;
}
} // namespace snap::rhi::backend::opengl
