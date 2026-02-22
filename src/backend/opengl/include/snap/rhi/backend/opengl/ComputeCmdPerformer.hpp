//
//  ComputeCmdPerformer.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 12/27/22.
//

#pragma once

#include "snap/rhi/backend/opengl/CmdPerformer.hpp"
#include "snap/rhi/backend/opengl/Commands.h"
#include "snap/rhi/backend/opengl/OpenGL.h"
#include "snap/rhi/backend/opengl/PipelineResourceState.hpp"
#include "snap/rhi/common/NonCopyable.h"

namespace snap::rhi::backend::opengl {
class Device;
class Profile;
} // namespace snap::rhi::backend::opengl

namespace snap::rhi::backend::opengl {
class Device;
class DescriptorSet;
class ComputePipeline;

class ComputeCmdPerformer final : public CmdPerformer {
public:
    ComputeCmdPerformer(snap::rhi::backend::opengl::Device* device);
    ~ComputeCmdPerformer() = default;

    void beginEncoding(const snap::rhi::backend::opengl::BeginComputePassCmd& cmd);
    void bindDescriptorSet(const snap::rhi::backend::opengl::BindDescriptorSetCmd& cmd);
    void bindComputePipeline(const snap::rhi::backend::opengl::BindComputePipelineCmd& cmd);
    void dispatch(const snap::rhi::backend::opengl::DispatchCmd& cmd);
    void pipelineBarrier(const snap::rhi::backend::opengl::PipelineBarrierCmd& cmd);
    void endEncoding(const snap::rhi::backend::opengl::EndComputePassCmd& cmd);

    void reset();

private:
    snap::rhi::backend::opengl::PipelineResourceState pipelineResourceState;
};
} // namespace snap::rhi::backend::opengl
