//
//  snap::rhi::FramebufferCreateInfo.h
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 21.09.2020.
//  Copyright © 2021 Snapchat. All rights reserved.
//

#pragma once

#include "snap/rhi/common/HashCombine.h"
#include <array>
#include <memory>
#include <vector>

#include "snap/rhi/Enums.h"
#include "snap/rhi/Limits.h"
#include "snap/rhi/PixelFormat.h"
#include "snap/rhi/Structs.h"

namespace snap::rhi {

/**
 * @brief Describes a render pass attachment.
 *
 * An attachment describes the format, sample count, and load/store behavior for a render target used by a render pass.
 * Attachment instances are referenced by index from `snap::rhi::SubpassDescription`.
 */
struct AttachmentDescription {
    /// Load operation applied when the attachment is first used.
    snap::rhi::AttachmentLoadOp loadOp = snap::rhi::AttachmentLoadOp::DontCare;
    /// Store operation applied when the attachment is no longer needed.
    snap::rhi::AttachmentStoreOp storeOp = snap::rhi::AttachmentStoreOp::DontCare;

    /// Stencil load operation (only relevant for depth-stencil formats).
    snap::rhi::AttachmentLoadOp stencilLoadOp = snap::rhi::AttachmentLoadOp::DontCare;
    /// Stencil store operation (only relevant for depth-stencil formats).
    snap::rhi::AttachmentStoreOp stencilStoreOp = snap::rhi::AttachmentStoreOp::DontCare;

    /// Pixel format of the attachment.
    PixelFormat format = PixelFormat::R8G8B8A8Unorm;

    /**
     * @brief Sample count for the attachment.
     */
    snap::rhi::SampleCount samples = snap::rhi::SampleCount::Count1;

    /**
     * Specifies the number of autoresolved samples of the texture.
     * This field can be used for cases with autoresolved samples:
     *      https://registry.khronos.org/OpenGL/extensions/EXT/EXT_multisampled_render_to_texture.txt (regular)
     *      https://registry.khronos.org/OpenGL/extensions/OVR/OVR_multiview_multisampled_render_to_texture.txt (stereo)
     *      (TODO: extensions for other graphics backends)
     *
     * What it means:
     * The actual attachment can be non-multisample texture (samples == 1), but the rendering will be done to the
     * intermediate multisample texture managed by the GPU driver. Depending on the API, at the end of the render pass
     * or on the next first usage (e.g., copy, sampling or reading), the GPU resolves the contents of the multisample
     * texture and writes the results into the attachment.
     *
     * @see snap::rhi::Capabilities::isMultiviewMSAAImplicitResolveEnabled
     */
    std::optional<AutoresolvedAttachmentDescription> autoresolvedAttachment;

    /**
     *  Vulkan:
     *      Vulkan used ImageView for attachments, but each element of pAttachments must only specify a single mip level
     *      https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkFramebufferCreateInfo.html
     *
     *  OpenGL:
     *      We always have to specify the mipmap level of texture to attach.
     *      glFramebufferTexture2D/glFramebufferTextureLayer
     *
     *  Metal:
     *      We always have to specify the mipmap level of texture to attach.
     *      https://developer.apple.com/documentation/metal/mtlrenderpassattachmentdescriptor?language=objc
     */
    uint32_t mipLevel = 0;

    /**
     *  The layer index of array texture object (usually referred to as "slice" in Metal and D3D docs).
     *  Vulkan, OpenGL and Metal API's supported multilayer rendering
     *  https://www.khronos.org/opengl/wiki/Geometry_Shader#Layered_rendering
     *  https://developer.apple.com/documentation/metal/mtlrenderpassdescriptor/rendering_to_multiple_texture_slices_in_a_draw_command
     *  Vulkan used VkImageView for describe mipLevel and layer
     *  (each element of pAttachments must only specify a single mip level).
     */
    uint32_t layer = 0;

