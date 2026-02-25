#include "snap/rhi/backend/vulkan/ComputeCommandEncoder.h"
#include "snap/rhi/backend/common/CommandBuffer.h"
#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/vulkan/Buffer.h"
#include "snap/rhi/backend/vulkan/CommandBuffer.h"
#include "snap/rhi/backend/vulkan/ComputePipeline.h"
#include "snap/rhi/backend/vulkan/DescriptorSet.h"
#include "snap/rhi/backend/vulkan/Device.h"
#include "snap/rhi/backend/vulkan/PipelineLayout.h"
#include "snap/rhi/backend/vulkan/Texture.h"
#include "snap/rhi/backend/vulkan/Utils.hpp"

namespace {

} // unnamed namespace

namespace snap::rhi::backend::vulkan {
ComputeCommandEncoder::ComputeCommandEncoder(snap::rhi::backend::vulkan::Device* vkDevice,
                                             snap::rhi::backend::vulkan::CommandBuffer* commandBuffer)
    : ComputeCommandEncoderBase(vkDevice, commandBuffer),
      DescriptorSetEncoder(vkDevice, commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE) {}

ComputeCommandEncoder::~ComputeCommandEncoder() {}

void ComputeCommandEncoder::beginEncoding() {
    ComputeCommandEncoderBase::onBeginEncoding();
    reset();
}

void ComputeCommandEncoder::bindDescriptorSet(uint32_t binding,
                                              snap::rhi::DescriptorSet* descriptorSet,
                                              std::span<const uint32_t> dynamicOffsets) {
    assignDescriptorSet(binding, descriptorSet, dynamicOffsets);
}

void ComputeCommandEncoder::bindComputePipeline(snap::rhi::ComputePipeline* pipeline) {
    auto* vkCmdBuffer =
        snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::CommandBuffer>(commandBuffer);

    VkCommandBuffer vkCommandBuffer = vkCmdBuffer->getActiveCommandBuffer();
    auto* vkPipeline = snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::ComputePipeline>(pipeline);
    assignPipelineLayout(vkPipeline->getPipelineLayout(), vkPipeline->getDescriptorSetCount());

    vkCmdBindPipeline(vkCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vkPipeline->getVkPipeline());
    {
        resourceResidencySet.track(vkPipeline);
    }
}

void ComputeCommandEncoder::dispatch(const uint32_t groupSizeX, const uint32_t groupSizeY, const uint32_t groupSizeZ) {
    auto* vkCmdBuffer =
        snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::CommandBuffer>(commandBuffer);

    VkCommandBuffer vkCommandBuffer = vkCmdBuffer->getActiveCommandBuffer();
    bindDescriptorSets();
    vkCmdDispatch(vkCommandBuffer, groupSizeX, groupSizeY, groupSizeZ);
}

void ComputeCommandEncoder::reset() {
    DescriptorSetEncoder::reset();
}

void ComputeCommandEncoder::endEncoding() {
    ComputeCommandEncoderBase::onEndEncoding();
    reset();
}
} // namespace snap::rhi::backend::vulkan
