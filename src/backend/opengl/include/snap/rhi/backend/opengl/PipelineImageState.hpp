//
//  PipelineImageState.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 6/23/22.
//

#pragma once

#include "snap/rhi/Limits.h"
#include "snap/rhi/backend/opengl/OpenGL.h"
#include "snap/rhi/backend/opengl/ResourceID.h"

#include <array>
#include <vector>

namespace snap::rhi::backend::opengl {
class Profile;
class Device;
class Pipeline;
class Texture;
class DeviceContext;

class PipelineImageState final {
    // https://registry.khronos.org/OpenGL-Refpages/es3/html/glGet.xhtml
    // GL_MAX_FRAGMENT_IMAGE_UNIFORMS
    static constexpr uint32_t MaxImageBinding = 8;

    struct ImageBinding {
        const snap::rhi::backend::opengl::Texture* image = nullptr;
        uint32_t mipLevel = 0;
        GLenum access = GL_NONE;
    };

    struct BindingInfo {
        uint32_t binding = 0;

        ImageBinding info{nullptr, 0, GL_NONE};
    };

public:
    explicit PipelineImageState(snap::rhi::backend::opengl::Device* device);
    ~PipelineImageState() = default;

    void setPipeline(const snap::rhi::backend::opengl::Pipeline* pipeline);

    void invalidateDescriptorSetBindings(const uint32_t dsID);
    void bindImage(const uint32_t dsID,
                   const uint32_t binding,
                   const snap::rhi::backend::opengl::Texture* image,
                   const uint32_t mipLevel);

    void setAllStates(DeviceContext* dc);
    void clearStates();

private:
    const snap::rhi::backend::opengl::Profile& gl;

    const snap::rhi::backend::opengl::Pipeline* pipeline = nullptr;

    std::array<std::vector<BindingInfo>, snap::rhi::SupportedLimit::MaxBoundDescriptorSets> bindingsList;
    std::array<ImageBinding, MaxImageBinding> bindings;
};
} // namespace snap::rhi::backend::opengl
