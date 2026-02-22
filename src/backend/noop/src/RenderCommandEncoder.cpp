#include "snap/rhi/backend/noop/RenderCommandEncoder.hpp"

#include "snap/rhi/backend/common/Utils.hpp"

#include "snap/rhi/Buffer.hpp"
#include "snap/rhi/DescriptorSet.hpp"
#include "snap/rhi/Device.hpp"
#include "snap/rhi/Framebuffer.hpp"
#include "snap/rhi/RenderPipeline.hpp"
#include "snap/rhi/Sampler.hpp"
#include "snap/rhi/Texture.hpp"
#include "snap/rhi/backend/common/Device.hpp"

#include "snap/rhi/Limits.h"
#include "snap/rhi/backend/noop/CommandBuffer.hpp"

#include <algorithm>
#include <sstream>

namespace snap::rhi::backend::noop {
RenderCommandEncoder::RenderCommandEncoder(snap::rhi::backend::common::Device* device, CommandBuffer* commandBuffer)
    : RenderCommandEncoderBase(device, commandBuffer), validationLayer(device->getValidationLayer()) {}

void RenderCommandEncoder::beginEncoding(const RenderPassBeginInfo& renderPassBeginInfo) {
    assert(renderPassBeginInfo.framebuffer);
    assert(renderPassBeginInfo.renderPass);
    RenderCommandEncoderBase::onBeginEncoding();
}

void RenderCommandEncoder::beginEncoding(const RenderingInfo& renderingInfo) {
    RenderCommandEncoderBase::onBeginEncoding();
}

void RenderCommandEncoder::writeTimestamp(snap::rhi::QueryPool* queryPool,
                                          uint32_t query,
                                          const snap::rhi::TimestampLocation location) {}

void RenderCommandEncoder::setViewport(const snap::rhi::Viewport& viewport) {}

void RenderCommandEncoder::bindRenderPipeline(snap::rhi::RenderPipeline* pipeline) {
    assert(pipeline);
}

void RenderCommandEncoder::bindDescriptorSet(uint32_t binding,
                                             snap::rhi::DescriptorSet* descriptorSet,
                                             std::span<const uint32_t> dynamicOffsets) {
    assert(descriptorSet);
}

void RenderCommandEncoder::bindVertexBuffers(const uint32_t firstBinding,
                                             std::span<Buffer*> buffers,
                                             std::span<uint32_t> offsets) {
    assert(buffers.size() <= offsets.size());
    for (uint32_t i = 0; i < buffers.size(); ++i) {
        assert(buffers[i]);
        const auto& bufferInfo = buffers[i]->getCreateInfo();
        SNAP_RHI_VALIDATE(validationLayer,
                          (bufferInfo.bufferUsage & snap::rhi::BufferUsage::VertexBuffer) ==
                              snap::rhi::BufferUsage::VertexBuffer,
                          snap::rhi::ReportLevel::Error,
                          snap::rhi::ValidationTag::CommandBufferOp,
                          "[RenderCommandEncoder::bindVertexBuffers] buffer must have buffer usage VertexBuffer");
    }
}

void RenderCommandEncoder::bindVertexBuffer(const uint32_t binding, snap::rhi::Buffer* buffer, const uint32_t offset) {
    assert(buffer);

    const auto& bufferInfo = buffer->getCreateInfo();
    SNAP_RHI_VALIDATE(validationLayer,
                      (bufferInfo.bufferUsage & snap::rhi::BufferUsage::VertexBuffer) ==
                          snap::rhi::BufferUsage::VertexBuffer,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::CommandBufferOp,
                      "[RenderCommandEncoder::bindVertexBuffer] buffer must have buffer usage VertexBuffer");
}

void RenderCommandEncoder::bindIndexBuffer(snap::rhi::Buffer* indexBuffer,
                                           const uint32_t offset,
                                           const snap::rhi::IndexType indexType) {
    assert(indexBuffer);

    const auto& bufferInfo = indexBuffer->getCreateInfo();
    SNAP_RHI_VALIDATE(validationLayer,
                      (bufferInfo.bufferUsage & snap::rhi::BufferUsage::IndexBuffer) ==
                          snap::rhi::BufferUsage::IndexBuffer,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::CommandBufferOp,
                      "[RenderCommandEncoder::bindIndexBuffer] buffer must have buffer usage IndexBuffer");
}

void RenderCommandEncoder::draw(const uint32_t vertexCount, const uint32_t firstVertex, const uint32_t instanceCount) {}

void RenderCommandEncoder::drawIndexed(const uint32_t indexCount,
                                       const uint32_t firstIndex,
                                       const uint32_t instanceCount) {}

void RenderCommandEncoder::setDepthBias(float depthBiasConstantFactor,
                                        float depthBiasSlopeFactor,
                                        float depthBiasClamp) {}

void RenderCommandEncoder::setStencilReference(const snap::rhi::StencilFace face, const uint32_t reference) {}

void RenderCommandEncoder::setBlendConstants(const float r, const float g, const float b, const float a) {}

void RenderCommandEncoder::pipelineBarrier(snap::rhi::PipelineStageBits srcStageMask,
                                           snap::rhi::PipelineStageBits dstStageMask,
                                           snap::rhi::DependencyFlags dependencyFlags,
                                           std::span<snap::rhi::MemoryBarrierInfo> memoryBarriers,
                                           std::span<snap::rhi::BufferMemoryBarrierInfo> bufferMemoryBarriers,
                                           std::span<snap::rhi::TextureMemoryBarrierInfo> textureMemoryBarriers) {}

void RenderCommandEncoder::invokeCustomCallback(std::function<void()>* callback) {}

void RenderCommandEncoder::endEncoding() {
    RenderCommandEncoderBase::onEndEncoding();
}

void RenderCommandEncoder::beginDebugGroup(std::string_view name) {}

void RenderCommandEncoder::endDebugGroup() {}
} // namespace snap::rhi::backend::noop
