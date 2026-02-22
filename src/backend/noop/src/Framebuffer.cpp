#include "snap/rhi/backend/noop/Framebuffer.hpp"

#include "snap/rhi/backend/noop/Device.hpp"
#include "snap/rhi/backend/noop/Texture.hpp"

#include <span>

namespace snap::rhi::backend::noop {
Framebuffer::Framebuffer(snap::rhi::backend::common::Device* device, const snap::rhi::FramebufferCreateInfo& info)
    : snap::rhi::Framebuffer(device, info) {}

} // namespace snap::rhi::backend::noop
