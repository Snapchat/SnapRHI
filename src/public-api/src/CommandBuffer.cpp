#include "snap/rhi/CommandBuffer.hpp"

namespace snap::rhi {
CommandBuffer::CommandBuffer(Device* device, const CommandBufferCreateInfo& info)
    : snap::rhi::DeviceChild(device, snap::rhi::ResourceType::CommandBuffer), info(info) {}

CommandQueue* CommandBuffer::getCommandQueue() {
    return info.commandQueue;
}
} // namespace snap::rhi
