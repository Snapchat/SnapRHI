//
//  PipelineTextureSamplerState.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 6/22/22.
//

#pragma once

#include "snap/rhi/Limits.h"
#include "snap/rhi/backend/opengl/TextureTarget.h"

#include <array>
#include <bitset>
#include <vector>

namespace snap::rhi::backend::opengl {
class Device;
class Profile;
class Pipeline;
class Texture;
class Sampler;
class DeviceContext;

class PipelineTextureSamplerState final {
    static constexpr uint32_t MAX_TEXTURE_SAMPLER_CACHE_SIZE = 256;
    std::array<std::array<const snap::rhi::backend::opengl::Texture*, MAX_TEXTURE_SAMPLER_CACHE_SIZE>,
               snap::rhi::SupportedLimit::MaxBoundDescriptorSets>
        activeTextures;
    std::array<std::array<const snap::rhi::backend::opengl::Sampler*, MAX_TEXTURE_SAMPLER_CACHE_SIZE>,
               snap::rhi::SupportedLimit::MaxBoundDescriptorSets>
        activeSamplers;

    // https://registry.khronos.org/OpenGL-Refpages/es3/html/glGet.xhtml
    // GL_MAX_TEXTURE_IMAGE_UNITS
    static constexpr uint32_t MaxTextureSamplerBinding = 32;

    struct TexBinding {
        const snap::rhi::backend::opengl::Texture* texture = nullptr;
        TextureTarget target = TextureTarget::Detached;
    };

    struct TexBindingInfo {
        uint32_t binding = 0;

        const snap::rhi::backend::opengl::Texture* texture = nullptr;
    };

    struct SmplBindingInfo {
        uint32_t binding = 0;

        const snap::rhi::backend::opengl::Sampler* sampler = nullptr;
    };

public:
    explicit PipelineTextureSamplerState(snap::rhi::backend::opengl::Device* device);
    ~PipelineTextureSamplerState() = default;

    void setPipeline(const snap::rhi::backend::opengl::Pipeline* pipeline) {
        assert(pipeline != nullptr);

        if (this->pipeline == pipeline) {
            return;
        }

        this->pipeline = pipeline;
        shouldBind = true;
    }

    void invalidateDescriptorSetBindings(const uint32_t dsID) {
        assert(dsID < snap::rhi::SupportedLimit::MaxBoundDescriptorSets);

        texBindingsList[dsID].clear();
        smplBindingsList[dsID].clear();
    }

    void bindTexture(const uint32_t dsID, const uint32_t binding, const snap::rhi::backend::opengl::Texture* object) {
        assert(dsID < snap::rhi::SupportedLimit::MaxBoundDescriptorSets);
        assert(texBindingsList[dsID].empty() || texBindingsList[dsID].back().binding < binding);

        if ((binding < MAX_TEXTURE_SAMPLER_CACHE_SIZE) && (activeTextures[dsID][binding] == object)) {
            return;
        }

        if (binding < MAX_TEXTURE_SAMPLER_CACHE_SIZE) {
            activeTextures[dsID][binding] = object;
        } else {
            texBindingsList[dsID].push_back({binding, object});
        }
        shouldBind = true;
    }

    void bindSampler(const uint32_t dsID, const uint32_t binding, const snap::rhi::backend::opengl::Sampler* object) {
        assert(dsID < snap::rhi::SupportedLimit::MaxBoundDescriptorSets);
        assert(smplBindingsList[dsID].empty() || smplBindingsList[dsID].back().binding < binding);

        if ((binding < MAX_TEXTURE_SAMPLER_CACHE_SIZE) && (activeSamplers[dsID][binding] == object)) {
            return;
        }

        if (binding < MAX_TEXTURE_SAMPLER_CACHE_SIZE) {
            activeSamplers[dsID][binding] = object;
        } else {
            smplBindingsList[dsID].push_back({binding, object});
        }
        shouldBind = true;
    }

    void setAllStates(DeviceContext* dc);
    void clearStates();

private:
    void bindTextures(std::bitset<MaxTextureSamplerBinding>& toUpdateUnits);
    void bindSamplers(std::bitset<MaxTextureSamplerBinding>& toUpdateUnits);

    const snap::rhi::backend::opengl::Profile& gl;

    const snap::rhi::backend::opengl::Pipeline* pipeline = nullptr;

    std::array<std::vector<TexBindingInfo>, snap::rhi::SupportedLimit::MaxBoundDescriptorSets> texBindingsList;
    std::array<std::vector<SmplBindingInfo>, snap::rhi::SupportedLimit::MaxBoundDescriptorSets> smplBindingsList;

    std::array<TexBinding, MaxTextureSamplerBinding> texBindings;
    std::array<const snap::rhi::backend::opengl::Sampler*, MaxTextureSamplerBinding> smplBindings;

    bool shouldBind = false;
};
} // namespace snap::rhi::backend::opengl
