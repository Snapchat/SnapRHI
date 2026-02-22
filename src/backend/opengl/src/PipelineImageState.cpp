#include "snap/rhi/backend/opengl/PipelineImageState.hpp"
#include "snap/rhi/backend/common/Math.hpp"
#include "snap/rhi/backend/common/Utils.hpp"

#include "snap/rhi/backend/opengl/Device.hpp"
#include "snap/rhi/backend/opengl/OpenGL.h"
#include "snap/rhi/backend/opengl/Pipeline.hpp"
#include "snap/rhi/backend/opengl/Profile.hpp"
#include "snap/rhi/backend/opengl/Sampler.hpp"
#include "snap/rhi/backend/opengl/Texture.hpp"

#include "snap/rhi/common/Throw.h"

namespace snap::rhi::backend::opengl {
PipelineImageState::PipelineImageState(snap::rhi::backend::opengl::Device* device) : gl(device->getOpenGL()) {
    clearStates();
}

void PipelineImageState::setPipeline(const snap::rhi::backend::opengl::Pipeline* pipeline) {
    assert(pipeline != nullptr);
    this->pipeline = pipeline;
}

void PipelineImageState::invalidateDescriptorSetBindings(const uint32_t dsID) {
    assert(dsID < snap::rhi::SupportedLimit::MaxBoundDescriptorSets);

    bindingsList[dsID].clear();
}

void PipelineImageState::bindImage(const uint32_t dsID,
                                   const uint32_t binding,
                                   const snap::rhi::backend::opengl::Texture* image,
                                   const uint32_t mipLevel) {
    assert(dsID < snap::rhi::SupportedLimit::MaxBoundDescriptorSets);
    assert(bindingsList[dsID].empty() || bindingsList[dsID].back().binding < binding);

    bindingsList[dsID].push_back({binding, {image, mipLevel, GL_NONE}});
}

void PipelineImageState::setAllStates(DeviceContext* dc) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(gl.getDevice(), "[PipelineImageState][setAllStates]");

    assert(pipeline != nullptr);
    assert(dc != nullptr);

    const auto* device = gl.getDevice();
    const auto& validationLayer = device->getValidationLayer();

    const auto& imagesDesc = pipeline->getPipelineImageDescription();
    for (size_t i = 0; i < imagesDesc.size(); ++i) {
        const auto& dsDesc = imagesDesc[i];
        const auto& dsBindingList = bindingsList[i];
        size_t dsBindingListIdx = 0;

        for (const auto& imageInfo : dsDesc) {
            assert(imageInfo.unit < bindings.size());

            while ((dsBindingListIdx < dsBindingList.size()) &&
                   (dsBindingList[dsBindingListIdx].binding < imageInfo.logicalBinding)) {
                ++dsBindingListIdx;
            }

            const bool found = (dsBindingListIdx < dsBindingList.size()) &&
                               (dsBindingList[dsBindingListIdx].binding == imageInfo.logicalBinding);
            SNAP_RHI_VALIDATE(validationLayer,
                              found,
                              snap::rhi::ReportLevel::Error,
                              snap::rhi::ValidationTag::DescriptorSetOp,
                              "[PipelineImageState::setAllStates] image: {%lld} not initialized for rendering",
                              imageInfo.logicalBinding);
            if (!found) {
                return;
            }

            const auto& imageBinding = dsBindingList[dsBindingListIdx];
            if (bindings[imageInfo.unit].mipLevel == imageBinding.info.mipLevel &&
                bindings[imageInfo.unit].image == imageBinding.info.image &&
                bindings[imageInfo.unit].access == imageInfo.access) {
                continue;
            }
            bindings[imageInfo.unit] = imageBinding.info;
            bindings[imageInfo.unit].access = imageBinding.info.access;

            const snap::rhi::backend::opengl::Texture* texture = imageBinding.info.image;
            if (texture) { // Binding
                const uint32_t mipLevel = imageBinding.info.mipLevel;
                const auto access = imageInfo.access;

                gl.bindImageTexture(imageInfo.unit,
                                    static_cast<GLuint>(texture->getTextureID(dc)),
                                    static_cast<GLint>(mipLevel),
                                    GL_TRUE,
                                    0,
                                    access,
                                    static_cast<GLenum>(texture->getInternalFormat()));
            }
        }
    }
}

void PipelineImageState::clearStates() {
    for (auto& binding : bindings) {
        binding = {nullptr, 0, GL_NONE};
    }

    for (auto& binding : bindingsList) {
        binding.clear();
    }
}
} // namespace snap::rhi::backend::opengl
