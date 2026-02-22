//
//  CopyInfo.h
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 23.09.2020.
//  Copyright © 2021 Snapchat. All rights reserved.
//

#pragma once

#include "snap/rhi/Structs.h"

namespace snap::rhi {
/**
 * @brief Identifies a set of texture subresources for copy/blit operations.
 *
 * This is a simplified variant of Vulkan's `VkImageSubresourceLayers` used by SnapRHI for copy commands.
 */
struct TextureSubresourceLayers {
    uint32_t mipLevel = 0;
};

/**
 * @brief Describes a copy between a buffer and a texture.
 *
 * Used by `BlitCommandEncoder::copyBufferToTexture()` and `BlitCommandEncoder::copyTextureToBuffer()`.
 *
 * ## Backend notes
 * - Vulkan maps this to `VkBufferImageCopy`. The `bufferOffset` must be 4-byte aligned.
 * - Metal maps this to `MTLBlitCommandEncoder` buffer-texture copy calls.
 * - OpenGL uses readback/upload paths (often via `glReadPixels` / staging) and interprets `bufferOffset` as a byte
 *   offset into mapped CPU memory when PBOs aren't used.
 */
struct BufferTextureCopy {
    /**
     * @brief Byte offset in the source/destination buffer.
     *
     * @note Vulkan requires this to be a multiple of 4.
     */
    uint64_t bufferOffset = 0;

    /**
     * @brief The number of bytes between adjacent rows of pixels in buffer memory.
     *
     * When set to 0, the copy is treated as tightly packed and backends derive the value from
     * `textureExtent` and the texture format.
     *
     * @note Compressed formats:
     * For block-compressed textures, this value is the number of bytes between the starts of two *row blocks*
     * (see `snap::rhi::bytesPerRow()` for details).
     */
    uint32_t bytesPerRow = 0;

    /**
     * @brief The number of bytes between adjacent 2D slices in buffer memory.
     *
     * When set to 0, the copy is treated as tightly packed and backends derive the value from
     * `bytesPerRow`, `textureExtent.height`, and the texture format.
     *
     * @note Meaning depends on texture type:
     * - For `Texture3D`, this is the stride between Z-slices.
     * - For `Texture2DArray` / `TextureCubemap`, backends typically copy slice-by-slice and increment
     *   `bufferOffset` by `bytesPerSlice` per array layer / cube face.
     */
    uint64_t bytesPerSlice = 0;

    TextureSubresourceLayers textureSubresource{};

    /**
     * @brief Texture offset in texels.
     *
     * `z` is interpreted as:
     * - array layer / cube face index for array/cubemap textures
     * - slice/depth index for 3D textures
     */
    Offset3D textureOffset{};

    /**
     * @brief Extent of the texture region to copy, in texels.
     *
     * `depth` is interpreted as:
     * - number of array layers / cube faces to copy for array/cubemap textures
     * - depth (number of slices) for 3D textures
     */
    Extent3D textureExtent{};
};

/**
 * @brief Describes a buffer-to-buffer copy region.
 *
 * Corresponds to Vulkan's `VkBufferCopy`.
 */
struct BufferCopy {
    /** @brief Byte offset in the source buffer. */
    uint64_t srcOffset = 0;

    /** @brief Byte offset in the destination buffer. */
    uint64_t dstOffset = 0;

    /** @brief Size of the copy in bytes. */
    uint64_t size = 0;
};

/**
 * @brief Describes a texture-to-texture copy region.
 *
 * Corresponds to Vulkan's `VkImageCopy`.
 */
struct TextureCopy {
    TextureSubresourceLayers srcSubresource{};

    /**
     * @brief Source offset in texels.
     *
     * The `z` component is interpreted as array layer / cube face index for array/cubemap textures,
     * or as depth index for 3D textures.
     */
    Offset3D srcOffset{};

    TextureSubresourceLayers dstSubresource{};

    /**
     * @brief Destination offset in texels.
     *
     * The `z` component is interpreted as array layer / cube face index for array/cubemap textures,
     * or as depth index for 3D textures.
     */
    Offset3D dstOffset{};

    /**
     * @brief Extent of the region to copy, in texels.
     */
    Extent3D extent{};
};
} // namespace snap::rhi
