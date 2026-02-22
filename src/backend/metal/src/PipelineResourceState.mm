#include "snap/rhi/backend/metal/PipelineResourceState.h"

#include "snap/rhi/backend/metal/CommandBuffer.h"
#include "snap/rhi/backend/metal/DescriptorSet.h"
#include "snap/rhi/backend/metal/RenderPipeline.h"

namespace snap::rhi::backend::metal {
PipelineResourceState::PipelineResourceState(CommandBuffer* commandBuffer) : commandBuffer(commandBuffer) {
    clearStates();
}

void PipelineResourceState::setAuxiliaryDynamicOffsetsBinding(const uint32_t auxiliaryDynamicOffsetsBinding) {
    this->auxiliaryDynamicOffsetsBinding = auxiliaryDynamicOffsetsBinding;
}

void PipelineResourceState::bindDescriptorSet(const uint32_t binding,
                                              DescriptorSet* mtlDescriptorSet,
                                              std::span<const uint32_t> dynamicOffsets) {
    assert(binding < snap::rhi::SupportedLimit::MaxBoundDescriptorSets);

    DescriptorSetData dsData{.descriptorSet = mtlDescriptorSet,
                             .dynamicOffsets = {dynamicOffsets.begin(), dynamicOffsets.end()}};
    if (descriptorSets[binding] != dsData) {
        descriptorSets[binding] = dsData;
        activeDescriptorSets[binding] = {};
    }
}

void PipelineResourceState::setAllStates() {
    dynamicOffsetsStorage.clear();
    auto& context = commandBuffer->getContext();
    for (size_t i = 0; i < descriptorSets.size(); ++i) {
        if (descriptorSets[i].descriptorSet && activeDescriptorSets[i] != descriptorSets[i]) {
            const auto& argumentBufferSubRange = descriptorSets[i].descriptorSet->getArgumentBufferSubRange();
            if (!argumentBufferSubRange) {
                continue;
            }

            if (activeDescriptorSets[i].descriptorSet != descriptorSets[i].descriptorSet) {
                if (const auto& encoder = context.getRenderEncoder(); encoder != nil) {
                    [encoder setVertexBuffer:argumentBufferSubRange->buffer
                                      offset:argumentBufferSubRange->offset
                                     atIndex:static_cast<uint32_t>(i)];
                    [encoder setFragmentBuffer:argumentBufferSubRange->buffer
                                        offset:argumentBufferSubRange->offset
                                       atIndex:static_cast<uint32_t>(i)];
                } else if (const auto& encoder = context.getComputeEncoder(); encoder != nil) {
                    [encoder setBuffer:argumentBufferSubRange->buffer
                                offset:argumentBufferSubRange->offset
                               atIndex:static_cast<uint32_t>(i)];
                }

                descriptorSets[i].descriptorSet->useResources(context);
                descriptorSets[i].descriptorSet->collectReferences(commandBuffer);
                commandBuffer->preserveInteropTextures(descriptorSets[i].descriptorSet->getInteropTextures());
            }
            activeDescriptorSets[i] = descriptorSets[i];
        }

        if (!descriptorSets[i].dynamicOffsets.empty()) {
            dynamicOffsetsStorage.insert(dynamicOffsetsStorage.end(),
                                         descriptorSets[i].dynamicOffsets.begin(),
                                         descriptorSets[i].dynamicOffsets.end());
        }
    }
    if (!dynamicOffsetsStorage.empty()) {
        if (const auto& encoder = context.getRenderEncoder(); encoder != nil) {
            [encoder setVertexBytes:dynamicOffsetsStorage.data()
                             length:dynamicOffsetsStorage.size() * sizeof(uint32_t)
                            atIndex:auxiliaryDynamicOffsetsBinding];
            [encoder setFragmentBytes:dynamicOffsetsStorage.data()
                               length:dynamicOffsetsStorage.size() * sizeof(uint32_t)
                              atIndex:auxiliaryDynamicOffsetsBinding];
        } else if (const auto& encoder = context.getComputeEncoder(); encoder != nil) {
            [encoder setBytes:dynamicOffsetsStorage.data()
                       length:dynamicOffsetsStorage.size() * sizeof(uint32_t)
                      atIndex:auxiliaryDynamicOffsetsBinding];
        }
    }
    activeAuxiliaryDynamicOffsetsBinding = auxiliaryDynamicOffsetsBinding;
}

void PipelineResourceState::clearStates() {
    descriptorSets.fill({});
    activeDescriptorSets.fill({});
    auxiliaryDynamicOffsetsBinding = 0;
    activeAuxiliaryDynamicOffsetsBinding = 0;
    dynamicOffsetsStorage.clear();
}
} // namespace snap::rhi::backend::metal
