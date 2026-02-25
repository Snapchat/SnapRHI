#pragma once

#include "snap/rhi/ComputePipeline.hpp"

namespace snap::rhi::backend::common {
class Device;
} // namespace snap::rhi::backend::common

namespace snap::rhi::backend::noop {
class ComputePipeline final : public snap::rhi::ComputePipeline {
public:
    explicit ComputePipeline(snap::rhi::backend::common::Device* device,
                             const snap::rhi::ComputePipelineCreateInfo& info);
    ~ComputePipeline() override = default;
};
} // namespace snap::rhi::backend::noop
