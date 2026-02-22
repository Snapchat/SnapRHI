#include "snap/rhi/ComputePipeline.hpp"

namespace snap::rhi {
ComputePipeline::ComputePipeline(Device* device, const ComputePipelineCreateInfo& info)
    : snap::rhi::DeviceChild(device, snap::rhi::ResourceType::ComputePipeline), info(info) {}

const std::optional<reflection::ComputePipelineInfo>& ComputePipeline::getReflectionInfo() const {
    return reflection;
}
} // namespace snap::rhi
