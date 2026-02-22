#include "snap/rhi/BlitCommandEncoder.hpp"

namespace snap::rhi {
BlitCommandEncoder::BlitCommandEncoder(Device* device, CommandBuffer* commandBuffer)
    : snap::rhi::CommandEncoder(device, commandBuffer, snap::rhi::ResourceType::CommandBuffer) {}
} // namespace snap::rhi
