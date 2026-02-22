#include "snap/rhi/backend/noop/CommandQueue.hpp"

#include "snap/rhi/backend/noop/Device.hpp"

namespace snap::rhi::backend::noop {
CommandQueue::CommandQueue(snap::rhi::backend::common::Device* device) : snap::rhi::CommandQueue(device) {}

CommandQueue::~CommandQueue() {}

void CommandQueue::submitCommands(std::span<snap::rhi::Semaphore*> waitSemaphores,
                                  std::span<const snap::rhi::PipelineStageBits> waitDstStageMask,
                                  std::span<snap::rhi::CommandBuffer*> buffers,
                                  std::span<snap::rhi::Semaphore*> signalSemaphores,
                                  CommandBufferWaitType waitType,
                                  snap::rhi::Fence* fence) {}

void CommandQueue::waitUntilScheduled() {}

void CommandQueue::waitIdle() {}
} // namespace snap::rhi::backend::noop
