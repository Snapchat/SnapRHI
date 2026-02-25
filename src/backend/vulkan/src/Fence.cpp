#include "snap/rhi/backend/vulkan/Fence.h"
#include "snap/rhi/backend/common/PlatformSyncHandle.h"
#include "snap/rhi/backend/vulkan/Device.h"
#include "snap/rhi/backend/vulkan/Utils.hpp"
#include "snap/rhi/backend/vulkan/VkDebugUtils.h"

#if SNAP_RHI_OS_ANDROID()
#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/common/platform/android/SyncHandle.h"
#include <unistd.h>
#endif

namespace {
constexpr VkFenceCreateInfo DefaultFenceCreateInfo{
    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
};
} // unnamed namespace

namespace snap::rhi::backend::vulkan {
Fence::Fence(Device* device, const snap::rhi::FenceCreateInfo& info)
    : snap::rhi::Fence(device, info),
      validationLayer(device->getValidationLayer()),
      vkDevice(device->getVkLogicalDevice()) {
    VkResult result = vkCreateFence(this->vkDevice, &DefaultFenceCreateInfo, nullptr, &fence);
    SNAP_RHI_VALIDATE(validationLayer,
                      result == VK_SUCCESS,
                      snap::rhi::ReportLevel::CriticalError,
                      snap::rhi::ValidationTag::CreateOp,
                      "[Vulkan::Fence] Cannot create VkFence");
}

snap::rhi::FenceStatus Fence::getStatus(uint64_t generationID) {
    std::lock_guard lock(accessMutex);
    if (generationID && this->generationID > generationID) {
        return snap::rhi::FenceStatus::Completed;
    }

    assert(fence != VK_NULL_HANDLE);

    VkResult result = vkGetFenceStatus(vkDevice, fence);
    switch (result) {
        case VK_SUCCESS:
            return snap::rhi::FenceStatus::Completed;

        case VK_NOT_READY:
            return snap::rhi::FenceStatus::NotReady;

        default: {
            SNAP_RHI_REPORT(validationLayer,
                            snap::rhi::ReportLevel::CriticalError,
                            snap::rhi::ValidationTag::FenceOp,
                            "[Vulkan::Fence] Cannot get VkFence status");
            return snap::rhi::FenceStatus::Error;
        }
    }
}

void Fence::waitForScheduled() {
    std::lock_guard lock(accessMutex);
    assert(fence != VK_NULL_HANDLE);
    // vkQueueSubmit is guaranteed to be scheduled once it returns VK_SUCCESS. So we can just return here.
}

void Fence::waitForComplete() {
    std::lock_guard lock(accessMutex);
    assert(fence != VK_NULL_HANDLE);

    VkResult result = vkWaitForFences(vkDevice, 1, &fence, VK_TRUE, UINT64_MAX);
    SNAP_RHI_VALIDATE(validationLayer,
                      result == VK_SUCCESS,
                      snap::rhi::ReportLevel::CriticalError,
                      snap::rhi::ValidationTag::FenceOp,
                      "[Vulkan::Fence] Cannot wait for VkFence to complete");
}

void Fence::reset() {
    std::lock_guard lock(accessMutex);
    assert(fence != VK_NULL_HANDLE);

    VkResult result = vkResetFences(vkDevice, 1, &fence);
    SNAP_RHI_VALIDATE(validationLayer,
                      result == VK_SUCCESS,
                      snap::rhi::ReportLevel::CriticalError,
                      snap::rhi::ValidationTag::FenceOp,
                      "[Vulkan::Fence] Cannot reset VkFence");
    snap::rhi::Fence::reset();
}

std::unique_ptr<snap::rhi::PlatformSyncHandle> Fence::exportPlatformSyncHandle() {
    std::lock_guard lock(accessMutex);

    auto* vkDevice_ = snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::Device>(device);
    auto retainable = vkDevice_->resolveResource(this);
    auto sharedFence = std::static_pointer_cast<snap::rhi::Fence>(retainable);

#if SNAP_RHI_OS_ANDROID()
    if (!GLAD_VK_KHR_external_fence_fd || vkGetFenceFdKHR == nullptr) {
        return std::make_unique<snap::rhi::backend::common::PlatformSyncHandle>(sharedFence);
    }

    VkFenceGetFdInfoKHR getFdInfo{};
    getFdInfo.sType = VK_STRUCTURE_TYPE_FENCE_GET_FD_INFO_KHR;
    getFdInfo.pNext = nullptr;
    getFdInfo.fence = fence;
    getFdInfo.handleType = VK_EXTERNAL_FENCE_HANDLE_TYPE_SYNC_FD_BIT;

    int fenceFd = -1;
    VkResult result = vkGetFenceFdKHR(this->vkDevice, &getFdInfo, &fenceFd);

    if (result != VK_SUCCESS || fenceFd < 0) {
        return std::make_unique<snap::rhi::backend::common::PlatformSyncHandle>(sharedFence);
    }

    return std::make_unique<snap::rhi::backend::common::platform::android::SyncHandle>(sharedFence, fenceFd);
#else
    return std::make_unique<snap::rhi::backend::common::PlatformSyncHandle>(sharedFence);
#endif
}

Fence::~Fence() {
    assert(fence != VK_NULL_HANDLE);

    vkDestroyFence(vkDevice, fence, nullptr);
    fence = VK_NULL_HANDLE;
}

void Fence::setDebugLabel(std::string_view label) {
#if SNAP_RHI_ENABLE_DEBUG_LABELS
    ObjectDebugMarkers::setDebugLabel(label);
    setVkObjectDebugName(vkDevice, VK_OBJECT_TYPE_FENCE, reinterpret_cast<uint64_t>(fence), label.data());
#endif
}
} // namespace snap::rhi::backend::vulkan
