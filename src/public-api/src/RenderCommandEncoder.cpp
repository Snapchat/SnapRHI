#include "snap/rhi/RenderCommandEncoder.hpp"

namespace snap::rhi {
RenderCommandEncoder::RenderCommandEncoder(Device* device, CommandBuffer* commandBuffer)
    : snap::rhi::CommandEncoder(device, commandBuffer, snap::rhi::ResourceType::RenderCommandEncoder) {}
} // namespace snap::rhi
