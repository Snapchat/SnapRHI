#include "snap/rhi/CommandQueue.hpp"

namespace snap::rhi {
CommandQueue::CommandQueue(Device* device) : snap::rhi::DeviceChild(device, snap::rhi::ResourceType::CommandQueue) {}

void CommandQueue::submitCommandBuffer(CommandBuffer* commandBuffer, CommandBufferWaitType waitType, Fence* fence) {
    std::array<snap::rhi::CommandBuffer*, 1> cmdBuffers{commandBuffer};
    submitCommands({}, {}, cmdBuffers, {}, waitType, fence);
}
} // namespace snap::rhi