    constexpr friend bool operator==(const AttachmentDescription&, const AttachmentDescription&) noexcept = default;
};

/**
 * @brief Reference to an attachment in `snap::rhi::RenderPassCreateInfo::attachments`.
 */
struct AttachmentReference {
    /**
     * @brief Attachment index.
     *
     * Use `snap::rhi::AttachmentUnused` to indicate that this reference is not used.
     */
    uint32_t attachment = AttachmentUnused;

    /**
     * @brief Three-way comparison.
     */
    constexpr friend auto operator<=>(const AttachmentReference&, const AttachmentReference&) noexcept = default;
};

/**
 * @brief Describes a subpass within a render pass.
 *
 * A subpass declares which attachments are used as color, resolve, and depth/stencil targets.
 * Attachment references index into `snap::rhi::RenderPassCreateInfo::attachments`.
 */
struct SubpassDescription {
    /**
     *  Attach to the texture property a TextureType::Texture2D multisample texture,
     *  and a 2D or cube texture to the resolveTexture property.
     *  When a rendering command executes, it renders to the multisample texture.
     *  At the end of the render pass,
     *  the GPU resolves the contents of the multisample texture and writes the results into the resolve texture.
     *
     *  Metal: https://developer.apple.com/documentation/metal/mtlrenderpassattachmentdescriptor?language=objc
     */

    std::array<AttachmentReference, MaxColorAttachments> colorAttachments;
    uint32_t colorAttachmentCount = 0;

    std::array<AttachmentReference, MaxColorAttachments> resolveAttachments;
    uint32_t resolveAttachmentCount = 0;

    AttachmentReference depthStencilAttachment;

    // For Vulkan we need to check - VK_KHR_depth_stencil_resolve
    AttachmentReference resolveDepthStencilAttachment;

    uint32_t viewMask = 0;
    /**
     * @brief Equality comparison.
     *
     * Compares the active portions of the attachment reference arrays (up to the corresponding `*Count` fields), along
     * with depth/stencil references and `viewMask`.
     */
    constexpr friend bool operator==(const SubpassDescription& l, const SubpassDescription& r) noexcept {
        if (l.viewMask != r.viewMask) {
            return false;
        }
        if (l.colorAttachmentCount != r.colorAttachmentCount || l.resolveAttachmentCount != r.resolveAttachmentCount) {
            return false;
        }

        for (uint32_t i = 0; i < l.colorAttachmentCount; ++i) {
            if (l.colorAttachments[i] != r.colorAttachments[i]) {
                return false;
            }
        }

        for (uint32_t i = 0; i < l.resolveAttachmentCount; ++i) {
            if (l.resolveAttachments[i] != r.resolveAttachments[i]) {
                return false;
            }
        }

        if (l.depthStencilAttachment != r.depthStencilAttachment) {
            return false;
        }

        if (l.resolveDepthStencilAttachment != r.resolveDepthStencilAttachment) {
            return false;
        }

        return true;
    }
};

/**
 * @brief Describes a dependency between two subpasses.
 */
struct SubpassDependency {
    uint32_t srcSubpass = 0;
    uint32_t dstSubpass = 0;
    PipelineStageBits srcStageMask = PipelineStageBits::None;
    PipelineStageBits dstStageMask = PipelineStageBits::None;
    AccessFlags srcAccessMask = AccessFlags::None;
    AccessFlags dstAccessMask = AccessFlags::None;
    DependencyFlags dependencyFlags = DependencyFlags::ByRegion;

    /**
     * @brief Three-way comparison.
     */
    constexpr friend auto operator<=>(const SubpassDependency&, const SubpassDependency&) noexcept = default;
};

/**
 * @brief Render pass creation parameters.
 *
 * This structure is an API-agnostic description of render pass state: a list of attachments, subpasses, and explicit
 * subpass dependencies.
 *
 * The `*Count` fields define the active prefix of each fixed-size array.
 */
struct RenderPassCreateInfo {
    std::array<AttachmentDescription, MaxAttachments> attachments;
    uint32_t attachmentCount = 0;

    std::array<SubpassDescription, MaxSubpasses> subpasses;
    uint32_t subpassCount = 0;

