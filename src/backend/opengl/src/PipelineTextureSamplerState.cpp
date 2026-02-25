#include "snap/rhi/backend/opengl/PipelineTextureSamplerState.hpp"
#include "snap/rhi/backend/common/Math.hpp"
#include "snap/rhi/backend/common/Utils.hpp"

#include "snap/rhi/backend/opengl/Device.hpp"
#include "snap/rhi/backend/opengl/OpenGL.h"
#include "snap/rhi/backend/opengl/Profile.hpp"
#include "snap/rhi/backend/opengl/RenderPipeline.hpp"
#include "snap/rhi/backend/opengl/Sampler.hpp"
#include "snap/rhi/backend/opengl/Texture.hpp"
#include <bitset>

#include "snap/rhi/common/Throw.h"

namespace snap::rhi::backend::opengl {
PipelineTextureSamplerState::PipelineTextureSamplerState(snap::rhi::backend::opengl::Device* device)
    : gl(device->getOpenGL()) {
    clearStates();
}

void PipelineTextureSamplerState::bindTextures(std::bitset<MaxTextureSamplerBinding>& toUpdateUnits) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(gl.getDevice(),
                                            "[PipelineTextureSamplerState][setAllStates][bindTextures]");

    assert(pipeline != nullptr);
    const auto* device = gl.getDevice();
    const auto& validationLayer = device->getValidationLayer();

    const auto& texturesDesc = pipeline->getPipelineTextureDescription();
    for (size_t i = 0; i < texturesDesc.size(); ++i) {
        const auto& dsDesc = texturesDesc[i];
        const auto& dsBindingList = texBindingsList[i];
        const auto& dsActiveTextures = activeTextures[i];
        size_t dsBindingListIdx = 0;

        for (const auto& texInfo : dsDesc) {
            assert(texInfo.binding < texBindings.size());

            const snap::rhi::backend::opengl::Texture* texture = nullptr;
            if (texInfo.logicalBinding < MAX_TEXTURE_SAMPLER_CACHE_SIZE) {
                texture = dsActiveTextures[texInfo.logicalBinding];
            } else {
                while ((dsBindingListIdx < dsBindingList.size()) &&
                       (dsBindingList[dsBindingListIdx].binding < texInfo.logicalBinding)) {
                    ++dsBindingListIdx;
                }

                const bool found = (dsBindingListIdx < dsBindingList.size()) &&
                                   (dsBindingList[dsBindingListIdx].binding == texInfo.logicalBinding);
                SNAP_RHI_VALIDATE(
                    validationLayer,
                    found,
                    snap::rhi::ReportLevel::Error,
                    snap::rhi::ValidationTag::DescriptorSetOp,
                    "[PipelineTextureSamplerState::setAllStates] texture: {%lld} not initialized for rendering",
                    texInfo.logicalBinding);
                if (!found) {
                    return;
                }

                const auto& textureBinding = dsBindingList[dsBindingListIdx];
                texture = textureBinding.texture;
            }

            if (!texture) {
                SNAP_RHI_REPORT(
                    validationLayer,
                    snap::rhi::ReportLevel::Error,
                    snap::rhi::ValidationTag::DescriptorSetOp,
                    "[PipelineTextureSamplerState::setAllStates] texture: {%lld} not initialized for rendering",
                    texInfo.logicalBinding);
                return;
            }

            const snap::rhi::backend::opengl::TextureTarget target = texture->getTarget();

            if (texBindings[texInfo.binding].texture == texture && texBindings[texInfo.binding].target == target) {
                continue;
            }

            texBindings[texInfo.binding].texture = texture;
            texBindings[texInfo.binding].target = target;
            toUpdateUnits.set(texInfo.binding);
        }
    }
}

