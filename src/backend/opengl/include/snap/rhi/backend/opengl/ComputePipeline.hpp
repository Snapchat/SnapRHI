//
//  ComputePipeline.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 12/22/22.
//

#pragma once

#include "snap/rhi/ComputePipeline.hpp"
#include "snap/rhi/backend/opengl/Pipeline.hpp"

namespace snap::rhi::backend::opengl {
class Device;
class Profile;

/**
 * https://www.khronos.org/opengl/wiki/Compute_Shader
 */
class ComputePipeline final : public snap::rhi::ComputePipeline, public snap::rhi::backend::opengl::Pipeline {
public:
    explicit ComputePipeline(Device* glesDevice, const snap::rhi::ComputePipelineCreateInfo& info);
    ~ComputePipeline() override = default;

    void setDebugLabel(std::string_view label) override;

    const std::optional<reflection::ComputePipelineInfo>& getReflectionInfo() const override;

private:
    void syncImpl() const override;
};
} // namespace snap::rhi::backend::opengl
