#pragma once

#include "snap/rhi/Limits.h"
#include "snap/rhi/RenderPipelineCreateInfo.h"
#include "snap/rhi/backend/opengl/OpenGL.h"
#include "snap/rhi/backend/opengl/PipelineNativeInfo.h"
#include "snap/rhi/backend/opengl/ResourceID.h"
#include "snap/rhi/backend/opengl/TextureTarget.h"
#include "snap/rhi/reflection/Info.hpp"

#include <array>
#include <bitset>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace snap::rhi::backend::opengl {
enum class UniformBufferType : uint32_t { Float = 0, Int };

struct SSBOInfo {                // glBindBufferRange
    uint32_t logicalBinding = 0; // SnapRHI resource binding inside of DescriptorSet

    GLint binding = InvalidLocation;
    GLsizeiptr size = 0;
};

struct UBOInfo {                 // glBindBufferRange
    uint32_t logicalBinding = 0; // SnapRHI resource binding inside of DescriptorSet

    GLint binding = InvalidLocation;
    GLsizeiptr size = 0;
};

struct CompatibleUBOInfo {       // glUniform4fv/glUniform4iv
    uint32_t logicalBinding = 0; // SnapRHI resource binding inside of DescriptorSet

    GLint location = InvalidLocation;
    UniformBufferType bufferType = UniformBufferType::Float;
    GLsizei arraySize = 0;
};

struct SampledTextureInfo {      // glBindTexture
    uint32_t logicalBinding = 0; // SnapRHI resource binding inside of DescriptorSet

    std::string name = "";
    GLint binding = InvalidLocation;
    TextureTarget target = TextureTarget::Detached;
};

struct SamplerInfo {             // glBindSampler/glTexParameter
    uint32_t logicalBinding = 0; // SnapRHI resource binding inside of DescriptorSet

    std::vector<GLint> binding;
};

struct ImageInfo {               // glBindImageTexture
    uint32_t logicalBinding = 0; // SnapRHI resource binding inside of DescriptorSet

    GLuint unit = InvalidLocation;
    GLenum access = GL_NONE;
};

struct PipelineLegacyUBODescription { // glUniform
    std::vector<UniformInfo> uniforms;
    uint32_t size = 0;
};

// assert(descriptor set binding ID < snap::rhi::SupportedLimit::MaxBoundDescriptorSets);
using PipelineSSBODescription = std::array<std::vector<SSBOInfo>, snap::rhi::SupportedLimit::MaxBoundDescriptorSets>;
using PipelineUBODescription = std::array<std::vector<UBOInfo>, snap::rhi::SupportedLimit::MaxBoundDescriptorSets>;
using PipelineCompatibleUBODescription =
    std::array<std::vector<CompatibleUBOInfo>, snap::rhi::SupportedLimit::MaxBoundDescriptorSets>;
using PipelineTextureDescription =
    std::array<std::vector<SampledTextureInfo>, snap::rhi::SupportedLimit::MaxBoundDescriptorSets>;
using PipelineImageDescription = std::array<std::vector<ImageInfo>, snap::rhi::SupportedLimit::MaxBoundDescriptorSets>;
using PipelineSamplerDescription =
    std::array<std::vector<SamplerInfo>, snap::rhi::SupportedLimit::MaxBoundDescriptorSets>;
} // namespace snap::rhi::backend::opengl
