//
//  PipelineResourceState.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 8/16/22.
//

#pragma once

#include "snap/rhi/DescriptorSetCreateInfo.h"
#include "snap/rhi/Limits.h"

#include "snap/rhi/backend/opengl/DescriptorSet.hpp"
#include "snap/rhi/backend/opengl/PipelineImageState.hpp"
#include "snap/rhi/backend/opengl/PipelineSSBOState.hpp"
#include "snap/rhi/backend/opengl/PipelineTextureSamplerState.hpp"
#include "snap/rhi/backend/opengl/PipelineUBOState.hpp"
#include <snap/rhi/common/inplace_vector.h>

namespace snap::rhi::backend::opengl {
class Device;
class Profile;
class DescriptorSet;
class Pipeline;
class DeviceContext;

class PipelineResourceState final {
public:
    PipelineResourceState(snap::rhi::backend::opengl::Device* device);
    ~PipelineResourceState() = default;

    void setPipeline(const snap::rhi::backend::opengl::Pipeline* pipeline);
    void bindDescriptorSet(const uint32_t binding,
                           snap::rhi::backend::opengl::DescriptorSet* descriptorSet,
                           std::span<const uint32_t> dynamicOffsets);

    void setAllStates(DeviceContext* dc);
    void clearStates();

private:
    struct DescriptorSetData {
        snap::rhi::backend::opengl::DescriptorSet* descriptorSet = nullptr;
        snap::rhi::backend::common::inplace_vector<uint32_t, snap::rhi::MaxDynamicOffsetsPerDescriptorSet>
            dynamicOffsets{};

        friend auto operator<=>(const DescriptorSetData& lhs, const DescriptorSetData& rhs) noexcept = default;
    };

    std::array<DescriptorSetData, snap::rhi::SupportedLimit::MaxBoundDescriptorSets> activeDescriptorSets;
    std::array<DescriptorSetData, snap::rhi::SupportedLimit::MaxBoundDescriptorSets> descriptorSets;

    [[maybe_unused]] snap::rhi::backend::opengl::Device* device = nullptr;
    const snap::rhi::backend::opengl::Pipeline* pipeline = nullptr;

    snap::rhi::backend::opengl::PipelineTextureSamplerState textureSamplerState;
    snap::rhi::backend::opengl::PipelineImageState imageState;

    snap::rhi::backend::opengl::PipelineUBOState uboState;
    snap::rhi::backend::opengl::PipelineSSBOState ssboState;
};
} // namespace snap::rhi::backend::opengl
