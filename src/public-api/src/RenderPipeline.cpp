#include "snap/rhi/RenderPipeline.hpp"

namespace snap::rhi {
RenderPipeline::RenderPipeline(Device* device, const RenderPipelineCreateInfo& info)
    : snap::rhi::DeviceChild(device, snap::rhi::ResourceType::RenderPipeline), info(info) {}

const std::optional<reflection::RenderPipelineInfo>& RenderPipeline::getReflectionInfo() const {
    return reflection;
}
} // namespace snap::rhi
