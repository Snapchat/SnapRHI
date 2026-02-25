#include "snap/rhi/backend/opengl/ComputePipeline.hpp"

#include "snap/rhi/backend/opengl/Device.hpp"
#include "snap/rhi/backend/opengl/ErrorCheckGuard.hpp"
#include "snap/rhi/backend/opengl/Profile.hpp"
#include "snap/rhi/backend/opengl/ProgramUtils.hpp"
#include "snap/rhi/backend/opengl/ShaderModule.hpp"
#include "snap/rhi/backend/opengl/UniformUtils.hpp"
#include "snap/rhi/common/Throw.h"

namespace {
} // unnamed namespace

namespace snap::rhi::backend::opengl {
ComputePipeline::ComputePipeline(Device* glesDevice, const snap::rhi::ComputePipelineCreateInfo& info)
    : snap::rhi::ComputePipeline(glesDevice, info),
      Pipeline(glesDevice,
               info.pipelineCreateFlags,
               false,
               ComputePipelineConfigurationInfo{.computePipelineInfo = info.glPipelineInfo},
               {&this->info.stage, 1},
               info.basePipeline ? snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::ComputePipeline>(
                                       info.basePipeline) :
                                   nullptr,
               info.pipelineCache) {}

void ComputePipeline::syncImpl() const {
    Pipeline::syncImpl();

    const GLuint programID = programState->getProgramID();
    if (programID == GL_NONE) {
        return;
    }

    reflection = snap::rhi::reflection::ComputePipelineInfo{.descriptorSetInfos = this->descriptorSetInfos};
}

const std::optional<reflection::ComputePipelineInfo>& ComputePipeline::getReflectionInfo() const {
    sync();
    return snap::rhi::ComputePipeline::getReflectionInfo();
}

void ComputePipeline::setDebugLabel(std::string_view label) {
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
