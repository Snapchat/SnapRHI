#include "snap/rhi/Sampler.hpp"

namespace snap::rhi {
Sampler::Sampler(Device* device, const SamplerCreateInfo& info)
    : snap::rhi::DeviceChild(device, snap::rhi::ResourceType::Sampler), info(info) {}
} // namespace snap::rhi
