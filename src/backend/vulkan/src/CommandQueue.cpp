#include "snap/rhi/backend/vulkan/CommandQueue.h"
#include "snap/rhi/TextureInterop.h"
#include "snap/rhi/backend/common/TextureInterop.h"
#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/vulkan/CommandBuffer.h"
#include "snap/rhi/backend/vulkan/Device.h"
#include "snap/rhi/backend/vulkan/Fence.h"
#include "snap/rhi/backend/vulkan/ImageWithMemory.h"
#include "snap/rhi/backend/vulkan/Semaphore.h"
#include "snap/rhi/backend/vulkan/Texture.h"
#include "snap/rhi/backend/vulkan/Utils.hpp"
#include <algorithm>
#include <ranges>
#include <snap/rhi/common/Scope.h>

namespace {
void transferInitialImageLayout(VkCommandBuffer commandBuffer, snap::rhi::backend::vulkan::Texture* texture) {
    VkImageMemoryBarrier imageMemoryBarrier{};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

    imageMemoryBarrier.srcAccessMask = 0;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    imageMemoryBarrier.oldLayout = texture->getInitialImageLayout();
    imageMemoryBarrier.newLayout = texture->getDefaultImageLayout();

    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    imageMemoryBarrier.image = texture->getImage();

    const auto& createInfo = texture->getCreateInfo();

    imageMemoryBarrier.subresourceRange = {
        .aspectMask = snap::rhi::backend::vulkan::getImageAspectFlags(createInfo.format),
        .baseMipLevel = 0,
        .levelCount = createInfo.mipLevels,
        .baseArrayLayer = 0,
        // Use getArraySize since for 2D textures layers have to be handled separately (e.g. cubemap should return 6 *
        // depth) while 3D texture must only have 1 layer
        .layerCount = snap::rhi::backend::vulkan::getArraySize(createInfo.textureType, createInfo.size)};

    vkCmdPipelineBarrier(commandBuffer,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         0,
                         0,
                         nullptr,
                         0,
                         nullptr,
                         1,
                         &imageMemoryBarrier);
}

void insertSemaphores(const auto& container, std::vector<VkSemaphore>& result) {
    const auto casted =
        container | std::views::transform([](const auto& semaphorePtr) {
            if constexpr (std::is_pointer_v<std::decay_t<decltype(semaphorePtr)>>) {
                return snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::Semaphore>(semaphorePtr);
            } else {
                return snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::Semaphore>(
                    semaphorePtr.get());
            }
        });

    const auto offset = static_cast<int32_t>(result.size());
    result.resize(offset + container.size());

    auto toVkHandle = [](snap::rhi::backend::vulkan::Semaphore* s) noexcept -> VkSemaphore {
        return s ? s->getSemaphore() : VK_NULL_HANDLE;
    };
    std::ranges::copy(casted | std::views::transform(toVkHandle), result.begin() + offset);
}

void preserveSemaphoreRefs(const auto& container, snap::rhi::CommandBuffer* commandBuffer) {
    auto* implCommandBuffer =
        snap::rhi::backend::common::smart_cast<snap::rhi::backend::common::CommandBuffer>(commandBuffer);
    auto& resourceResidencySet = implCommandBuffer->getResourceResidencySet();

    for (const auto& resource : container) {
        if constexpr (std::is_pointer_v<std::decay_t<decltype(resource)>>) {
            resourceResidencySet.track(resource);
        } else {
            resourceResidencySet.track(resource.get());
        }
    }
}

std::shared_ptr<snap::rhi::Fence> buildFence(snap::rhi::Fence*& fence,
                                             snap::rhi::backend::vulkan::Device* vkDevice,
                                             std::span<snap::rhi::CommandBuffer*> buffers) {
    if (fence) {
        return nullptr;
    }

    bool needFence = false;
    for (auto* cmdBuffer : buffers) {
        auto* vkCmdBuffer =
            snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::CommandBuffer>(cmdBuffer);
        if (SNAP_RHI_ENABLE_SLOW_VALIDATIONS() ||
            !static_cast<bool>(vkCmdBuffer->getCreateInfo().commandBufferCreateFlags &
                               snap::rhi::CommandBufferCreateFlags::UnretainedResources) ||
            !vkCmdBuffer->getResourcesInitInfo().textures.empty()) {
            needFence = true;
            break;
        }
    }

    if (!needFence) {
        return nullptr;
    }

    auto syncFenceRef = vkDevice->getFencePool().acquireFence();
    fence = syncFenceRef.get();
    return syncFenceRef;
}

std::shared_ptr<snap::rhi::CommandBuffer> processTextureToInit(
    snap::rhi::backend::vulkan::Device* device,
    snap::rhi::backend::vulkan::CommandQueue* commandQueue,
    const std::vector<snap::rhi::backend::vulkan::Texture*>& texturesToInit,
    snap::rhi::backend::common::SubmissionTracker& submissionTracker,
    snap::rhi::Fence* fence,
    std::vector<VkCommandBuffer>& commandBuffers) {
    if (texturesToInit.empty()) {
        return nullptr;
    }

    std::shared_ptr<snap::rhi::CommandBuffer> resourcesInitCommandBuffer = device->createCommandBuffer(
        {.commandBufferCreateFlags = snap::rhi::CommandBufferCreateFlags::UnretainedResources,
         .commandQueue = commandQueue});

    const auto nativeCommandBuffer = snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::CommandBuffer>(
        resourcesInitCommandBuffer.get());
    commandBuffers[0] = nativeCommandBuffer->getSyncCommandBuffer();
    for (auto* texture : texturesToInit) {
        transferInitialImageLayout(commandBuffers[0], texture);
    }
    nativeCommandBuffer->finishRecording();

    return resourcesInitCommandBuffer;
}

auto processCommandBuffers(snap::rhi::backend::vulkan::Device* device,
                           snap::rhi::backend::vulkan::CommandQueue* commandQueue,
                           std::span<snap::rhi::CommandBuffer*> buffers,
                           snap::rhi::backend::common::SubmissionTracker& submissionTracker,
                           snap::rhi::Fence* fence) {
    std::vector<snap::rhi::backend::vulkan::Texture*> texturesToInit;
    for (auto* cmdBuffer : buffers) {
        auto* commandBuffer =
            snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::CommandBuffer>(cmdBuffer);
        std::ranges::copy(commandBuffer->getResourcesInitInfo().textures | std::views::filter([](const auto& texture) {
                              return texture->shouldUpdateLayoutToDefault();
                          }),
                          std::back_inserter(texturesToInit));
    }
    std::ranges::sort(texturesToInit);
    texturesToInit.resize(std::unique(texturesToInit.begin(), texturesToInit.end()) - texturesToInit.begin());

    std::vector<VkCommandBuffer> commandBuffers(
        !texturesToInit.empty()); // Preallocate for resource init command buffer
    for (auto* cmdBuffer : buffers) {
        auto* commandBuffer =
            snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::CommandBuffer>(cmdBuffer);
        std::ranges::copy(commandBuffer->getCommandBuffers(), std::back_inserter(commandBuffers));
        commandBuffer->finishRecording();
    }

    std::shared_ptr<snap::rhi::CommandBuffer> resourcesInitCommandBuffer =
        processTextureToInit(device, commandQueue, texturesToInit, submissionTracker, fence, commandBuffers);
    return std::make_pair(commandBuffers, resourcesInitCommandBuffer);
}

std::vector<VkSemaphore> processWaitSemaphores(
    std::span<snap::rhi::Semaphore*> waitSemaphores,
    std::span<std::shared_ptr<snap::rhi::Semaphore>> interopWaitSemaphores,
    std::span<snap::rhi::CommandBuffer*> buffers,
    const std::shared_ptr<snap::rhi::CommandBuffer>& resourcesInitCommandBuffer) {
    std::vector<VkSemaphore> waitSemaphoresNative;
    snap::rhi::CommandBuffer* waitSemaphoresCommandBuffer =
        resourcesInitCommandBuffer ? resourcesInitCommandBuffer.get() : buffers[0];

    insertSemaphores(waitSemaphores, waitSemaphoresNative);
    preserveSemaphoreRefs(waitSemaphores, waitSemaphoresCommandBuffer);

    insertSemaphores(interopWaitSemaphores, waitSemaphoresNative);
    preserveSemaphoreRefs(interopWaitSemaphores, waitSemaphoresCommandBuffer);

    return waitSemaphoresNative;
}

std::vector<VkSemaphore> processSignalSemaphores(std::span<snap::rhi::Semaphore*> signalSemaphores,
                                                 std::span<snap::rhi::CommandBuffer*> buffers) {
    std::vector<VkSemaphore> signalSemaphoresNative;
    snap::rhi::CommandBuffer* signalSemaphoresCommandBuffer = buffers.back();

    insertSemaphores(signalSemaphores, signalSemaphoresNative);
    preserveSemaphoreRefs(signalSemaphores, signalSemaphoresCommandBuffer);

    return signalSemaphoresNative;
}

std::vector<VkPipelineStageFlags> processDstStageMasks(
    std::span<const snap::rhi::PipelineStageBits> waitDstStageMask,
    std::span<std::shared_ptr<snap::rhi::Semaphore>> interopWaitSemaphores) {
    std::vector<VkPipelineStageFlags> waitDstStageMaskNative(waitDstStageMask.size() + interopWaitSemaphores.size());

    std::ranges::copy(waitDstStageMask | std::views::transform(snap::rhi::backend::vulkan::toVkPipelineStageFlags),
                      waitDstStageMaskNative.begin());
    std::fill(waitDstStageMaskNative.begin() + static_cast<int32_t>(waitDstStageMask.size()),
              waitDstStageMaskNative.end(),
              VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);

    return waitDstStageMaskNative;
}
} // unnamed namespace

