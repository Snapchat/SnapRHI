//
//  Common.h
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 02.06.2021.
//

#pragma once

#include "snap/rhi/Enums.h"
#include "snap/rhi/PixelFormat.h"
#include "snap/rhi/TextureCreateInfo.h"
#include "snap/rhi/TextureViewCreateInfo.h"
#include "snap/rhi/VertexAttributeFormat.h"

#include <cassert>

namespace snap::rhi {
/**
 * @brief Returns the number of array layers for a texture described by @p info.
 *
 * @param info Texture creation info.
 * @return Number of array layers.
 *
 * @note Backend usage:
 * - Metal uses this value to set `MTLTextureDescriptor.arrayLength`.
 * - For non-array texture types this returns 1.
 */
uint32_t getArrayLevelCount(const TextureCreateInfo& info);

/**
 * @brief Checks whether all dimensions of the texture size are powers of two.
 *
 * @param info Texture creation info.
 * @return `true` if width/height/depth are powers of two.
 *
 * @note Used by OpenGL to decide whether NPOT sampler restrictions apply for a given texture.
 */
bool isSizePowerOfTwo(const TextureCreateInfo& info);

/**
 * @brief Computes the logical extent of a given mip level.
 *
 * Computes `max(1, size >> mipLevel)` for each dimension, with backend-friendly adjustments for array/cubemap types.
 *
 * @param sourceSize Base level (mip 0) extent.
 * @param textureType Texture type.
 * @param mipmapLevel Mip level to compute (0 == base level).
 * @return Extent for the given mip.
 *
 * @note Semantics in this codebase:
 * - For `Texture2DArray`, `depth` (array layer count) is kept equal to the original `sourceSize.depth`.
 * - For `TextureCubemap`, `depth` is treated as array size and the function returns `sourceSize.depth * 6`.
 */
snap::rhi::Extent3D getTexSizeForMip(const snap::rhi::Extent3D& sourceSize,
                                     snap::rhi::TextureType textureType,
                                     uint32_t mipmapLevel);

/**
 * @brief Returns the number of channels/components for a pixel format.
 *
 * @param format Pixel format.
 * @return Number of channels (1..4).
 *
 * @warning This helper asserts on unknown/unsupported formats.
 */
constexpr uint32_t numChannels(const snap::rhi::PixelFormat format) {
    switch (format) {
        case snap::rhi::PixelFormat::Depth16Unorm:
        case snap::rhi::PixelFormat::DepthFloat:

        case snap::rhi::PixelFormat::R8Sint:
        case snap::rhi::PixelFormat::R8Uint:
        case snap::rhi::PixelFormat::R16Float:
        case snap::rhi::PixelFormat::R16Unorm:
        case snap::rhi::PixelFormat::R16Snorm:
        case snap::rhi::PixelFormat::R16Sint:
        case snap::rhi::PixelFormat::R16Uint:
        case snap::rhi::PixelFormat::R32Float:
        case snap::rhi::PixelFormat::R32Sint:
        case snap::rhi::PixelFormat::R32Uint:
        case snap::rhi::PixelFormat::R8Snorm:
        case snap::rhi::PixelFormat::R8Unorm:
        case snap::rhi::PixelFormat::Grayscale:
            return 1;

        case snap::rhi::PixelFormat::DepthStencil:
        case snap::rhi::PixelFormat::R8G8Snorm:
        case snap::rhi::PixelFormat::R8G8Unorm:
        case snap::rhi::PixelFormat::R8G8Sint:
        case snap::rhi::PixelFormat::R8G8Uint:
        case snap::rhi::PixelFormat::R16G16Float:
        case snap::rhi::PixelFormat::R16G16Sint:
        case snap::rhi::PixelFormat::R16G16Uint:
        case snap::rhi::PixelFormat::R32G32Float:
        case snap::rhi::PixelFormat::R32G32Sint:
        case snap::rhi::PixelFormat::R32G32Uint:
        case snap::rhi::PixelFormat::R16G16Unorm:
        case snap::rhi::PixelFormat::R16G16Snorm:
            return 2;

        case snap::rhi::PixelFormat::R8G8B8Unorm:
        case snap::rhi::PixelFormat::R11G11B10Float:
        case snap::rhi::PixelFormat::ETC_R8G8B8_Unorm:
        case snap::rhi::PixelFormat::ETC2_R8G8B8_Unorm:
        case snap::rhi::PixelFormat::ETC2_R8G8B8_sRGB:
            return 3;

        case snap::rhi::PixelFormat::BC3_RGBA:
        case snap::rhi::PixelFormat::BC3_sRGBA:
        case snap::rhi::PixelFormat::BC7_RGBA:
        case snap::rhi::PixelFormat::BC7_sRGBA:
        case snap::rhi::PixelFormat::ETC2_R8G8B8A1_Unorm:
        case snap::rhi::PixelFormat::ETC2_R8G8B8A1_sRGB:
        case snap::rhi::PixelFormat::ETC2_R8G8B8A8_Unorm:
        case snap::rhi::PixelFormat::ETC2_R8G8B8A8_sRGB:
        case snap::rhi::PixelFormat::ASTC_4x4_Unorm:
        case snap::rhi::PixelFormat::ASTC_4x4_sRGB:
        case snap::rhi::PixelFormat::R8G8B8A8Srgb:
        case snap::rhi::PixelFormat::R8G8B8A8Snorm:
        case snap::rhi::PixelFormat::R8G8B8A8Unorm:
        case snap::rhi::PixelFormat::B8G8R8A8Unorm:
        case snap::rhi::PixelFormat::R8G8B8A8Sint:
        case snap::rhi::PixelFormat::R8G8B8A8Uint:
        case snap::rhi::PixelFormat::R32G32B32A32Float:
        case snap::rhi::PixelFormat::R32G32B32A32Sint:
        case snap::rhi::PixelFormat::R32G32B32A32Uint:
        case snap::rhi::PixelFormat::R16G16B16A16Unorm:
        case snap::rhi::PixelFormat::R16G16B16A16Snorm:
        case snap::rhi::PixelFormat::R16G16B16A16Float:
        case snap::rhi::PixelFormat::R16G16B16A16Sint:
        case snap::rhi::PixelFormat::R16G16B16A16Uint:
        case snap::rhi::PixelFormat::R10G10B10A2Unorm:
        case snap::rhi::PixelFormat::R10G10B10A2Uint:
            return 4;

        default:
            assert(0);
            return 0;
    }
}

constexpr bool isCompressedFormat(const snap::rhi::PixelFormat format) {
    switch (format) {
        case snap::rhi::PixelFormat::BC3_RGBA:
        case snap::rhi::PixelFormat::BC3_sRGBA:
        case snap::rhi::PixelFormat::BC7_RGBA:
        case snap::rhi::PixelFormat::BC7_sRGBA:
        case snap::rhi::PixelFormat::ETC_R8G8B8_Unorm:
        case snap::rhi::PixelFormat::ETC2_R8G8B8_Unorm:
        case snap::rhi::PixelFormat::ETC2_R8G8B8_sRGB:
        case snap::rhi::PixelFormat::ETC2_R8G8B8A1_Unorm:
        case snap::rhi::PixelFormat::ETC2_R8G8B8A1_sRGB:
        case snap::rhi::PixelFormat::ETC2_R8G8B8A8_Unorm:
        case snap::rhi::PixelFormat::ETC2_R8G8B8A8_sRGB:
        case snap::rhi::PixelFormat::ASTC_4x4_Unorm:
        case snap::rhi::PixelFormat::ASTC_4x4_sRGB:
            return true;
        default:
            return false;
    }
}

constexpr bool isIntFormat(const snap::rhi::PixelFormat format) {
    switch (format) {
        case snap::rhi::PixelFormat::R8Sint:
        case snap::rhi::PixelFormat::R16Sint:
        case snap::rhi::PixelFormat::R32Sint:
        case snap::rhi::PixelFormat::R8G8Sint:
        case snap::rhi::PixelFormat::R16G16Sint:
        case snap::rhi::PixelFormat::R32G32Sint:
        case snap::rhi::PixelFormat::R8G8B8A8Sint:
        case snap::rhi::PixelFormat::R16G16B16A16Sint:
        case snap::rhi::PixelFormat::R32G32B32A32Sint:
            return true;
        default:
            return false;
    }
}

constexpr bool isUintFormat(const snap::rhi::PixelFormat format) {
    switch (format) {
        case snap::rhi::PixelFormat::R8Uint:
        case snap::rhi::PixelFormat::R16Uint:
        case snap::rhi::PixelFormat::R32Uint:
        case snap::rhi::PixelFormat::R8G8Uint:
        case snap::rhi::PixelFormat::R16G16Uint:
        case snap::rhi::PixelFormat::R32G32Uint:
        case snap::rhi::PixelFormat::R8G8B8A8Uint:
        case snap::rhi::PixelFormat::R16G16B16A16Uint:
        case snap::rhi::PixelFormat::R32G32B32A32Uint:
        case snap::rhi::PixelFormat::R10G10B10A2Uint:
            return true;
        default:
            return false;
    }
}

/**
 * @brief Returns depth/stencil traits for the given pixel format.
 *
 * @return Combination of `DepthStencilFormatTraits` bits describing which aspects are present.
 */
[[nodiscard]] static inline constexpr snap::rhi::DepthStencilFormatTraits getDepthStencilFormatTraits(
    const snap::rhi::PixelFormat format) noexcept {
    switch (format) {
        case snap::rhi::PixelFormat::DepthFloat:
        case snap::rhi::PixelFormat::Depth16Unorm:
            return snap::rhi::DepthStencilFormatTraits::HasDepthAspect;
        case snap::rhi::PixelFormat::DepthStencil:
            return snap::rhi::DepthStencilFormatTraits::HasDepthStencilAspects;

        default:
            return snap::rhi::DepthStencilFormatTraits::None;
    }
}

[[nodiscard]] static inline constexpr bool hasDepthAspect(const snap::rhi::PixelFormat format) noexcept {
    return snap::rhi::DepthStencilFormatTraits::HasDepthAspect ==
           (snap::rhi::getDepthStencilFormatTraits(format) & snap::rhi::DepthStencilFormatTraits::HasDepthAspect);
}

[[nodiscard]] static inline constexpr bool hasStencilAspect(const snap::rhi::PixelFormat format) noexcept {
    return snap::rhi::DepthStencilFormatTraits::HasStencilAspect ==
           (snap::rhi::getDepthStencilFormatTraits(format) & snap::rhi::DepthStencilFormatTraits::HasStencilAspect);
}

/**
 * @brief Returns the size in bytes for a single vertex attribute element of the specified format.
 */
constexpr uint32_t getAttributeByteSize(const snap::rhi::VertexAttributeFormat format) noexcept {
    switch (format) {
        case snap::rhi::VertexAttributeFormat::Byte2:
        case snap::rhi::VertexAttributeFormat::Byte2Normalized:
        case snap::rhi::VertexAttributeFormat::UnsignedByte2:
        case snap::rhi::VertexAttributeFormat::UnsignedByte2Normalized:
            return 2 * sizeof(uint8_t);

        case snap::rhi::VertexAttributeFormat::Byte3:
        case snap::rhi::VertexAttributeFormat::Byte3Normalized:
        case snap::rhi::VertexAttributeFormat::UnsignedByte3:
        case snap::rhi::VertexAttributeFormat::UnsignedByte3Normalized:
            return 3 * sizeof(uint8_t);

        case snap::rhi::VertexAttributeFormat::Byte4:
        case snap::rhi::VertexAttributeFormat::Byte4Normalized:
        case snap::rhi::VertexAttributeFormat::UnsignedByte4:
        case snap::rhi::VertexAttributeFormat::UnsignedByte4Normalized:
            return 4 * sizeof(uint8_t);

        case snap::rhi::VertexAttributeFormat::Short2:
        case snap::rhi::VertexAttributeFormat::Short2Normalized:
        case snap::rhi::VertexAttributeFormat::UnsignedShort2:
        case snap::rhi::VertexAttributeFormat::UnsignedShort2Normalized:
            return 2 * sizeof(uint16_t);

        case snap::rhi::VertexAttributeFormat::Short3:
        case snap::rhi::VertexAttributeFormat::Short3Normalized:
        case snap::rhi::VertexAttributeFormat::UnsignedShort3:
        case snap::rhi::VertexAttributeFormat::UnsignedShort3Normalized:
            return 3 * sizeof(uint16_t);

        case snap::rhi::VertexAttributeFormat::Short4:
        case snap::rhi::VertexAttributeFormat::Short4Normalized:
        case snap::rhi::VertexAttributeFormat::UnsignedShort4:
        case snap::rhi::VertexAttributeFormat::UnsignedShort4Normalized:
            return 4 * sizeof(uint16_t);

        case snap::rhi::VertexAttributeFormat::HalfFloat2:
            return 2 * sizeof(uint16_t);
        case snap::rhi::VertexAttributeFormat::HalfFloat3:
            return 3 * sizeof(uint16_t);
        case snap::rhi::VertexAttributeFormat::HalfFloat4:
            return 4 * sizeof(uint16_t);

        case snap::rhi::VertexAttributeFormat::Float:
            return 1 * sizeof(float);
        case snap::rhi::VertexAttributeFormat::Float2:
            return 2 * sizeof(float);
        case snap::rhi::VertexAttributeFormat::Float3:
            return 3 * sizeof(float);
        case snap::rhi::VertexAttributeFormat::Float4:
            return 4 * sizeof(float);

        default:
            assert(0);
            return 0;
    }
}

/**
 * @brief Computes the number of bytes between adjacent rows of pixels.
 *
 * For uncompressed formats this is essentially `bytesPerPixel * width`.
 * For compressed formats this returns the number of bytes between the start of adjacent *block rows*
 * (e.g., for 4x4 block compression).
 *
 * @param width Texture region width in texels.
 * @param height Texture region height in texels.
 * @param format Pixel format.
 * @return Bytes per row (or bytes per block-row for compressed formats).
 *
 * @note Used by Metal/OpenGL/Vulkan upload/copy code paths where APIs require a `bytesPerRow`/`sourceBytesPerRow`
 * value.
 * @warning Depth formats with undefined size are not supported and will throw.
 */
uint32_t bytesPerRow(uint32_t width, uint32_t height, snap::rhi::PixelFormat format);

/**
 * @brief Computes the number of bytes for a single 2D image slice.
 *
 * For uncompressed formats this is `bytesPerRow(width, height, format) * height`.
 * For compressed formats the height is rounded up to the compression block height.
 *
 * @param width Texture region width in texels.
 * @param height Texture region height in texels.
 * @param format Pixel format.
 * @return Bytes per slice.
 */
uint64_t bytesPerSlice(uint32_t width, uint32_t height, snap::rhi::PixelFormat format);

/**
 * @brief Computes bytes per slice given an already computed @p bytesPerRow.
 *
 * Equivalent to `bytesPerSlice(width, height, format)` but allows callers to reuse a precomputed row stride.
 *
 * @param bytesPerRow Bytes per row (or bytes per block-row for compressed formats).
 * @param height Texture region height in texels.
 * @param format Pixel format.
 * @return Bytes per slice.
 */
uint64_t bytesPerSliceWithRow(uint32_t bytesPerRow, uint32_t height, snap::rhi::PixelFormat format);

/**
 * @brief Generates a default texture view creation info for a given texture creation info.
 *
 * The returned view covers all mip levels and array layers of the texture, and uses the same format.
 *
 * @param textureCreateInfo Texture creation info.
 * @return Texture view creation info.
 */
snap::rhi::TextureViewInfo defaultTextureViewInfo(const snap::rhi::TextureCreateInfo& textureCreateInfo);
} // namespace snap::rhi
