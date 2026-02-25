#include "snap/rhi/backend/vulkan/platform/android/Semaphore.h"
#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/vulkan/Device.h"
#include "snap/rhi/backend/vulkan/Semaphore.h"

#include "snap/rhi/common/Throw.h"

#if SNAP_RHI_OS_ANDROID()
#include <unistd.h>

namespace {
VkSemaphore createSemaphore(VkDevice vkDevice,
                            const snap::rhi::backend::common::ValidationLayer& validationLayer,
                            int32_t fd) {
    VkSemaphore semaphore = VK_NULL_HANDLE;

    VkSemaphoreCreateInfo createInfo{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

    VkResult result = vkCreateSemaphore(vkDevice, &createInfo, nullptr, &semaphore);
    if (result != VK_SUCCESS) {
        snap::rhi::common::throwException("[ExternalSemaphore] Failed to create Vulkan Android Semaphore");
    }

    VkImportSemaphoreFdInfoKHR importInfo{};
    importInfo.sType = VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_FD_INFO_KHR;
    importInfo.semaphore = semaphore;
    importInfo.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT;
    importInfo.fd = fd;

    result = vkImportSemaphoreFdKHR(vkDevice, &importInfo);
    if (result != VK_SUCCESS) {
        snap::rhi::common::throwException("[ExternalSemaphore] Failed to import FD into semaphore");
    }

    return semaphore;
}
} // unnamed namespace

namespace snap::rhi::backend::vulkan::platform::android {
Semaphore::Semaphore(Device* device, const snap::rhi::SemaphoreCreateInfo& info, int32_t fd)
    : snap::rhi::backend::vulkan::Semaphore(
          device, info, createSemaphore(device->getVkLogicalDevice(), device->getValidationLayer(), fd)) {}
} // namespace snap::rhi::backend::vulkan::platform::android
#endif
