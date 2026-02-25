#include "snap/rhi/backend/opengl/ShaderUtils.hpp"

#include "snap/rhi/backend/opengl/Device.hpp"

#include "snap/rhi/common/Throw.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace {
constexpr std::string_view SpecConstName{"SNAP_RHI_SPECIALIZATION_CONSTANT_"};
constexpr std::array<GLenum, static_cast<uint8_t>(snap::rhi::ShaderStage::Count)> glShaderType{
    GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, GL_COMPUTE_SHADER};

std::string getShaderStatusLog(const snap::rhi::backend::opengl::Profile& gl, const GLuint shader) {
    GLint logLength = 0;
    gl.getShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

    std::string log(logLength + 1, 0);

    if (logLength > 0) {
        gl.getShaderInfoLog(shader, logLength, &logLength, log.data());
    }

    return log;
}

bool isShaderCompiled(const snap::rhi::backend::opengl::Profile& gl, const GLuint shader) {
    GLint status = 0;
    gl.getShaderiv(shader, GL_COMPILE_STATUS, &status);

    const bool isOK = status == GL_TRUE;
    return isOK;
}

std::string buildDefineValue(const snap::rhi::SpecializationInfo& specializationInfo, const uint32_t idx) {
    const auto& specConstInfo = specializationInfo.pMapEntries[idx];
    const uint8_t* pData = static_cast<const uint8_t*>(specializationInfo.pData) + specConstInfo.offset;

    switch (specConstInfo.format) {
        case snap::rhi::SpecializationConstantFormat::Bool32: {
            assert(specConstInfo.offset + sizeof(uint32_t) <= specializationInfo.dataSize);
            const bool boolValue = *reinterpret_cast<const uint32_t*>(pData);
            return std::to_string(static_cast<uint32_t>(boolValue));
        }

        case snap::rhi::SpecializationConstantFormat::Float: {
            assert(specConstInfo.offset + sizeof(float) <= specializationInfo.dataSize);
            std::ostringstream out;
            out.precision(12);
            out << std::fixed << *reinterpret_cast<const float*>(pData);
            return out.str();
        }

        case snap::rhi::SpecializationConstantFormat::Int32: {
            assert(specConstInfo.offset + sizeof(int32_t) <= specializationInfo.dataSize);
            return std::to_string(*reinterpret_cast<const int32_t*>(pData));
        }

        case snap::rhi::SpecializationConstantFormat::UInt32: {
            assert(specConstInfo.offset + sizeof(uint32_t) <= specializationInfo.dataSize);
            return std::to_string(*reinterpret_cast<const uint32_t*>(pData));
        }

        default:
            snap::rhi::common::throwException("[buildDefines::buildValue] invalid SpecializationConstantFormat");
    }
}
} // unnamed namespace

namespace snap::rhi::backend::opengl {
GLuint createShaderFromSource(const snap::rhi::backend::opengl::Profile& gl,
                              const std::span<const std::string_view>& source,
                              const snap::rhi::ShaderStage type) {
    std::vector<const GLchar*> shaderSources(source.size());
    std::vector<GLint> shaderLength(source.size());

    for (size_t i = 0; i < source.size(); ++i) {
        shaderSources[i] = source[i].data();
        shaderLength[i] = static_cast<GLint>(source[i].length());
    }

    GLuint shader = gl.createShader(glShaderType[static_cast<uint8_t>(type)]);

    gl.shaderSource(shader, static_cast<GLsizei>(shaderSources.size()), shaderSources.data(), shaderLength.data());
    gl.compileShader(shader);

    return shader;
}

std::variant<GLuint, std::string> validateShaderCompileStatus(const snap::rhi::backend::opengl::Profile& gl,
                                                              GLuint& shader) {
    const auto& validationLayer = gl.getDevice()->getValidationLayer();

    const bool isOK = isShaderCompiled(gl, shader);
    std::string log =
        (snap::rhi::backend::common::enableSlowSafetyChecks() || !isOK) ? getShaderStatusLog(gl, shader) : "";

    if (!log.empty()) {
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Warning,
                        snap::rhi::ValidationTag::ShaderModuleOp,
                        "Shader compile log: %s\n",
                        log.c_str());
    }

    if (!isOK) {
        log = "[validateShaderCompileStatus] Shader compilation issue: " + log;
        gl.deleteShader(shader);
        shader = GL_NONE;

        return log;
    }

    return shader;
}

std::string buildSpecializationDefines(const snap::rhi::SpecializationInfo& specializationInfo,
                                       const std::optional<snap::rhi::opengl::ShaderModuleInfo>& glShaderModuleInfo) {
    std::string result;
    for (uint32_t i = 0; i < specializationInfo.mapEntryCount; ++i) {
        const auto& specConstInfo = specializationInfo.pMapEntries[i];

        std::string name;
        if (glShaderModuleInfo.has_value()) {
            const auto& names = glShaderModuleInfo->specializationConstantNames;
            const auto it = std::find_if(names.begin(), names.end(), [&](const auto& entry) {
                return entry.constantID == specConstInfo.constantID;
            });
            if (it != names.end()) {
                name = it->name;
            }
        }
        if (name.empty()) {
            name = std::string(SpecConstName) + std::to_string(specConstInfo.constantID);
        }

        std::string specConst = "#define " + name + " ";
        specConst += buildDefineValue(specializationInfo, i) + " \n";

        result += specConst;
    }
    return result;
}
} // namespace snap::rhi::backend::opengl
