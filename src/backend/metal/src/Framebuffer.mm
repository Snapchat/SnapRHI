#include "snap/rhi/backend/metal/Framebuffer.h"
#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/metal/Device.h"
#include "snap/rhi/backend/metal/Texture.h"
#include "snap/rhi/common/Throw.h"

namespace snap::rhi::backend::metal {
Framebuffer::Framebuffer(Device* mtlDevice, const snap::rhi::FramebufferCreateInfo& info)
    : snap::rhi::Framebuffer(mtlDevice, info) {
    const auto& renderPassInfo = info.renderPass->getCreateInfo();
    if (renderPassInfo.subpassCount > 1) {
        snap::rhi::common::throwException("[SnapRHI Metal] support only 1 subpasss");
    }
    doesFramebufferHasDepthStencilAttachment =
        renderPassInfo.subpasses[0].depthStencilAttachment.attachment != snap::rhi::AttachmentUnused;
}

bool Framebuffer::hasDepthStencilAttachment() const {
    return doesFramebufferHasDepthStencilAttachment;
}
} // namespace snap::rhi::backend::metal
