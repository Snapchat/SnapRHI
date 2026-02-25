#pragma once

#include "snap/rhi/Sampler.hpp"
#include "snap/rhi/SamplerCreateInfo.h"
#include "snap/rhi/backend/vulkan/vulkan.h"

namespace snap::rhi::backend::vulkan {
class Device;

class Sampler final : public snap::rhi::Sampler {
public:
    explicit Sampler(Device* device, const snap::rhi::SamplerCreateInfo& info);
    ~Sampler() final;

    void setDebugLabel(std::string_view label) override;

    VkSampler getSampler() const {
        return sampler;
    }

private:
    VkDevice vkDevice = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
};
} // namespace snap::rhi::backend::vulkan