namespace snap::rhi::backend::vulkan {
CommandQueue::CommandQueue(snap::rhi::backend::vulkan::Device* vkDevice,
                           const uint32_t queueFamilyIndex,
                           const uint32_t queueIndex)
    : snap::rhi::CommandQueue(vkDevice), commandPool(vkDevice, queueFamilyIndex) {
    vkGetDeviceQueue(vkDevice->getVkLogicalDevice(), queueFamilyIndex, queueIndex, &queue);
    assert(queue);
}

CommandQueue::~CommandQueue() {
    waitIdle();
}

void CommandQueue::submitCommands(std::span<snap::rhi::Semaphore*> waitSemaphores,
                                  std::span<const snap::rhi::PipelineStageBits> waitDstStageMask,
                                  std::span<snap::rhi::CommandBuffer*> buffers,
                                  std::span<snap::rhi::Semaphore*> signalSemaphores,
                                  snap::rhi::CommandBufferWaitType waitType,
                                  snap::rhi::Fence* fence) {
    auto* vkDevice = snap::rhi::backend::common::smart_cast<Device>(device);
    const auto& validationLayer = vkDevice->getValidationLayer();
    auto& submissionTracker = vkDevice->getSubmissionTracker();

    /**
     * Try to reclaim completed submissions to free up resources
     */
    submissionTracker.tryReclaim();

    if (!buffers.empty()) {
        auto [interopSignalFence, interopWaitSemaphores] = common::processTextureInterop(buffers, fence);
        auto syncFenceRef = buildFence(fence, vkDevice, buffers);
        auto [commandBuffers, resourcesInitCommandBuffer] =
            processCommandBuffers(vkDevice, this, buffers, submissionTracker, fence);
        auto waitSemaphoresNative =
            processWaitSemaphores(waitSemaphores, interopWaitSemaphores, buffers, resourcesInitCommandBuffer);
        auto signalSemaphoresNative = processSignalSemaphores(signalSemaphores, buffers);
        auto waitDstStageMaskNative = processDstStageMasks(waitDstStageMask, interopWaitSemaphores);

        SNAP_RHI_ON_SCOPE_EXIT {
            for (auto* cmdBuffer : buffers) {
                auto* commandBuffer =
                    snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::CommandBuffer>(cmdBuffer);
                if (SNAP_RHI_ENABLE_SLOW_VALIDATIONS() ||
                    !static_cast<bool>(commandBuffer->getCreateInfo().commandBufferCreateFlags &
                                       snap::rhi::CommandBufferCreateFlags::UnretainedResources)) {
                    submissionTracker.track(commandBuffer, fence);
                }
            }

            if (resourcesInitCommandBuffer) {
                submissionTracker.track(resourcesInitCommandBuffer.get(), fence);
            }
        };

        const VkSubmitInfo submitInfo{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = nullptr,
            .waitSemaphoreCount = static_cast<uint32_t>(waitSemaphoresNative.size()),
            .pWaitSemaphores = waitSemaphoresNative.empty() ? nullptr : waitSemaphoresNative.data(),
            .pWaitDstStageMask = waitDstStageMaskNative.empty() ? nullptr : waitDstStageMaskNative.data(),
            .commandBufferCount = static_cast<uint32_t>(commandBuffers.size()),
            .pCommandBuffers = commandBuffers.empty() ? nullptr : commandBuffers.data(),
            .signalSemaphoreCount = static_cast<uint32_t>(signalSemaphoresNative.size()),
            .pSignalSemaphores = signalSemaphoresNative.empty() ? nullptr : signalSemaphoresNative.data(),
        };

        auto syncFence = fence ? snap::rhi::backend::common::smart_cast<Fence>(fence)->getFence() : VK_NULL_HANDLE;
        const VkResult result = vkQueueSubmit(queue, 1, &submitInfo, syncFence);
        SNAP_RHI_VALIDATE(validationLayer,
                          result == VK_SUCCESS,
                          snap::rhi::ReportLevel::CriticalError,
                          snap::rhi::ValidationTag::CommandQueueOp,
                          "[Vulkan::CommandQueue] Cannot submit command buffer to queue");
    }

    if (waitType == snap::rhi::CommandBufferWaitType::WaitUntilCompleted) {
        waitIdle();
    }
}

void CommandQueue::waitUntilScheduled() {
    // Vulkan schedules the execution of a command buffer when call vkQueueSubmit
}

void CommandQueue::waitIdle() {
    vkQueueWaitIdle(queue);

    auto* vkDevice = snap::rhi::backend::common::smart_cast<Device>(device);
    auto& submissionTracker = vkDevice->getSubmissionTracker();
    submissionTracker.tryReclaim();
}
} // namespace snap::rhi::backend::vulkan
