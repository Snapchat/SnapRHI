//
//  BlitCommandEncoder.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 21.09.2020.
//  Copyright © 2021 Snapchat. All rights reserved.
//

#pragma once

#include "snap/rhi/CommandEncoder.h"
#include "snap/rhi/CopyInfo.h"
#include <span>

namespace snap::rhi {

/**
 * @brief Command encoder used to record transfer (blit/copy) operations into a command buffer.
 *
 * Blit encoders are used for GPU-side data movement:
 * - buffer-to-buffer copies
 * - buffer-to-texture uploads
 * - texture-to-buffer readbacks
 * - texture-to-texture copies
 * - mipmap generation
 *
 * ## Typical usage
 * ```
 * auto* blit = commandBuffer->createBlitCommandEncoder();
 * blit->beginEncoding();
 * blit->copyBufferToTexture(staging, texture, regions);
 * blit->endEncoding();
 * ```
 *
 * ## Ordering / call contract
 * - Call `beginEncoding()` before recording any commands.
 * - Call `endEncoding()` when finished.
 * - Unless otherwise stated, the encoder does not perform implicit synchronization; callers must ensure resources are
 *   in appropriate states/usages for the operation (depending on backend).
 *
 * ## Thread-safety
 * Not thread-safe. Recording must be externally synchronized and typically happens from a single thread.
 */
class BlitCommandEncoder : public snap::rhi::CommandEncoder {
public:
    BlitCommandEncoder(Device* device, CommandBuffer* commandBuffer);
    ~BlitCommandEncoder() override = default;

    /**
     * @brief Begins a blit encoding scope.
     *
     * Must be called exactly once before recording any blit commands.
     */
    virtual void beginEncoding() = 0;

    /**
     * @brief Copies one or more regions from a source buffer into a destination buffer.
     *
     * @param srcBuffer Buffer providing the source bytes. Must cover all source ranges described by `info`.
     * @param dstBuffer Buffer receiving the copied bytes. Must be large enough to fit all destination ranges described
     *        by `info`.
     * @param info Copy regions expressed in bytes (offsets and sizes) in the source and destination buffers.
     *
     * @note Source and destination buffers may be the same buffer, but regions must not overlap unless the backend
     * explicitly supports it.
     */
    virtual void copyBuffer(snap::rhi::Buffer* srcBuffer,
                            snap::rhi::Buffer* dstBuffer,
                            std::span<const snap::rhi::BufferCopy> info) = 0;

    /**
     * @brief Copies one or more regions from a buffer into a texture (upload).
     *
     * @param srcBuffer Buffer containing the texel data to upload (typically a staging/upload buffer).
     * @param dstTexture Texture subresources to receive the uploaded data.
     * @param info Regions describing how bytes in @p srcBuffer map to texture subresources in @p dstTexture.
     *
     * @note Backends may impose alignment requirements. For example, Vulkan requires `BufferTextureCopy::bufferOffset`
     * to be a multiple of 4 bytes (validated by the implementation).
     *
     * @note When `dstTexture` wraps an external/native texture (interop), implementations may need to preserve it for
     * the duration of the command buffer execution.
     */
    virtual void copyBufferToTexture(snap::rhi::Buffer* srcBuffer,
                                     snap::rhi::Texture* dstTexture,
                                     std::span<const snap::rhi::BufferTextureCopy> info) = 0;

    /**
     * @brief Copies one or more regions from a texture into a buffer (readback).
     *
     * @param srcTexture Texture subresources providing the texel data to read back.
     * @param dstBuffer Buffer receiving the copied texel data (typically a readback buffer).
     * @param info Regions describing how texture subresources in @p srcTexture map to byte ranges in @p dstBuffer.
     *
     * @note Backends may impose alignment requirements. For example, Vulkan requires `BufferTextureCopy::bufferOffset`
     * to be a multiple of 4 bytes (validated by the implementation).
     *
     * @note When `srcTexture` wraps an external/native texture (interop), implementations may need to preserve it for
     * the duration of the command buffer execution.
     */
    virtual void copyTextureToBuffer(snap::rhi::Texture* srcTexture,
                                     snap::rhi::Buffer* dstBuffer,
                                     std::span<const snap::rhi::BufferTextureCopy> info) = 0;

    /**
     * @brief Copies one or more regions from a source texture into a destination texture.
     *
     * @param srcTexture Texture subresources providing the source texels.
     * @param dstTexture Texture subresources receiving the copied texels.
     * @param info Copy regions describing which areas/subresources are copied.
     *
     * @note When either texture wraps an external/native texture (interop), implementations may need to preserve it
     * for the duration of the command buffer execution.
     */
    virtual void copyTexture(snap::rhi::Texture* srcTexture,
                             snap::rhi::Texture* dstTexture,
                             std::span<const snap::rhi::TextureCopy> info) = 0;

    /**
     * @brief Generates mipmaps for a texture.
     *
     * @param texture Texture to generate mipmaps for.
     *
     * @warning The texture must have `mipLevels > 1`.
     *
     * @note Mipmap generation usually requires that the format is filterable. On Vulkan, the implementation selects
     * a linear filter only if the format reports `FormatFeatures::SampledFilterLinear`; otherwise it falls back to
     * nearest filtering.
     *
     * @note When `texture` wraps an external/native texture (interop), implementations may need to preserve it for
     * the duration of the command buffer execution.
     */
    virtual void generateMipmaps(snap::rhi::Texture* texture) = 0;

protected:
};
} // namespace snap::rhi
