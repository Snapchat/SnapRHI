#include "snap/rhi/backend/metal/Fence.h"
#include "snap/rhi/backend/common/PlatformSyncHandle.h"
#include "snap/rhi/backend/metal/Device.h"

namespace snap::rhi::backend::metal {
Fence::Fence(Device* mtlDevice, const snap::rhi::FenceCreateInfo& info) : snap::rhi::Fence(mtlDevice, info) {}

snap::rhi::FenceStatus Fence::getStatus(uint64_t generationID) {
    std::lock_guard lock(accessMutex);
    if (generationID && this->generationID > generationID) {
        return snap::rhi::FenceStatus::Completed;
    }

    if (commandBuffer != nil) {
        result = snap::rhi::FenceStatus::NotReady;
        if (commandBuffer.status == MTLCommandBufferStatusCompleted) {
            commandBuffer = nil;
            result = snap::rhi::FenceStatus::Completed;
        }
    }

    return result;
}

void Fence::waitForComplete() {
    std::lock_guard lock(accessMutex);
    if (commandBuffer != nil && commandBuffer.status != MTLCommandBufferStatusCompleted) {
        [commandBuffer waitUntilCompleted];
    }
    commandBuffer = nil;
    result = snap::rhi::FenceStatus::Completed;
}

void Fence::waitForScheduled() {
    std::lock_guard lock(accessMutex);
    if (commandBuffer != nil && commandBuffer.status < MTLCommandBufferStatusScheduled) {
        [commandBuffer waitUntilScheduled];
    }
}

void Fence::reset() {
    std::lock_guard lock(accessMutex);
    result = snap::rhi::FenceStatus::NotReady;
    commandBuffer = nil;
    snap::rhi::Fence::reset();
}

void Fence::setWaitCommandBuffer(const id<MTLCommandBuffer>& commandBuffer) {
    std::lock_guard lock(accessMutex);
    this->commandBuffer = commandBuffer;
}

std::unique_ptr<snap::rhi::PlatformSyncHandle> Fence::exportPlatformSyncHandle() {
    std::lock_guard lock(accessMutex);
    auto* mtlDevice = snap::rhi::backend::common::smart_cast<snap::rhi::backend::metal::Device>(device);
    auto retainable = mtlDevice->resolveResource(this);
    auto sharedFence = std::static_pointer_cast<snap::rhi::Fence>(retainable);
    return std::make_unique<snap::rhi::backend::common::PlatformSyncHandle>(sharedFence);
}
} // namespace snap::rhi::backend::metal
