#pragma once

#include "snap/rhi/RenderPipeline.hpp"
#include "snap/rhi/backend/common/ResourceResidencySet.h"
#include "snap/rhi/backend/common/ValidationLayer.hpp"
#include "snap/rhi/backend/vulkan/vulkan.h"

namespace snap::rhi::backend::vulkan {
class Device;

class RenderPipeline final : public snap::rhi::RenderPipeline {
public:
    RenderPipeline(snap::rhi::backend::vulkan::Device* device, const snap::rhi::RenderPipelineCreateInfo& info);
    ~RenderPipeline() final;

    void setDebugLabel(std::string_view label) override;

    VkPipeline getVkPipeline() const {
        return pipeline;
    }

    VkPipelineLayout getPipelineLayout() const {
        return pipelineLayout;
    }

    uint32_t getDescriptorSetCount() const {
        return descriptorSetCount;
    }

private:
    const snap::rhi::backend::common::ValidationLayer& validationLayer;

    VkDevice vkDevice = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    uint32_t descriptorSetCount = 0;

    snap::rhi::backend::common::ResourceResidencySet resourceResidencySet;
};
} // namespace snap::rhi::backend::vulkan
