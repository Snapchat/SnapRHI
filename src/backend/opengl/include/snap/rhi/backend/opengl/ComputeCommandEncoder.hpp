#pragma once

#include "snap/rhi/ComputeCommandEncoder.hpp"
#include "snap/rhi/backend/common/CommandEncoder.h"
#include "snap/rhi/backend/common/ResourceResidencySet.h"
#include "snap/rhi/backend/common/ValidationLayer.hpp"
#include "snap/rhi/backend/opengl/CommandAllocator.hpp"
#include "snap/rhi/backend/opengl/CommandEncoder.hpp"

namespace snap::rhi::backend::opengl {
class Device;
class CommandBuffer;

using ComputeCommandEncoderBase = snap::rhi::backend::opengl::CommandEncoder<snap::rhi::ComputeCommandEncoder>;
class ComputeCommandEncoder final : public ComputeCommandEncoderBase {
public:
    ComputeCommandEncoder(snap::rhi::backend::opengl::Device* device,
                          snap::rhi::backend::opengl::CommandBuffer* commandBuffer);
    ~ComputeCommandEncoder() = default;

    void beginEncoding() override;
    void bindDescriptorSet(uint32_t binding,
                           snap::rhi::DescriptorSet* descriptorSet,
                           std::span<const uint32_t> dynamicOffsets) override;
    void bindComputePipeline(snap::rhi::ComputePipeline* pipeline) override;
    void dispatch(const uint32_t groupSizeX, const uint32_t groupSizeY, const uint32_t groupSizeZ) override;
    void endEncoding() override;

private:
};
} // namespace snap::rhi::backend::opengl
