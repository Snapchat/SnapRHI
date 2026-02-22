#pragma once

#include "snap/rhi/backend/opengl/OpenGL.h"
#include <cassert>
#include <cstdint>
#include <string_view>

namespace snap::rhi::backend::opengl {

enum class FramebufferAttachmentTarget : uint32_t {
    Detached = 0,
    ColorAttachment0 = GL_COLOR_ATTACHMENT0,
    ColorAttachment1 = GL_COLOR_ATTACHMENT1,
    ColorAttachment2 = GL_COLOR_ATTACHMENT2,
    ColorAttachment3 = GL_COLOR_ATTACHMENT3,
    ColorAttachment4 = GL_COLOR_ATTACHMENT4,
    ColorAttachment5 = GL_COLOR_ATTACHMENT5,
    ColorAttachment6 = GL_COLOR_ATTACHMENT6,
    ColorAttachment7 = GL_COLOR_ATTACHMENT7,
    ColorAttachment8 = GL_COLOR_ATTACHMENT8,
    ColorAttachment9 = GL_COLOR_ATTACHMENT9,
    ColorAttachment10 = GL_COLOR_ATTACHMENT10,
    ColorAttachment11 = GL_COLOR_ATTACHMENT11,
    ColorAttachment12 = GL_COLOR_ATTACHMENT12,
    ColorAttachment13 = GL_COLOR_ATTACHMENT13,
    ColorAttachment14 = GL_COLOR_ATTACHMENT14,
    ColorAttachment15 = GL_COLOR_ATTACHMENT15,
    DepthStencilAttachment = GL_DEPTH_STENCIL_ATTACHMENT,
    DepthAttachment = GL_DEPTH_ATTACHMENT,
    StencilAttachment = GL_STENCIL_ATTACHMENT,
    FirstColorAttachment = ColorAttachment0,
    LastColorAttachment = ColorAttachment15,
    ColorAttachmentCount = LastColorAttachment - FirstColorAttachment + 1,
};

static_assert(uint32_t(FramebufferAttachmentTarget::ColorAttachmentCount) == 16u);

constexpr uint32_t NumFramebufferColorAttachmentTargets =
    static_cast<uint32_t>(FramebufferAttachmentTarget::ColorAttachmentCount);
constexpr uint32_t MaxNumFramebufferTargets = NumFramebufferColorAttachmentTargets + 2;

[[nodiscard]] static inline constexpr FramebufferAttachmentTarget getFramebufferColorAttachmentTarget(uint8_t i) {
    return static_cast<FramebufferAttachmentTarget>(
        static_cast<uint32_t>(FramebufferAttachmentTarget::FirstColorAttachment) + i);
}

[[nodiscard]] static inline constexpr std::string_view toString(FramebufferAttachmentTarget target) noexcept {
    switch (target) {
        case FramebufferAttachmentTarget::Detached:
            return "Detached";
        case FramebufferAttachmentTarget::ColorAttachment0:
            return "ColorAttachment0";
        case FramebufferAttachmentTarget::ColorAttachment1:
            return "ColorAttachment1";
        case FramebufferAttachmentTarget::ColorAttachment2:
            return "ColorAttachment2";
        case FramebufferAttachmentTarget::ColorAttachment3:
            return "ColorAttachment3";
        case FramebufferAttachmentTarget::ColorAttachment4:
            return "ColorAttachment4";
        case FramebufferAttachmentTarget::ColorAttachment5:
            return "ColorAttachment5";
        case FramebufferAttachmentTarget::ColorAttachment6:
            return "ColorAttachment6";
        case FramebufferAttachmentTarget::ColorAttachment7:
            return "ColorAttachment7";
        case FramebufferAttachmentTarget::ColorAttachment8:
            return "ColorAttachment8";
        case FramebufferAttachmentTarget::ColorAttachment9:
            return "ColorAttachment9";
        case FramebufferAttachmentTarget::ColorAttachment10:
            return "ColorAttachment10";
        case FramebufferAttachmentTarget::ColorAttachment11:
            return "ColorAttachment11";
        case FramebufferAttachmentTarget::ColorAttachment12:
            return "ColorAttachment12";
        case FramebufferAttachmentTarget::ColorAttachment13:
            return "ColorAttachment13";
        case FramebufferAttachmentTarget::ColorAttachment14:
            return "ColorAttachment14";
        case FramebufferAttachmentTarget::ColorAttachment15:
            return "ColorAttachment15";
        case FramebufferAttachmentTarget::DepthStencilAttachment:
            return "DepthStencilAttachment";
        case FramebufferAttachmentTarget::DepthAttachment:
            return "DepthAttachment";
        case FramebufferAttachmentTarget::StencilAttachment:
            return "StencilAttachment";
        default:
            assert(false && "Caught unknown framebuffer attachment target.");
            return "<Error>";
    }
}

enum class FramebufferAttachmentTargetIndex : uint8_t {
    Detached = 0,
    ColorAttachment0,
    ColorAttachment1,
    ColorAttachment2,
    ColorAttachment3,
    ColorAttachment4,
    ColorAttachment5,
    ColorAttachment6,
    ColorAttachment7,
    ColorAttachment8,
    ColorAttachment9,
    ColorAttachment10,
    ColorAttachment11,
    ColorAttachment12,
    ColorAttachment13,
    ColorAttachment14,
    ColorAttachment15,
    DepthStencilAttachment,
    DepthAttachment,
    StencilAttachment,
    Error
};

[[nodiscard]] static inline constexpr FramebufferAttachmentTargetIndex toIndex(
    FramebufferAttachmentTarget target) noexcept {
    switch (target) {
        case FramebufferAttachmentTarget::Detached:
            return FramebufferAttachmentTargetIndex::Detached;
        case FramebufferAttachmentTarget::ColorAttachment0:
            return FramebufferAttachmentTargetIndex::ColorAttachment0;
        case FramebufferAttachmentTarget::ColorAttachment1:
            return FramebufferAttachmentTargetIndex::ColorAttachment1;
        case FramebufferAttachmentTarget::ColorAttachment2:
            return FramebufferAttachmentTargetIndex::ColorAttachment2;
        case FramebufferAttachmentTarget::ColorAttachment3:
            return FramebufferAttachmentTargetIndex::ColorAttachment3;
        case FramebufferAttachmentTarget::ColorAttachment4:
            return FramebufferAttachmentTargetIndex::ColorAttachment4;
        case FramebufferAttachmentTarget::ColorAttachment5:
            return FramebufferAttachmentTargetIndex::ColorAttachment5;
        case FramebufferAttachmentTarget::ColorAttachment6:
            return FramebufferAttachmentTargetIndex::ColorAttachment6;
        case FramebufferAttachmentTarget::ColorAttachment7:
            return FramebufferAttachmentTargetIndex::ColorAttachment7;
        case FramebufferAttachmentTarget::ColorAttachment8:
            return FramebufferAttachmentTargetIndex::ColorAttachment8;
        case FramebufferAttachmentTarget::ColorAttachment9:
            return FramebufferAttachmentTargetIndex::ColorAttachment9;
        case FramebufferAttachmentTarget::ColorAttachment10:
            return FramebufferAttachmentTargetIndex::ColorAttachment10;
        case FramebufferAttachmentTarget::ColorAttachment11:
            return FramebufferAttachmentTargetIndex::ColorAttachment11;
        case FramebufferAttachmentTarget::ColorAttachment12:
            return FramebufferAttachmentTargetIndex::ColorAttachment12;
        case FramebufferAttachmentTarget::ColorAttachment13:
            return FramebufferAttachmentTargetIndex::ColorAttachment13;
        case FramebufferAttachmentTarget::ColorAttachment14:
            return FramebufferAttachmentTargetIndex::ColorAttachment14;
        case FramebufferAttachmentTarget::ColorAttachment15:
            return FramebufferAttachmentTargetIndex::ColorAttachment15;
        case FramebufferAttachmentTarget::DepthStencilAttachment:
            return FramebufferAttachmentTargetIndex::DepthStencilAttachment;
        case FramebufferAttachmentTarget::DepthAttachment:
            return FramebufferAttachmentTargetIndex::DepthAttachment;
        case FramebufferAttachmentTarget::StencilAttachment:
            return FramebufferAttachmentTargetIndex::StencilAttachment;
        default:
            assert(false && "Caught unknown framebuffer attachment target.");
            return FramebufferAttachmentTargetIndex::Error;
    }
}

} // namespace snap::rhi::backend::opengl