    std::array<SubpassDependency, MaxDependencies> dependencies;
    uint32_t dependencyCount = 0;

    /**
     * @brief Equality comparison.
     *
     * Compares the active portions of the attachments/subpasses/dependencies arrays (up to their corresponding count
     * fields).
     */
    constexpr friend bool operator==(const RenderPassCreateInfo& l, const RenderPassCreateInfo& r) noexcept {
        if (l.attachmentCount != r.attachmentCount || l.subpassCount != r.subpassCount ||
            l.dependencyCount != r.dependencyCount) {
            return false;
        }

        for (uint32_t i = 0; i < l.attachmentCount; ++i) {
            if (l.attachments[i] != r.attachments[i]) {
                return false;
            }
        }

        for (uint32_t i = 0; i < l.subpassCount; ++i) {
            if (l.subpasses[i] != r.subpasses[i]) {
                return false;
            }
        }

        for (uint32_t i = 0; i < l.dependencyCount; ++i) {
            if (l.dependencies[i] != r.dependencies[i]) {
                return false;
            }
        }

        return true;
    }

    /**
     * @brief Returns true if this render pass description is compatible for graphics pipeline reuse with @other.
     *
     * This predicate is intended to answer: "Can I reuse a graphics pipeline created for one render pass description
     * with another render pass description?"
     *
     * Compatibility rules (as implemented):
     * - `attachmentCount`, `subpassCount` and `dependencyCount` must match.
     * - For each subpass: `colorAttachmentCount` and `resolveAttachmentCount` must match.
     * - Presence of depth/stencil and resolve depth/stencil attachments must match.
     * - For each referenced attachment within each subpass, `format` and `samples` must match.
     * - For each attachment index `i` in `[0, attachmentCount)`, `format` must match.
     * - Dependencies must match.
     *
     * Intentionally ignored:
     * - Attachment load/store ops and stencil load/store ops.
     * - Attachment `mipLevel`, `layer`, and `autoresolvedAttachment`.
     *
     * @note This function does not validate that attachment reference indices are within bounds; the caller must ensure
     * all referenced attachment indices are valid.
     */
    [[nodiscard]] constexpr bool isPipelineCompatible(const RenderPassCreateInfo& other) const noexcept {
        if (attachmentCount != other.attachmentCount || subpassCount != other.subpassCount ||
            dependencyCount != other.dependencyCount) {
            return false;
        }

        for (uint32_t subpassIndex = 0; subpassIndex < other.subpassCount; ++subpassIndex) {
            const auto& l = subpasses[subpassIndex];
            const auto& r = other.subpasses[subpassIndex];

            if (l.colorAttachmentCount != r.colorAttachmentCount) {
                return false;
            }

            if (l.resolveAttachmentCount != r.resolveAttachmentCount) {
                return false;
            }

            if ((l.depthStencilAttachment.attachment != AttachmentUnused &&
                 r.depthStencilAttachment.attachment == AttachmentUnused) ||
                (r.depthStencilAttachment.attachment != AttachmentUnused &&
                 l.depthStencilAttachment.attachment == AttachmentUnused)) {
                return false;
            }

            if ((l.resolveDepthStencilAttachment.attachment != AttachmentUnused &&
                 r.resolveDepthStencilAttachment.attachment == AttachmentUnused) ||
                (r.resolveDepthStencilAttachment.attachment != AttachmentUnused &&
                 l.resolveDepthStencilAttachment.attachment == AttachmentUnused)) {
                return false;
            }

            for (uint32_t colorIndex = 0; colorIndex < l.colorAttachmentCount; ++colorIndex) {
                const auto& l_id = l.colorAttachments[colorIndex].attachment;
                const auto& r_id = r.colorAttachments[colorIndex].attachment;

                if (attachments[l_id].format != other.attachments[r_id].format) {
                    return false;
                }

                if (attachments[l_id].samples != other.attachments[r_id].samples) {
                    return false;
                }
            }

            for (uint32_t resolveIndex = 0; resolveIndex < l.resolveAttachmentCount; ++resolveIndex) {
                const auto& l_id = l.resolveAttachments[resolveIndex].attachment;
                const auto& r_id = r.resolveAttachments[resolveIndex].attachment;

                if (attachments[l_id].format != other.attachments[r_id].format) {
                    return false;
                }

                if (attachments[l_id].samples != other.attachments[r_id].samples) {
                    return false;
                }
            }

            if (l.depthStencilAttachment.attachment != AttachmentUnused) {
                const auto& l_id = l.depthStencilAttachment.attachment;
                const auto& r_id = r.depthStencilAttachment.attachment;

                if (attachments[l_id].format != other.attachments[r_id].format) {
                    return false;
                }

                if (attachments[l_id].samples != other.attachments[r_id].samples) {
                    return false;
                }
            }

            if (l.resolveDepthStencilAttachment.attachment != AttachmentUnused) {
                const auto& l_id = l.resolveDepthStencilAttachment.attachment;
                const auto& r_id = r.resolveDepthStencilAttachment.attachment;

                if (attachments[l_id].format != other.attachments[r_id].format) {
                    return false;
                }

                if (attachments[l_id].samples != other.attachments[r_id].samples) {
                    return false;
                }
            }
        }

        for (uint32_t attachmentIndex = 0; attachmentIndex < other.attachmentCount; ++attachmentIndex) {
            if (attachments[attachmentIndex].format != other.attachments[attachmentIndex].format) {
                return false;
            }
        }

        for (uint32_t dependencyIndex = 0; dependencyIndex < other.dependencyCount; ++dependencyIndex) {
            if (dependencies[dependencyIndex] != other.dependencies[dependencyIndex]) {
                return false;
            }
        }

        return true;
    }

