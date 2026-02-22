#include "snap/rhi/PipelineLayout.hpp"

namespace snap::rhi {
PipelineLayout::PipelineLayout(Device* device, const PipelineLayoutCreateInfo& info)
    : snap::rhi::DeviceChild(device, snap::rhi::ResourceType::PipelineLayout), info(info) {}
} // namespace snap::rhi
