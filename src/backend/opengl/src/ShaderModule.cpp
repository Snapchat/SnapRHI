#include "snap/rhi/backend/opengl/ShaderModule.hpp"
#include "snap/rhi/backend/common/Logging.hpp"
#include "snap/rhi/backend/opengl/Device.hpp"
#include "snap/rhi/backend/opengl/ErrorCheckGuard.hpp"
#include "snap/rhi/backend/opengl/Profile.hpp"
#include "snap/rhi/backend/opengl/ShaderLibrary.hpp"
#include "snap/rhi/common/HashCombine.h"

#include "snap/rhi/common/Throw.h"
#include <array>
#include <snap/rhi/common/Scope.h>
#include <string>
#include <string_view>
#include <vector>

namespace {
constexpr std::string_view headerKey{"#version"};
constexpr std::string_view defineKey{"#define "};

bool buildHeaderAndSrc(const std::string& source, std::string_view& header, std::string_view& shader) {
    const auto posVersion = source.find(headerKey);
    if (posVersion == std::string::npos) {
        return false;
    }

    const auto posNextLine = source.find("\n", posVersion);
    if (posNextLine == std::string::npos) {
        return false;
    }

    const size_t toHeaderStrSize = posNextLine + 1;
    header = std::string_view{source.data(), toHeaderStrSize};
    shader = std::string_view{source.data() + toHeaderStrSize, source.size() - toHeaderStrSize};
    return true;
}

std::string buildDefine(const std::string_view name) {
    return std::string(defineKey) + std::string(name) + " \n";
}
} // namespace

