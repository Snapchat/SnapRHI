#include "snap/rhi/DescriptorSet.hpp"

namespace snap::rhi {
DescriptorSet::DescriptorSet(Device* device, const DescriptorSetCreateInfo& info)
    : snap::rhi::DeviceChild(device, snap::rhi::ResourceType::DescriptorSet), info(info) {}
} // namespace snap::rhi
