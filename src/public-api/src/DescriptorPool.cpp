#include "snap/rhi/DescriptorPool.hpp"

namespace snap::rhi {
DescriptorPool::DescriptorPool(Device* device, const DescriptorPoolCreateInfo& info)
    : snap::rhi::DeviceChild(device, snap::rhi::ResourceType::DescriptorPool), info(info) {}
} // namespace snap::rhi
