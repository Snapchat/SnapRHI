#pragma once

#include "snap/rhi/Sampler.hpp"
#include "snap/rhi/SamplerCreateInfo.h"

namespace snap::rhi::backend::common {
class Device;
} // namespace snap::rhi::backend::common

namespace snap::rhi::backend::noop {
class Sampler final : public snap::rhi::Sampler {
public:
    explicit Sampler(snap::rhi::backend::common::Device* device, const snap::rhi::SamplerCreateInfo& info);
    ~Sampler() override = default;
};
} // namespace snap::rhi::backend::noop
