#pragma once

#include "snap/rhi/backend/common/ValidationLayer.hpp"
#include "snap/rhi/backend/vulkan/vulkan.h"
#include "snap/rhi/common/NonCopyable.h"
#include <functional>
#include <limits>
#include <memory>
#include <mutex>
#include <queue>

namespace snap::rhi::backend::vulkan {
class Device;

class CommandPool final : public snap::rhi::common::NonCopyable {
    static constexpr uint32_t MaxUncompletedCommandBuffers = 64;

public:
    CommandPool(snap::rhi::backend::vulkan::Device* device, uint32_t queueFamilyIndex);
    ~CommandPool();

    std::unique_ptr<VkCommandPool, std::function<void(VkCommandPool*)>> allocate();

private:
    snap::rhi::backend::vulkan::Device* device = nullptr;
    const uint32_t queueFamilyIndex = std::numeric_limits<uint32_t>::max();
    const snap::rhi::backend::common::ValidationLayer& validationLayer;

    std::array<VkCommandPool, MaxUncompletedCommandBuffers> pools;
    std::queue<uint32_t> available;
    std::mutex accessMutex;
};
} // namespace snap::rhi::backend::vulkan
