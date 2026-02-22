#include "snap/rhi/backend/noop/Sampler.hpp"

#include "snap/rhi/backend/noop/Device.hpp"

namespace snap::rhi::backend::noop {
Sampler::Sampler(snap::rhi::backend::common::Device* device, const snap::rhi::SamplerCreateInfo& info)
    : snap::rhi::Sampler(device, info) {}
} // namespace snap::rhi::backend::noop
