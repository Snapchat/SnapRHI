#pragma once

#include "snap/rhi/CommandQueue.hpp"
#include "snap/rhi/backend/vulkan/CommandPool.h"
#include "snap/rhi/backend/vulkan/ResourcesInitInfo.h"
#include "snap/rhi/backend/vulkan/vulkan.h"

namespace snap::rhi::backend::vulkan {
class Device;
class CommandBuffer;

/**
 * CommandQueue - thread unsafe object, should be sync manually
 * */
class CommandQueue final : public snap::rhi::CommandQueue {
public:
    CommandQueue(snap::rhi::backend::vulkan::Device* vkDevice,
                 const uint32_t queueFamilyIndex,
                 const uint32_t queueIndex);
    ~CommandQueue() override;

    void submitCommands(std::span<snap::rhi::Semaphore*> waitSemaphores,
                        std::span<const snap::rhi::PipelineStageBits> waitDstStageMask,
                        std::span<snap::rhi::CommandBuffer*> buffers,
                        std::span<snap::rhi::Semaphore*> signalSemaphores,
                        snap::rhi::CommandBufferWaitType waitType,
                        snap::rhi::Fence* fence) override;
    void waitUntilScheduled() override;
    void waitIdle() override;

    std::unique_ptr<VkCommandPool, std::function<void(VkCommandPool*)>> allocateCommandBuffer() {
        return commandPool.allocate();
    }

    VkQueue getQueue() const {
        return queue;
    }

private:
    VkQueue queue = VK_NULL_HANDLE;
    CommandPool commandPool;
};
} // namespace snap::rhi::backend::vulkan