void PipelineTextureSamplerState::bindSamplers(std::bitset<MaxTextureSamplerBinding>& toUpdateUnits) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(gl.getDevice(),
                                            "[PipelineTextureSamplerState][setAllStates][bindTextures]");

    assert(pipeline != nullptr);
    const auto* device = gl.getDevice();
    const auto& validationLayer = device->getValidationLayer();

    const auto& samplersDesc = pipeline->getPipelineSamplerDescription();
    for (size_t i = 0; i < samplersDesc.size(); ++i) {
        const auto& dsDesc = samplersDesc[i];
        const auto& dsBindingList = smplBindingsList[i];
        const auto& dsActiveSamplers = activeSamplers[i];
        size_t dsBindingListIdx = 0;

        for (const auto& smplInfo : dsDesc) {
            const snap::rhi::backend::opengl::Sampler* sampler = nullptr;
            if (smplInfo.logicalBinding < MAX_TEXTURE_SAMPLER_CACHE_SIZE) {
                sampler = dsActiveSamplers[smplInfo.logicalBinding];
            } else {
                while ((dsBindingListIdx < dsBindingList.size()) &&
                       (dsBindingList[dsBindingListIdx].binding < smplInfo.logicalBinding)) {
                    ++dsBindingListIdx;
                }

                const bool found = (dsBindingListIdx < dsBindingList.size()) &&
                                   (dsBindingList[dsBindingListIdx].binding == smplInfo.logicalBinding);
                SNAP_RHI_VALIDATE(
                    validationLayer,
                    found,
                    snap::rhi::ReportLevel::Error,
                    snap::rhi::ValidationTag::DescriptorSetOp,
                    "[PipelineTextureSamplerState::setAllStates] sampler: {%lld} not initialized for rendering",
                    smplInfo.logicalBinding);
                if (!found) {
                    return;
                }

                const auto& samplerBinding = dsBindingList[dsBindingListIdx];
                sampler = samplerBinding.sampler;
            }

            if (!sampler) {
                SNAP_RHI_REPORT(
                    validationLayer,
                    snap::rhi::ReportLevel::Error,
                    snap::rhi::ValidationTag::DescriptorSetOp,
                    "[PipelineTextureSamplerState::setAllStates] sampler: {%lld} not initialized for rendering",
                    smplInfo.logicalBinding);
                return;
            }

            for (const auto binding : smplInfo.binding) {
                assert(binding < smplBindings.size());
                if (smplBindings[binding] == sampler) {
                    continue;
                }

                smplBindings[binding] = sampler;
                toUpdateUnits.set(binding);
            }
        }
    }
}

void PipelineTextureSamplerState::setAllStates(DeviceContext* dc) {
    assert(dc != nullptr);
    if (!shouldBind) {
        return;
    }

    std::bitset<MaxTextureSamplerBinding> toUpdateUnits;

    bindTextures(toUpdateUnits);
    bindSamplers(toUpdateUnits);

    {
        SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(gl.getDevice(),
                                                "[PipelineTextureSamplerState][setAllStates][native bind]");

        /**
         * To remain compatible with Legacy Scenarium rendering, SnapRHI must bind all textures in this order.
         */
        for (int32_t unit = static_cast<int32_t>(toUpdateUnits.size()) - 1; unit >= 0; --unit) {
            if (toUpdateUnits[unit]) {
                const auto target = texBindings[unit].target;
                const snap::rhi::backend::opengl::Texture* texture = texBindings[unit].texture;
                const snap::rhi::backend::opengl::Sampler* sampler = smplBindings[unit];

                if (texture && sampler) {
                    sampler->bindTextureSampler(static_cast<GLuint>(unit), texture, target, dc);
                }
            }
        }
    }
    shouldBind = false;
}

void PipelineTextureSamplerState::clearStates() {
    texBindings.fill({nullptr, TextureTarget::Detached});
    smplBindings.fill(nullptr);

    for (auto& binding : texBindingsList) {
        binding.clear();
    }

    for (auto& binding : smplBindingsList) {
        binding.clear();
    }

    for (auto& textures : activeTextures) {
        textures.fill(nullptr);
    }

    for (auto& samplers : activeSamplers) {
        samplers.fill(nullptr);
    }
}
} // namespace snap::rhi::backend::opengl
