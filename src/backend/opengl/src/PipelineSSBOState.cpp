#include "snap/rhi/backend/opengl/PipelineSSBOState.hpp"
#include "snap/rhi/backend/common/Math.hpp"
#include "snap/rhi/backend/common/Utils.hpp"

#include "snap/rhi/backend/opengl/Buffer.hpp"
#include "snap/rhi/backend/opengl/Device.hpp"
#include "snap/rhi/backend/opengl/OpenGL.h"
#include "snap/rhi/backend/opengl/Pipeline.hpp"
#include "snap/rhi/backend/opengl/Profile.hpp"
#include "snap/rhi/backend/opengl/UniformUtils.hpp"

#include "snap/rhi/common/Throw.h"

namespace snap::rhi::backend::opengl {
PipelineSSBOState::PipelineSSBOState(snap::rhi::backend::opengl::Device* device) : gl(device->getOpenGL()) {
    clearStates();
}

void PipelineSSBOState::setAllStates(DeviceContext* dc) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(gl.getDevice(), "[PipelineSSBOState][setAllStates]");

    assert(pipeline != nullptr);
    assert(dc != nullptr);

    const auto* device = gl.getDevice();
    const auto& validationLayer = device->getValidationLayer();

    const auto& ssbosDesc = pipeline->getPipelineSSBODescription();
    for (size_t i = 0; i < ssbosDesc.size(); ++i) {
        const auto& dsDesc = ssbosDesc[i];
        const auto& dsBindingList = bindingsList[i];
        size_t dsBindingListIdx = 0;

        for (const auto& ssboInfo : dsDesc) {
            assert(ssboInfo.binding < bindings.size());

            while ((dsBindingListIdx < dsBindingList.size()) &&
                   (dsBindingList[dsBindingListIdx].binding < ssboInfo.logicalBinding)) {
                ++dsBindingListIdx;
            }

            const bool found = (dsBindingListIdx < dsBindingList.size()) &&
                               (dsBindingList[dsBindingListIdx].binding == ssboInfo.logicalBinding);
            SNAP_RHI_VALIDATE(validationLayer,
                              found,
                              snap::rhi::ReportLevel::Error,
                              snap::rhi::ValidationTag::DescriptorSetOp,
                              "[PipelineSSBOState::setAllStates] ssbo: {%lld} not initialized for rendering",
                              ssboInfo.logicalBinding);
            if (!found) {
                return;
            }

            const auto& bufferBinding = dsBindingList[dsBindingListIdx];
            if (bindings[ssboInfo.binding].offset == bufferBinding.info.offset &&
                bindings[ssboInfo.binding].buffer == bufferBinding.info.buffer) {
                continue;
            }
            bindings[ssboInfo.binding] = bufferBinding.info;

            const auto buffer = bufferBinding.info.buffer;
            if (buffer) { // Binding
                const auto binding = ssboInfo.binding;
                const auto offset = bufferBinding.info.offset;
                const auto size = buffer->getCreateInfo().size;

                assert(offset < size);
                gl.bindBufferRange(
                    GL_SHADER_STORAGE_BUFFER, binding, buffer->getGLBuffer(dc), offset, size - offset, dc);
            }
        }
    }
}

void PipelineSSBOState::clearStates() {
    bindings.fill({nullptr, 0});

    for (auto& binding : bindingsList) {
        binding.clear();
    }
}
} // namespace snap::rhi::backend::opengl
