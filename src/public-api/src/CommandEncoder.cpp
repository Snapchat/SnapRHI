//
// Created by Vladyslav Deviatkov on 10/31/25.
//

#include "snap/rhi/CommandEncoder.h"

namespace snap::rhi {
CommandEncoder::CommandEncoder(Device* device, CommandBuffer* commandBuffer, const snap::rhi::ResourceType resourceType)
    : snap::rhi::DeviceChild(device, resourceType), commandBuffer(commandBuffer) {}

CommandBuffer* CommandEncoder::getCommandBuffer() const {
    return commandBuffer;
}
} // namespace snap::rhi
