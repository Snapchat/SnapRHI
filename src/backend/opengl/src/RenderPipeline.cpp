#include "snap/rhi/backend/opengl/RenderPipeline.hpp"
#include "snap/rhi/backend/opengl/AttribsUtils.hpp"
#include "snap/rhi/backend/opengl/Device.hpp"
#include "snap/rhi/backend/opengl/ErrorCheckGuard.hpp"
#include "snap/rhi/backend/opengl/Profile.hpp"
#include "snap/rhi/backend/opengl/ProgramUtils.hpp"
#include "snap/rhi/backend/opengl/ShaderModule.hpp"
#include "snap/rhi/backend/opengl/UniformUtils.hpp"
#include "snap/rhi/common/Throw.h"

namespace {
bool isLegacyPipeline(std::span<snap::rhi::ShaderModule* const> stages) {
    for (snap::rhi::ShaderModule* stage : stages) {
        if (stage) {
            snap::rhi::backend::opengl::ShaderModule* glesShader =
                snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::ShaderModule>(stage);

            if (glesShader->doesShaderContainRegularUniformsOnly()) {
                return true;
            }
        }
    }

    return false;
}
} // unnamed namespace

namespace snap::rhi::backend::opengl {
RenderPipeline::RenderPipeline(Device* glesDevice,
                               const snap::rhi::RenderPipelineCreateInfo& info,
                               const RenderPipelineStatesUUID& stateUUID)
    : snap::rhi::RenderPipeline(glesDevice, info),
      Pipeline(glesDevice,
               info.pipelineCreateFlags,
               isLegacyPipeline(this->info.stages),
               RenderPipelineConfigurationInfo{.layout = info.vertexInputState,
                                               .renderPipelineInfo = info.glRenderPipelineInfo},
               this->info.stages,
               info.basePipeline ? snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::RenderPipeline>(
                                       info.basePipeline) :
                                   nullptr,
               info.pipelineCache),
      gl(glesDevice->getOpenGL()),
      stateUUID(stateUUID) {}

RenderPipeline::RenderPipeline(Device* glesDevice, bool isLegacyPipeline, GLuint programID)
    : snap::rhi::RenderPipeline(glesDevice, {}),
      Pipeline(glesDevice, isLegacyPipeline, programID),
      gl(glesDevice->getOpenGL()) {}

void RenderPipeline::syncImpl() const {
    Pipeline::syncImpl();

    const GLuint programID = programState->getProgramID();
    if (programID == GL_NONE) {
        return;
    }

    reflection = snap::rhi::reflection::RenderPipelineInfo{.descriptorSetInfos = descriptorSetInfos,
                                                           .vertexAttributes = vertexAttributeInfos};
}

const std::optional<reflection::RenderPipelineInfo>& RenderPipeline::getReflectionInfo() const {
    sync();
    return snap::rhi::RenderPipeline::getReflectionInfo();
}

void RenderPipeline::setDebugLabel(std::string_view label) {
#if SNAP_RHI_ENABLE_DEBUG_LABELS
    ObjectDebugMarkers::setDebugLabel(label);
    const auto& ps = getProgramState();
    if (ps) {
        const GLuint programID = ps->getProgramID();
        if (programID != GL_NONE) {
            gl.objectLabel(GL_PROGRAM, programID, label);
        }
    }
#endif
}
} // namespace snap::rhi::backend::opengl
