//
//  ClearValue.h
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 08.03.2022.
//

#pragma once

#include <array>

#include "snap/rhi/Enums.h"

namespace snap::rhi {

/**
 * @brief Clear color value used when clearing a color attachment.
 *
 * This is a union matching the spirit of Vulkan's `VkClearColorValue`.
 * The correct member to use depends on the attachment's pixel format:
 * - For normalized and floating-point formats, populate `float32`.
 * - For signed integer formats, populate `int32`.
 * - For unsigned integer formats, populate `uint32`.
 *
 * ## Backend behavior
 * - **OpenGL** chooses the clear API based on the attachment format:
 *   - float formats: `glClearColor` / `glClearBufferfv`
 *   - signed int formats: `glClearBufferiv`
 *   - unsigned int formats: `glClearBufferuiv`
 * - **Vulkan** converts this to `VkClearValue` when beginning a render pass.
 * - **Metal** uses the clear values to set `MTLRenderPassDescriptor` clear colors.
 *
 * @note `float32` is value-initialized to `{0,0,0,0}`.
 */
union ClearColorValue {
    /** @brief Float clear value (r,g,b,a). */
    std::array<float, 4> float32{0.0f, 0.0f, 0.0f, 0.0f};

    /** @brief Signed integer clear value (r,g,b,a). */
    std::array<int32_t, 4> int32;

    /** @brief Unsigned integer clear value (r,g,b,a). */
    std::array<uint32_t, 4> uint32;
};

/**
 * @brief Clear value used when clearing a depth/stencil attachment.
 *
 * Mirrors Vulkan's `VkClearDepthStencilValue`.
 *
 * ## Defaults
 * - `depth` defaults to 1.0 (far plane).
 * - `stencil` defaults to 0.
 */
struct ClearDepthStencilValue {
    /** @brief Depth clear value. */
    float depth = 1.0f;

    /** @brief Stencil clear value. */
    uint32_t stencil = 0u;
};

/**
 * @brief Union of clear values for color or depth/stencil attachments.
 *
 * This mirrors Vulkan's `VkClearValue` and is used by render pass / dynamic rendering APIs to specify per-attachment
 * clear values.
 *
 * @note Which member is read depends on the attachment type:
 * - color attachments read `color`
 * - depth/stencil attachments read `depthStencil`
 *
 * @warning Because this is a union, writing one member and reading a different member is undefined behavior.
 * Always populate the member matching the attachment being cleared.
 */
union ClearValue {
    /** @brief Clear value for color attachments. */
    ClearColorValue color;

    /** @brief Clear value for depth/stencil attachments. */
    ClearDepthStencilValue depthStencil;

    /**
     * @brief Constructs a clear value with an all-zero color clear ({0,0,0,0}).
     */
    ClearValue() : color({}) {}
};
} // namespace snap::rhi
