#include "snap/rhi/backend/noop/DescriptorPool.hpp"
#include "snap/rhi/backend/noop/Device.hpp"

namespace snap::rhi::backend::noop {
DescriptorPool::DescriptorPool(snap::rhi::backend::common::Device* device, const DescriptorPoolCreateInfo& info)
    : snap::rhi::DescriptorPool(device, info) {}
} // namespace snap::rhi::backend::noop
