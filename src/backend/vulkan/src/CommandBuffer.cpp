#include "snap/rhi/backend/vulkan/CommandBuffer.h"
#include "snap/rhi/backend/vulkan/CommandQueue.h"
#include "snap/rhi/backend/vulkan/Device.h"
#include "snap/rhi/backend/vulkan/QueryPool.h"
#include "snap/rhi/backend/vulkan/VkDebugUtils.h"

namespace {
VkCommandBuffer allocatePrimaryCommandBuffer(VkDevice vkDevice,
                                             VkCommandPool pool,
                                             const snap::rhi::backend::common::ValidationLayer& validationLayer) {
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

    VkCommandBufferAllocateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    createInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    createInfo.commandPool = pool;
    createInfo.commandBufferCount = 1;

    VkResult result = vkAllocateCommandBuffers(vkDevice, &createInfo, &commandBuffer);
    SNAP_RHI_VALIDATE(validationLayer,
                      result == VK_SUCCESS,
                      snap::rhi::ReportLevel::CriticalError,
                      snap::rhi::ValidationTag::CreateOp,
                      "[CommandBuffer] Failed to allocate command buffer, error: %d",
                      result);

    return commandBuffer;
}

VkCommandBuffer prepareCommandBuffer(VkDevice vkDevice,
                                     VkCommandPool pool,
                                     const snap::rhi::backend::common::ValidationLayer& validationLayer) {
    auto commandBuffer = allocatePrimaryCommandBuffer(vkDevice, pool, validationLayer);

    VkCommandBufferBeginInfo commandBufferBeginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);

    return commandBuffer;
}
} // unnamed namespace

namespace snap::rhi::backend::vulkan {
CommandBuffer::CommandBuffer(snap::rhi::backend::vulkan::Device* device, const snap::rhi::CommandBufferCreateInfo& info)
    : snap::rhi::backend::common::CommandBuffer(device, info),
      validationLayer(device->getValidationLayer()),
      vkDevice(device->getVkLogicalDevice()),
      pool(snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::CommandQueue>(info.commandQueue)
               ->allocateCommandBuffer()),
      imageLayoutManager(device, this),
      blitCommandEncoder(device, this),
      computeCommandEncoder(device, this),
      renderCommandEncoder(device, this) {
    commandBuffers.emplace_back(prepareCommandBuffer(vkDevice, *pool, validationLayer));
    activeCommandBufferIndex = syncCommandBufferIndex = 0;
}

CommandBuffer::~CommandBuffer() {
    if (!commandBuffers.empty() && pool) {
        vkFreeCommandBuffers(vkDevice, *pool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
        commandBuffers.clear();
    }
}

void CommandBuffer::resetQueryPool(snap::rhi::QueryPool* queryPool, uint32_t firstQuery, uint32_t queryCount) {
    vulkan::QueryPool* vulkanQueryPool =
        snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::QueryPool>(queryPool);
    if (!vulkanQueryPool) {
        return;
    }
    VkQueryPool vkQueryPool = vulkanQueryPool->getVkQueryPool();
    VkCommandBuffer vkCommandBuffer = getActiveCommandBuffer();
    vkCmdResetQueryPool(vkCommandBuffer, vkQueryPool, firstQuery, queryCount);
}

void CommandBuffer::beginRenderPass() {
    { // Sync command buffer update
        if (syncCommandBufferIndex != activeCommandBufferIndex) {
            vkEndCommandBuffer(commandBuffers[syncCommandBufferIndex]);
        }
        syncCommandBufferIndex = activeCommandBufferIndex;
    }

    { // Active command buffer update
        ++activeCommandBufferIndex;
        commandBuffers.emplace_back(prepareCommandBuffer(vkDevice, *pool, validationLayer));
    }
}

void CommandBuffer::endRenderPass() {
    vkEndCommandBuffer(commandBuffers[syncCommandBufferIndex]);
    syncCommandBufferIndex = activeCommandBufferIndex;
}

snap::rhi::RenderCommandEncoder* CommandBuffer::getRenderCommandEncoder() {
    return &renderCommandEncoder;
}

snap::rhi::BlitCommandEncoder* CommandBuffer::getBlitCommandEncoder() {
    return &blitCommandEncoder;
}

snap::rhi::ComputeCommandEncoder* CommandBuffer::getComputeCommandEncoder() {
    return &computeCommandEncoder;
}

void CommandBuffer::finishRecording() {
    imageLayoutManager.transferImagesIntoDefaultLayout();
    vkEndCommandBuffer(commandBuffers[activeCommandBufferIndex]);
    snap::rhi::backend::common::CommandBuffer::onSubmitted(validationLayer);
}

void CommandBuffer::setDebugLabel(std::string_view label) {
#if SNAP_RHI_ENABLE_DEBUG_LABELS
    ObjectDebugMarkers::setDebugLabel(label);
    setVkObjectDebugName(vkDevice,
                         VK_OBJECT_TYPE_COMMAND_BUFFER,
                         reinterpret_cast<uint64_t>(commandBuffers[activeCommandBufferIndex]),
                         label.data());
#endif
}
} // namespace snap::rhi::backend::vulkan
