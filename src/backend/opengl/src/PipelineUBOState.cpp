#include "snap/rhi/backend/opengl/PipelineUBOState.hpp"
#include "snap/rhi/backend/common/Math.hpp"
#include "snap/rhi/backend/common/Utils.hpp"

#include "snap/rhi/backend/opengl/Buffer.hpp"
#include "snap/rhi/backend/opengl/Device.hpp"
#include "snap/rhi/backend/opengl/DeviceContext.hpp"
#include "snap/rhi/backend/opengl/OpenGL.h"
#include "snap/rhi/backend/opengl/Pipeline.hpp"
#include "snap/rhi/backend/opengl/Profile.hpp"
#include "snap/rhi/backend/opengl/UniformUtils.hpp"

#include "snap/rhi/common/Throw.h"
#include <snap/rhi/common/Scope.h>

namespace snap::rhi::backend::opengl {
PipelineUBOState::PipelineUBOState(snap::rhi::backend::opengl::Device* device) : gl(device->getOpenGL()) {
    clearStates();
}

void PipelineUBOState::setAllStates(DeviceContext* dc) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(gl.getDevice(), "[PipelineUBOState][setAllStates]");

    assert(pipeline != nullptr);
    assert(dc != nullptr);

    const auto* device = gl.getDevice();
    const auto& validationLayer = device->getValidationLayer();

