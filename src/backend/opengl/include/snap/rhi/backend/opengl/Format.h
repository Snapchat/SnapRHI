#pragma once

#include "snap/rhi/Enums.h"
#include "snap/rhi/backend/opengl/OpenGL.h"

#include <cassert>
#include <cstdint>
#include <snap/rhi/common/zstring_view.h>
#include <string_view>

namespace snap::rhi::backend::opengl {

enum class FormatDataType : uint32_t {
    UNKNOWN_OR_COMPRESSED = 0,

    UNSIGNED_BYTE = GL_UNSIGNED_BYTE,
    BYTE = GL_BYTE,
    HALF_FLOAT = GL_HALF_FLOAT,
    HALF_FLOAT_OES = GL_HALF_FLOAT_OES,
    FLOAT = GL_FLOAT,
    UNSIGNED_SHORT = GL_UNSIGNED_SHORT,
    SHORT = GL_SHORT,
    UNSIGNED_INT = GL_UNSIGNED_INT,
    INT = GL_INT,
    UNSIGNED_INT_10F_11F_11F_REV = GL_UNSIGNED_INT_10F_11F_11F_REV,
    UNSIGNED_INT_5_9_9_9_REV = GL_UNSIGNED_INT_5_9_9_9_REV,
    UNSIGNED_INT_2_10_10_10_REV = GL_UNSIGNED_INT_2_10_10_10_REV,
    UNSIGNED_INT_24_8 = GL_UNSIGNED_INT_24_8,
    FLOAT_32_UNSIGNED_INT_24_8_REV = GL_FLOAT_32_UNSIGNED_INT_24_8_REV,
};

enum class FormatGroup : uint32_t {
    UNKNOWN = 0,

    R = GL_RED,
    R_INTEGER = GL_RED_INTEGER,
    RG = GL_RG,
    RG_INTEGER = GL_RG_INTEGER,
    RGB = GL_RGB,
    RGB_INTEGER = GL_RGB_INTEGER,
    RGBA = GL_RGBA,
    RGBA_INTEGER = GL_RGBA_INTEGER,

    BGRA = GL_BGRA,

    DEPTH = GL_DEPTH_COMPONENT,
    DEPTH_STENCIL = GL_DEPTH_STENCIL,

    DEPRECATED_LUMINANCE = GL_LUMINANCE,
    DEPRECATED_LUMINANCE_ALPHA = GL_LUMINANCE_ALPHA,
    DEPRECATED_INTENSITY = GL_INTENSITY,
};

enum class SizedInternalFormat : uint32_t {
    UNKNOWN = 0,

    R8 = GL_R8,
    R8_SNORM = GL_R8_SNORM,
    R16F = GL_R16F,
    R32F = GL_R32F,
    R8UI = GL_R8UI,
    R8I = GL_R8I,
    R16UI = GL_R16UI,
    R16I = GL_R16I,
    R32UI = GL_R32UI,
    R32I = GL_R32I,

    RG8 = GL_RG8,
    RG8_SNORM = GL_RG8_SNORM,
    RG16F = GL_RG16F,
    RG32F = GL_RG32F,
    RG8UI = GL_RG8UI,
    RG8I = GL_RG8I,
    RG16UI = GL_RG16UI,
    RG16I = GL_RG16I,
    RG32UI = GL_RG32UI,
    RG32I = GL_RG32I,

    RGB8 = GL_RGB8,
    SRGB8 = GL_SRGB8,
    RGB565 = GL_RGB565,
    RGB8_SNORM = GL_RGB8_SNORM,
    R11F_G11F_B10F = GL_R11F_G11F_B10F,
    RGB9_E5 = GL_RGB9_E5,
    RGB16F = GL_RGB16F,
    RGB32F = GL_RGB32F,
    RGB8UI = GL_RGB8UI,
    RGB8I = GL_RGB8I,
    RGB16UI = GL_RGB16UI,
    RGB16I = GL_RGB16I,
    RGB32UI = GL_RGB32UI,
    RGB32I = GL_RGB32I,

