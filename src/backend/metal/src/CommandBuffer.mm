#include "snap/rhi/backend/metal/CommandBuffer.h"
#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/metal/CommandQueue.h"
#include "snap/rhi/backend/metal/Device.h"
#include "snap/rhi/backend/metal/Framebuffer.h"

namespace {
} // unnamed namespace

namespace snap::rhi::backend::metal {
CommandBuffer::CommandBuffer(Device* mtlDevice, const snap::rhi::CommandBufferCreateInfo& info)
    : snap::rhi::backend::common::CommandBuffer(mtlDevice, info),
      context(mtlDevice, info),
      blitEncoder(mtlDevice, this),
      renderEncoder(mtlDevice, this),
      computeEncoder(mtlDevice, this) {}

CommandBuffer::~CommandBuffer() {
    auto* mtlDevice = snap::rhi::backend::common::smart_cast<snap::rhi::backend::metal::Device>(device);
    const auto& mtlCommandBuffer = context.getCommandBuffer();
    const MTLCommandBufferStatus status = mtlCommandBuffer.status;

    SNAP_RHI_VALIDATE(mtlDevice->getValidationLayer(),
                      status == MTLCommandBufferStatusCompleted || status == MTLCommandBufferStatusError,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::DestroyOp,
                      "[CommandBuffer::~CommandBuffer] command buffer wasn't completed before destruction {%d}",
                      static_cast<int32_t>(status));
}

void CommandBuffer::resetQueryPool(snap::rhi::QueryPool* queryPool, uint32_t firstQuery, uint32_t queryCount) {
    // TODO: implement QueryPool reset for Metal
}

snap::rhi::RenderCommandEncoder* CommandBuffer::getRenderCommandEncoder() {
    return &renderEncoder;
}

snap::rhi::BlitCommandEncoder* CommandBuffer::getBlitCommandEncoder() {
    return &blitEncoder;
}

snap::rhi::ComputeCommandEncoder* CommandBuffer::getComputeCommandEncoder() {
    return &computeEncoder;
}

Context& CommandBuffer::getContext() {
    return context;
}

void CommandBuffer::setDebugLabel(std::string_view label) {
#if SNAP_RHI_ENABLE_DEBUG_LABELS
    ObjectDebugMarkers::setDebugLabel(label);
    context.getCommandBuffer().label = [NSString stringWithUTF8String:label.data()];
#endif
}
} // namespace snap::rhi::backend::metal
