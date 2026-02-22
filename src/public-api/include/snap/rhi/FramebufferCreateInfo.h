//
//  snap::rhi::FramebufferCreateInfo.h
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 21.09.2020.
//  Copyright © 2021 Snapchat. All rights reserved.
//

#pragma once

#include <array>
#include <memory>
#include <vector>

#include "snap/rhi/Enums.h"
#include "snap/rhi/Limits.h"
#include "snap/rhi/RenderPassCreateInfo.h"

namespace snap::rhi {
class RenderPass;
class Texture;

/**
 * @brief Framebuffer creation parameters.
 *
 * A framebuffer binds a @ref snap::rhi::RenderPass to a concrete set of attachment textures and defines the render area
 * dimensions used by render pass encoding.
 *
 * @note This structure is API-agnostic. Backends interpret it in their native way:
 * - Vulkan creates a native framebuffer object from the provided attachment views.
 * - Metal/OpenGL treat it as a description used while configuring render targets.
 * - WebGPU uses it to build a render pass descriptor with attachment views.
 */
struct FramebufferCreateInfo {
    /**
     * @brief Render pass describing attachment formats, load/store ops, and subpasses.
     *
     * The referenced render pass must outlive the framebuffer and any GPU work that uses it.
     */
    snap::rhi::RenderPass* renderPass = nullptr;

    /**
     * @brief Attachment textures used by the framebuffer.
     *
     * Attachment ordering must match the render pass attachment descriptions:
     * `attachments[i]` corresponds to `renderPass->getCreateInfo().attachments[i]`.
     *
     * @note The framebuffer does not take ownership of the textures.
     */
    std::vector<snap::rhi::Texture*> attachments{};

    /**
     * @brief Framebuffer width in pixels.
     *
     * Backends use this to define the render area / target extent.
     */
    uint32_t width = 0;

    /**
     * @brief Framebuffer height in pixels.
     *
     * Backends use this to define the render area / target extent.
     */
    uint32_t height = 0;

    /**
     * @brief Number of layers rendered by this framebuffer.
     *
     * - If @ref layers is `1`, the framebuffer targets a *single* layer/slice of each attachment as specified by the
     *   render pass attachment description (`AttachmentDescription::layer`).
     * - If @ref layers is greater than `1`, layered rendering is enabled and the framebuffer targets a contiguous
     *   layer range starting at `AttachmentDescription::layer` with length @ref layers.
     *
     * @note For 2D-array and cubemap textures, layers correspond to array layers / cubemap faces.
     * For 3D textures, backends may treat the attachment as a single image layer (Vulkan uses `baseArrayLayer = 0` for
     * 3D attachments and does not index depth slices as array layers).
     *
     * @warning All attachment textures must be compatible with this layer count (sufficient array layers/faces).
     */
    uint32_t layers = 1;
};
} // namespace snap::rhi