    if (pipeline->getPipelineUniformManagmentType() == PipelineUniformManagmentType::Legacy) {
        const auto& dsInfo = bindingsList[DefaultDescriptorSetID];
        if (dsInfo.empty()) {
            /**
             * It mean there is no bound UBO
             */
            return;
        }

        assert(dsInfo.size() == 1);
        assert(dsInfo[0].binding == LegacyUBOBinding);

        const PipelineUBOState::BufferBinding& legacyBufferInfo = dsInfo[0].info;

        if (bindings[0].offset == legacyBufferInfo.offset && bindings[0].buffer == legacyBufferInfo.buffer) {
            return;
        }
        bindings[0] = legacyBufferInfo;

        snap::rhi::backend::opengl::Buffer* legacyBuffer = legacyBufferInfo.buffer;
        if (!legacyBuffer) {
            return;
        }

        const uint64_t bufferSize = legacyBuffer->getCreateInfo().size;
        assert(legacyBufferInfo.offset < bufferSize);

        assert(((bufferSize - legacyBufferInfo.offset) & 3) == 0);
        const auto* data = legacyBuffer->map(snap::rhi::MemoryAccess::Read, 0, WholeSize, dc) + legacyBufferInfo.offset;
        SNAP_RHI_ON_SCOPE_EXIT {
            legacyBuffer->unmap(dc);
        };

        const uint64_t uboSize = bufferSize - legacyBufferInfo.offset;

        pipeline->bindLegacyUBO(dc, {reinterpret_cast<const uint8_t*>(data), uboSize});
    } else if (pipeline->getPipelineUniformManagmentType() == PipelineUniformManagmentType::Compatible) {
        const auto& ubosDesc = pipeline->getPipelineCompatibleUBODescription();
        for (size_t i = 0; i < ubosDesc.size(); ++i) {
            const auto& dsDesc = ubosDesc[i];
            const auto& dsBindingList = bindingsList[i];
            size_t dsBindingListIdx = 0;

            for (const auto& uboInfo : dsDesc) {
                if (uboInfo.location == snap::rhi::backend::opengl::InvalidLocation) {
                    continue;
                }
                assert(uboInfo.location < bindings.size());

                while ((dsBindingListIdx < dsBindingList.size()) &&
                       (dsBindingList[dsBindingListIdx].binding < uboInfo.logicalBinding)) {
                    ++dsBindingListIdx;
                }

                const bool found = (dsBindingListIdx < dsBindingList.size()) &&
                                   (dsBindingList[dsBindingListIdx].binding == uboInfo.logicalBinding);
                SNAP_RHI_VALIDATE(validationLayer,
                                  found,
                                  snap::rhi::ReportLevel::Error,
                                  snap::rhi::ValidationTag::DescriptorSetOp,
                                  "[PipelineUBOState::setAllStates] ubo: {%lld} not initialized for rendering",
                                  uboInfo.logicalBinding);
                if (!found) {
                    return;
                }

                const auto& bufferBinding = dsBindingList[dsBindingListIdx];
                if (bindings[uboInfo.location].offset == bufferBinding.info.offset &&
                    bindings[uboInfo.location].buffer == bufferBinding.info.buffer) {
                    continue;
                }
                bindings[uboInfo.location] = bufferBinding.info;

                const auto buffer = bufferBinding.info.buffer;
                if (buffer) { // Binding
                    const auto location = uboInfo.location;
                    const auto offset = bufferBinding.info.offset;

                    [[maybe_unused]] const auto& uniformBufferInfo = buffer->getCreateInfo();
                    assert((uniformBufferInfo.size - offset) >= uboInfo.arraySize * 16u);

                    const auto* data = buffer->map(snap::rhi::MemoryAccess::Read, 0, WholeSize, dc) + offset;
                    SNAP_RHI_ON_SCOPE_EXIT {
                        buffer->unmap(dc);
                    };
                    switch (uboInfo.bufferType) {
                        case snap::rhi::backend::opengl::UniformBufferType::Float: {
                            gl.uniform4fv(location, uboInfo.arraySize, reinterpret_cast<const GLfloat*>(data));
                        } break;

                        case snap::rhi::backend::opengl::UniformBufferType::Int: {
                            gl.uniform4iv(location, uboInfo.arraySize, reinterpret_cast<const GLint*>(data));
                        } break;

                        default:
                            snap::rhi::common::throwException("invalid uniform buffer type");
                    }
                }
            }
        }
    } else { // Native UBO
        const auto& ubosDesc = pipeline->getPipelineUBODescription();
        for (size_t i = 0; i < ubosDesc.size(); ++i) {
            const auto& dsDesc = ubosDesc[i];
            const auto& dsBindingList = bindingsList[i];
            size_t dsBindingListIdx = 0;

            for (const auto& uboInfo : dsDesc) {
                assert(uboInfo.binding < bindings.size());

                while ((dsBindingListIdx < dsBindingList.size()) &&
                       (dsBindingList[dsBindingListIdx].binding < uboInfo.logicalBinding)) {
                    ++dsBindingListIdx;
                }

                const bool found = (dsBindingListIdx < dsBindingList.size()) &&
                                   (dsBindingList[dsBindingListIdx].binding == uboInfo.logicalBinding);
                SNAP_RHI_VALIDATE(validationLayer,
                                  found,
                                  snap::rhi::ReportLevel::Error,
                                  snap::rhi::ValidationTag::DescriptorSetOp,
                                  "[PipelineUBOState::setAllStates] ubo: {%lld} not initialized for rendering",
                                  uboInfo.logicalBinding);
                if (!found) {
                    return;
                }

                const auto& bufferBinding = dsBindingList[dsBindingListIdx];
                if (bindings[uboInfo.binding].offset == bufferBinding.info.offset &&
                    bindings[uboInfo.binding].buffer == bufferBinding.info.buffer) {
                    continue;
                }
                bindings[uboInfo.binding] = bufferBinding.info;

                const auto buffer = bufferBinding.info.buffer;
                if (buffer) { // Binding
                    const auto& info = buffer->getCreateInfo();
                    const auto binding = uboInfo.binding;
                    const auto offset = bufferBinding.info.offset;

                    SNAP_RHI_VALIDATE(validationLayer,
                                      info.size > offset,
                                      snap::rhi::ReportLevel::Error,
                                      snap::rhi::ValidationTag::DescriptorSetOp,
                                      "[PipelineUBOState::setAllStates] info.size: {%lld} <= offset{%lld}",
                                      info.size,
                                      offset);
                    const auto range =
                        bufferBinding.info.range == WholeSize ? info.size - offset : bufferBinding.info.range;

                    SNAP_RHI_VALIDATE(validationLayer,
                                      (range + offset) <= info.size,
                                      snap::rhi::ReportLevel::Error,
                                      snap::rhi::ValidationTag::DescriptorSetOp,
                                      "[PipelineUBOState::setAllStates] (range{%lld} + offset{%lld}) > info.size{%lld}",
                                      range,
                                      offset,
                                      info.size);
                    gl.bindBufferRange(
                        GL_UNIFORM_BUFFER, binding, buffer->getOrAllocateGLBuffer(dc), offset, range, dc);
                }
            }
        }
    }
}

void PipelineUBOState::clearStates() {
    bindings.fill({nullptr, 0});

    for (auto& binding : bindingsList) {
        binding.clear();
    }
}
} // namespace snap::rhi::backend::opengl
