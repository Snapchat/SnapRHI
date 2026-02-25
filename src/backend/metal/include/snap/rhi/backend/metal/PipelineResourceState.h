//
//  PipelineResourceState.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 9/16/22.
//

#pragma once

#include <array>
#include <span>

#include "snap/rhi/Limits.h"
#include "snap/rhi/backend/metal/Context.h"
#include <snap/rhi/common/inplace_vector.h>

namespace snap::rhi::backend::metal {
class Device;
class DescriptorSet;
class RenderPipeline;
class CommandBuffer;

class PipelineResourceState {
public:
    PipelineResourceState(CommandBuffer* commandBuffer);
    virtual ~PipelineResourceState() = default;

    void bindDescriptorSet(const uint32_t binding,
                           DescriptorSet* mtlDescriptorSet,
                           std::span<const uint32_t> dynamicOffsets);
    void setAuxiliaryDynamicOffsetsBinding(uint32_t auxiliaryDynamicOffsetsBinding);

    virtual void setAllStates();
    virtual void clearStates();

protected:
    struct DescriptorSetData {
        snap::rhi::backend::metal::DescriptorSet* descriptorSet = nullptr;
        snap::rhi::backend::common::inplace_vector<uint32_t, snap::rhi::MaxDynamicOffsetsPerDescriptorSet>
            dynamicOffsets{};

        friend auto operator<=>(const DescriptorSetData& lhs, const DescriptorSetData& rhs) noexcept = default;
    };

    std::array<DescriptorSetData, snap::rhi::SupportedLimit::MaxBoundDescriptorSets> activeDescriptorSets{};
    uint32_t activeAuxiliaryDynamicOffsetsBinding = 0;

    std::array<DescriptorSetData, snap::rhi::SupportedLimit::MaxBoundDescriptorSets> descriptorSets{};
    uint32_t auxiliaryDynamicOffsetsBinding = 0;

    std::vector<uint32_t> dynamicOffsetsStorage{};

    CommandBuffer* commandBuffer = nullptr;
};
} // namespace snap::rhi::backend::metal
