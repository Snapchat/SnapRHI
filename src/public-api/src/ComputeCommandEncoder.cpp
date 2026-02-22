#include "snap/rhi/ComputeCommandEncoder.hpp"

namespace snap::rhi {
ComputeCommandEncoder::ComputeCommandEncoder(Device* device, CommandBuffer* commandBuffer)
    : snap::rhi::CommandEncoder(device, commandBuffer, snap::rhi::ResourceType::ComputeCommandEncoder) {}
} // namespace snap::rhi
