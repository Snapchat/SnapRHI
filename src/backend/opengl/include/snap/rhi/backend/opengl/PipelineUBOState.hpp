//
//  PipelineUBOState.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 6/23/22.
//

#pragma once

#include "snap/rhi/Limits.h"

#include <array>
#include <cassert>
#include <vector>

namespace snap::rhi::backend::opengl {
class Profile;
class Device;
class Pipeline;
class Buffer;
class DeviceContext;

class PipelineUBOState final {
    // https://registry.khronos.org/OpenGL-Refpages/es3/html/glGet.xhtml
    // GL_MAX_UNIFORM_BUFFER_BINDINGS
    static constexpr uint32_t MaxBufferBinding = 128;

    struct BufferBinding {
        snap::rhi::backend::opengl::Buffer* buffer = nullptr;
        uint64_t offset = 0;
        uint64_t range = 0;
    };

    struct BindingInfo {
        uint32_t binding = 0;

        BufferBinding info{nullptr, 0};
    };

public:
    explicit PipelineUBOState(snap::rhi::backend::opengl::Device* device);
    ~PipelineUBOState() = default;

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
              const uint64_t offset,
              const uint64_t range) {
        assert(dsID < snap::rhi::SupportedLimit::MaxBoundDescriptorSets);
        assert(bindingsList[dsID].empty() || bindingsList[dsID].back().binding < binding);

        bindingsList[dsID].push_back({binding, {buffer, offset, range}});
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
