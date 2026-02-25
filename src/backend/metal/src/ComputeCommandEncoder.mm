#include "snap/rhi/backend/metal/ComputeCommandEncoder.h"
#include "snap/rhi/Common.h"
#include "snap/rhi/backend/common/CommandBuffer.h"
#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/metal/Buffer.h"
#include "snap/rhi/backend/metal/CommandBuffer.h"
#include "snap/rhi/backend/metal/ComputePipeline.h"
#include "snap/rhi/backend/metal/DescriptorSet.h"
#include "snap/rhi/backend/metal/Device.h"
#include "snap/rhi/backend/metal/Texture.h"
#include "snap/rhi/backend/metal/Utils.h"
#include "snap/rhi/common/OS.h"
#include <vector>

namespace snap::rhi::backend::metal {
ComputeCommandEncoder::ComputeCommandEncoder(Device* mtlDevice, CommandBuffer* commandBuffer)
    : ComputeCommandEncoderBase(mtlDevice, commandBuffer),
      context(commandBuffer->getContext()),
      pipelineState(commandBuffer) {}

void ComputeCommandEncoder::beginEncoding() {
    const id<MTLComputeCommandEncoder>& encoder = context.beginCompute();
    ComputeCommandEncoderBase::onBeginEncoding(encoder);
}

void ComputeCommandEncoder::bindDescriptorSet(uint32_t binding,
                                              snap::rhi::DescriptorSet* descriptorSet,
                                              std::span<const uint32_t> dynamicOffsets) {
    auto* mtlDescriptorSet = snap::rhi::backend::common::smart_cast<DescriptorSet>(descriptorSet);
    pipelineState.bindDescriptorSet(binding, mtlDescriptorSet, dynamicOffsets);
}

void ComputeCommandEncoder::bindComputePipeline(snap::rhi::ComputePipeline* pipeline) {
    auto* mtlComputePipeline = snap::rhi::backend::common::smart_cast<ComputePipeline>(pipeline);
    const auto& pipelineInfo = pipeline->getCreateInfo();

    pipelineState.setAuxiliaryDynamicOffsetsBinding(pipelineInfo.mtlPipelineInfo->auxiliaryDynamicOffsetsBinding);
    [context.getComputeEncoder() setComputePipelineState:mtlComputePipeline->getComputePipeline()];

    const auto& localThreadGroupSize = mtlComputePipeline->getCreateInfo().localThreadGroupSize;
    threadsPerThreadgroup =
        MTLSizeMake(localThreadGroupSize.width, localThreadGroupSize.height, localThreadGroupSize.depth);

    {
        resourceResidencySet.track(pipeline);
    }
}

void ComputeCommandEncoder::dispatch(const uint32_t groupSizeX, const uint32_t groupSizeY, const uint32_t groupSizeZ) {
    pipelineState.setAllStates();

    MTLSize threadgroupsPerGrid = MTLSizeMake(groupSizeX, groupSizeY, groupSizeZ);
    [context.getComputeEncoder() dispatchThreadgroups:threadgroupsPerGrid threadsPerThreadgroup:threadsPerThreadgroup];
}

void ComputeCommandEncoder::pipelineBarrier(snap::rhi::PipelineStageBits srcStageMask,
                                            snap::rhi::PipelineStageBits dstStageMask,
                                            snap::rhi::DependencyFlags dependencyFlags,
                                            std::span<snap::rhi::MemoryBarrierInfo> memoryBarriers,
                                            std::span<snap::rhi::BufferMemoryBarrierInfo> bufferMemoryBarriers,
                                            std::span<snap::rhi::TextureMemoryBarrierInfo> textureMemoryBarriers) {
    if (@available(macOS 10.14, macCatalyst 13.0, ios 12.0, *)) {
        const uint32_t objectsCount = static_cast<uint32_t>(bufferMemoryBarriers.size() + textureMemoryBarriers.size());
        std::vector<id> objects(objectsCount);

        uint32_t idx = 0;
        for (uint32_t i = 0; i < bufferMemoryBarriers.size(); ++i) {
            Buffer* mtlBuffer = snap::rhi::backend::common::smart_cast<Buffer>(bufferMemoryBarriers[i].buffer);
            objects[idx++] = mtlBuffer->getBuffer();
        }

        for (uint32_t i = 0; i < textureMemoryBarriers.size(); ++i) {
            Texture* mtlTextures = snap::rhi::backend::common::smart_cast<Texture>(textureMemoryBarriers[i].texture);
            objects[idx++] = mtlTextures->getTexture();

            commandBuffer->tryPreserveInteropTexture(textureMemoryBarriers[i].texture);
        }

        [context.getComputeEncoder() memoryBarrierWithResources:objects.data() count:objectsCount];
#if SNAP_RHI_OS_MACOS()
        MTLBarrierScope scope = MTLBarrierScopeBuffers | MTLBarrierScopeTextures;
        if (!memoryBarriers.empty()) {
            if (@available(macOS 10.14, macCatalyst 13.0, *)) {
                scope = scope | MTLBarrierScopeRenderTargets;
            }

            [context.getComputeEncoder() memoryBarrierWithScope:scope];
        }
#endif
    }
}

void ComputeCommandEncoder::endEncoding() {
    context.endCompute();
    ComputeCommandEncoderBase::onEndEncoding();
}
} // namespace snap::rhi::backend::metal
