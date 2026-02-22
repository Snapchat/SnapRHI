#include "snap/rhi/backend/noop/RenderPipeline.hpp"

#include "snap/rhi/backend/noop/Device.hpp"

namespace snap::rhi::backend::noop {
RenderPipeline::RenderPipeline(snap::rhi::backend::common::Device* device,
                               const snap::rhi::RenderPipelineCreateInfo& info)
    : snap::rhi::RenderPipeline(device, info) {}
} // namespace snap::rhi::backend::noop
