//
//  Sampler.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 11.02.2022.
//

#pragma once

#include "snap/rhi/Sampler.hpp"
#include "snap/rhi/SamplerCreateInfo.h"
#include "snap/rhi/backend/opengl/OpenGL.h"
#include "snap/rhi/backend/opengl/TextureTarget.h"

namespace snap::rhi::backend::opengl {
class Device;
class Profile;
class Texture;
class DeviceContext;
} // namespace snap::rhi::backend::opengl

namespace snap::rhi::backend::opengl {
// https://www.khronos.org/opengl/wiki/Sampler_Object
class Sampler final : public snap::rhi::Sampler {
public:
    explicit Sampler(Device* device, const snap::rhi::SamplerCreateInfo& info);
    ~Sampler() override;

    void setDebugLabel(std::string_view label) override;

    void bindTextureSampler(GLuint unit,
                            const snap::rhi::backend::opengl::Texture* texture,
                            const TextureTarget target,
                            DeviceContext* dc) const;

private:
    void tryAllocate() const;

private:
    snap::rhi::backend::opengl::Profile& gl;

    mutable GLuint sampler = GL_NONE;
};
} // namespace snap::rhi::backend::opengl
