#include "snap/rhi/backend/opengl/PipelineResourceState.hpp"

#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/opengl/Buffer.hpp"
#include "snap/rhi/backend/opengl/DescriptorSet.hpp"
#include "snap/rhi/backend/opengl/Device.hpp"
#include "snap/rhi/backend/opengl/Profile.hpp"
#include "snap/rhi/backend/opengl/Sampler.hpp"
#include "snap/rhi/backend/opengl/Texture.hpp"

namespace snap::rhi::backend::opengl {
PipelineResourceState::PipelineResourceState(snap::rhi::backend::opengl::Device* device)
    : device(device), textureSamplerState(device), imageState(device), uboState(device), ssboState(device) {
    clearStates();
}

void PipelineResourceState::setPipeline(const snap::rhi::backend::opengl::Pipeline* pipeline) {
    assert(pipeline);
    if (this->pipeline != pipeline) {
        textureSamplerState.setPipeline(pipeline);
        imageState.setPipeline(pipeline);
        uboState.setPipeline(pipeline);
        ssboState.setPipeline(pipeline);
    }
    this->pipeline = pipeline;
}

void PipelineResourceState::bindDescriptorSet(const uint32_t binding,
                                              snap::rhi::backend::opengl::DescriptorSet* descriptorSet,
                                              std::span<const uint32_t> dynamicOffsets) {
    assert(binding < snap::rhi::SupportedLimit::MaxBoundDescriptorSets);
    DescriptorSetData dsData{.descriptorSet = descriptorSet,
                             .dynamicOffsets = {dynamicOffsets.begin(), dynamicOffsets.end()}};

    if (descriptorSets[binding] != dsData) {
        descriptorSets[binding] = dsData;
        activeDescriptorSets[binding] = {};
    }
}

void PipelineResourceState::setAllStates(DeviceContext* dc) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[PipelineResourceState][setAllStates]");
    [[maybe_unused]] const auto& validationLayer = device->getValidationLayer();

    for (size_t i = 0; i < descriptorSets.size(); ++i) {
        const auto dsId = static_cast<uint32_t>(i);

        if (descriptorSets[i].descriptorSet == nullptr) {
            textureSamplerState.invalidateDescriptorSetBindings(dsId);
            imageState.invalidateDescriptorSetBindings(dsId);
            uboState.invalidateDescriptorSetBindings(dsId);
            ssboState.invalidateDescriptorSetBindings(dsId);
        } else if (activeDescriptorSets[i] != descriptorSets[i]) {
            /**
             * All descriptors in descriptor set have to be in a sorted order
             */
            textureSamplerState.invalidateDescriptorSetBindings(dsId);
            imageState.invalidateDescriptorSetBindings(dsId);
            uboState.invalidateDescriptorSetBindings(dsId);
            ssboState.invalidateDescriptorSetBindings(dsId);

            auto dynamicOffsetItr = descriptorSets[i].dynamicOffsets.begin();
            auto dynamicOffsetEndItr = descriptorSets[i].dynamicOffsets.end();
            auto bindDescriptorFunc = [this, dsId, &validationLayer, &dynamicOffsetItr, &dynamicOffsetEndItr](
                                          const snap::rhi::DescriptorSetLayoutBinding& descriptorInfo,
                                          const DescriptorData& descriptor) {
                if (!descriptor.resourcePtr) {
                    return;
                }

                switch (descriptorInfo.descriptorType) {
                    case snap::rhi::DescriptorType::Sampler: {
                        auto* sampler = snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Sampler>(
                            descriptor.resourcePtr);
                        textureSamplerState.bindSampler(dsId, descriptorInfo.binding, sampler);
                    } break;

                    case snap::rhi::DescriptorType::SampledTexture: {
                        auto* texture = snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Texture>(
                            descriptor.resourcePtr);
                        textureSamplerState.bindTexture(dsId, descriptorInfo.binding, texture);
                    } break;

                    case snap::rhi::DescriptorType::UniformBuffer: {
                        auto* buffer = snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Buffer>(
                            descriptor.resourcePtr);
                        uboState.bind(dsId, descriptorInfo.binding, buffer, descriptor.offset, descriptor.range);
                    } break;

                    case snap::rhi::DescriptorType::UniformBufferDynamic: {
                        (void)dynamicOffsetEndItr;
                        SNAP_RHI_VALIDATE(validationLayer,
                                          dynamicOffsetItr != dynamicOffsetEndItr,
                                          snap::rhi::ReportLevel::Error,
                                          snap::rhi::ValidationTag::DescriptorSetOp,
                                          "[PipelineResourceState::setAllStates] dynamicOffsetItr "
                                          "exceeds the dynamicOffsets size");

                        auto* buffer = snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Buffer>(
                            descriptor.resourcePtr);
                        uboState.bind(dsId,
                                      descriptorInfo.binding,
                                      buffer,
                                      descriptor.offset + *dynamicOffsetItr,
                                      descriptor.range);
                        ++dynamicOffsetItr;
                    } break;

                    case snap::rhi::DescriptorType::StorageTexture: {
                        auto* texture = snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Texture>(
                            descriptor.resourcePtr);
                        imageState.bindImage(dsId, descriptorInfo.binding, texture, descriptor.mipLevel);
                    } break;

                    case snap::rhi::DescriptorType::StorageBuffer: {
                        auto* buffer = snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Buffer>(
                            descriptor.resourcePtr);
                        ssboState.bind(dsId, descriptorInfo.binding, buffer, descriptor.offset /*, descriptor.range*/);
                    } break;

                    case snap::rhi::DescriptorType::StorageBufferDynamic: {
                        (void)dynamicOffsetEndItr;
                        SNAP_RHI_VALIDATE(validationLayer,
                                          dynamicOffsetItr != dynamicOffsetEndItr,
                                          snap::rhi::ReportLevel::Error,
                                          snap::rhi::ValidationTag::DescriptorSetOp,
                                          "[PipelineResourceState::setAllStates] dynamicOffsetItr "
                                          "exceeds the dynamicOffsets size");

                        auto* buffer = snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Buffer>(
                            descriptor.resourcePtr);
                        ssboState.bind(dsId,
                                       descriptorInfo.binding,
                                       buffer,
                                       descriptor.offset + *dynamicOffsetItr /*, descriptor.range*/);
                        ++dynamicOffsetItr;
                    } break;

                    default: {
                        (void)validationLayer; // to suppress unused variable warning in release builds
                        SNAP_RHI_REPORT(
                            validationLayer,
                            snap::rhi::ReportLevel::Error,
                            snap::rhi::ValidationTag::DescriptorSetOp,
                            "[PipelineResourceState::setAllStates] unsupported descriptor.descriptorType{%d}",
                            descriptorInfo.descriptorType);
                    } break;
                }
            };
            descriptorSets[i].descriptorSet->for_each(bindDescriptorFunc);
        }
        activeDescriptorSets[i] = descriptorSets[i];
    }

    textureSamplerState.setAllStates(dc);
    imageState.setAllStates(dc);
    uboState.setAllStates(dc);
    ssboState.setAllStates(dc);
}

void PipelineResourceState::clearStates() {
    descriptorSets.fill({});
    activeDescriptorSets.fill({});

    textureSamplerState.clearStates();
    imageState.clearStates();
    uboState.clearStates();
    ssboState.clearStates();
}
} // namespace snap::rhi::backend::opengl
