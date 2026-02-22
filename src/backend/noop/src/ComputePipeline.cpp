#include "snap/rhi/backend/noop/ComputePipeline.hpp"

#include "snap/rhi/backend/noop/Device.hpp"
#include "snap/rhi/backend/noop/ShaderModule.hpp"

namespace snap::rhi::backend::noop {
ComputePipeline::ComputePipeline(snap::rhi::backend::common::Device* device,
                                 const snap::rhi::ComputePipelineCreateInfo& info)
    : snap::rhi::ComputePipeline(device, info) {}
} // namespace snap::rhi::backend::noop
