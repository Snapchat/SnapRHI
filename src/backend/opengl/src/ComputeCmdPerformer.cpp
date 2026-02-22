#include "snap/rhi/backend/opengl/ComputeCmdPerformer.hpp"
#include "snap/rhi/backend/opengl/ComputePipeline.hpp"
#include "snap/rhi/backend/opengl/DescriptorSet.hpp"
#include "snap/rhi/backend/opengl/Device.hpp"
#include "snap/rhi/backend/opengl/DeviceContext.hpp"
#include "snap/rhi/backend/opengl/Profile.hpp"
#include "snap/rhi/backend/opengl/Texture.hpp"
#include "snap/rhi/backend/opengl/Utils.hpp"
#include <iostream>

namespace snap::rhi::backend::opengl {
ComputeCmdPerformer::ComputeCmdPerformer(snap::rhi::backend::opengl::Device* device)
    : CmdPerformer(device), pipelineResourceState(device) {}

void ComputeCmdPerformer::beginEncoding(const snap::rhi::backend::opengl::BeginComputePassCmd& cmd) {}

void ComputeCmdPerformer::bindDescriptorSet(const snap::rhi::backend::opengl::BindDescriptorSetCmd& cmd) {
    SNAP_RHI_VALIDATE(device->getValidationLayer(),
                      cmd.binding < snap::rhi::SupportedLimit::MaxBoundDescriptorSets,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::CommandBufferOp,
                      "[bindDescriptorSet] invalid binding");
    pipelineResourceState.bindDescriptorSet(
        cmd.binding,
        snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::DescriptorSet>(cmd.descriptorSet),
        cmd.dynamicOffsets);
}

void ComputeCmdPerformer::bindComputePipeline(const snap::rhi::backend::opengl::BindComputePipelineCmd& cmd) {
    auto* computePipeline =
        snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::ComputePipeline>(cmd.pipeline);

    pipelineResourceState.setPipeline(computePipeline);
    computePipeline->useProgram(dc);
}

void ComputeCmdPerformer::dispatch(const snap::rhi::backend::opengl::DispatchCmd& cmd) {
    pipelineResourceState.setAllStates(dc);

    gl.dispatchCompute(cmd.groupSizeX, cmd.groupSizeY, cmd.groupSizeZ);
}

void ComputeCmdPerformer::pipelineBarrier(const snap::rhi::backend::opengl::PipelineBarrierCmd& cmd) {
    barrier(gl, cmd);
}

void ComputeCmdPerformer::endEncoding(const snap::rhi::backend::opengl::EndComputePassCmd& cmd) {
    reset();
}

void ComputeCmdPerformer::reset() {
    pipelineResourceState.clearStates();
}
} // namespace snap::rhi::backend::opengl
