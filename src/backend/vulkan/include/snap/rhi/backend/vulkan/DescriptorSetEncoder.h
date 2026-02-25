#pragma once

#include "snap/rhi/ComputeCommandEncoder.hpp"
#include "snap/rhi/Limits.h"
#include "snap/rhi/backend/common/CommandEncoder.h"
#include "snap/rhi/backend/common/ResourceResidencySet.h"
#include "snap/rhi/backend/common/ValidationLayer.hpp"
#include "snap/rhi/backend/vulkan/vulkan.h"
#include <bitset>

namespace snap::rhi::backend::vulkan {
class Device;
class CommandBuffer;

class DescriptorSetEncoder {
public:
    DescriptorSetEncoder(snap::rhi::backend::vulkan::Device* vkDevice,
                         snap::rhi::backend::vulkan::CommandBuffer* commandBuffer,
                         VkPipelineBindPoint pipelineBindingPoint);
    virtual ~DescriptorSetEncoder();

    void assignDescriptorSet(const uint32_t binding,
                             snap::rhi::DescriptorSet* descriptorSet,
                             std::span<const uint32_t> dynamicOffsets);
    void assignPipelineLayout(VkPipelineLayout pipelineLayout, uint32_t expectedDescriptorSetCount);
    void bindDescriptorSets();
    void reset();

private:
    const VkPipelineBindPoint pipelineBindingPoint;

    snap::rhi::backend::vulkan::CommandBuffer* cmdBuffer = nullptr;

    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    uint32_t expectedDescriptorSetCount = 0;

    std::array<std::vector<uint32_t>, snap::rhi::SupportedLimit::MaxBoundDescriptorSets> pipelineDynamicOffsets{};
    std::vector<uint32_t> dynamicOffsetsCache;
    std::array<VkDescriptorSet, snap::rhi::SupportedLimit::MaxBoundDescriptorSets> boundDescriptorSets{};
    std::bitset<snap::rhi::SupportedLimit::MaxBoundDescriptorSets> activeDescriptorSets;
    bool haveDescriptorSetsToBind = false;
};
} // namespace snap::rhi::backend::vulkan