    /**
     * @brief Returns true if this render pass description is compatible for framebuffer reuse with @p other.
     *
     * This predicate is intended to answer: "Can I reuse the same attachment set / framebuffer configuration with a
     * different render pass description?"
     *
     * Compatibility rules (as implemented):
     * - `attachmentCount`, `subpassCount` and `dependencyCount` must match.
     * - For each attachment index `i` in `[0, attachmentCount)`, `format` and `samples` must match.
     * - Dependencies must match.
     *
     * @note Framebuffer dimensions (width/height/layers) are not represented here and therefore are not checked.
     * @note Attachment `mipLevel` and `layer` are not checked; callers must ensure framebuffer bindings match the
     * intended usage.
     */
    [[nodiscard]] constexpr bool isFramebufferCompatible(const RenderPassCreateInfo& other) const noexcept {
        if (attachmentCount != other.attachmentCount || subpassCount != other.subpassCount ||
            dependencyCount != other.dependencyCount) {
            return false;
        }

        for (uint32_t i = 0; i < other.attachmentCount; ++i) {
            if (attachments[i].format != other.attachments[i].format) {
                return false;
            }

            if (attachments[i].samples != other.attachments[i].samples) {
                return false;
            }
        }

        for (uint32_t i = 0; i < other.dependencyCount; ++i) {
            if (dependencies[i] != other.dependencies[i]) {
                return false;
            }
        }

        return true;
    }
};

/**
 * [Layered rendering shader side] {
 * Metal : {
 *      In vertex shader:
 *          const uint instanceId [[ instance_id ]]; // read from
 *          uint face [[render_target_array_index]]; // write to
 *  }
 *
 *  OpenGL : {
 *      In geometry shader:
 *          User should pass gl_InstanceID from vertex shader
 *          gl_Layer; // write to
 *      or
 *          layout (invocations = x) in; // Geometry shader{gl_InvocationID}
 *          gl_Layer; // write to
 *  }
 *
 *  Vulkan : {
 *      In geometry shader:
 *          User should pass gl_InstanceID from vertex shader
 *          gl_Layer; // write to
 *      or
 *          layout (invocations = x) in; // Geometry shader{gl_InvocationID}
 *          gl_Layer; // write to
 *  }
 * }
 */

/**
 * [Multiview rendering shader side] {
 * Metal : {
 *      In vertex shader:
 *          const uint instanceId [[ instance_id ]]; // read from
 *          uint face [[render_target_array_index]]; // write to
 *  }
 *
 *  OpenGL : {
 *      In vertex shader:
 *          gl_ViewID_OVR;// read from
 *  }
 *
 *  Vulkan : {
 *      In vertex shader:
 *          gl_ViewID_OVR;// read from
 *  }
 * }
 */
} // namespace snap::rhi

