#include "snap/rhi/RenderPass.hpp"

namespace snap::rhi {
RenderPass::RenderPass(Device* device, const RenderPassCreateInfo& info)
    : snap::rhi::DeviceChild(device, snap::rhi::ResourceType::RenderPass), info(info) {}
} // namespace snap::rhi
