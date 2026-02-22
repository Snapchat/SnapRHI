#include "snap/rhi/backend/vulkan/DescriptorSetEncoder.h"
#include "snap/rhi/backend/common/CommandBuffer.h"
#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/vulkan/CommandBuffer.h"
#include "snap/rhi/backend/vulkan/DescriptorSet.h"
#include "snap/rhi/backend/vulkan/Device.h"
#include "snap/rhi/backend/vulkan/Texture.h"
#include "snap/rhi/backend/vulkan/Utils.hpp"

namespace {

} // unnamed namespace

namespace snap::rhi::backend::vulkan {
DescriptorSetEncoder::DescriptorSetEncoder(snap::rhi::backend::vulkan::Device* vkDevice,
                                           snap::rhi::backend::vulkan::CommandBuffer* commandBuffer,
                                           VkPipelineBindPoint pipelineBindingPoint)
    : pipelineBindingPoint(pipelineBindingPoint), cmdBuffer(commandBuffer) {}

DescriptorSetEncoder::~DescriptorSetEncoder() {}

void DescriptorSetEncoder::assignDescriptorSet(const uint32_t binding,
                                               snap::rhi::DescriptorSet* descriptorSet,
                                               std::span<const uint32_t> dynamicOffsets) {
    auto* pDescriptorSet =
        snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::DescriptorSet>(descriptorSet);
    auto* pDevice =
        snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::Device>(pDescriptorSet->getDevice());
    const snap::rhi::backend::common::ValidationLayer& validationLayer = pDevice->getValidationLayer();

    auto vkDescriptorSet = pDescriptorSet->getVkDescriptorSet();
    SNAP_RHI_VALIDATE(validationLayer,
                      binding < snap::rhi::SupportedLimit::MaxBoundDescriptorSets,
                      snap::rhi::ReportLevel::CriticalError,
                      snap::rhi::ValidationTag::DescriptorSetOp,
                      "[DescriptorSetEncoder::bindDescriptorSet] binding {%u} is out of range",
                      binding);
    boundDescriptorSets[binding] = vkDescriptorSet;
    activeDescriptorSets.set(binding);
    haveDescriptorSetsToBind = true;

    pipelineDynamicOffsets[binding].clear();
    pipelineDynamicOffsets[binding].insert(
        pipelineDynamicOffsets[binding].end(), dynamicOffsets.begin(), dynamicOffsets.end());

    /**
     * bindToCommandBuffer retains all resources in the command buffer
     */
    pDescriptorSet->bindToCommandBuffer(cmdBuffer);
}

void DescriptorSetEncoder::assignPipelineLayout(VkPipelineLayout layout, uint32_t count) {
    expectedDescriptorSetCount = count;
    pipelineLayout = layout;
}

void DescriptorSetEncoder::bindDescriptorSets() {
    if (!haveDescriptorSetsToBind) {
        return;
    }

    auto* pDevice = snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::Device>(cmdBuffer->getDevice());
    const snap::rhi::backend::common::ValidationLayer& validationLayer = pDevice->getValidationLayer();

    VkCommandBuffer vkCommandBuffer = cmdBuffer->getActiveCommandBuffer();
    SNAP_RHI_VALIDATE(validationLayer,
                      pipelineLayout != VK_NULL_HANDLE,
                      snap::rhi::ReportLevel::CriticalError,
                      snap::rhi::ValidationTag::CommandBufferOp,
                      "[DescriptorSetEncoder::bindDescriptorSets] PipelineLayout is not set");

    auto expectedSetsMask =
        std::bitset<snap::rhi::SupportedLimit::MaxBoundDescriptorSets>((1 << expectedDescriptorSetCount) - 1);
    auto setsMask = activeDescriptorSets & expectedSetsMask;
    /**
     * The Vulkan spec states: If the graphicsPipelineLibrary feature is not enabled,
     * each element of pDescriptorSets must be a valid VkDescriptorSet
     * (https://docs.vulkan.org/spec/latest/chapters/descriptorsets.html#VUID-vkCmdBindDescriptorSets-pDescriptorSets-06563)
     *
     * https://registry.khronos.org/vulkan/specs/latest/man/html/VkPipelineLayoutCreateFlagBits.html
     * VK_PIPELINE_LAYOUT_CREATE_INDEPENDENT_SETS_BIT_EXT specifies that implementations must ensure
     * that the properties and/or absence of a particular descriptor set do not influence
     * any other properties of the pipeline layout.
     * This allows pipelines libraries linked without VK_PIPELINE_CREATE_LINK_TIME_OPTIMIZATION_BIT_EXT
     * to be created with a subset of the total descriptor sets.
     *
     * So in order to avoid performance issues, we have to bind all valid descriptor sets at once
     * */
    if (setsMask == expectedSetsMask) {
        dynamicOffsetsCache.clear();
        for (int32_t i = 0; i < snap::rhi::SupportedLimit::MaxBoundDescriptorSets; ++i) {
            if (!pipelineDynamicOffsets[i].empty()) {
                dynamicOffsetsCache.insert(
                    dynamicOffsetsCache.end(), pipelineDynamicOffsets[i].begin(), pipelineDynamicOffsets[i].end());
            }
        }

        vkCmdBindDescriptorSets(vkCommandBuffer,
                                pipelineBindingPoint,
                                pipelineLayout,
                                0,
                                expectedDescriptorSetCount,
                                boundDescriptorSets.data(),
                                static_cast<uint32_t>(dynamicOffsetsCache.size()),
                                dynamicOffsetsCache.empty() ? nullptr : dynamicOffsetsCache.data());
    } else {
        for (int32_t i = 0; i < snap::rhi::SupportedLimit::MaxBoundDescriptorSets; ++i) {
            if (boundDescriptorSets[i] != VK_NULL_HANDLE) {
                vkCmdBindDescriptorSets(vkCommandBuffer,
                                        pipelineBindingPoint,
                                        pipelineLayout,
                                        i,
                                        1,
                                        boundDescriptorSets.data() + i,
                                        static_cast<uint32_t>(pipelineDynamicOffsets[i].size()),
                                        pipelineDynamicOffsets[i].empty() ? nullptr : pipelineDynamicOffsets[i].data());
            }
        }
    }

    haveDescriptorSetsToBind = false;
}

void DescriptorSetEncoder::reset() {
    pipelineLayout = VK_NULL_HANDLE;
    boundDescriptorSets.fill(VK_NULL_HANDLE);
    activeDescriptorSets.reset();
    haveDescriptorSetsToBind = false;
    for (auto& offsets : pipelineDynamicOffsets) {
        offsets.clear();
    }
}
} // namespace snap::rhi::backend::vulkan
