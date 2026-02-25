#pragma once

#include "snap/rhi/Semaphore.hpp"
#include "snap/rhi/SemaphoreCreateInfo.h"
#include "snap/rhi/backend/vulkan/vulkan.h"

namespace snap::rhi::backend::vulkan {
class Device;

class Semaphore : public snap::rhi::Semaphore {
public:
    explicit Semaphore(Device* device, const snap::rhi::SemaphoreCreateInfo& info);
    ~Semaphore() override;

    void setDebugLabel(std::string_view label) override;

    VkSemaphore getSemaphore() const {
        return semaphore;
    }

protected:
    explicit Semaphore(Device* device, const snap::rhi::SemaphoreCreateInfo& info, VkSemaphore semaphore);

private:
    VkDevice vkDevice = VK_NULL_HANDLE;
    VkSemaphore semaphore = VK_NULL_HANDLE;
};
} // namespace snap::rhi::backend::vulkan
