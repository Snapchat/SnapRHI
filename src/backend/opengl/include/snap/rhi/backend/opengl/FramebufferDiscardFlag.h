#pragma once

#include "snap/rhi/EnumOps.h"
#include <cassert>
#include <cstdint>

namespace snap::rhi::backend::opengl {

// FramebufferDiscardFlag defines bits and masks for framebuffer attachments discarding.
enum class FramebufferDiscardFlag : uint32_t {
    None = 0,

    ColorAttachment0 = 1 << 0,
    ColorAttachment1 = 1 << 1,
    ColorAttachment2 = 1 << 2,
    ColorAttachment3 = 1 << 3,
    ColorAttachment4 = 1 << 4,
    ColorAttachment5 = 1 << 5,
    ColorAttachment6 = 1 << 6,
    ColorAttachment7 = 1 << 7,
    ColorAttachment8 = 1 << 8,
    ColorAttachment9 = 1 << 9,
    ColorAttachment10 = 1 << 10,
    ColorAttachment11 = 1 << 11,
    ColorAttachment12 = 1 << 12,
    ColorAttachment13 = 1 << 13,
    ColorAttachment14 = 1 << 14,
    ColorAttachment15 = 1 << 15,

    DepthAttachment = 1 << 16,
    StencilAttachment = 1 << 17,

    AllColorAttachments = ColorAttachment0 | ColorAttachment1 | ColorAttachment2 | ColorAttachment3 | ColorAttachment4 |
                          ColorAttachment5 | ColorAttachment6 | ColorAttachment7 | ColorAttachment8 | ColorAttachment9 |
                          ColorAttachment10 | ColorAttachment11 | ColorAttachment12 | ColorAttachment13 |
                          ColorAttachment14 | ColorAttachment15,

    DepthStencilAttachments = DepthAttachment | StencilAttachment,
    AllAttachments = AllColorAttachments | DepthStencilAttachments,
};

SNAP_RHI_DEFINE_ENUM_OPS(FramebufferDiscardFlag);

constexpr FramebufferDiscardFlag getFramebufferDiscardColorAttachment(uint32_t index) {
    assert(index < 16);
    return static_cast<FramebufferDiscardFlag>(1 << index);
}
constexpr bool shouldDiscardFramebufferColorAttachment(FramebufferDiscardFlag discardFlags, uint32_t index) {
    return (discardFlags & getFramebufferDiscardColorAttachment(index)) == getFramebufferDiscardColorAttachment(index);
}
constexpr bool shouldDiscardFramebufferDepthAttachment(FramebufferDiscardFlag discardFlags) {
    return (discardFlags & FramebufferDiscardFlag::DepthAttachment) == FramebufferDiscardFlag::DepthAttachment;
}
constexpr bool shouldDiscardFramebufferStencilAttachment(FramebufferDiscardFlag discardFlags) {
    return (discardFlags & FramebufferDiscardFlag::StencilAttachment) == FramebufferDiscardFlag::StencilAttachment;
}

} // namespace snap::rhi::backend::opengl