namespace std {
template<>
struct hash<snap::rhi::AttachmentDescription> {
    size_t operator()(const snap::rhi::AttachmentDescription& info) const noexcept {
        return snap::rhi::common::hash_combine(static_cast<uint32_t>(info.loadOp),
                                               static_cast<uint32_t>(info.storeOp),
                                               static_cast<uint32_t>(info.stencilLoadOp),
                                               static_cast<uint32_t>(info.stencilStoreOp),
                                               static_cast<uint32_t>(info.format),
                                               static_cast<uint32_t>(info.samples),
                                               static_cast<uint32_t>(info.mipLevel),
                                               static_cast<uint32_t>(info.layer));
    }
};

template<>
struct hash<snap::rhi::AttachmentReference> {
    size_t operator()(const snap::rhi::AttachmentReference& info) const noexcept {
        return snap::rhi::common::hash_combine(info.attachment);
    }
};

template<>
struct hash<snap::rhi::SubpassDescription> {
    size_t operator()(const snap::rhi::SubpassDescription& info) const noexcept {
        size_t result = 0;
        for (uint32_t i = 0; i < info.colorAttachmentCount; ++i) {
            result = snap::rhi::common::hash_combine(
                result, std::hash<snap::rhi::AttachmentReference>()(info.colorAttachments[i]));
        }

        for (uint32_t i = 0; i < info.resolveAttachmentCount; ++i) {
            result = snap::rhi::common::hash_combine(
                result, std::hash<snap::rhi::AttachmentReference>()(info.resolveAttachments[i]));
        }

        result = snap::rhi::common::hash_combine(
            result, std::hash<snap::rhi::AttachmentReference>()(info.depthStencilAttachment));
        result = snap::rhi::common::hash_combine(
            result, std::hash<snap::rhi::AttachmentReference>()(info.resolveDepthStencilAttachment));

        return result;
    }
};

template<>
struct hash<snap::rhi::SubpassDependency> {
    size_t operator()(const snap::rhi::SubpassDependency& info) const noexcept {
        return snap::rhi::common::hash_combine(static_cast<uint32_t>(info.srcSubpass),
                                               static_cast<uint32_t>(info.dstSubpass),
                                               static_cast<uint32_t>(info.srcStageMask),
                                               static_cast<uint32_t>(info.dstStageMask),
                                               static_cast<uint32_t>(info.srcAccessMask),
                                               static_cast<uint32_t>(info.dstAccessMask),
                                               static_cast<uint32_t>(info.dependencyFlags));
    }
};

template<>
struct hash<snap::rhi::RenderPassCreateInfo> {
    size_t operator()(const snap::rhi::RenderPassCreateInfo& info) const noexcept {
        size_t result = 0;
        for (uint32_t i = 0; i < info.attachmentCount; ++i) {
            result = snap::rhi::common::hash_combine(
                result, std::hash<snap::rhi::AttachmentDescription>()(info.attachments[i]));
        }

        for (uint32_t i = 0; i < info.subpassCount; ++i) {
            result =
                snap::rhi::common::hash_combine(result, std::hash<snap::rhi::SubpassDescription>()(info.subpasses[i]));
        }

        for (uint32_t i = 0; i < info.dependencyCount; ++i) {
            result = snap::rhi::common::hash_combine(result,
                                                     std::hash<snap::rhi::SubpassDependency>()(info.dependencies[i]));
        }

        return result;
    }
};
} // namespace std
