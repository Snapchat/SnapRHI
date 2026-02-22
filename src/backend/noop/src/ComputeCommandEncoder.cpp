#include "snap/rhi/backend/noop/ComputeCommandEncoder.hpp"

#include "snap/rhi/backend/common/Utils.hpp"

#include "snap/rhi/backend/noop/CommandBuffer.hpp"

#include "snap/rhi/Buffer.hpp"
#include "snap/rhi/ComputePipeline.hpp"
#include "snap/rhi/DescriptorSet.hpp"
#include "snap/rhi/Device.hpp"
#include "snap/rhi/Texture.hpp"
#include "snap/rhi/backend/common/Device.hpp"

#include <algorithm>

namespace snap::rhi::backend::noop {
ComputeCommandEncoder::ComputeCommandEncoder(snap::rhi::backend::common::Device* device,
                                             snap::rhi::backend::noop::CommandBuffer* commandBuffer)
    : ComputeCommandEncoderBase(device, commandBuffer), validationLayer(device->getValidationLayer()) {}

void ComputeCommandEncoder::beginEncoding() {
    ComputeCommandEncoderBase::onBeginEncoding();
}

void ComputeCommandEncoder::writeTimestamp(snap::rhi::QueryPool* queryPool,
                                           uint32_t query,
                                           const snap::rhi::TimestampLocation location) {}

void ComputeCommandEncoder::bindDescriptorSet(uint32_t binding,
                                              snap::rhi::DescriptorSet* descriptorSet,
                                              std::span<const uint32_t> dynamicOffsets) {
    assert(descriptorSet);
}

void ComputeCommandEncoder::bindComputePipeline(snap::rhi::ComputePipeline* pipeline) {
    assert(pipeline);
}

void ComputeCommandEncoder::dispatch(const uint32_t groupSizeX, const uint32_t groupSizeY, const uint32_t groupSizeZ) {}

void ComputeCommandEncoder::pipelineBarrier(snap::rhi::PipelineStageBits srcStageMask,
                                            snap::rhi::PipelineStageBits dstStageMask,
                                            snap::rhi::DependencyFlags dependencyFlags,
                                            std::span<snap::rhi::MemoryBarrierInfo> memoryBarriers,
                                            std::span<snap::rhi::BufferMemoryBarrierInfo> bufferMemoryBarriers,
                                            std::span<snap::rhi::TextureMemoryBarrierInfo> textureMemoryBarriers) {}

void ComputeCommandEncoder::endEncoding() {
    ComputeCommandEncoderBase::onEndEncoding();
}

void ComputeCommandEncoder::beginDebugGroup(std::string_view name) {}

void ComputeCommandEncoder::endDebugGroup() {}
} // namespace snap::rhi::backend::noop
