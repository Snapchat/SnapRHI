//
//  ProgramUtils.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 22.10.2021.
//

#pragma once

#include "snap/rhi/RenderPipelineCreateInfo.h"
#include "snap/rhi/backend/opengl/HashUtils.hpp"
#include "snap/rhi/backend/opengl/Profile.hpp"
#include "snap/rhi/backend/opengl/ShaderModule.hpp"
#include <array>
#include <variant>

namespace snap::rhi::backend::opengl {
GLuint createProgramFromShaders(const Profile& gl,
                                const std::span<snap::rhi::ShaderModule*>& shaderStages,
                                bool isProgramBinaryRetrievable);
std::variant<GLuint, std::string> validateProgramLinkStatus(const Profile& gl,
                                                            GLuint& program,
                                                            std::span<snap::rhi::ShaderModule*> shaderStages);
GLuint tryLoadFromCache(const Profile& gl,
                        snap::rhi::PipelineCache* pipelineCache,
                        const snap::rhi::backend::opengl::hash64 pipelineSrcHash);
} // namespace snap::rhi::backend::opengl
