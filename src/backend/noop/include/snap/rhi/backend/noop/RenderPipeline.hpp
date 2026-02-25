#pragma once

#include "snap/rhi/RenderPipeline.hpp"

namespace snap::rhi {
class ShaderModule;
class PipelineCache;
} // namespace snap::rhi

namespace snap::rhi::backend::common {
class Device;
} // namespace snap::rhi::backend::common

namespace snap::rhi::backend::noop {
class RenderPipeline final : public snap::rhi::RenderPipeline {
public:
    RenderPipeline(snap::rhi::backend::common::Device* device, const snap::rhi::RenderPipelineCreateInfo& info);
};
} // namespace snap::rhi::backend::noop
