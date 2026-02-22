#include "snap/rhi/backend/vulkan/Semaphore.h"
#include "snap/rhi/backend/vulkan/Device.h"
#include "snap/rhi/backend/vulkan/Utils.hpp"
#include "snap/rhi/backend/vulkan/VkDebugUtils.h"

namespace {
constexpr VkSemaphoreCreateInfo DefaultSemaphoreCreateInfo{
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
};

VkSemaphore createSemaphore(VkDevice vkDevice, const snap::rhi::backend::common::ValidationLayer& validationLayer) {
    VkSemaphore semaphore = VK_NULL_HANDLE;
    VkResult result = vkCreateSemaphore(vkDevice, &DefaultSemaphoreCreateInfo, nullptr, &semaphore);
    SNAP_RHI_VALIDATE(validationLayer,
                      result == VK_SUCCESS,
                      snap::rhi::ReportLevel::CriticalError,
                      snap::rhi::ValidationTag::CreateOp,
                      "[Vulkan::Semaphore] Cannot create VkSemaphore");
    return semaphore;
}
} // unnamed namespace

namespace snap::rhi::backend::vulkan {
Semaphore::Semaphore(Device* device, const snap::rhi::SemaphoreCreateInfo& info, VkSemaphore semaphore)
    : snap::rhi::Semaphore(device, info), vkDevice(device->getVkLogicalDevice()), semaphore(semaphore) {}

Semaphore::Semaphore(Device* device, const snap::rhi::SemaphoreCreateInfo& info)
    : Semaphore(device, info, createSemaphore(device->getVkLogicalDevice(), device->getValidationLayer())) {}

Semaphore::~Semaphore() {
    assert(semaphore != VK_NULL_HANDLE);

    vkDestroySemaphore(vkDevice, semaphore, nullptr);
    semaphore = VK_NULL_HANDLE;
}

void Semaphore::setDebugLabel(std::string_view label) {
#if SNAP_RHI_ENABLE_DEBUG_LABELS
    ObjectDebugMarkers::setDebugLabel(label);
    setVkObjectDebugName(vkDevice, VK_OBJECT_TYPE_SEMAPHORE, reinterpret_cast<uint64_t>(semaphore), label.data());
#endif
}
} // namespace snap::rhi::backend::vulkan
