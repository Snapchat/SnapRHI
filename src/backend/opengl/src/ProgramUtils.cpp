#include "snap/rhi/backend/opengl/ProgramUtils.hpp"
#include "snap/rhi/backend/opengl/Device.hpp"
#include "snap/rhi/backend/opengl/PipelineCache.hpp"

namespace {
std::string getProgramStatusLog(const snap::rhi::backend::opengl::Profile& gl, const GLuint program) {
    GLint logLength = 0;
    gl.getProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
    std::string log;

    if (logLength > 0) {
        log = std::string(logLength + 1, 0);
        gl.getProgramInfoLog(program, logLength, &logLength, log.data());
    }

    return log;
}

bool checkProgramStatus(const snap::rhi::backend::opengl::Profile& gl, const GLuint program) {
    GLint status = 0;
    gl.getProgramiv(program, GL_LINK_STATUS, &status);

    return status == GL_TRUE;
}
} // unnamed namespace

namespace snap::rhi::backend::opengl {
GLuint createProgramFromShaders(const Profile& gl,
                                const std::span<snap::rhi::ShaderModule*>& shaderStages,
                                bool isProgramBinaryRetrievable) {
    GLuint program = gl.createProgram();
    bool isOK = true;

    assert(program);
    std::vector<GLuint> shaders;

    {
        SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(gl.getDevice(), "[createProgram][attachShader]");

        for (const auto& shaderModule : shaderStages) {
            if (shaderModule) {
                auto* glShaderModule =
                    snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::ShaderModule>(shaderModule);

                const std::variant<GLuint, std::string>& shader = glShaderModule->getGLShaderIdOrErrorMessage();
                const GLuint* shaderIDPtr = std::get_if<GLuint>(&shader);

                if ((shaderIDPtr != nullptr) && (*shaderIDPtr != GL_NONE)) {
                    gl.attachShader(program, *shaderIDPtr);
                    shaders.push_back(*shaderIDPtr);
                } else {
                    isOK = false;
                    break;
                }
            }
        }
    }

    if (isOK) {
        {
            SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(gl.getDevice(), "[createProgram][linkProgram]");
            gl.linkProgram(program);
        }

        if (isProgramBinaryRetrievable && gl.getFeatures().isProgramBinarySupported) {
            gl.programParameteri(program, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);
        }
    }

    {
        SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(gl.getDevice(), "[createProgram][detach]");

        for (const auto shader : shaders) {
            gl.detachShader(program, shader);
        }
    }

    return program;
}

std::variant<GLuint, std::string> validateProgramLinkStatus(const Profile& gl,
                                                            GLuint& program,
                                                            std::span<snap::rhi::ShaderModule*> shaderStages) {
    const auto& validationLayer = gl.getDevice()->getValidationLayer();

    std::string errorLogs;
    {
        for (const auto& shaderModule : shaderStages) {
            if (shaderModule) {
                auto* glShaderModule =
                    snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::ShaderModule>(shaderModule);

                const std::variant<GLuint, std::string>& shader = glShaderModule->getGLShaderIdOrErrorMessage();
                const GLuint* shaderIDPtr = std::get_if<GLuint>(&shader);
                const std::string* shaderErrorMessagePtr = std::get_if<std::string>(&shader);

                if (shaderErrorMessagePtr != nullptr) {
                    errorLogs += *shaderErrorMessagePtr;
                    break;
                } else if ((shaderIDPtr == nullptr) || (*shaderIDPtr == GL_NONE)) {
                    errorLogs += "[validateProgramLinkStatus] cannot attach shader";
                    break;
                }
            }
        }
    }

    bool isOK = errorLogs.empty();
    if (isOK) {
        isOK = checkProgramStatus(gl, program);

        std::string log =
            (snap::rhi::backend::common::enableSlowSafetyChecks() || !isOK) ? getProgramStatusLog(gl, program) : "";
        if (!log.empty()) {
            SNAP_RHI_REPORT(validationLayer,
                            snap::rhi::ReportLevel::Warning,
                            snap::rhi::ValidationTag::GLProgramValidationOp,
                            "Program link log: %s\n",
                            log.c_str());
        }

        if (!isOK) {
            errorLogs += "[validateProgramLinkStatus] Program link log: " + log;
        }
    }

    if (!isOK) {
        gl.deleteProgram(program);
        program = GL_NONE;

        return errorLogs;
    }

    return program;
}

GLuint tryLoadFromCache(const Profile& gl,
                        snap::rhi::PipelineCache* pipelineCache,
                        const snap::rhi::backend::opengl::hash64 pipelineSrcHash) {
    const auto& features = gl.getFeatures();
    const auto& validationLayer = gl.getDevice()->getValidationLayer();

    GLuint programID = GL_NONE;
    if (pipelineCache && features.isProgramBinarySupported) {
        auto* glPipelineCache =
            snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::PipelineCache>(pipelineCache);
        PipelineCacheValue pipelineCacheValue = glPipelineCache->load(pipelineSrcHash);
        if (pipelineCacheValue.binaryFormat != GL_NONE) {
            programID = gl.createProgram();
            assert(programID);

            SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(gl.getDevice(), "[Pipeline][programBinary]");
            gl.programBinary(programID,
                             pipelineCacheValue.binaryFormat,
                             pipelineCacheValue.data.data(),
                             static_cast<GLsizei>(pipelineCacheValue.data.size()));

            if (!checkProgramStatus(gl, programID)) {
                std::string log = getProgramStatusLog(gl, programID);
                SNAP_RHI_REPORT(validationLayer,
                                snap::rhi::ReportLevel::Warning,
                                snap::rhi::ValidationTag::GLProgramValidationOp,
                                "Program loadFromCache log: %s\n",
                                log.c_str());

                gl.deleteProgram(programID);
                programID = GL_NONE;
            }
        }
    }

    return programID;
}
} // namespace snap::rhi::backend::opengl
