//
//  Sampler.h
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 1/17/20.
//  Copyright © 2020 Snapchat. All rights reserved.
//

#pragma once

#include "snap/rhi/Sampler.hpp"
#include "snap/rhi/SamplerCreateInfo.h"
#include <Metal/Metal.h>
#include <cstdint>

namespace snap::rhi::backend::metal {
class Device;

class Sampler final : public snap::rhi::Sampler {
public:
    explicit Sampler(Device* mtlDevice, const snap::rhi::SamplerCreateInfo& info);
    ~Sampler() override = default;

    [[nodiscard]] uint64_t getGPUResourceID() const {
        return gpuResourceID;
    }

    const id<MTLSamplerState>& getSampler() const;

private:
    uint64_t gpuResourceID = 0;
    id<MTLSamplerState> sampler = nil;
};
} // namespace snap::rhi::backend::metal
