#include "snap/rhi/backend/opengl/ProgramState.hpp"
#include "snap/rhi/backend/common/Math.hpp"
#include "snap/rhi/backend/opengl/AttribsUtils.hpp"
#include "snap/rhi/backend/opengl/Device.hpp"
#include "snap/rhi/backend/opengl/Profile.hpp"
#include "snap/rhi/backend/opengl/ProgramUtils.hpp"
#include "snap/rhi/backend/opengl/UniformUtils.hpp"

#include <snap/rhi/common/Scope.h>

namespace snap::rhi::backend::opengl {
ProgramState::ProgramState(const Profile& gl,
                           const GLuint programID,
                           const bool isValidated,
                           const bool acquireNativeReflection,
                           bool isLegacyPipeline)
    : device(common::smart_cast<Device>(gl.getDevice())),
      gl(gl),
      programID(programID),
      isValidated(isValidated),
      acquireNativeReflection(acquireNativeReflection),
      isLegacyPipeline(isLegacyPipeline) {}

ProgramState::~ProgramState() {
    if (programID != GL_NONE && gl.isProgram(programID)) {
        gl.deleteProgram(programID);
        programID = 0;
    }
}

void ProgramState::validate() const {
    if (isValidated) {
        return;
    }

    SNAP_RHI_ON_SCOPE_EXIT {
        isValidated = true;
    };

    auto programOrErrorMessage = validateProgramLinkStatus(gl, programID, {});
    const std::string* programErrorMessagePtr = std::get_if<std::string>(&programOrErrorMessage);

    SNAP_RHI_VALIDATE(device->getValidationLayer(),
                      !programErrorMessagePtr,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::GLProgramValidationOp,
                      "[ProgramState::validate] Pipeline compile log: %s\n",
                      programErrorMessagePtr ? programErrorMessagePtr->c_str() : "");
}

void ProgramState::initPipelineInfo(const PipelineConfigurationInfo& info) {
    validate();

    if (!pipelineNativeInfo.has_value()) {
        if (programID == GL_NONE) {
            snap::rhi::common::throwException(
                "[ProgramState][initPipelineDescriptorSets] program - GL_NONE, this means that the "
                "program was not compiled");
        }

        pipelineNativeInfo = buildPipelineNativeInfo(gl, programID, acquireNativeReflection, isLegacyPipeline, info);
    }
}

void ProgramState::useProgram(snap::rhi::backend::opengl::DeviceContext* dc) const {
    validate();

    if (programID == GL_NONE) {
        snap::rhi::common::throwException(
            "[ProgramState] active program - GL_NONE, this means that the program was not compiled");
    }

    gl.useProgram(programID, dc);
}

std::vector<uint8_t>& ProgramState::getLegacyUBOState(DeviceContextUUID dcUUID) {
    if (this->dcUUID != dcUUID) {
        this->dcUUID = dcUUID;
        legacyUBOState.clear();
    }

    return legacyUBOState;
}
} // namespace snap::rhi::backend::opengl
