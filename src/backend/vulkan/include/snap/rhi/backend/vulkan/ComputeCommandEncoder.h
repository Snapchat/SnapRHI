#pragma once

#include "snap/rhi/ComputeCommandEncoder.hpp"
#include "snap/rhi/Limits.h"
#include "snap/rhi/backend/common/CommandEncoder.h"
#include "snap/rhi/backend/common/ResourceResidencySet.h"
#include "snap/rhi/backend/common/ValidationLayer.hpp"
#include "snap/rhi/backend/vulkan/CommandEncoder.hpp"
#include "snap/rhi/backend/vulkan/DescriptorSetEncoder.h"
#include "snap/rhi/backend/vulkan/vulkan.h"

namespace snap::rhi::backend::vulkan {
class Device;
class CommandBuffer;
class ComputePipeline;

using ComputeCommandEncoderBase = CommandEncoder<snap::rhi::ComputeCommandEncoder>;
class ComputeCommandEncoder final : public ComputeCommandEncoderBase, public DescriptorSetEncoder {
public:
    ComputeCommandEncoder(snap::rhi::backend::vulkan::Device* vkDevice,
                          snap::rhi::backend::vulkan::CommandBuffer* commandBuffer);
    ~ComputeCommandEncoder() final;

    void beginEncoding() final;
    void bindDescriptorSet(uint32_t binding,
                           snap::rhi::DescriptorSet* descriptorSet,
                           std::span<const uint32_t> dynamicOffsets) final;
    void bindComputePipeline(snap::rhi::ComputePipeline* pipeline) final;
    void dispatch(const uint32_t groupSizeX, const uint32_t groupSizeY, const uint32_t groupSizeZ) final;
    void endEncoding() final;

private:
    void reset();
};
} // namespace snap::rhi::backend::vulkan
