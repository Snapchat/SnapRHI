#include "snap/rhi/Common.h"

#include "snap/rhi/Exception.h"

namespace {
bool isPowerOfTwo(uint64_t n) {
    assert(n != 0);
    return (n & (n - 1)) == 0;
}

template<typename T>
constexpr T divCeil(const T numerator, const T denominator) {
    static_assert(std::is_integral_v<T>, "only for integral types");
    return (numerator + (denominator - static_cast<T>(1))) / denominator;
}
} // unnamed namespace

namespace snap::rhi {
uint32_t getArrayLevelCount(const TextureCreateInfo& info) {
    if (info.textureType == snap::rhi::TextureType::Texture2DArray) {
        return info.size.depth;
    }

    //    if (info.textureType == TextureType::TextureCubemapArray) {
    //        assert(info.size.depth % 6 == 0);
    //        return info.size.depth / 6;
    //    }

    return 1;
}

bool isSizePowerOfTwo(const TextureCreateInfo& info) {
    return isPowerOfTwo(info.size.width) && isPowerOfTwo(info.size.height) && isPowerOfTwo(info.size.depth);
}

snap::rhi::Extent3D getTexSizeForMip(const snap::rhi::Extent3D& sourceSize,
                                     const snap::rhi::TextureType textureType,
                                     const uint32_t mipmapLevel) {
    snap::rhi::Extent3D result = sourceSize;
    result.width = std::max(1u, result.width >> mipmapLevel);
    result.height = std::max(1u, result.height >> mipmapLevel);
    result.depth = std::max(1u, result.depth >> mipmapLevel);

    if (textureType == snap::rhi::TextureType::Texture2DArray) {
        result.depth = sourceSize.depth;
    } else if (textureType == snap::rhi::TextureType::TextureCubemap) {
        result.depth = sourceSize.depth * 6; // For cubemap array
    }

    return result;
}

uint32_t bytesPerRow(const uint32_t width, const uint32_t height, const snap::rhi::PixelFormat format) {
    switch (format) {
        case snap::rhi::PixelFormat::R8Unorm:
        case snap::rhi::PixelFormat::R8Snorm:
        case snap::rhi::PixelFormat::R8Sint:
        case snap::rhi::PixelFormat::R8Uint:
        case snap::rhi::PixelFormat::Grayscale:
            return 1 * width;

        case snap::rhi::PixelFormat::R8G8Unorm:
        case snap::rhi::PixelFormat::R8G8Snorm:
        case snap::rhi::PixelFormat::R8G8Sint:
        case snap::rhi::PixelFormat::R8G8Uint:
        case snap::rhi::PixelFormat::R16Unorm:
        case snap::rhi::PixelFormat::R16Snorm:
        case snap::rhi::PixelFormat::R16Float:
        case snap::rhi::PixelFormat::R16Sint:
        case snap::rhi::PixelFormat::R16Uint:
        case snap::rhi::PixelFormat::Depth16Unorm:
            return 2 * width;

        case snap::rhi::PixelFormat::R8G8B8Unorm:
            return 3 * width;

        case snap::rhi::PixelFormat::R8G8B8A8Srgb:
        case snap::rhi::PixelFormat::R8G8B8A8Unorm:
        case snap::rhi::PixelFormat::R8G8B8A8Snorm:
        case snap::rhi::PixelFormat::R8G8B8A8Sint:
        case snap::rhi::PixelFormat::R8G8B8A8Uint:
        case snap::rhi::PixelFormat::B8G8R8A8Unorm:
        case snap::rhi::PixelFormat::R10G10B10A2Unorm:
        case snap::rhi::PixelFormat::R10G10B10A2Uint:
        case snap::rhi::PixelFormat::R11G11B10Float:
        case snap::rhi::PixelFormat::R32Float:
        case snap::rhi::PixelFormat::R32Sint:
        case snap::rhi::PixelFormat::R32Uint:
        case snap::rhi::PixelFormat::R16G16Float:
        case snap::rhi::PixelFormat::R16G16Unorm:
        case snap::rhi::PixelFormat::R16G16Snorm:
        case snap::rhi::PixelFormat::R16G16Sint:
        case snap::rhi::PixelFormat::R16G16Uint:
        case snap::rhi::PixelFormat::DepthFloat:
            return 4 * width;

        case snap::rhi::PixelFormat::DepthStencil:
            return 4 * width;

        case snap::rhi::PixelFormat::R32G32Float:
        case snap::rhi::PixelFormat::R32G32Sint:
        case snap::rhi::PixelFormat::R32G32Uint:
        case snap::rhi::PixelFormat::R16G16B16A16Unorm:
        case snap::rhi::PixelFormat::R16G16B16A16Snorm:
        case snap::rhi::PixelFormat::R16G16B16A16Float:
        case snap::rhi::PixelFormat::R16G16B16A16Sint:
        case snap::rhi::PixelFormat::R16G16B16A16Uint:
            return 8 * width;

        case snap::rhi::PixelFormat::R32G32B32A32Float:
        case snap::rhi::PixelFormat::R32G32B32A32Sint:
        case snap::rhi::PixelFormat::R32G32B32A32Uint:
            return 16 * width;

        /**
         * https://registry.khronos.org/OpenGL-Refpages/es3.0/html/glCompressedTexImage3D.xhtml
         * https://developer.apple.com/documentation/metal/mtlblitcommandencoder/1400752-copyfrombuffer
         *
         * If destinationTexture uses a compressed pixel format, set sourceBytesPerRow to the number of bytes between
         * the starts of two row blocks.
         */
        case snap::rhi::PixelFormat::ETC_R8G8B8_Unorm:
        case snap::rhi::PixelFormat::ETC2_R8G8B8_Unorm:
        case snap::rhi::PixelFormat::ETC2_R8G8B8_sRGB:
        case snap::rhi::PixelFormat::ETC2_R8G8B8A1_Unorm:
        case snap::rhi::PixelFormat::ETC2_R8G8B8A1_sRGB: {
            return divCeil(width, 4u) * 8;
        }

        case snap::rhi::PixelFormat::BC3_RGBA:
        case snap::rhi::PixelFormat::BC3_sRGBA:
        case snap::rhi::PixelFormat::BC7_RGBA:
        case snap::rhi::PixelFormat::BC7_sRGBA:
        case snap::rhi::PixelFormat::ETC2_R8G8B8A8_Unorm:
        case snap::rhi::PixelFormat::ETC2_R8G8B8A8_sRGB:
        case snap::rhi::PixelFormat::ASTC_4x4_Unorm:
        case snap::rhi::PixelFormat::ASTC_4x4_sRGB: {
            return divCeil(width, 4u) * 16;
        }

        default:
            snap::rhi::common::throwException("[bytesPerRow] unsupported format");
    }
    return 0;
}

uint64_t bytesPerSlice(const uint32_t width, const uint32_t height, const snap::rhi::PixelFormat format) {
    switch (format) {
        /**
         * https://registry.khronos.org/OpenGL-Refpages/es3.0/html/glCompressedTexImage3D.xhtml
         * https://developer.apple.com/documentation/metal/mtlblitcommandencoder/1400752-copyfrombuffer
         *
         * If destinationTexture uses a compressed pixel format, set sourceBytesPerRow to the number of bytes between
         * the starts of two row blocks.
         */
        case snap::rhi::PixelFormat::ETC_R8G8B8_Unorm:
        case snap::rhi::PixelFormat::ETC2_R8G8B8_Unorm:
        case snap::rhi::PixelFormat::ETC2_R8G8B8_sRGB:
        case snap::rhi::PixelFormat::ETC2_R8G8B8A1_Unorm:
        case snap::rhi::PixelFormat::ETC2_R8G8B8A1_sRGB:
        case snap::rhi::PixelFormat::ETC2_R8G8B8A8_Unorm:
        case snap::rhi::PixelFormat::ETC2_R8G8B8A8_sRGB:
        case snap::rhi::PixelFormat::BC3_RGBA:
        case snap::rhi::PixelFormat::BC3_sRGBA:
        case snap::rhi::PixelFormat::BC7_RGBA:
        case snap::rhi::PixelFormat::BC7_sRGBA:
        case snap::rhi::PixelFormat::ASTC_4x4_Unorm:
        case snap::rhi::PixelFormat::ASTC_4x4_sRGB: {
            return static_cast<uint64_t>(bytesPerRow(width, height, format)) * divCeil(height, 4u);
        }

        default:
            return static_cast<uint64_t>(bytesPerRow(width, height, format)) * height;
    }
}

uint64_t bytesPerSliceWithRow(const uint32_t bytesPerRow, const uint32_t height, const snap::rhi::PixelFormat format) {
    switch (format) {
        /**
         * https://registry.khronos.org/OpenGL-Refpages/es3.0/html/glCompressedTexImage3D.xhtml
         * https://developer.apple.com/documentation/metal/mtlblitcommandencoder/1400752-copyfrombuffer
         *
         * If destinationTexture uses a compressed pixel format, set sourceBytesPerRow to the number of bytes between
         * the starts of two row blocks.
         */
        case snap::rhi::PixelFormat::ETC_R8G8B8_Unorm:
        case snap::rhi::PixelFormat::ETC2_R8G8B8_Unorm:
        case snap::rhi::PixelFormat::ETC2_R8G8B8_sRGB:
        case snap::rhi::PixelFormat::ETC2_R8G8B8A1_Unorm:
        case snap::rhi::PixelFormat::ETC2_R8G8B8A1_sRGB:
        case snap::rhi::PixelFormat::ETC2_R8G8B8A8_Unorm:
        case snap::rhi::PixelFormat::ETC2_R8G8B8A8_sRGB:
        case snap::rhi::PixelFormat::BC3_RGBA:
        case snap::rhi::PixelFormat::BC3_sRGBA:
        case snap::rhi::PixelFormat::BC7_RGBA:
        case snap::rhi::PixelFormat::BC7_sRGBA:
        case snap::rhi::PixelFormat::ASTC_4x4_Unorm:
        case snap::rhi::PixelFormat::ASTC_4x4_sRGB: {
            return static_cast<uint64_t>(bytesPerRow) * divCeil(height, 4u);
        }

        default:
            return static_cast<uint64_t>(bytesPerRow) * height;
    }
}

snap::rhi::TextureViewInfo defaultTextureViewInfo(const snap::rhi::TextureCreateInfo& textureCreateInfo) {
    const snap::rhi::TextureViewInfo viewCreateInfo{
        .format = textureCreateInfo.format,
        .textureType = textureCreateInfo.textureType,
        .range =
            {
                .baseMipLevel = 0,
                .levelCount = textureCreateInfo.mipLevels,
                .baseArrayLayer = 0,
                .layerCount = getArrayLevelCount(textureCreateInfo),
            },
        .components = snap::rhi::DefaultComponentMapping,
    };
    return viewCreateInfo;
}
} // namespace snap::rhi
