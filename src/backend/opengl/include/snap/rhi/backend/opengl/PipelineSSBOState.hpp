//
//  PipelineSSBOState.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 6/23/22.
//

#pragma once

#include "snap/rhi/Limits.h"
#include "snap/rhi/backend/opengl/ResourceID.h"

#include <array>
#include <cassert>
#include <vector>

namespace snap::rhi::backend::opengl {
class Profile;
class Device;
class Pipeline;
class Buffer;
class DeviceContext;

class PipelineSSBOState final {
    // https://registry.khronos.org/OpenGL-Refpages/es3/html/glGet.xhtml
    // GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS
    static constexpr uint32_t MaxBufferBinding = 16;

    struct BufferBinding {
        snap::rhi::backend::opengl::Buffer* buffer = nullptr;
        uint64_t offset = 0;
    };

    struct BindingInfo {
        uint32_t binding = 0;

        BufferBinding info{nullptr, 0};
    };

public:
    explicit PipelineSSBOState(snap::rhi::backend::opengl::Device* device);
    ~PipelineSSBOState() = default;

    void setPipeline(const snap::rhi::backend::opengl::Pipeline* pipeline) {
        assert(pipeline != nullptr);
        this->pipeline = pipeline;
    }

    void invalidateDescriptorSetBindings(const uint32_t dsID) {
        assert(dsID < snap::rhi::SupportedLimit::MaxBoundDescriptorSets);

        bindingsList[dsID].clear();
    }

    void bind(const uint32_t dsID,
              const uint32_t binding,
              snap::rhi::backend::opengl::Buffer* buffer,
              const uint64_t offset) {
        assert(dsID < snap::rhi::SupportedLimit::MaxBoundDescriptorSets);
        assert(bindingsList[dsID].empty() || bindingsList[dsID].back().binding < binding);

        bindingsList[dsID].push_back({binding, {buffer, offset}});
    }

    void setAllStates(DeviceContext* dc);
    void clearStates();

private:
    const snap::rhi::backend::opengl::Profile& gl;

    const snap::rhi::backend::opengl::Pipeline* pipeline = nullptr;

    std::array<std::vector<BindingInfo>, snap::rhi::SupportedLimit::MaxBoundDescriptorSets> bindingsList;
    std::array<BufferBinding, MaxBufferBinding> bindings;
};
} // namespace snap::rhi::backend::opengl
