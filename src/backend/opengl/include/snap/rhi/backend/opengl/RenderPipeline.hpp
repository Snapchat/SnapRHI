//
//  RenderPipeline.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 15.01.2021.
//  Copyright © 2021 Apple. All rights reserved.
//

#pragma once

#include "snap/rhi/Enums.h"

#include "snap/rhi/backend/opengl/UniformsDescription.h"
#include "snap/rhi/backend/opengl/VertexDescriptor.h"

#include "snap/rhi/reflection/RenderPipelineInfo.h"

#include "snap/rhi/RenderPipeline.hpp"
#include "snap/rhi/backend/opengl/Pipeline.hpp"
#include "snap/rhi/backend/opengl/RenderPipelineState.hpp"
#include <mutex>
#include <unordered_map>

#include <span>

namespace snap::rhi {
class DeviceContext;
} // namespace snap::rhi

namespace snap::rhi::backend::opengl {
class Device;
class Profile;

/**
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
class RenderPipeline final : public snap::rhi::RenderPipeline, public snap::rhi::backend::opengl::Pipeline {
public:
    explicit RenderPipeline(Device* glesDevice,
                            const snap::rhi::RenderPipelineCreateInfo& info,
                            const RenderPipelineStatesUUID& stateUUID);
    explicit RenderPipeline(Device* glesDevice, bool isLegacyPipeline, GLuint programID);
    ~RenderPipeline() override = default;

    void setDebugLabel(std::string_view label) override;

    const std::optional<reflection::RenderPipelineInfo>& getReflectionInfo() const override;

    const VertexDescriptor& getVertexDescriptor() const {
        sync();
        return vertexDescription;
    }

    const RenderPipelineStatesUUID& getStatesUUID() const {
        return stateUUID;
    }

private:
    void syncImpl() const override;

    const Profile& gl;
    RenderPipelineStatesUUID stateUUID{};
};
} // namespace snap::rhi::backend::opengl
