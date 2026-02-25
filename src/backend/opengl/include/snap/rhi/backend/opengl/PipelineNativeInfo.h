#pragma once

#include "snap/rhi/Limits.h"
#include "snap/rhi/RenderPipelineCreateInfo.h"
#include "snap/rhi/backend/opengl/OpenGL.h"
#include "snap/rhi/backend/opengl/ResourceID.h"
#include "snap/rhi/backend/opengl/TextureTarget.h"
#include "snap/rhi/reflection/Info.hpp"

#include <array>
#include <bitset>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace snap::rhi::backend::opengl {
struct UniformInfo { // glUniform
    GLint location = InvalidLocation;
    GLenum type = GL_NONE;
    GLint arraySize = 0;

    uint32_t uboOffset = 0;
    uint32_t byteSize = 0;
};

enum class PipelineUniformManagmentType : uint32_t {
    /*
     * Pipeline have to use only native UBO's
     */
    Default = 0,

    /*
     * Pipeline have to use only UBO's packed into vec4/ivec4 arrays
     */
    Compatible,

    /*
     * Pipeline have to use only regular uniforms.
     * No UBO's allowed.
     */
    Legacy
};

struct PipelineNativeInfo {
    struct AttribInfo {
        static constexpr GLint InvalidLocation = -1;

        std::string name = "";
        GLint location = InvalidLocation;
        snap::rhi::VertexAttributeFormat format = snap::rhi::VertexAttributeFormat::Undefined;
    };

    struct UniformInfo {
        GLuint index = std::numeric_limits<GLuint>::max();

        std::string name = "";
        GLint location = InvalidLocation;
        GLenum type = GL_NONE;
        GLint arraySize = 0;
    };

    struct CompatibleUBOInfo {
        GLuint index = std::numeric_limits<GLuint>::max();

        std::string name = "";
        GLint location = InvalidLocation;
        uint32_t binding = std::numeric_limits<uint32_t>::max();
        GLenum type = GL_NONE;
        GLint arraySize = 0;
    };

    struct LegacyUBOInfo {
        snap::rhi::reflection::UniformBufferInfo uboInfo{};
        std::vector<snap::rhi::backend::opengl::UniformInfo> uniforms;
    };

    struct ImageInfo {
        GLuint index = std::numeric_limits<GLuint>::max();
        static constexpr GLint InvalidUnit = -1;

        std::string name = "";
        GLint unit = InvalidUnit;
        uint32_t binding = std::numeric_limits<uint32_t>::max();
    };

    struct SampledTextureInfo {
        GLuint index = std::numeric_limits<GLuint>::max();

        std::string name = "";
        GLint location = InvalidLocation;
        uint32_t binding = std::numeric_limits<uint32_t>::max();
        GLenum type = GL_NONE;
    };

    struct SSBOInfo {
        GLuint index = std::numeric_limits<GLuint>::max();

        std::string name = "";
        GLint binding = InvalidBinding;
        GLint size = 0;
    };

    std::vector<UniformInfo> uniformsInfo{};

    std::vector<ImageInfo> imagesInfo{};
    std::vector<SampledTextureInfo> sampledTexturesInfo{};
    std::vector<snap::rhi::reflection::UniformBufferInfo> nativeUBOsInfo{};
    std::vector<CompatibleUBOInfo> compatibleUBOsInfo{};
    LegacyUBOInfo legacyUBOInfo{};
    std::vector<SSBOInfo> ssbosInfo{};

    std::vector<AttribInfo> attribsInfo;
    PipelineUniformManagmentType pipelineUniformManagmentType = PipelineUniformManagmentType::Default;
};

struct ComputePipelineConfigurationInfo {
    std::optional<snap::rhi::opengl::PipelineInfo> computePipelineInfo = std::nullopt;
};

struct RenderPipelineConfigurationInfo {
    snap::rhi::VertexInputStateCreateInfo layout{};
    std::optional<snap::rhi::opengl::RenderPipelineInfo> renderPipelineInfo = std::nullopt;
};

using PipelineConfigurationInfo =
    std::variant<std::monostate, ComputePipelineConfigurationInfo, RenderPipelineConfigurationInfo>;
} // namespace snap::rhi::backend::opengl
