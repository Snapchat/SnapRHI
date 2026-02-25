#include "snap/rhi/backend/opengl/CommandBuffer.hpp"
#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/opengl/Device.hpp"

namespace snap::rhi::backend::opengl {
CommandBuffer::CommandBuffer(snap::rhi::backend::opengl::Device* device, const snap::rhi::CommandBufferCreateInfo& info)
    : snap::rhi::backend::common::CommandBuffer(device, info),
      blitCommandEncoder(device, this),
      renderCommandEncoder(device, this),
      computeCommandEncoder(device, this) {}

void CommandBuffer::resetQueryPool(snap::rhi::QueryPool* queryPool, uint32_t firstQuery, uint32_t queryCount) {
    // TODO: Implement
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

void CommandBuffer::finishRecording() {
    auto* glDevice = snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Device>(device);
    snap::rhi::backend::common::CommandBuffer::onSubmitted(glDevice->getValidationLayer());
    commandAllocator.finish();
}

void CommandBuffer::reset(const snap::rhi::CommandBufferCreateInfo& info) {
    commandAllocator.reset();
    snap::rhi::backend::common::CommandBuffer::onReset(info);
}

snap::rhi::backend::opengl::CommandAllocator& CommandBuffer::getCommandAllocator() {
    return commandAllocator;
}
} // namespace snap::rhi::backend::opengl
