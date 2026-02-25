#include "snap/rhi/backend/opengl/ComputeCommandEncoder.hpp"
#include "snap/rhi/backend/common/Utils.hpp"

#include "snap/rhi/backend/opengl/CommandBuffer.hpp"
#include "snap/rhi/backend/opengl/Commands.h"
#include "snap/rhi/backend/opengl/Device.hpp"

#include "snap/rhi/Buffer.hpp"
#include "snap/rhi/ComputePipeline.hpp"
#include "snap/rhi/DescriptorSet.hpp"
#include "snap/rhi/Device.hpp"
#include "snap/rhi/Texture.hpp"
#include "snap/rhi/backend/opengl/DescriptorSet.hpp"

#include <algorithm>
#include <ranges>

namespace snap::rhi::backend::opengl {
ComputeCommandEncoder::ComputeCommandEncoder(snap::rhi::backend::opengl::Device* device,
                                             snap::rhi::backend::opengl::CommandBuffer* commandBuffer)
    : ComputeCommandEncoderBase(device, commandBuffer) {}

void ComputeCommandEncoder::beginEncoding() {
    ComputeCommandEncoderBase::onBeginEncoding();
    commandAllocator.allocateCommand<BeginComputePassCmd>();
}

void ComputeCommandEncoder::bindDescriptorSet(uint32_t binding,
                                              snap::rhi::DescriptorSet* descriptorSet,
                                              std::span<const uint32_t> dynamicOffsets) {
    assert(descriptorSet);

    BindDescriptorSetCmd* cmd = commandAllocator.allocateCommand<BindDescriptorSetCmd>();
    cmd->binding = binding;
    cmd->descriptorSet = descriptorSet;
    cmd->dynamicOffsetCount = static_cast<uint32_t>(dynamicOffsets.size());
    assert(dynamicOffsets.size() <= cmd->dynamicOffsets.size());
    std::ranges::copy(dynamicOffsets, cmd->dynamicOffsets.begin());

    auto* glDescriptorSet = snap::rhi::backend::common::smart_cast<DescriptorSet>(descriptorSet);
    glDescriptorSet->collectReferences(commandBuffer);
    commandBuffer->preserveInteropTextures(glDescriptorSet->getInteropTextures());
}

void ComputeCommandEncoder::bindComputePipeline(snap::rhi::ComputePipeline* pipeline) {
    assert(pipeline);

    BindComputePipelineCmd* cmd = commandAllocator.allocateCommand<BindComputePipelineCmd>();
    cmd->pipeline = pipeline;

    resourceResidencySet.track(pipeline);
}

void ComputeCommandEncoder::dispatch(const uint32_t groupSizeX, const uint32_t groupSizeY, const uint32_t groupSizeZ) {
    DispatchCmd* cmd = commandAllocator.allocateCommand<DispatchCmd>();
    cmd->groupSizeX = groupSizeX;
    cmd->groupSizeY = groupSizeY;
    cmd->groupSizeZ = groupSizeZ;
}

void ComputeCommandEncoder::endEncoding() {
    ComputeCommandEncoderBase::onEndEncoding();
    commandAllocator.allocateCommand<EndComputePassCmd>();
}
} // namespace snap::rhi::backend::opengl
