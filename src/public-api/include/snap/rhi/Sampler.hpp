//
//  Sampler.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 23.09.2020.
//  Copyright © 2021 Snapchat. All rights reserved.
//

#pragma once

#include "snap/rhi/DeviceChild.hpp"
#include "snap/rhi/SamplerCreateInfo.h"

namespace snap::rhi {
class Device;

/**
 * @brief Immutable texture sampling state.
 *
 * A Sampler encodes how a texture is sampled: min/mag/mip filters, address modes (wrap/clamp), LOD range, comparison
 * sampling, anisotropy, and border color.
 *
 * Sampler objects are created by `snap::rhi::Device::createSampler()` and are safe to reuse across multiple textures
 * and pipelines.
 *
 * Backend notes:
 * - Vulkan and Metal typically create a native sampler object.
 * - OpenGL may create a native sampler object when supported, otherwise it may apply equivalent state using texture
 *   parameters when binding.
 */
class Sampler : public snap::rhi::DeviceChild {
public:
    Sampler(Device* device, const SamplerCreateInfo& info);
    ~Sampler() override = default;

    /**
     * @brief Returns the sampler creation parameters.
     */
    const SamplerCreateInfo& getCreateInfo() const {
        return info;
    }

    /**
     * @brief Returns an estimate of CPU-side memory used by this object.
     */
    uint64_t getCPUMemoryUsage() const override {
        return 0;
    }

    /**
     * @brief Returns an estimate of GPU-side memory used by this object.
     */
    uint64_t getGPUMemoryUsage() const override {
        return 0;
    }

protected:
    const SamplerCreateInfo info;

private:
};
} // namespace snap::rhi
