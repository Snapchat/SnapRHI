#include "snap/rhi/backend/vulkan/CommandEncoder.hpp"
#include "snap/rhi/backend/vulkan/CommandBuffer.h"
#include "snap/rhi/backend/vulkan/Device.h"
#include "snap/rhi/backend/vulkan/QueryPool.h"
#include "snap/rhi/backend/vulkan/VkDebugUtils.h"

namespace snap::rhi::backend::vulkan {
template<typename CommandEncoderBase>
CommandEncoder<CommandEncoderBase>::CommandEncoder(snap::rhi::backend::vulkan::Device* vkDevice,
                                                   snap::rhi::backend::vulkan::CommandBuffer* commandBuffer)
    : snap::rhi::backend::common::CommandEncoder<CommandEncoderBase>(vkDevice, commandBuffer) {}

template<typename CommandEncoderBase>
void CommandEncoder<CommandEncoderBase>::writeTimestamp(snap::rhi::QueryPool* queryPool,
                                                        uint32_t query,
                                                        const snap::rhi::TimestampLocation location) {
    auto* vulkanCmdBuffer = snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::CommandBuffer>(
        CommandEncoderBase::commandBuffer);
    auto* vulkanQueryPool = snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::QueryPool>(queryPool);
    if (!vulkanCmdBuffer || !vulkanQueryPool) {
        return;
    }
    VkQueryPool vkQueryPool = vulkanQueryPool->getVkQueryPool();
    VkCommandBuffer vkCommandBuffer = vulkanCmdBuffer->getActiveCommandBuffer();
    if (location == snap::rhi::TimestampLocation::Start) {
        vkCmdWriteTimestamp(vkCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, vkQueryPool, query);
    } else {
        vkCmdWriteTimestamp(vkCommandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, vkQueryPool, query);
    }
}

template<typename CommandEncoderBase>
void CommandEncoder<CommandEncoderBase>::pipelineBarrier(
    snap::rhi::PipelineStageBits srcStageMask,
    snap::rhi::PipelineStageBits dstStageMask,
    snap::rhi::DependencyFlags dependencyFlags,
    std::span<snap::rhi::MemoryBarrierInfo> memoryBarriers,
    std::span<snap::rhi::BufferMemoryBarrierInfo> bufferMemoryBarriers,
    std::span<snap::rhi::TextureMemoryBarrierInfo> textureMemoryBarriers) {
    // TODO: Implement barriers
    for (const auto& textureMemoryBarrier : textureMemoryBarriers) {
        if (auto* cmdBufferBase = snap::rhi::backend::common::smart_cast<snap::rhi::backend::common::CommandBuffer>(
                CommandEncoderBase::commandBuffer)) {
            cmdBufferBase->tryPreserveInteropTexture(textureMemoryBarrier.texture);
        }
    }
}

template<typename CommandEncoderBase>
void CommandEncoder<CommandEncoderBase>::beginDebugGroup(std::string_view label) {
#if SNAP_RHI_ENABLE_DEBUG_LABELS
    auto* vulkanCmdBuffer = snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::CommandBuffer>(
        CommandEncoderBase::commandBuffer);
    if (vulkanCmdBuffer) {
        beginVkDebugLabel(vulkanCmdBuffer->getActiveCommandBuffer(), label.data());
    }
#endif
}

template<typename CommandEncoderBase>
void CommandEncoder<CommandEncoderBase>::endDebugGroup() {
#if SNAP_RHI_ENABLE_DEBUG_LABELS
    auto* vulkanCmdBuffer = snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::CommandBuffer>(
        CommandEncoderBase::commandBuffer);
    if (vulkanCmdBuffer) {
        endVkDebugLabel(vulkanCmdBuffer->getActiveCommandBuffer());
    }
#endif
}

template class CommandEncoder<snap::rhi::BlitCommandEncoder>;
template class CommandEncoder<snap::rhi::ComputeCommandEncoder>;
template class CommandEncoder<snap::rhi::RenderCommandEncoder>;
} // namespace snap::rhi::backend::vulkan
