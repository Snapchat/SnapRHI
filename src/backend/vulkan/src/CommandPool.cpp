#include "snap/rhi/backend/vulkan/CommandPool.h"
#include "snap/rhi/backend/vulkan/Device.h"

namespace snap::rhi::backend::vulkan {
CommandPool::CommandPool(snap::rhi::backend::vulkan::Device* device, uint32_t queueFamilyIndex)
    : device(device), queueFamilyIndex(queueFamilyIndex), validationLayer(device->getValidationLayer()) {
    const VkCommandPoolCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = 0,
        .queueFamilyIndex = queueFamilyIndex,
    };

    for (uint32_t i = 0; i < pools.size(); ++i) {
        VkResult result = vkCreateCommandPool(device->getVkLogicalDevice(), &createInfo, nullptr, &pools[i]);
        SNAP_RHI_VALIDATE(validationLayer,
                          result == VK_SUCCESS,
                          snap::rhi::ReportLevel::CriticalError,
                          snap::rhi::ValidationTag::CreateOp,
                          "[CommandPool] cannot create command pool, error: %d",
                          result);
        available.push(i);
    }
}

CommandPool::~CommandPool() {
    std::lock_guard<std::mutex> lock(accessMutex);
    SNAP_RHI_VALIDATE(validationLayer,
                      available.size() == pools.size(),
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::DestroyOp,
                      "[CommandPool] uncompleted command buffers on CommandPool destroy");

    for (auto& pool : pools) {
        vkDestroyCommandPool(device->getVkLogicalDevice(), pool, nullptr);
        pool = VK_NULL_HANDLE;
    }
}

std::unique_ptr<VkCommandPool, std::function<void(VkCommandPool*)>> CommandPool::allocate() {
    std::lock_guard<std::mutex> lock(accessMutex);

    SNAP_RHI_VALIDATE(validationLayer,
                      !available.empty(),
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::CommandBufferOp,
                      "There is more than %zu uncompleted command buffers at the same time!",
                      pools.size() - available.size());

    uint32_t idx = available.front();
    available.pop();

    vkResetCommandPool(device->getVkLogicalDevice(), pools[idx], 0);
    return {&pools[idx], [this, idx](VkCommandPool* pool) {
                std::lock_guard<std::mutex> lock(accessMutex);
                available.push(idx);
            }};
}
} // namespace snap::rhi::backend::vulkan
