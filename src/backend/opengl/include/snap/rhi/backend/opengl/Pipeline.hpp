#pragma once

#include "snap/rhi/backend/opengl/DeviceContext.hpp"
#include "snap/rhi/backend/opengl/ProgramState.hpp"
#include "snap/rhi/backend/opengl/ShaderModule.hpp"
#include "snap/rhi/backend/opengl/UniformsDescription.h"
#include "snap/rhi/backend/opengl/VertexDescriptor.h"

#include "snap/rhi/reflection/RenderPipelineInfo.h"

#include <array>
#include <string>

namespace snap::rhi {
class ShaderModule;
class PipelineCache;
} // namespace snap::rhi

namespace snap::rhi::backend::opengl {
class Device;
class Profile;
class ShaderModule;

/**
 * ========> SnapRHI doesn't support multithreading for legacy pipeline!!!!
 *
 * [UBO]
 * All uniform buffer used binding id's based on index in sorted ubo names list.
 * We should use Metal alignment rule for uniform pack into vec4 array.
 *
 * [SSBO]
 * All ssbo used binding id's provided by OpenGL.
 * We should use Metal alignment rule for uniform pack into vec4 array.
 *
 * [Texture]
 * All texture used binding id's based on index in sorted texture names list.
 *
 * [Image]
 * All images used binding id's based on index in sorted image names list.
 * https://www.khronos.org/opengl/wiki/Image_Load_Store
 *
 * [Sampler]
 * Not used in glsl.
 *
 * [Vertex Attribute]
 * All Vertex Attributes used location's provided by OpenGL.
 */
class Pipeline {
public:
    explicit Pipeline(Device* device,
                      const snap::rhi::PipelineCreateFlags pipelineCreateFlags,
                      const bool doesShaderContainUniformsOnly,
                      const PipelineConfigurationInfo& pipelineInfo,
                      const std::span<snap::rhi::ShaderModule*>& shaderStages,
                      const Pipeline* basePipeline = nullptr,
                      snap::rhi::PipelineCache* pipelineCache = nullptr);
    explicit Pipeline(Device* device, const bool doesShaderContainUniformsOnly, GLuint programID);
    ~Pipeline();

    const std::shared_ptr<ProgramState>& getProgramState() const {
        tryToCompilePipeline();
        return programState;
    }

    void useProgram(snap::rhi::backend::opengl::DeviceContext* dc) const;

    const PipelineSSBODescription& getPipelineSSBODescription() const {
        sync();
        return ssbo;
    }

    const PipelineLegacyUBODescription& getPipelineLegacyUBODescription() const {
        sync();
        return legacyDescription;
    }

    const PipelineUBODescription& getPipelineUBODescription() const {
        sync();
        return ubo;
    }

    const PipelineCompatibleUBODescription& getPipelineCompatibleUBODescription() const {
        sync();
        return compatibleUBO;
    }

    const PipelineTextureDescription& getPipelineTextureDescription() const {
        sync();
        return texture;
    }

    const PipelineImageDescription& getPipelineImageDescription() const {
        sync();
        return image;
    }

    const PipelineSamplerDescription& getPipelineSamplerDescription() const {
        sync();
        return sampler;
    }

    PipelineUniformManagmentType getPipelineUniformManagmentType() const {
        sync();

        const GLuint programID = programState->getProgramID();
        if (programID == GL_NONE) {
            snap::rhi::common::throwException("[getPipelineUniformManagmentType] invalid programID");
        }

        const auto& nativeInfo = programState->getPipelineNativeInfo();
        return nativeInfo.pipelineUniformManagmentType;
    }

    void bindLegacyUBO(DeviceContext* dc, std::span<const uint8_t> data) const;

protected:
    void init(const std::span<snap::rhi::ShaderModule*>& shaderStages, snap::rhi::PipelineCache* pipelineCache);
    void tryToCompilePipeline() const;
    void tryToCompilePipeline(const std::span<snap::rhi::ShaderModule*>& shaderStages,
                              snap::rhi::PipelineCache* pipelineCache) const;

    void sync() const {
        if (isSynchronized) {
            return;
        }

        tryToCompilePipeline();
        syncImpl();

        isSynchronized = true;
    }

    virtual void syncImpl() const;

    const Profile& gl;
    Device* device = nullptr;

    mutable std::shared_ptr<ProgramState> programState = nullptr;

    PipelineConfigurationInfo pipelineInfo;
    snap::rhi::backend::opengl::hash64 pipelineSrcHash;

    mutable bool isSynchronized = false;

    // Reflection info with logical bindings
    mutable std::vector<snap::rhi::reflection::VertexAttributeInfo> vertexAttributeInfos;
    mutable std::vector<snap::rhi::reflection::DescriptorSetInfo> descriptorSetInfos;

    // Logical to Physical bindings info
    mutable VertexDescriptor vertexDescription{};
    mutable PipelineSSBODescription ssbo{};
    mutable PipelineUBODescription ubo{};
    mutable PipelineCompatibleUBODescription compatibleUBO{};
    mutable PipelineTextureDescription texture{};
    mutable PipelineImageDescription image{};
    mutable PipelineSamplerDescription sampler{};
    mutable PipelineLegacyUBODescription legacyDescription{};

    bool isLegacyFlag = false;
    snap::rhi::PipelineCreateFlags pipelineCreateFlags = snap::rhi::PipelineCreateFlags::None;

    struct LazyInitRetainedRefs {
        std::vector<std::shared_ptr<snap::rhi::DeviceChild>> shaderStagesRefs{};
        std::shared_ptr<snap::rhi::DeviceChild> pipelineCacheRef = nullptr;

        void release() {
            pipelineCacheRef = nullptr;
            shaderStagesRefs.clear();
        }
    } mutable lazyInitRetainedRefs;
};
} // namespace snap::rhi::backend::opengl
