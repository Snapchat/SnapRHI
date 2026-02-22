#include "snap/rhi/backend/opengl/Format.h"

namespace snap::rhi::backend::opengl {

// https://www.khronos.org/registry/OpenGL-Refpages/es3.0/html/glTexStorage2D.xhtml
FormatDataType getFormatDataType(SizedInternalFormat internalFormat) noexcept {
    switch (internalFormat) {
        case SizedInternalFormat::R8:
            return FormatDataType::UNSIGNED_BYTE;
        case SizedInternalFormat::R8_SNORM:
            return FormatDataType::BYTE;
        case SizedInternalFormat::R16F:
            return FormatDataType::HALF_FLOAT;
        case SizedInternalFormat::R32F:
            return FormatDataType::FLOAT;
        case SizedInternalFormat::R8UI:
            return FormatDataType::UNSIGNED_BYTE;
        case SizedInternalFormat::R8I:
            return FormatDataType::BYTE;
        case SizedInternalFormat::R16UI:
            return FormatDataType::UNSIGNED_SHORT;
        case SizedInternalFormat::R16I:
            return FormatDataType::SHORT;
        case SizedInternalFormat::R32UI:
            return FormatDataType::UNSIGNED_INT;
        case SizedInternalFormat::R32I:
            return FormatDataType::INT;
        case SizedInternalFormat::RG8:
            return FormatDataType::UNSIGNED_BYTE;
        case SizedInternalFormat::RG8_SNORM:
            return FormatDataType::BYTE;
        case SizedInternalFormat::RG16F:
            return FormatDataType::HALF_FLOAT;
        case SizedInternalFormat::RG32F:
            return FormatDataType::FLOAT;
        case SizedInternalFormat::RG8UI:
            return FormatDataType::UNSIGNED_BYTE;
        case SizedInternalFormat::RG8I:
            return FormatDataType::BYTE;
        case SizedInternalFormat::RG16UI:
            return FormatDataType::UNSIGNED_SHORT;
        case SizedInternalFormat::RG16I:
            return FormatDataType::SHORT;
        case SizedInternalFormat::RG32UI:
            return FormatDataType::UNSIGNED_INT;
        case SizedInternalFormat::RG32I:
            return FormatDataType::INT;
        case SizedInternalFormat::RGB8:
            return FormatDataType::UNSIGNED_BYTE;
        case SizedInternalFormat::SRGB8:
            return FormatDataType::UNSIGNED_BYTE;
        case SizedInternalFormat::RGB565:
            return FormatDataType::UNSIGNED_BYTE;
        case SizedInternalFormat::RGB8_SNORM:
            return FormatDataType::BYTE;
        case SizedInternalFormat::R11F_G11F_B10F:
            return FormatDataType::UNSIGNED_INT_10F_11F_11F_REV;
        case SizedInternalFormat::RGB9_E5:
            return FormatDataType::UNSIGNED_INT_5_9_9_9_REV;
        case SizedInternalFormat::RGB16F:
            return FormatDataType::HALF_FLOAT;
        case SizedInternalFormat::RGB32F:
            return FormatDataType::FLOAT;
        case SizedInternalFormat::RGB8UI:
            return FormatDataType::UNSIGNED_BYTE;
        case SizedInternalFormat::RGB8I:
            return FormatDataType::BYTE;
        case SizedInternalFormat::RGB16UI:
            return FormatDataType::UNSIGNED_SHORT;
        case SizedInternalFormat::RGB16I:
            return FormatDataType::SHORT;
        case SizedInternalFormat::RGB32UI:
            return FormatDataType::UNSIGNED_INT;
        case SizedInternalFormat::RGB32I:
            return FormatDataType::INT;
        case SizedInternalFormat::RGBA8:
            return FormatDataType::UNSIGNED_BYTE;
        case SizedInternalFormat::SRGB8_ALPHA8:
            return FormatDataType::UNSIGNED_BYTE;
        case SizedInternalFormat::RGBA8_SNORM:
            return FormatDataType::BYTE;
        case SizedInternalFormat::RGB5_A1:
            return FormatDataType::UNSIGNED_BYTE;
        case SizedInternalFormat::RGBA4:
            return FormatDataType::UNSIGNED_BYTE;
        case SizedInternalFormat::RGB10_A2:
            return FormatDataType::UNSIGNED_INT_2_10_10_10_REV;
        case SizedInternalFormat::RGBA16F:
            return FormatDataType::HALF_FLOAT;
        case SizedInternalFormat::RGBA32F:
            return FormatDataType::FLOAT;
        case SizedInternalFormat::RGBA8UI:
            return FormatDataType::UNSIGNED_BYTE;
        case SizedInternalFormat::RGBA8I:
            return FormatDataType::BYTE;
        case SizedInternalFormat::RGB10_A2UI:
            return FormatDataType::UNSIGNED_INT_2_10_10_10_REV;
        case SizedInternalFormat::RGBA16UI:
            return FormatDataType::UNSIGNED_SHORT;
        case SizedInternalFormat::RGBA16I:
            return FormatDataType::SHORT;
        case SizedInternalFormat::RGBA32I:
            return FormatDataType::INT;
        case SizedInternalFormat::RGBA32UI:
            return FormatDataType::UNSIGNED_INT;
        case SizedInternalFormat::DEPTH16:
            return FormatDataType::UNSIGNED_SHORT;
        case SizedInternalFormat::DEPTH24:
            return FormatDataType::UNSIGNED_INT;
        case SizedInternalFormat::DEPTH32F:
            return FormatDataType::FLOAT;
        case SizedInternalFormat::DEPTH24_STENCIL8:
            return FormatDataType::UNSIGNED_INT_24_8;
        case SizedInternalFormat::DEPTH32F_STENCIL8:
            return FormatDataType::FLOAT_32_UNSIGNED_INT_24_8_REV;

#if !SNAP_RHI_OS_IOS()
        case SizedInternalFormat::R16:
            return FormatDataType::UNSIGNED_SHORT;
        case SizedInternalFormat::R16_SNORM:
            return FormatDataType::SHORT;
        case SizedInternalFormat::RG16:
            return FormatDataType::UNSIGNED_SHORT;
        case SizedInternalFormat::RG16_SNORM:
            return FormatDataType::SHORT;
        case SizedInternalFormat::RGBA16:
            return FormatDataType::UNSIGNED_SHORT;
        case SizedInternalFormat::RGBA16_SNORM:
            return FormatDataType::SHORT;
#endif // !SNAP_RHI_OS_IOS()

        case SizedInternalFormat::DEPRECATED_BGRA:
            return FormatDataType::UNSIGNED_BYTE;

        case SizedInternalFormat::BGRA8:
            return FormatDataType::UNSIGNED_BYTE;

            // https://www.khronos.org/registry/OpenGL-Refpages/es3.0/html/glTexImage2D.xhtml
        case SizedInternalFormat::DEPRECATED_LUMINANCE:
        case SizedInternalFormat::DEPRECATED_LUMINANCE_ALPHA:
        case SizedInternalFormat::DEPRECATED_INTENSITY:
            return FormatDataType::UNSIGNED_BYTE;

            // https://github.com/alexzk1/luminia-qt5/blob/1ae744caeb8870299b868ee2c14edcd8f15714a8/src/textureformat.h
        case SizedInternalFormat::DEPRECATED_LUMINANCE4:
        case SizedInternalFormat::DEPRECATED_LUMINANCE8:
            return FormatDataType::UNSIGNED_BYTE;

        case SizedInternalFormat::DEPRECATED_LUMINANCE12:
        case SizedInternalFormat::DEPRECATED_LUMINANCE16:
            return FormatDataType::UNSIGNED_SHORT;

        case SizedInternalFormat::DEPRECATED_LUMINANCE4_ALPHA4:
        case SizedInternalFormat::DEPRECATED_LUMINANCE6_ALPHA2:
        case SizedInternalFormat::DEPRECATED_LUMINANCE8_ALPHA8:
        case SizedInternalFormat::DEPRECATED_LUMINANCE12_ALPHA4:
            return FormatDataType::UNSIGNED_BYTE;

        case SizedInternalFormat::DEPRECATED_LUMINANCE12_ALPHA12:
        case SizedInternalFormat::DEPRECATED_LUMINANCE16_ALPHA16:
            return FormatDataType::UNSIGNED_SHORT;

        case SizedInternalFormat::DEPRECATED_INTENSITY4:
        case SizedInternalFormat::DEPRECATED_INTENSITY8:
            return FormatDataType::UNSIGNED_BYTE;

        case SizedInternalFormat::DEPRECATED_INTENSITY12:
        case SizedInternalFormat::DEPRECATED_INTENSITY16:
            return FormatDataType::UNSIGNED_SHORT;

        case SizedInternalFormat::DEPRECATED_R:
        case SizedInternalFormat::DEPRECATED_RGB:
        case SizedInternalFormat::DEPRECATED_RGBA:
            return FormatDataType::UNSIGNED_BYTE;

        case SizedInternalFormat::DEPRECATED_DEPTH_STENCIL:
            return FormatDataType::UNSIGNED_INT_24_8;

        case SizedInternalFormat::COMPRESSED_RGB8_ETC1:
        case SizedInternalFormat::COMPRESSED_R11_EAC:
        case SizedInternalFormat::COMPRESSED_SIGNED_R11_EAC:
        case SizedInternalFormat::COMPRESSED_RG11_EAC:
        case SizedInternalFormat::COMPRESSED_SIGNED_RG11_EAC:
        case SizedInternalFormat::COMPRESSED_RGB8_ETC2:
        case SizedInternalFormat::COMPRESSED_SRGB8_ETC2:
        case SizedInternalFormat::COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:
        case SizedInternalFormat::COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:
        case SizedInternalFormat::COMPRESSED_RGBA8_ETC2_EAC:
        case SizedInternalFormat::COMPRESSED_SRGB8_ALPHA8_ETC2_EAC:
        case SizedInternalFormat::COMPRESSED_SRGB_ALPHA_BPTC_UNORM:
        case SizedInternalFormat::COMPRESSED_RGBA_BPTC_UNORM:
        case SizedInternalFormat::COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
        case SizedInternalFormat::COMPRESSED_RGBA_S3TC_DXT5_EXT:
        case SizedInternalFormat::COMPRESSED_RGBA_ASTC_4x4:
        case SizedInternalFormat::COMPRESSED_SRGB8_ALPHA8_ASTC_4x4:
            return FormatDataType::UNKNOWN_OR_COMPRESSED;

        default:
            assert(false && "Caught unkown internal size.");
            return FormatDataType::UNKNOWN_OR_COMPRESSED;
    }
}

// https://www.khronos.org/registry/OpenGL-Refpages/es3.0/html/glTexStorage2D.xhtml
FormatGroup getFormatGroup(SizedInternalFormat internalFormat) noexcept {
    switch (internalFormat) {
        case SizedInternalFormat::R8:
        case SizedInternalFormat::R8_SNORM:
        case SizedInternalFormat::R16F:
        case SizedInternalFormat::R32F:
            return FormatGroup::R;

        case SizedInternalFormat::R8UI:
        case SizedInternalFormat::R8I:
        case SizedInternalFormat::R16UI:
        case SizedInternalFormat::R16I:
        case SizedInternalFormat::R32UI:
        case SizedInternalFormat::R32I:
            return FormatGroup::R_INTEGER;

        case SizedInternalFormat::RG8:
        case SizedInternalFormat::RG8_SNORM:
        case SizedInternalFormat::RG16F:
        case SizedInternalFormat::RG32F:
            return FormatGroup::RG;

        case SizedInternalFormat::RG8UI:
        case SizedInternalFormat::RG8I:
        case SizedInternalFormat::RG16UI:
        case SizedInternalFormat::RG16I:
        case SizedInternalFormat::RG32UI:
        case SizedInternalFormat::RG32I:
            return FormatGroup::RG_INTEGER;

        case SizedInternalFormat::RGB8:
        case SizedInternalFormat::SRGB8:
        case SizedInternalFormat::RGB565:
        case SizedInternalFormat::RGB8_SNORM:
        case SizedInternalFormat::R11F_G11F_B10F:
        case SizedInternalFormat::RGB9_E5:
        case SizedInternalFormat::RGB16F:
        case SizedInternalFormat::RGB32F:
            return FormatGroup::RGB;

        case SizedInternalFormat::RGB8UI:
        case SizedInternalFormat::RGB8I:
        case SizedInternalFormat::RGB16UI:
        case SizedInternalFormat::RGB16I:
        case SizedInternalFormat::RGB32UI:
        case SizedInternalFormat::RGB32I:
            return FormatGroup::RGB_INTEGER;

        case SizedInternalFormat::RGBA8:
        case SizedInternalFormat::SRGB8_ALPHA8:
        case SizedInternalFormat::RGBA8_SNORM:
        case SizedInternalFormat::RGB5_A1:
        case SizedInternalFormat::RGBA4:
        case SizedInternalFormat::RGB10_A2:
        case SizedInternalFormat::RGBA16F:
        case SizedInternalFormat::RGBA32F:
        case SizedInternalFormat::DEPRECATED_RGBA:
            return FormatGroup::RGBA;

        case SizedInternalFormat::RGBA8UI:
        case SizedInternalFormat::RGBA8I:
        case SizedInternalFormat::RGB10_A2UI:
        case SizedInternalFormat::RGBA16UI:
        case SizedInternalFormat::RGBA16I:
        case SizedInternalFormat::RGBA32I:
        case SizedInternalFormat::RGBA32UI:
            return FormatGroup::RGBA_INTEGER;

        case SizedInternalFormat::BGRA8:
            return FormatGroup::BGRA;

        case SizedInternalFormat::DEPTH16:
        case SizedInternalFormat::DEPTH24:
        case SizedInternalFormat::DEPTH32F:
            return FormatGroup::DEPTH;

        case SizedInternalFormat::DEPTH24_STENCIL8:
        case SizedInternalFormat::DEPTH32F_STENCIL8:
            return FormatGroup::DEPTH_STENCIL;

        case SizedInternalFormat::COMPRESSED_R11_EAC:
        case SizedInternalFormat::COMPRESSED_SIGNED_R11_EAC:
            return FormatGroup::R;

        case SizedInternalFormat::COMPRESSED_RG11_EAC:
        case SizedInternalFormat::COMPRESSED_SIGNED_RG11_EAC:
            return FormatGroup::RG;

        case SizedInternalFormat::COMPRESSED_RGB8_ETC1:
        case SizedInternalFormat::COMPRESSED_RGB8_ETC2:
        case SizedInternalFormat::COMPRESSED_SRGB8_ETC2:
            return FormatGroup::RGB;

        case SizedInternalFormat::COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:
        case SizedInternalFormat::COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:
        case SizedInternalFormat::COMPRESSED_RGBA8_ETC2_EAC:
        case SizedInternalFormat::COMPRESSED_SRGB8_ALPHA8_ETC2_EAC:
        case SizedInternalFormat::COMPRESSED_SRGB_ALPHA_BPTC_UNORM:
        case SizedInternalFormat::COMPRESSED_RGBA_BPTC_UNORM:
        case SizedInternalFormat::COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
        case SizedInternalFormat::COMPRESSED_RGBA_S3TC_DXT5_EXT:
        case SizedInternalFormat::COMPRESSED_RGBA_ASTC_4x4:
        case SizedInternalFormat::COMPRESSED_SRGB8_ALPHA8_ASTC_4x4:
            return FormatGroup::RGBA;

#if !SNAP_RHI_OS_IOS()
        case SizedInternalFormat::R16:
        case SizedInternalFormat::R16_SNORM:
            return FormatGroup::R;
        case SizedInternalFormat::RG16:
        case SizedInternalFormat::RG16_SNORM:
            return FormatGroup::RG;
        case SizedInternalFormat::RGBA16:
        case SizedInternalFormat::RGBA16_SNORM:
            return FormatGroup::RGBA;
#endif // !SNAP_RHI_OS_IOS()

        case SizedInternalFormat::DEPRECATED_LUMINANCE:
        case SizedInternalFormat::DEPRECATED_LUMINANCE4:
        case SizedInternalFormat::DEPRECATED_LUMINANCE8:
        case SizedInternalFormat::DEPRECATED_LUMINANCE12:
        case SizedInternalFormat::DEPRECATED_LUMINANCE16:
            return FormatGroup::DEPRECATED_LUMINANCE;

        case SizedInternalFormat::DEPRECATED_LUMINANCE_ALPHA:
        case SizedInternalFormat::DEPRECATED_LUMINANCE4_ALPHA4:
        case SizedInternalFormat::DEPRECATED_LUMINANCE6_ALPHA2:
        case SizedInternalFormat::DEPRECATED_LUMINANCE8_ALPHA8:
        case SizedInternalFormat::DEPRECATED_LUMINANCE12_ALPHA4:
        case SizedInternalFormat::DEPRECATED_LUMINANCE12_ALPHA12:
        case SizedInternalFormat::DEPRECATED_LUMINANCE16_ALPHA16:
            return FormatGroup::DEPRECATED_LUMINANCE_ALPHA;

        case SizedInternalFormat::DEPRECATED_INTENSITY:
        case SizedInternalFormat::DEPRECATED_INTENSITY4:
        case SizedInternalFormat::DEPRECATED_INTENSITY8:
        case SizedInternalFormat::DEPRECATED_INTENSITY12:
        case SizedInternalFormat::DEPRECATED_INTENSITY16:
            return FormatGroup::DEPRECATED_INTENSITY;

        case SizedInternalFormat::DEPRECATED_R:
            return FormatGroup::R;

        case SizedInternalFormat::DEPRECATED_RGB:
            return FormatGroup::RGB;

        case SizedInternalFormat::DEPRECATED_BGRA:
            return FormatGroup::BGRA;

        case SizedInternalFormat::DEPRECATED_DEPTH_STENCIL:
            return FormatGroup::DEPTH_STENCIL;

        default:
            assert(false && "Caught unkown internal size.");
            return FormatGroup::UNKNOWN;
    }
}

bool isFormatCompressed(SizedInternalFormat internalFormat) noexcept {
    switch (internalFormat) {
        case SizedInternalFormat::COMPRESSED_RGB8_ETC1:
        case SizedInternalFormat::COMPRESSED_R11_EAC:
        case SizedInternalFormat::COMPRESSED_SIGNED_R11_EAC:
        case SizedInternalFormat::COMPRESSED_RG11_EAC:
        case SizedInternalFormat::COMPRESSED_SIGNED_RG11_EAC:
        case SizedInternalFormat::COMPRESSED_RGB8_ETC2:
        case SizedInternalFormat::COMPRESSED_SRGB8_ETC2:
        case SizedInternalFormat::COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:
        case SizedInternalFormat::COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:
        case SizedInternalFormat::COMPRESSED_RGBA8_ETC2_EAC:
        case SizedInternalFormat::COMPRESSED_SRGB8_ALPHA8_ETC2_EAC:
        case SizedInternalFormat::COMPRESSED_SRGB_ALPHA_BPTC_UNORM:
        case SizedInternalFormat::COMPRESSED_RGBA_BPTC_UNORM:
        case SizedInternalFormat::COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
        case SizedInternalFormat::COMPRESSED_RGBA_S3TC_DXT5_EXT:
        case SizedInternalFormat::COMPRESSED_RGBA_ASTC_4x4:
        case SizedInternalFormat::COMPRESSED_SRGB8_ALPHA8_ASTC_4x4:
            return true;

        default:
            return false;
    }
}

DepthStencilFormatTraits getDepthStencilFormatTraits(SizedInternalFormat internalFormat) noexcept {
    switch (internalFormat) {
        case SizedInternalFormat::DEPTH16:
        case SizedInternalFormat::DEPTH24:
        case SizedInternalFormat::DEPTH32F:
            return DepthStencilFormatTraits::HasDepthAspect;
        case SizedInternalFormat::DEPTH24_STENCIL8:
        case SizedInternalFormat::DEPTH32F_STENCIL8:
        case SizedInternalFormat::DEPRECATED_DEPTH_STENCIL:
            return DepthStencilFormatTraits::HasDepthStencilAspects;
        default:
            return DepthStencilFormatTraits::None;
    }
}

snap::rhi::common::zstring_view toString(SizedInternalFormat internalFormat) noexcept {
    switch (internalFormat) {
        case SizedInternalFormat::UNKNOWN:
            return "UNKNOWN";
        case SizedInternalFormat::R8:
            return "R8";
        case SizedInternalFormat::R8_SNORM:
            return "R8_SNORM";
        case SizedInternalFormat::R16F:
            return "R16F";
        case SizedInternalFormat::R32F:
            return "R32F";
        case SizedInternalFormat::R8UI:
            return "R8UI";
        case SizedInternalFormat::R8I:
            return "R8I";
        case SizedInternalFormat::R16UI:
            return "R16UI";
        case SizedInternalFormat::R16I:
            return "R16I";
        case SizedInternalFormat::R32UI:
            return "R32UI";
        case SizedInternalFormat::R32I:
            return "R32I";
        case SizedInternalFormat::RG8:
            return "RG8";
        case SizedInternalFormat::RG8_SNORM:
            return "RG8_SNORM";
        case SizedInternalFormat::RG16F:
            return "RG16F";
        case SizedInternalFormat::RG32F:
            return "RG32F";
        case SizedInternalFormat::RG8UI:
            return "RG8UI";
        case SizedInternalFormat::RG8I:
            return "RG8I";
        case SizedInternalFormat::RG16UI:
            return "RG16UI";
        case SizedInternalFormat::RG16I:
            return "RG16I";
        case SizedInternalFormat::RG32UI:
            return "RG32UI";
        case SizedInternalFormat::RG32I:
            return "RG32I";
        case SizedInternalFormat::RGB8:
            return "RGB8";
        case SizedInternalFormat::SRGB8:
            return "SRGB8";
        case SizedInternalFormat::RGB565:
            return "RGB565";
        case SizedInternalFormat::RGB8_SNORM:
            return "RGB8_SNORM";
        case SizedInternalFormat::R11F_G11F_B10F:
            return "R11F_G11F_B10F";
        case SizedInternalFormat::RGB9_E5:
            return "RGB9_E5";
        case SizedInternalFormat::RGB16F:
            return "RGB16F";
        case SizedInternalFormat::RGB32F:
            return "RGB32F";
        case SizedInternalFormat::RGB8UI:
            return "RGB8UI";
        case SizedInternalFormat::RGB8I:
            return "RGB8I";
        case SizedInternalFormat::RGB16UI:
            return "RGB16UI";
        case SizedInternalFormat::RGB16I:
            return "RGB16I";
        case SizedInternalFormat::RGB32UI:
            return "RGB32UI";
        case SizedInternalFormat::RGB32I:
            return "RGB32I";
        case SizedInternalFormat::RGBA8:
            return "RGBA8";
        case SizedInternalFormat::SRGB8_ALPHA8:
            return "SRGB8_ALPHA8";
        case SizedInternalFormat::RGBA8_SNORM:
            return "RGBA8_SNORM";
        case SizedInternalFormat::RGB5_A1:
            return "RGB5_A1";
        case SizedInternalFormat::RGBA4:
            return "RGBA4";
        case SizedInternalFormat::RGB10_A2:
            return "RGB10_A2";
        case SizedInternalFormat::RGBA16F:
            return "RGBA16F";
        case SizedInternalFormat::RGBA32F:
            return "RGBA32F";
        case SizedInternalFormat::RGBA8UI:
            return "RGBA8UI";
        case SizedInternalFormat::RGBA8I:
            return "RGBA8I";
        case SizedInternalFormat::RGB10_A2UI:
            return "RGB10_A2UI";
        case SizedInternalFormat::RGBA16UI:
            return "RGBA16UI";
        case SizedInternalFormat::RGBA16I:
            return "RGBA16I";
        case SizedInternalFormat::RGBA32I:
            return "RGBA32I";
        case SizedInternalFormat::RGBA32UI:
            return "RGBA32UI";
        case SizedInternalFormat::BGRA8:
            return "BGRA8";
        case SizedInternalFormat::DEPTH16:
            return "DEPTH16";
        case SizedInternalFormat::DEPTH24:
            return "DEPTH24";
        case SizedInternalFormat::DEPTH32:
            return "DEPTH32";
        case SizedInternalFormat::DEPTH32F:
            return "DEPTH32F";
        case SizedInternalFormat::DEPTH24_STENCIL8:
            return "DEPTH24_STENCIL8";
        case SizedInternalFormat::DEPTH32F_STENCIL8:
            return "DEPTH32F_STENCIL8";
        case SizedInternalFormat::COMPRESSED_RGB8_ETC1:
            return "COMPRESSED_RGB8_ETC1";
        case SizedInternalFormat::COMPRESSED_R11_EAC:
            return "COMPRESSED_R11_EAC";
        case SizedInternalFormat::COMPRESSED_SIGNED_R11_EAC:
            return "COMPRESSED_SIGNED_R11_EAC";
        case SizedInternalFormat::COMPRESSED_RG11_EAC:
            return "COMPRESSED_RG11_EAC";
        case SizedInternalFormat::COMPRESSED_SIGNED_RG11_EAC:
            return "COMPRESSED_SIGNED_RG11_EAC";
        case SizedInternalFormat::COMPRESSED_RGB8_ETC2:
            return "COMPRESSED_RGB8_ETC2";
        case SizedInternalFormat::COMPRESSED_SRGB8_ETC2:
            return "COMPRESSED_SRGB8_ETC2";
        case SizedInternalFormat::COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:
            return "COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2";
        case SizedInternalFormat::COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:
            return "COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2";
        case SizedInternalFormat::COMPRESSED_RGBA8_ETC2_EAC:
            return "COMPRESSED_RGBA8_ETC2_EAC";
        case SizedInternalFormat::COMPRESSED_SRGB8_ALPHA8_ETC2_EAC:
            return "COMPRESSED_SRGB8_ALPHA8_ETC2_EAC";
        case SizedInternalFormat::COMPRESSED_SRGB_ALPHA_BPTC_UNORM:
            return "COMPRESSED_SRGB_ALPHA_BPTC_UNORM";
        case SizedInternalFormat::COMPRESSED_RGBA_BPTC_UNORM:
            return "COMPRESSED_RGBA_BPTC_UNORM";
        case SizedInternalFormat::COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
            return "COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT";
        case SizedInternalFormat::COMPRESSED_RGBA_S3TC_DXT5_EXT:
            return "COMPRESSED_RGBA_S3TC_DXT5_EXT";
        case SizedInternalFormat::COMPRESSED_RGBA_ASTC_4x4:
            return "COMPRESSED_RGBA_ASTC_4x4";
        case SizedInternalFormat::COMPRESSED_SRGB8_ALPHA8_ASTC_4x4:
            return "COMPRESSED_SRGB8_ALPHA8_ASTC_4x4";
#if !SNAP_RHI_OS_IOS()
        case SizedInternalFormat::R16:
            return "R16";
        case SizedInternalFormat::R16_SNORM:
            return "R16_SNORM";
        case SizedInternalFormat::RG16:
            return "RG16";
        case SizedInternalFormat::RG16_SNORM:
            return "RG16_SNORM";
        case SizedInternalFormat::RGBA16:
            return "RGBA16";
        case SizedInternalFormat::RGBA16_SNORM:
            return "RGBA16_SNORM";
#endif // !SNAP_RHI_OS_IOS()
        case SizedInternalFormat::DEPRECATED_LUMINANCE:
            return "DEPRECATED_LUMINANCE";
        case SizedInternalFormat::DEPRECATED_LUMINANCE4:
            return "DEPRECATED_LUMINANCE4";
        case SizedInternalFormat::DEPRECATED_LUMINANCE8:
            return "DEPRECATED_LUMINANCE8";
        case SizedInternalFormat::DEPRECATED_LUMINANCE12:
            return "DEPRECATED_LUMINANCE12";
        case SizedInternalFormat::DEPRECATED_LUMINANCE16:
            return "DEPRECATED_LUMINANCE16";
        case SizedInternalFormat::DEPRECATED_LUMINANCE_ALPHA:
            return "DEPRECATED_LUMINANCE_ALPHA";
        case SizedInternalFormat::DEPRECATED_LUMINANCE4_ALPHA4:
            return "DEPRECATED_LUMINANCE4_ALPHA4";
        case SizedInternalFormat::DEPRECATED_LUMINANCE6_ALPHA2:
            return "DEPRECATED_LUMINANCE6_ALPHA2";
        case SizedInternalFormat::DEPRECATED_LUMINANCE8_ALPHA8:
            return "DEPRECATED_LUMINANCE8_ALPHA8";
        case SizedInternalFormat::DEPRECATED_LUMINANCE12_ALPHA4:
            return "DEPRECATED_LUMINANCE12_ALPHA4";
        case SizedInternalFormat::DEPRECATED_LUMINANCE12_ALPHA12:
            return "DEPRECATED_LUMINANCE12_ALPHA12";
        case SizedInternalFormat::DEPRECATED_LUMINANCE16_ALPHA16:
            return "DEPRECATED_LUMINANCE16_ALPHA16";
        case SizedInternalFormat::DEPRECATED_INTENSITY:
            return "DEPRECATED_INTENSITY";
        case SizedInternalFormat::DEPRECATED_INTENSITY4:
            return "DEPRECATED_INTENSITY4";
        case SizedInternalFormat::DEPRECATED_INTENSITY8:
            return "DEPRECATED_INTENSITY8";
        case SizedInternalFormat::DEPRECATED_INTENSITY12:
            return "DEPRECATED_INTENSITY12";
        case SizedInternalFormat::DEPRECATED_INTENSITY16:
            return "DEPRECATED_INTENSITY16";
        case SizedInternalFormat::DEPRECATED_R:
            return "DEPRECATED_R";
        case SizedInternalFormat::DEPRECATED_RG:
            return "DEPRECATED_RG";
        case SizedInternalFormat::DEPRECATED_RGB:
            return "DEPRECATED_RGB";
        case SizedInternalFormat::DEPRECATED_RGBA:
            return "DEPRECATED_RGBA";
        case SizedInternalFormat::DEPRECATED_BGRA:
            return "DEPRECATED_BGRA";
        case SizedInternalFormat::DEPRECATED_DEPTH:
            return "DEPRECATED_DEPTH";
        case SizedInternalFormat::DEPRECATED_DEPTH_STENCIL:
            return "DEPRECATED_DEPTH_STENCIL";
        default:
            assert(false && "Caught unkown internal size.");
            return "<ERROR>";
    }
}

snap::rhi::common::zstring_view toString(FormatGroup formatGroup) noexcept {
    switch (formatGroup) {
        case FormatGroup::UNKNOWN:
            return "UNKNOWN";
        case FormatGroup::R:
            return "R";
        case FormatGroup::R_INTEGER:
            return "R_INTEGER";
        case FormatGroup::RG:
            return "RG";
        case FormatGroup::RG_INTEGER:
            return "RG_INTEGER";
        case FormatGroup::RGB:
            return "RGB";
        case FormatGroup::RGB_INTEGER:
            return "RGB_INTEGER";
        case FormatGroup::RGBA:
            return "RGBA";
        case FormatGroup::RGBA_INTEGER:
            return "RGBA_INTEGER";
        case FormatGroup::BGRA:
            return "BGRA";
        case FormatGroup::DEPTH:
            return "DEPTH";
        case FormatGroup::DEPTH_STENCIL:
            return "DEPTH_STENCIL";
        case FormatGroup::DEPRECATED_LUMINANCE:
            return "DEPRECATED_LUMINANCE";
        case FormatGroup::DEPRECATED_LUMINANCE_ALPHA:
            return "DEPRECATED_LUMINANCE_ALPHA";
        case FormatGroup::DEPRECATED_INTENSITY:
            return "DEPRECATED_INTENSITY";
        default:
            assert(false && "Caught unkown format group.");
            return "<ERROR>";
    }
}

snap::rhi::common::zstring_view toString(FormatDataType formatDataType) noexcept {
    switch (formatDataType) {
        case FormatDataType::UNKNOWN_OR_COMPRESSED:
            return "UNKNOWN_OR_COMPRESSED";
        case FormatDataType::UNSIGNED_BYTE:
            return "UNSIGNED_BYTE";
        case FormatDataType::BYTE:
            return "BYTE";
        case FormatDataType::HALF_FLOAT:
            return "HALF_FLOAT";
        case FormatDataType::HALF_FLOAT_OES:
            return "HALF_FLOAT_OES";
        case FormatDataType::FLOAT:
            return "FLOAT";
        case FormatDataType::UNSIGNED_SHORT:
            return "UNSIGNED_SHORT";
        case FormatDataType::SHORT:
            return "SHORT";
        case FormatDataType::UNSIGNED_INT:
            return "UNSIGNED_INT";
        case FormatDataType::INT:
            return "INT";
        case FormatDataType::UNSIGNED_INT_10F_11F_11F_REV:
            return "UNSIGNED_INT_10F_11F_11F_REV";
        case FormatDataType::UNSIGNED_INT_5_9_9_9_REV:
            return "UNSIGNED_INT_5_9_9_9_REV";
        case FormatDataType::UNSIGNED_INT_2_10_10_10_REV:
            return "UNSIGNED_INT_2_10_10_10_REV";
        case FormatDataType::UNSIGNED_INT_24_8:
            return "UNSIGNED_INT_24_8";
        case FormatDataType::FLOAT_32_UNSIGNED_INT_24_8_REV:
            return "FLOAT_32_UNSIGNED_INT_24_8_REV";
        default:
            assert(false && "Caught unkown format data type.");
            return "<ERROR>";
    }
}

} // namespace snap::rhi::backend::opengl
