#include "snap/rhi/backend/noop/CommandBuffer.hpp"

#include "snap/rhi/backend/common/Device.hpp"
#include "snap/rhi/backend/common/Utils.hpp"

#include "snap/rhi/backend/noop/Device.hpp"

namespace snap::rhi::backend::noop {
CommandBuffer::CommandBuffer(snap::rhi::backend::common::Device* device, const snap::rhi::CommandBufferCreateInfo& info)
    : snap::rhi::backend::common::CommandBuffer(device, info),
      blitCommandEncoder(device, this),
      renderCommandEncoder(device, this),
      computeCommandEncoder(device, this) {}

void CommandBuffer::resetQueryPool(snap::rhi::QueryPool* queryPool, uint32_t firstQuery, uint32_t queryCount) {
    // NoOp backend does not support queries, so this is a no-op.
}

snap::rhi::RenderCommandEncoder* CommandBuffer::getRenderCommandEncoder() {
    return &renderCommandEncoder;
}

snap::rhi::BlitCommandEncoder* CommandBuffer::getBlitCommandEncoder() {
    return &blitCommandEncoder;
}

snap::rhi::ComputeCommandEncoder* CommandBuffer::getComputeCommandEncoder() {
    return &computeCommandEncoder;
}
} // namespace snap::rhi::backend::noop
