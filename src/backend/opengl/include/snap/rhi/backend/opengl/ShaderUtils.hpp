//
//  ShaderUtils.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 11.10.2021.
//

#pragma once

#include "snap/rhi/ShaderModuleCreateInfo.h"
#include "snap/rhi/backend/opengl/Profile.hpp"
#include "snap/rhi/opengl/ShaderModuleInfo.h"

#include <array>
#include <optional>
#include <span>
#include <string_view>
#include <variant>

namespace snap::rhi::backend::opengl {
enum class ShaderCodeBlock : uint32_t {
    VersionHeader = 0,
    SpecializationConstants,
    EntryPoint,
    Src,

    Count
};

GLuint createShaderFromSource(const snap::rhi::backend::opengl::Profile& gl,
                              const std::span<const std::string_view>& source,
                              const snap::rhi::ShaderStage type);
std::variant<GLuint, std::string> validateShaderCompileStatus(const snap::rhi::backend::opengl::Profile& gl,
                                                              GLuint& shader);
std::string buildSpecializationDefines(
    const snap::rhi::SpecializationInfo& specializationInfo,
    const std::optional<snap::rhi::opengl::ShaderModuleInfo>& glShaderModuleInfo = std::nullopt);
} // namespace snap::rhi::backend::opengl
