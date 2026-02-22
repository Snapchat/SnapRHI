#include "snap/rhi/backend/metal/CommandQueue.h"
#include "snap/rhi/backend/common/TextureInterop.h"
#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/metal/CommandBuffer.h"
#include "snap/rhi/backend/metal/Device.h"
#include "snap/rhi/backend/metal/Fence.h"
#include "snap/rhi/backend/metal/Semaphore.h"
#include "snap/rhi/common/Throw.h"
#include <future>

namespace {
std::shared_ptr<snap::rhi::Fence> buildFence(snap::rhi::Fence*& fence,
                                             snap::rhi::backend::metal::Device* device,
                                             std::span<snap::rhi::CommandBuffer*> buffers) {
    if (fence) {
        return nullptr;
    }

    bool needFence = false;
    for (auto* cmdBuffer : buffers) {
        if (SNAP_RHI_ENABLE_SLOW_VALIDATIONS() ||
            !static_cast<bool>(cmdBuffer->getCreateInfo().commandBufferCreateFlags &
                               snap::rhi::CommandBufferCreateFlags::UnretainedResources)) {
            needFence = true;
            break;
        }
    }

    if (!needFence) {
        return nullptr;
    }

    auto syncFenceRef = device->getFencePool().acquireFence();
    fence = syncFenceRef.get();
    return syncFenceRef;
}
} // unnamed namespace

namespace snap::rhi::backend::metal {
CommandQueue::CommandQueue(Device* mtlDevice, const id<MTLCommandQueue>& commandQueue)
    : snap::rhi::CommandQueue(mtlDevice), commandQueue(commandQueue) {}

CommandQueue::~CommandQueue() {
    waitIdle();
}

void CommandQueue::setDebugLabel(std::string_view label) {
#if SNAP_RHI_ENABLE_DEBUG_LABELS
    ObjectDebugMarkers::setDebugLabel(label);
    commandQueue.label = [NSString stringWithUTF8String:label.data()];
#endif
}

void CommandQueue::waitQueue(std::span<snap::rhi::Semaphore*> semaphores) {
    if (!semaphores.empty()) {
        id<MTLCommandBuffer> nativeCommandBuffer = [commandQueue commandBuffer];
        for (size_t i = 0; i < semaphores.size(); ++i) {
            Semaphore* semaphore = snap::rhi::backend::common::smart_cast<Semaphore>(semaphores[i]);
            semaphore->wait(nativeCommandBuffer);
        }

        [nativeCommandBuffer commit];
    }
}

void CommandQueue::signalQueue(std::span<snap::rhi::Semaphore*> semaphores, CommandBuffer* commandBuffer) {
    const auto& nativeCommandBuffer = commandBuffer->getContext().getCommandBuffer();
    for (size_t i = 0; i < semaphores.size(); ++i) {
        Semaphore* semaphore = snap::rhi::backend::common::smart_cast<Semaphore>(semaphores[i]);
        semaphore->signal(nativeCommandBuffer);
    }
}

void CommandQueue::submitCommands(std::span<snap::rhi::Semaphore*> waitSemaphores,
                                  std::span<const snap::rhi::PipelineStageBits> waitDstStageMask,
                                  std::span<snap::rhi::CommandBuffer*> buffers,
                                  std::span<snap::rhi::Semaphore*> signalSemaphores,
                                  snap::rhi::CommandBufferWaitType waitType,
                                  snap::rhi::Fence* fence) {
    assert(buffers.size());

    auto* mtlDevice = snap::rhi::backend::common::smart_cast<Device>(device);

    /**
     * Try to reclaim completed submissions to free up resources
     */
    auto& submissionTracker = mtlDevice->getSubmissionTracker();
    submissionTracker.tryReclaim();

    /**
     * For Metal, SnapRHI might ignore waitSemaphores.
     * Only fence will be used to track the last submitted command buffer.
     */
    [[maybe_unused]] auto textureInteropsSyncInfo = common::processTextureInterop(buffers, fence);
    auto syncFenceRef = buildFence(fence, mtlDevice, buffers);

    waitQueue(waitSemaphores);

    auto* commandBuffer =
        snap::rhi::backend::common::smart_cast<snap::rhi::backend::metal::CommandBuffer>(buffers.back());
    const id<MTLCommandBuffer>& lastCmdBuffer = commandBuffer->getContext().getCommandBuffer();

    for (auto* buffer : buffers) {
        auto* commandBuffer = snap::rhi::backend::common::smart_cast<CommandBuffer>(buffer);
        const auto& mtlCommandBuffer = commandBuffer->getContext().getCommandBuffer();

        [mtlCommandBuffer commit];
        commandBuffer->onSubmitted();
    }

    {
        std::unique_lock _(accessLock);
        lastSubmitCommandBuffer = lastCmdBuffer;
    }

    signalQueue(signalSemaphores, commandBuffer);

    if (fence) {
        auto* mtlFence = snap::rhi::backend::common::smart_cast<Fence>(fence);
        mtlFence->setWaitCommandBuffer(lastCmdBuffer);
    }

    for (auto* buffer : buffers) {
        const auto& commandBufferCreateInfo = buffer->getCreateInfo();
        const bool retainResources = SNAP_RHI_ENABLE_SLOW_VALIDATIONS() ||
                                     !static_cast<bool>(commandBufferCreateInfo.commandBufferCreateFlags &
                                                        snap::rhi::CommandBufferCreateFlags::UnretainedResources);
        if (retainResources) {
            submissionTracker.track(buffer, fence);
        }
    }
    wait(waitType);
}

void CommandQueue::wait(const snap::rhi::CommandBufferWaitType waitType) {
    if (waitType == snap::rhi::CommandBufferWaitType::DoNotWait) {
        return;
    }

    id<MTLCommandBuffer> lastCmdBuffer = nil;
    {
        std::unique_lock _(accessLock);
        lastCmdBuffer = lastSubmitCommandBuffer;
    }

    switch (waitType) {
        case snap::rhi::CommandBufferWaitType::WaitUntilCompleted: {
            if (lastCmdBuffer != nil && lastCmdBuffer.status < MTLCommandBufferStatusCompleted) {
                [lastCmdBuffer waitUntilCompleted];
            }
        } break;

        case snap::rhi::CommandBufferWaitType::WaitUntilScheduled: {
            if (lastCmdBuffer != nil && lastCmdBuffer.status < MTLCommandBufferStatusScheduled) {
                [lastCmdBuffer waitUntilScheduled];
            }
        } break;

        default:
            snap::rhi::common::throwException("[CommandQueue::wait] invalid waitType");
    }
}

void CommandQueue::waitUntilScheduled() {
    wait(snap::rhi::CommandBufferWaitType::WaitUntilScheduled);
}

void CommandQueue::waitIdle() {
    wait(snap::rhi::CommandBufferWaitType::WaitUntilCompleted);

    auto* mtlDevice = snap::rhi::backend::common::smart_cast<Device>(device);
    auto& submissionTracker = mtlDevice->getSubmissionTracker();
    submissionTracker.tryReclaim();
}

const id<MTLCommandQueue>& CommandQueue::getMtlCommandQueue() const {
    return commandQueue;
}
} // namespace snap::rhi::backend::metal
