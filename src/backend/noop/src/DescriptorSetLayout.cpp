#include "snap/rhi/backend/noop/DescriptorSetLayout.hpp"
#include "snap/rhi/backend/noop/Device.hpp"

namespace snap::rhi::backend::noop {
DescriptorSetLayout::DescriptorSetLayout(snap::rhi::backend::common::Device* device,
                                         const DescriptorSetLayoutCreateInfo& info)
    : snap::rhi::DescriptorSetLayout(device, info) {}
} // namespace snap::rhi::backend::noop
