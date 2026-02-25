// Copyright © 2023 Snap, Inc. All rights reserved.

#pragma once

#include "snap/rhi/backend/opengl/DeviceContext.hpp"
#include "snap/rhi/backend/opengl/OpenGL.h"
#include "snap/rhi/backend/opengl/UniformsDescription.h"
#include "snap/rhi/reflection/Info.hpp"
#include <optional>

namespace snap::rhi::backend::opengl {
class Profile;
class Device;
class DeviceContext;

class ProgramState final {
public:
    ProgramState(const Profile& gl,
                 const GLuint programID,
                 const bool isValidated,
                 const bool acquireNativeReflection,
                 bool isLegacyPipeline);
    ~ProgramState();

    GLuint getProgramID() const {
        return programID;
    }

    // Assign physical bindings if necessary, and build info about resources physical bindings
    void initPipelineInfo(const PipelineConfigurationInfo& info);

    const PipelineNativeInfo& getPipelineNativeInfo() const {
        return *pipelineNativeInfo;
    }

    void useProgram(snap::rhi::backend::opengl::DeviceContext* dc) const;

    std::vector<uint8_t>& getLegacyUBOState(DeviceContextUUID dcUUID);

private:
    void validate() const;

    snap::rhi::backend::opengl::Device* device = nullptr;
    const Profile& gl;

    mutable GLuint programID = GL_NONE;
    mutable std::optional<PipelineNativeInfo> pipelineNativeInfo = std::nullopt;

    mutable bool isValidated = false;
    const bool acquireNativeReflection = false;
    const bool isLegacyPipeline = false;

    std::vector<uint8_t> legacyUBOState;
    DeviceContextUUID dcUUID = UndefinedDeviceContextUUID; // gl uniforms cache stored in current OpenGL context
};
} // namespace snap::rhi::backend::opengl