namespace snap::rhi::backend::opengl {
ShaderModule::ShaderModule(Device* glesDevice, const snap::rhi::ShaderModuleCreateInfo& info)
    : snap::rhi::ShaderModule(glesDevice, info), gl(glesDevice->getOpenGL()), doesShaderContainUniformsOnly(false) {
    source.fill("");
    snap::rhi::backend::opengl::ShaderLibrary* glesLibrary =
        snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::ShaderLibrary>(info.shaderLibrary);

    const auto& src = glesLibrary->getSource();
    std::string_view header;
    std::string_view shaderSrc;
    std::string entryPoint = buildDefine(info.name);

    if (!buildHeaderAndSrc(src, header, shaderSrc)) {
        header = glesDevice->getShaderVersionString();
        shaderSrc = src;
    }

    source[static_cast<uint32_t>(snap::rhi::backend::opengl::ShaderCodeBlock::VersionHeader)] = std::string(header);
    source[static_cast<uint32_t>(snap::rhi::backend::opengl::ShaderCodeBlock::SpecializationConstants)] =
        snap::rhi::backend::opengl::buildSpecializationDefines(info.specializationInfo, info.glShaderModuleInfo);
    source[static_cast<uint32_t>(snap::rhi::backend::opengl::ShaderCodeBlock::EntryPoint)] = entryPoint;
    source[static_cast<uint32_t>(snap::rhi::backend::opengl::ShaderCodeBlock::Src)] = std::string(shaderSrc);

    shaderSrcHash = snap::rhi::backend::opengl::computeShaderSrcHash(source);

    if (!glesDevice->areResourcesLazyAllocationsEnabled()) {
        tryCompileShader();
    }
}

ShaderModule::ShaderModule(Device* glesDevice,
                           const snap::rhi::ShaderStage shaderStage,
                           std::string_view src,
                           bool doesShaderContainUniformsOnly)
    : snap::rhi::ShaderModule(glesDevice, {.shaderStage = shaderStage}),
      gl(glesDevice->getOpenGL()),
      doesShaderContainUniformsOnly(doesShaderContainUniformsOnly) {
    source.fill("");
    shaderSrcHash = 0;

    if (!glesDevice->areResourcesLazyAllocationsEnabled()) {
        shaderIdOrErrorMessage = snap::rhi::backend::opengl::createShaderFromSource(gl, {&src, 1}, shaderStage);

        if (glesDevice->shouldValidatePipelineOnCreation()) {
            validateShader();
        }
    } else {
        source[static_cast<uint32_t>(snap::rhi::backend::opengl::ShaderCodeBlock::Src)] = std::string(src);
    }
}

ShaderModule::~ShaderModule() {
    try {
        auto* glDevice = snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Device>(device);

        SNAP_RHI_REPORT(glDevice->getValidationLayer(),
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::DestroyOp,
                        "[snap::rhi::backend::opengl::~ShaderModule] start destruction");
        SNAP_RHI_VALIDATE(glDevice->getValidationLayer(),
                          device->getCurrentDeviceContext(),
                          snap::rhi::ReportLevel::Warning,
                          snap::rhi::ValidationTag::DestroyOp,
                          "[snap::rhi::backend::opengl::~ShaderModule] DeviceContext isn't attached to thread");
        SNAP_RHI_VALIDATE(glDevice->getValidationLayer(),
                          glDevice->isNativeContextAttached(),
                          snap::rhi::ReportLevel::CriticalError,
                          snap::rhi::ValidationTag::DestroyOp,
                          "[snap::rhi::backend::opengl::~ShaderModule] GLES context isn't attached to thread");
        {
            const GLuint* shaderIDPtr = std::get_if<GLuint>(&shaderIdOrErrorMessage);

            if ((shaderIDPtr != nullptr) && (*shaderIDPtr != GL_NONE)) {
                gl.deleteShader(*shaderIDPtr);
            }

            isValidated = false;
            shaderIdOrErrorMessage = static_cast<GLuint>(GL_NONE);
            shaderSrcHash = 0;
        }

        SNAP_RHI_REPORT(glDevice->getValidationLayer(),
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::DestroyOp,
                        "[snap::rhi::backend::opengl::~ShaderModule] end of destruction");
    } catch (const snap::rhi::common::Exception& e) {
        SNAP_RHI_LOGE("[snap::rhi::backend::opengl::ShaderModule::~ShaderModule] Caught: %s (possible resource leak).",
                      e.what());
    } catch (...) {
        SNAP_RHI_LOGE("[snap::rhi::backend::opengl::ShaderModule::~ShaderModule] Caught unexpected error (possible "
                      "resource leak).");
    }
}

void ShaderModule::validateShader() const {
    if (isValidated) {
        return;
    }
    SNAP_RHI_ON_SCOPE_EXIT {
        isValidated = true;
    };

    const GLuint* shaderIdPtr = std::get_if<GLuint>(&shaderIdOrErrorMessage);
    GLuint shaderId = shaderIdPtr == nullptr ? GL_NONE : *shaderIdPtr;
    if (shaderId == GL_NONE) {
        return;
    }

    shaderIdOrErrorMessage = validateShaderCompileStatus(gl, shaderId);
}

void ShaderModule::tryCompileShader() const {
    const GLuint* shaderIDPtr = std::get_if<GLuint>(&shaderIdOrErrorMessage);
    const std::string* compilationErrorMessagePtr = std::get_if<std::string>(&shaderIdOrErrorMessage);

    if (((shaderIDPtr != nullptr) && (*shaderIDPtr != GL_NONE)) || (compilationErrorMessagePtr != nullptr)) {
        return;
    }

    std::vector<std::string_view> sourceView(source.size());
    for (size_t i = 0; i < source.size(); ++i) {
        sourceView[i] = source[i];
    }

    shaderIdOrErrorMessage = snap::rhi::backend::opengl::createShaderFromSource(gl, sourceView, info.shaderStage);
    isValidated = false;

    if (common::smart_cast<Device>(gl.getDevice())->shouldValidatePipelineOnCreation()) {
        validateShader();
    }
}

bool ShaderModule::doesShaderContainRegularUniformsOnly() const {
    return doesShaderContainUniformsOnly;
}

snap::rhi::backend::opengl::hash64 ShaderModule::getShaderSrcHash() const {
    return shaderSrcHash;
}

void ShaderModule::setDebugLabel(std::string_view label) {
#if SNAP_RHI_ENABLE_DEBUG_LABELS
    ObjectDebugMarkers::setDebugLabel(label);
    if (std::holds_alternative<GLuint>(shaderIdOrErrorMessage)) {
        const GLuint shaderId = std::get<GLuint>(shaderIdOrErrorMessage);
        if (shaderId != GL_NONE) {
            gl.objectLabel(GL_SHADER, shaderId, label);
        }
    }
#endif
}
} // namespace snap::rhi::backend::opengl