    RGBA8 = GL_RGBA8,
    SRGB8_ALPHA8 = GL_SRGB8_ALPHA8,
    RGBA8_SNORM = GL_RGBA8_SNORM,
    RGB5_A1 = GL_RGB5_A1,
    RGBA4 = GL_RGBA4,
    RGB10_A2 = GL_RGB10_A2,
    RGBA16F = GL_RGBA16F,
    RGBA32F = GL_RGBA32F,
    RGBA8UI = GL_RGBA8UI,
    RGBA8I = GL_RGBA8I,
    RGB10_A2UI = GL_RGB10_A2UI,
    RGBA16UI = GL_RGBA16UI,
    RGBA16I = GL_RGBA16I,
    RGBA32I = GL_RGBA32I,
    RGBA32UI = GL_RGBA32UI,

#if SNAP_RHI_OS_ANDROID() || SNAP_RHI_PLATFORM_WEBASSEMBLY()
    R16 = GL_R16_EXT,
    R16_SNORM = GL_R16_SNORM_EXT,
    RG16 = GL_RG16_EXT,
    RG16_SNORM = GL_RG16_SNORM_EXT,
    RGBA16 = GL_RGBA16_EXT,
    RGBA16_SNORM = GL_RGBA16_SNORM_EXT,
#elif !SNAP_RHI_OS_IOS()
    R16 = GL_R16,
    R16_SNORM = GL_R16_SNORM,
    RG16 = GL_RG16,
    RG16_SNORM = GL_RG16_SNORM,
    RGBA16 = GL_RGBA16,
    RGBA16_SNORM = GL_RGBA16_SNORM,
#endif // !SNAP_RHI_OS_IOS()

    BGRA8 = GL_BGRA8,

    DEPTH16 = GL_DEPTH_COMPONENT16,
    DEPTH24 = GL_DEPTH_COMPONENT24,
    DEPTH32 = GL_DEPTH_COMPONENT32,
    DEPTH32F = GL_DEPTH_COMPONENT32F,
    DEPTH24_STENCIL8 = GL_DEPTH24_STENCIL8,
    DEPTH32F_STENCIL8 = GL_DEPTH32F_STENCIL8,

    COMPRESSED_RGB8_ETC1 = GL_ETC1_RGB8_OES,
    COMPRESSED_R11_EAC = GL_COMPRESSED_R11_EAC,
    COMPRESSED_SIGNED_R11_EAC = GL_COMPRESSED_SIGNED_R11_EAC,
    COMPRESSED_RG11_EAC = GL_COMPRESSED_RG11_EAC,
    COMPRESSED_SIGNED_RG11_EAC = GL_COMPRESSED_SIGNED_RG11_EAC,
    COMPRESSED_RGB8_ETC2 = GL_COMPRESSED_RGB8_ETC2,
    COMPRESSED_SRGB8_ETC2 = GL_COMPRESSED_SRGB8_ETC2,
    COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2 = GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2,
    COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2 = GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2,
    COMPRESSED_RGBA8_ETC2_EAC = GL_COMPRESSED_RGBA8_ETC2_EAC,
    COMPRESSED_SRGB8_ALPHA8_ETC2_EAC = GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC,
    COMPRESSED_SRGB_ALPHA_BPTC_UNORM = GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM,
    COMPRESSED_RGBA_BPTC_UNORM = GL_COMPRESSED_RGBA_BPTC_UNORM,
    COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT,
    COMPRESSED_RGBA_S3TC_DXT5_EXT = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,
    COMPRESSED_RGBA_ASTC_4x4 = GL_COMPRESSED_RGBA_ASTC_4x4,
    COMPRESSED_SRGB8_ALPHA8_ASTC_4x4 = GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4,

    DEPRECATED_LUMINANCE = GL_LUMINANCE,
    DEPRECATED_LUMINANCE4 = GL_LUMINANCE4,
    DEPRECATED_LUMINANCE8 = GL_LUMINANCE8,
    DEPRECATED_LUMINANCE12 = GL_LUMINANCE12,
    DEPRECATED_LUMINANCE16 = GL_LUMINANCE16,

