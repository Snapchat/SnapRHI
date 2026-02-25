#include "snap/rhi/backend/opengl/Framebuffer.hpp"
#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/opengl/Device.hpp"
#include "snap/rhi/backend/opengl/Texture.hpp"

namespace snap::rhi::backend::opengl {
Framebuffer::Framebuffer(Device* glesDevice, const snap::rhi::FramebufferCreateInfo& info)
    : snap::rhi::Framebuffer(glesDevice, info) {}
} // namespace snap::rhi::backend::opengl
