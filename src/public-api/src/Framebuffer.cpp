#include "snap/rhi/Framebuffer.hpp"

namespace snap::rhi {
Framebuffer::Framebuffer(Device* device, const FramebufferCreateInfo& info)
    : snap::rhi::DeviceChild(device, snap::rhi::ResourceType::Framebuffer), info(info) {}
} // namespace snap::rhi