    DEPRECATED_LUMINANCE_ALPHA = GL_LUMINANCE_ALPHA,
    DEPRECATED_LUMINANCE4_ALPHA4 = GL_LUMINANCE4_ALPHA4,
    DEPRECATED_LUMINANCE6_ALPHA2 = GL_LUMINANCE6_ALPHA2,
    DEPRECATED_LUMINANCE8_ALPHA8 = GL_LUMINANCE8_ALPHA8,
    DEPRECATED_LUMINANCE12_ALPHA4 = GL_LUMINANCE12_ALPHA4,
    DEPRECATED_LUMINANCE12_ALPHA12 = GL_LUMINANCE12_ALPHA12,
    DEPRECATED_LUMINANCE16_ALPHA16 = GL_LUMINANCE16_ALPHA16,

    DEPRECATED_INTENSITY = GL_INTENSITY,
    DEPRECATED_INTENSITY4 = GL_INTENSITY4,
    DEPRECATED_INTENSITY8 = GL_INTENSITY8,
    DEPRECATED_INTENSITY12 = GL_INTENSITY12,
    DEPRECATED_INTENSITY16 = GL_INTENSITY16,

    DEPRECATED_R = GL_RED, // GL_RED_EXT has the same value - 0x1903
    DEPRECATED_RG = GL_RG,
    DEPRECATED_RGB = GL_RGB,
    DEPRECATED_RGBA = GL_RGBA,

    DEPRECATED_BGRA = GL_BGRA,

