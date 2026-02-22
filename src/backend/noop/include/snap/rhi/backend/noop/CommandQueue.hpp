#pragma once

#include "snap/rhi/CommandQueue.hpp"

namespace snap::rhi::backend::common {
class Device;
} // namespace snap::rhi::backend::common

namespace snap::rhi::backend::noop {
class CommandQueue final : public snap::rhi::CommandQueue {
public:
    CommandQueue(snap::rhi::backend::common::Device* device);
    ~CommandQueue() override;

    void submitCommands(std::span<snap::rhi::Semaphore*> waitSemaphores,
                        std::span<const snap::rhi::PipelineStageBits> waitDstStageMask,
                        std::span<snap::rhi::CommandBuffer*> buffers,
                        std::span<snap::rhi::Semaphore*> signalSemaphores,
                        CommandBufferWaitType waitType,
                        snap::rhi::Fence* fence) override;

    void waitUntilScheduled() override;

    void waitIdle() override;
};

} // namespace snap::rhi::backend::noop
