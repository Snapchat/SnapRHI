#pragma once

#include "snap/rhi/Limits.h"
#include "snap/rhi/backend/opengl/OpenGL.h"
#include "snap/rhi/backend/opengl/Profile.hpp"
#include "snap/rhi/backend/opengl/UniformsDescription.h"
#include "snap/rhi/reflection/RenderPipelineInfo.h"
#include <array>
#include <bitset>
#include <string_view>
#include <vector>

namespace snap::rhi::backend::opengl {
class Pipeline;

PipelineNativeInfo buildPipelineNativeInfo(const Profile& gl,
                                           const GLuint program,
                                           const bool acquireNativeReflection,
                                           bool isLegacyPipeline,
                                           const PipelineConfigurationInfo& info);

PipelineSSBODescription buildSSBODesc(const PipelineNativeInfo& pipelineNativeInfo,
                                      const snap::rhi::opengl::PipelineInfo* pipelineInfo,
                                      std::vector<snap::rhi::reflection::DescriptorSetInfo>& physicalReflection);
PipelineUBODescription buildUBODesc(const PipelineNativeInfo& pipelineNativeInfo,
                                    const snap::rhi::opengl::PipelineInfo* pipelineInfo,
                                    std::vector<snap::rhi::reflection::DescriptorSetInfo>& physicalReflection);
PipelineCompatibleUBODescription buildCompatibleUBODesc(
    const PipelineNativeInfo& pipelineNativeInfo,
    const snap::rhi::opengl::PipelineInfo* pipelineInfo,
    std::vector<snap::rhi::reflection::DescriptorSetInfo>& physicalReflection);
PipelineTextureDescription buildTextureDesc(const PipelineNativeInfo& pipelineNativeInfo,
                                            const snap::rhi::opengl::PipelineInfo* pipelineInfo,
                                            std::vector<snap::rhi::reflection::DescriptorSetInfo>& physicalReflection);
PipelineSamplerDescription buildSamplerDesc(const PipelineNativeInfo& pipelineNativeInfo,
                                            const RenderPipelineConfigurationInfo& renderPipelineInfo,
                                            std::vector<snap::rhi::reflection::DescriptorSetInfo>& physicalReflection,
                                            const PipelineTextureDescription& texInfo);
PipelineImageDescription buildImageDesc(const PipelineNativeInfo& pipelineNativeInfo,
                                        const snap::rhi::opengl::PipelineInfo* pipelineInfo,
                                        std::vector<snap::rhi::reflection::DescriptorSetInfo>& physicalReflection);
PipelineLegacyUBODescription buildLegacyUBODesc(
    const PipelineNativeInfo& pipelineNativeInfo,
    std::vector<snap::rhi::reflection::DescriptorSetInfo>& physicalReflection);
uint32_t getAlignedUniformSize(const GLenum type);

uint32_t legacyTextureBindingToSamplerBinding(const uint32_t binding);
} // namespace snap::rhi::backend::opengl