    // Used by glTexImage2D in ES 2.0, should be GL_DEPTH24_STENCIL8_OES for render buffer storage.
    // But we don't use MSAA in ES 2.0 so we don't use render buffer with D24_S8 format in ES 2.0
    DEPRECATED_DEPTH = GL_DEPTH_COMPONENT,
    DEPRECATED_DEPTH_STENCIL = GL_DEPTH_STENCIL // GL_DEPTH_STENCIL_OES has the same value - 0x84F9
};

struct CompositeFormat {
    SizedInternalFormat internalFormat = SizedInternalFormat::UNKNOWN;
    FormatGroup format = FormatGroup::UNKNOWN;
    FormatDataType dataType = FormatDataType::UNKNOWN_OR_COMPRESSED;
};

FormatGroup getFormatGroup(SizedInternalFormat internalFormat) noexcept;
FormatDataType getFormatDataType(SizedInternalFormat internalFormat) noexcept;
DepthStencilFormatTraits getDepthStencilFormatTraits(SizedInternalFormat internalFormat) noexcept;

snap::rhi::common::zstring_view toString(SizedInternalFormat internalFormat) noexcept;
snap::rhi::common::zstring_view toString(FormatGroup formatGroup) noexcept;
snap::rhi::common::zstring_view toString(FormatDataType formatDataType) noexcept;

enum class FormatDataTypeIndex : uint8_t {
    UNKNOWN_OR_COMPRESSED = 0,
    UNSIGNED_BYTE,
    BYTE,
    HALF_FLOAT,
    FLOAT,
    UNSIGNED_SHORT,
    SHORT,
    UNSIGNED_INT,
    INT,
    UNSIGNED_INT_10F_11F_11F_REV,
    UNSIGNED_INT_5_9_9_9_REV,
    UNSIGNED_INT_2_10_10_10_REV,
    UNSIGNED_INT_24_8,
    FLOAT_32_UNSIGNED_INT_24_8_REV,
    Error
};

enum class FormatGroupIndex : uint8_t {
    UNKNOWN = 0,
    R,
    R_INTEGER,
    RG,
    RG_INTEGER,
    RGB,
    RGB_INTEGER,
    RGBA,
    RGBA_INTEGER,
    BGRA,
    DEPTH,
    DEPTH_STENCIL,
    DEPRECATED_LUMINANCE,
    DEPRECATED_LUMINANCE_ALPHA,
    DEPRECATED_INTENSITY,
    Error
};

[[nodiscard]] static inline constexpr FormatGroupIndex toIndex(FormatGroup formatGroup) noexcept {
    switch (formatGroup) {
        case FormatGroup::UNKNOWN:
            return FormatGroupIndex::UNKNOWN;
        case FormatGroup::R:
            return FormatGroupIndex::R;
        case FormatGroup::R_INTEGER:
            return FormatGroupIndex::R_INTEGER;
        case FormatGroup::RG:
            return FormatGroupIndex::RG;
        case FormatGroup::RG_INTEGER:
            return FormatGroupIndex::RG_INTEGER;
        case FormatGroup::RGB:
            return FormatGroupIndex::RGB;
        case FormatGroup::RGB_INTEGER:
            return FormatGroupIndex::RGB_INTEGER;
        case FormatGroup::RGBA:
            return FormatGroupIndex::RGBA;
        case FormatGroup::RGBA_INTEGER:
            return FormatGroupIndex::RGBA_INTEGER;
        case FormatGroup::DEPTH:
            return FormatGroupIndex::DEPTH;
        case FormatGroup::DEPTH_STENCIL:
            return FormatGroupIndex::DEPTH_STENCIL;
        case FormatGroup::BGRA:
            return FormatGroupIndex::BGRA;
        case FormatGroup::DEPRECATED_LUMINANCE:
            return FormatGroupIndex::DEPRECATED_LUMINANCE;
        case FormatGroup::DEPRECATED_LUMINANCE_ALPHA:
            return FormatGroupIndex::DEPRECATED_LUMINANCE_ALPHA;
        case FormatGroup::DEPRECATED_INTENSITY:
            return FormatGroupIndex::DEPRECATED_INTENSITY;
        default:
            assert(false && "Caught unkown format group.");
            return FormatGroupIndex::Error;
    }
}

[[nodiscard]] static inline constexpr FormatDataTypeIndex toIndex(FormatDataType formatDataType) noexcept {
    switch (formatDataType) {
        case FormatDataType::UNKNOWN_OR_COMPRESSED:
            return FormatDataTypeIndex::UNKNOWN_OR_COMPRESSED;
        case FormatDataType::UNSIGNED_BYTE:
            return FormatDataTypeIndex::UNSIGNED_BYTE;
        case FormatDataType::BYTE:
            return FormatDataTypeIndex::BYTE;
        case FormatDataType::HALF_FLOAT:
            return FormatDataTypeIndex::HALF_FLOAT;
        case FormatDataType::FLOAT:
            return FormatDataTypeIndex::FLOAT;
        case FormatDataType::UNSIGNED_SHORT:
            return FormatDataTypeIndex::UNSIGNED_SHORT;
        case FormatDataType::SHORT:
            return FormatDataTypeIndex::SHORT;
        case FormatDataType::UNSIGNED_INT:
            return FormatDataTypeIndex::UNSIGNED_INT;
        case FormatDataType::INT:
            return FormatDataTypeIndex::INT;
        case FormatDataType::UNSIGNED_INT_10F_11F_11F_REV:
            return FormatDataTypeIndex::UNSIGNED_INT_10F_11F_11F_REV;
        case FormatDataType::UNSIGNED_INT_5_9_9_9_REV:
            return FormatDataTypeIndex::UNSIGNED_INT_5_9_9_9_REV;
        case FormatDataType::UNSIGNED_INT_2_10_10_10_REV:
            return FormatDataTypeIndex::UNSIGNED_INT_2_10_10_10_REV;
        case FormatDataType::UNSIGNED_INT_24_8:
            return FormatDataTypeIndex::UNSIGNED_INT_24_8;
        case FormatDataType::FLOAT_32_UNSIGNED_INT_24_8_REV:
            return FormatDataTypeIndex::FLOAT_32_UNSIGNED_INT_24_8_REV;
        default:
            assert(false && "Caught unkown format data type.");
            return FormatDataTypeIndex::Error;
    }
}

} // namespace snap::rhi::backend::opengl
