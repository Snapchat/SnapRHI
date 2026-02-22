#include "snap/rhi/DeviceContext.hpp"

namespace snap::rhi {
DeviceContext::DeviceContext(Device* device, const DeviceContextCreateInfo& info)
    : snap::rhi::DeviceChild(device, snap::rhi::ResourceType::DeviceContext), info(info) {}
} // namespace snap::rhi
