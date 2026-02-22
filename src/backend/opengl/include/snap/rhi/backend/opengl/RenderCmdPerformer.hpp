//
//  RenderCmdPerformer.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 15.02.2022.
//

#pragma once

#include "snap/rhi/backend/opengl/CmdPerformer.hpp"
#include "snap/rhi/backend/opengl/CommandRecorderUtils.hpp"
#include "snap/rhi/backend/opengl/Commands.h"
#include "snap/rhi/backend/opengl/OpenGL.h"
#include "snap/rhi/backend/opengl/PipelineResourceState.hpp"
#include "snap/rhi/common/NonCopyable.h"

#include <memory>
#include <mutex>

namespace snap::rhi::backend::opengl {
class Device;
class DeviceContext;
class Profile;
class DescriptorSet;
class RenderPipeline;

class RenderCmdPerformer final : public CmdPerformer {
public:
    RenderCmdPerformer(snap::rhi::backend::opengl::Device* device);
    ~RenderCmdPerformer() = default;

    void beginEncoding(const snap::rhi::backend::opengl::BeginRenderPassCmd& cmd);
    void beginEncoding(const snap::rhi::backend::opengl::BeginRenderPass1Cmd& cmd);
    void setViewport(const snap::rhi::backend::opengl::SetViewportCmd& cmd);
    void setDepthBias(const snap::rhi::backend::opengl::SetDepthBiasCmd& cmd);
    void setStencilReference(const snap::rhi::backend::opengl::SetStencilReferenceCmd& cmd);
    void setBlendConstants(const snap::rhi::backend::opengl::SetBlendConstantsCmd& cmd);
    void invokeCustomCallback(const snap::rhi::backend::opengl::InvokeCustomCallbackCmd& cmd);
    void bindDescriptorSet(const snap::rhi::backend::opengl::BindDescriptorSetCmd& cmd);
    void bindVertexBuffers(const snap::rhi::backend::opengl::SetVertexBuffersCmd& cmd);
    void bindVertexBuffer(const snap::rhi::backend::opengl::SetVertexBufferCmd& cmd);
    void pipelineBarrier(const snap::rhi::backend::opengl::PipelineBarrierCmd& cmd);
    void bindIndexBuffer(const snap::rhi::backend::opengl::SetIndexBufferCmd& cmd);
    void draw(const snap::rhi::backend::opengl::DrawCmd& cmd);
    void drawIndexed(const snap::rhi::backend::opengl::DrawIndexedCmd& cmd);
    void bindRenderPipeline(const snap::rhi::backend::opengl::SetRenderPipelineCmd& cmd);
    void endEncoding(const snap::rhi::backend::opengl::EndRenderPassCmd& cmd);

    void reset();

private:
    snap::rhi::backend::opengl::FramebufferId beginRenderPass() const;
    void loadFBO() const;
    bool resolveAttachments() const;
    void endRenderPass() const;

    void attachRenderPipeline() const;

    void tryBindFBO();
    void applyResources();

    bool isDirty = false;
    snap::rhi::backend::opengl::RenderPipeline* activePipeline = nullptr;
    mutable snap::rhi::backend::opengl::RenderInfo renderInfo;
    snap::rhi::backend::opengl::PipelineResourceState pipelineResourceState;
};
} // namespace snap::rhi::backend::opengl
