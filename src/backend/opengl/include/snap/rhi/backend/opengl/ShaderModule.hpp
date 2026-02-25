//
//  ShaderModule.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 08.02.2022.
//

#pragma once

#include "snap/rhi/ShaderModule.hpp"
#include "snap/rhi/backend/opengl/HashUtils.hpp"
#include "snap/rhi/backend/opengl/OpenGL.h"
#include "snap/rhi/backend/opengl/ShaderUtils.hpp"
#include <variant>

namespace snap::rhi {
class ShaderLibrary;
} // namespace snap::rhi

namespace snap::rhi::backend::opengl {
class Device;
class Profile;

// https://www.khronos.org/opengl/wiki/Shader_Compilation#Program_pipelines
class ShaderModule final : public snap::rhi::ShaderModule {
public:
    /**
     * doesShaderContainUniforms - means that the shader will contain only regulat uniforms(e.g 'uniform type name;' ),
     * instead of UBO's.
     *
     * SnapRHI currently only supports these 2 shader types:
     * - Shader that only used regular uniforms (no UBOs);
     * - Shader that only used UBOs for ES3.0+ or vec4/ivec4 arrays(as packed UBO) for ES2.0.
     *
     *
     * SnapRHI only supported uniform block or regular uniform shaders, not both shaders in the same shader.
     * https://www.khronos.org/opengl/wiki/Interface_Block_(GLSL)#Uniform_blocks
     * https://www.khronos.org/opengl/wiki/Uniform_(GLSL)
     */
    explicit ShaderModule(Device* glesDevice,
                          const snap::rhi::ShaderStage shaderStage,
                          std::string_view src,
                          bool doesShaderContainUniformsOnly);
    explicit ShaderModule(Device* glesDevice, const snap::rhi::ShaderModuleCreateInfo& info);
    ~ShaderModule() override;

    void setDebugLabel(std::string_view label) override;

    bool doesShaderContainRegularUniformsOnly() const;
    void validateShader() const;

    snap::rhi::backend::opengl::hash64 getShaderSrcHash() const;

    const std::variant<GLuint, std::string>& getGLShaderIdOrErrorMessage() const {
        tryCompileShader();

        return shaderIdOrErrorMessage;
    }

private:
    void tryCompileShader() const;

private:
    const Profile& gl;

    std::array<std::string, static_cast<uint32_t>(snap::rhi::backend::opengl::ShaderCodeBlock::Count)> source{};

    mutable bool isValidated = false;
    mutable std::variant<GLuint, std::string> shaderIdOrErrorMessage = static_cast<GLuint>(GL_NONE);
    mutable snap::rhi::backend::opengl::hash64 shaderSrcHash = 0;

    bool doesShaderContainUniformsOnly = false;
};
} // namespace snap::rhi::backend::opengl
