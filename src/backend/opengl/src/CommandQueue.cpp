#include "snap/rhi/backend/opengl/CommandQueue.hpp"
#include "snap/rhi/TextureInterop.h"
#include "snap/rhi/backend/common/TextureInterop.h"
#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/opengl/CommandBuffer.hpp"
#include "snap/rhi/backend/opengl/CommandIterator.hpp"
#include "snap/rhi/backend/opengl/Commands.h"
#include "snap/rhi/backend/opengl/Device.hpp"
#include "snap/rhi/backend/opengl/DeviceContext.hpp"
#include "snap/rhi/backend/opengl/ErrorCheckGuard.hpp"
#include "snap/rhi/backend/opengl/Fence.hpp"
#include "snap/rhi/backend/opengl/Semaphore.hpp"
#include "snap/rhi/common/Throw.h"

namespace {
void bindVAO(const snap::rhi::backend::opengl::Profile& gl, snap::rhi::backend::opengl::DeviceContext* dc) {
#if !SNAP_RHI_GL_ES
    /**
     * Core OpenGL(Desktop) requires that we use a VAO so it knows what to do with our vertex inputs.
     * If we fail to bind a VAO, OpenGL will most likely refuse to draw anything.
     */
    const GLuint vao = dc->getVAO();
    gl.bindVertexArray(vao);
    /**
     *  TODO: We can try to create a large list of VAOs inside the DeviceContext,
     *  for the key use a pair of RenderPipeline + VertextBuffers.
     */
#endif
}

void unbindVAO(const snap::rhi::backend::opengl::Profile& gl, snap::rhi::backend::opengl::DeviceContext* dc) {
#if !SNAP_RHI_GL_ES
    gl.bindVertexArray(0);
#endif
}

std::shared_ptr<snap::rhi::Fence> buildFence(snap::rhi::Fence*& fence,
                                             snap::rhi::backend::opengl::Device* glDevice,
                                             std::span<snap::rhi::CommandBuffer*> buffers) {
    if (fence) {
        return nullptr;
    }

    bool needFence = false;
    for (auto* cmdBuffer : buffers) {
        auto* glCmdBuffer =
            snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::CommandBuffer>(cmdBuffer);
        if (SNAP_RHI_ENABLE_SLOW_VALIDATIONS() ||
            !static_cast<bool>(glCmdBuffer->getCreateInfo().commandBufferCreateFlags &
                               snap::rhi::CommandBufferCreateFlags::UnretainedResources)) {
            needFence = true;
            break;
        }
    }

    if (!needFence) {
        return nullptr;
    }

    auto syncFenceRef = glDevice->getFencePool().acquireFence();
    fence = syncFenceRef.get();
    return syncFenceRef;
}
} // unnamed namespace

namespace snap::rhi::backend::opengl {
CommandQueue::CommandQueue(snap::rhi::backend::opengl::Device* device)
    : snap::rhi::CommandQueue(device),
      glesDevice(device),
      gl(device->getOpenGL()),
      blitPerformer(glesDevice),
      renderPerformer(glesDevice),
      computePerformer(glesDevice) {
    SNAP_RHI_REPORT(device->getValidationLayer(),
                    snap::rhi::ReportLevel::Info,
                    snap::rhi::ValidationTag::CreateOp,
                    "[snap::rhi::backend::opengl::CommandQueue] CommandQueue created");
}

CommandQueue::~CommandQueue() {
    SNAP_RHI_REPORT(glesDevice->getValidationLayer(),
                    snap::rhi::ReportLevel::Info,
                    snap::rhi::ValidationTag::DestroyOp,
                    "[snap::rhi::backend::opengl::CommandQueue] CommandQueue destroyed");
}

void CommandQueue::submitCommands(std::span<snap::rhi::Semaphore*> waitSemaphores,
                                  std::span<const snap::rhi::PipelineStageBits> waitDstStageMask,
                                  std::span<snap::rhi::CommandBuffer*> buffers,
                                  std::span<snap::rhi::Semaphore*> signalSemaphores,
                                  CommandBufferWaitType waitType,
                                  snap::rhi::Fence* fence) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[CommandQueue::submitCommands]");

    const auto commandQueueErrorCheckOptions = glesDevice->getGLCommandQueueErrorCheckOptions();
    auto onErrorCallback = []() {
        // Do nothing
    };
    ErrorCheckGuard glErrorCheckGuard(glesDevice,
                                      commandQueueErrorCheckOptions ==
                                          GLCommandQueueErrorCheckOptions::EnableAllGLErrorChecks,
                                      onErrorCallback);

    snap::rhi::DeviceContext* activeDeviceContext = device->getCurrentDeviceContext();
    assert(activeDeviceContext);

    if (!activeDeviceContext) {
        return;
    }

    DeviceContext* dc = snap::rhi::backend::common::smart_cast<DeviceContext>(activeDeviceContext);
    assert(dc);
    dc->validateContext();

    /**
     * Try to reclaim completed submissions to free up resources
     */
    auto& submissionTracker = glesDevice->getSubmissionTracker();
    submissionTracker.tryReclaim();

    {
        SNAP_RHI_ON_SCOPE_EXIT {
            for (auto* cmdBuffer : buffers) {
                auto* commandBuffer = snap::rhi::backend::common::smart_cast<CommandBuffer>(cmdBuffer);
                if (SNAP_RHI_ENABLE_SLOW_VALIDATIONS() ||
                    !static_cast<bool>(commandBuffer->getCreateInfo().commandBufferCreateFlags &
                                       snap::rhi::CommandBufferCreateFlags::UnretainedResources)) {
                    submissionTracker.track(commandBuffer, fence);
                }
            }

            glesDevice->clearLazyDestroyResources();
        };

        auto [interopSignalFence, interopWaitSemaphores] = common::processTextureInterop(buffers, fence);
        auto syncFenceRef = buildFence(fence, glesDevice, buffers);

        auto processWaitSemaphores = [](const auto& semaphores) {
            for (const auto& textureInteropWaitSemaphore : semaphores) {
                auto* ptr = &(*textureInteropWaitSemaphore);
                auto* semaphore = snap::rhi::backend::common::smart_cast<Semaphore>(ptr);
                semaphore->wait();
            }
        };
        processWaitSemaphores(waitSemaphores);
        processWaitSemaphores(interopWaitSemaphores);

        {
            SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[CommandQueue::submsitCommands][Execute all commands]");
            std::lock_guard<std::mutex> lock(accessMutex);

            {
                /**
                 * SnapRHI always resets OpenGL states after executing command buffers because some resources may
                 *be removed after doing so and any associated ones in the gl state cache will be invalid.
                 **/
                dc->resetGLStateCache();

                /**
                 * SnapRHI always resets internal states to recover  from exception handling.
                 **/
                resetStates();

                computePerformer.enableCache(dc);
                renderPerformer.enableCache(dc);
                blitPerformer.enableCache(dc);
            }

            SNAP_RHI_ON_SCOPE_EXIT {
                computePerformer.disableCache();
                renderPerformer.disableCache();
                blitPerformer.disableCache();

                if (dc) {
                    dc->freeUnusedFBOs();
                }

                /*
                 * Some OpenGL states need to be reset on the SnapRHI side,
                 * as even SnapRHI expected these states with disabled values.
                 *
                 * GL_CLIP_DISTANCE are very dangerous states,
                 * as they are implemented buggy in many drivers,
                 * so for now the safest approach for SnapRHI is to expect these states to be disabled by default,
                 * and disable them after each CommandBuffer submit.
                 *
                 * Potentially SnapRHI could improve this,
                 * but to make it safe SnapRHI would have to A/B about disabling all GL_CLIP_DISTANCE states by
                 * default.
                 *
                 * **/
                auto& glCache = dc->getGLStateCache();
                glCache.resetOpenGLClipDistance();
            };

            bindVAO(gl, dc);
            SNAP_RHI_ON_SCOPE_EXIT {
                unbindVAO(gl, dc);
            };

            for (auto* cmdBuffer : buffers) {
                auto* commandBuffer = snap::rhi::backend::common::smart_cast<CommandBuffer>(cmdBuffer);
                commandBuffer->finishRecording();
                auto& commandAllocator = commandBuffer->getCommandAllocator();
                {
                    /**
                     * Scissor test should be disabled by default, only through RenderCommandEncoder::setScissor it will
                     * be enabled
                     * */
                    gl.disable(GL_SCISSOR_TEST, dc);
                }
                replayCommands(commandAllocator, dc);
            }
        }

        for (size_t i = 0; i < signalSemaphores.size(); ++i) {
            Semaphore* semaphore = snap::rhi::backend::common::smart_cast<Semaphore>(signalSemaphores[i]);
            semaphore->signal();
        }

        if (fence) {
            auto* glesFence = snap::rhi::backend::common::smart_cast<Fence>(fence);
            glesFence->init();
        }

        if ((fence && dc->shouldFlushOnFenceCreation()) ||
            waitType == snap::rhi::CommandBufferWaitType::WaitUntilScheduled) {
            // We have to call glFlush to submit all OpenGL commands in proper order
            // https://www.khronos.org/opengl/wiki/Sync_Object
            waitUntilScheduled();
        }
    }

    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[CommandQueue::submitCommands][Execute CommandBufferWaitType]");
    if (waitType == snap::rhi::CommandBufferWaitType::WaitUntilCompleted) {
        waitIdle();
    }
}

void CommandQueue::resetStates() {
    blitPerformer.reset();
    renderPerformer.reset();
    computePerformer.reset();
    activeEncodingType = EncodingType::None;
}

void CommandQueue::replayCommands(snap::rhi::backend::opengl::CommandAllocator& commandAllocator,
                                  snap::rhi::backend::opengl::DeviceContext* dc) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[CommandQueue][recordCommands]");

    snap::rhi::backend::opengl::CommandIterator commandIterator{commandAllocator};
    snap::rhi::backend::opengl::Command command{};

    if (!dc) {
        snap::rhi::common::throwException<NoBoundDeviceContextException>(
            snap::rhi::common::stringFormat("[CommandRecorder::recordCommands] "
                                            "No current device context for device: %p",
                                            glesDevice));
    }

    while (commandIterator.nextCommandId(&command)) {
        switch (command) {
            // Common commands
            case snap::rhi::backend::opengl::Command::BindDescriptorSet: {
                SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[CommandQueue][BindDescriptorSet]");

                const snap::rhi::backend::opengl::BindDescriptorSetCmd* cmd =
                    commandIterator.nextCommand<snap::rhi::backend::opengl::BindDescriptorSetCmd>();

                switch (activeEncodingType) {
                    case EncodingType::Render: {
                        renderPerformer.bindDescriptorSet(*cmd);
                    } break;

                    case EncodingType::Compute: {
                        computePerformer.bindDescriptorSet(*cmd);
                    } break;

                    default:
                        SNAP_RHI_REPORT(glesDevice->getValidationLayer(),
                                        snap::rhi::ReportLevel::Error,
                                        snap::rhi::ValidationTag::CommandBufferOp,
                                        "[BindDescriptorSet] invalid encoder type");
                }
            } break;

            case snap::rhi::backend::opengl::Command::PipelineBarrier: {
                SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[CommandQueue][PipelineBarrier]");

                const snap::rhi::backend::opengl::PipelineBarrierCmd* cmd =
                    commandIterator.nextCommand<snap::rhi::backend::opengl::PipelineBarrierCmd>();

                switch (activeEncodingType) {
                    case EncodingType::Render: {
                        renderPerformer.pipelineBarrier(*cmd);
                    } break;

                    case EncodingType::Compute: {
                        computePerformer.pipelineBarrier(*cmd);
                    } break;

                    default:
                        SNAP_RHI_REPORT(glesDevice->getValidationLayer(),
                                        snap::rhi::ReportLevel::Error,
                                        snap::rhi::ValidationTag::CommandBufferOp,
                                        "[PipelineBarrier] invalid encoder type");
                }
            } break;

            // Render commands
            case snap::rhi::backend::opengl::Command::BeginRenderPass: {
                SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[CommandQueue][BeginRenderPass]");
                activeEncodingType = EncodingType::Render;

                auto* cmd = commandIterator.nextCommand<snap::rhi::backend::opengl::BeginRenderPassCmd>();
                renderPerformer.beginEncoding(*cmd);
            } break;

            case snap::rhi::backend::opengl::Command::BeginRenderPass1: {
                SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[CommandQueue][BeginRenderPass1]");
                activeEncodingType = EncodingType::Render;

                auto* cmd = commandIterator.nextCommand<snap::rhi::backend::opengl::BeginRenderPass1Cmd>();
                renderPerformer.beginEncoding(*cmd);
            } break;

            case snap::rhi::backend::opengl::Command::SetViewport: {
                SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[CommandQueue][SetViewport]");

                const snap::rhi::backend::opengl::SetViewportCmd* cmd =
                    commandIterator.nextCommand<snap::rhi::backend::opengl::SetViewportCmd>();
                renderPerformer.setViewport(*cmd);
            } break;

            case snap::rhi::backend::opengl::Command::SetRenderPipeline: {
                SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[CommandQueue][SetRenderPipeline]");

                const snap::rhi::backend::opengl::SetRenderPipelineCmd* cmd =
                    commandIterator.nextCommand<snap::rhi::backend::opengl::SetRenderPipelineCmd>();
                renderPerformer.bindRenderPipeline(*cmd);
            } break;

            case snap::rhi::backend::opengl::Command::SetVertexBuffers: {
                SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[CommandQueue][SetVertexBuffers]");

                const snap::rhi::backend::opengl::SetVertexBuffersCmd* cmd =
                    commandIterator.nextCommand<snap::rhi::backend::opengl::SetVertexBuffersCmd>();
                renderPerformer.bindVertexBuffers(*cmd);
            } break;

            case snap::rhi::backend::opengl::Command::SetVertexBuffer: {
                SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[CommandQueue][SetVertexBuffer]");

                const snap::rhi::backend::opengl::SetVertexBufferCmd* cmd =
                    commandIterator.nextCommand<snap::rhi::backend::opengl::SetVertexBufferCmd>();
                renderPerformer.bindVertexBuffer(*cmd);
            } break;

            case snap::rhi::backend::opengl::Command::SetIndexBuffer: {
                SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[CommandQueue][SetIndexBuffer]");

                const snap::rhi::backend::opengl::SetIndexBufferCmd* cmd =
                    commandIterator.nextCommand<snap::rhi::backend::opengl::SetIndexBufferCmd>();
                renderPerformer.bindIndexBuffer(*cmd);
            } break;

            case snap::rhi::backend::opengl::Command::Draw: {
                SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[CommandQueue][Draw]");

                const snap::rhi::backend::opengl::DrawCmd* cmd =
                    commandIterator.nextCommand<snap::rhi::backend::opengl::DrawCmd>();
                renderPerformer.draw(*cmd);
            } break;

            case snap::rhi::backend::opengl::Command::DrawIndexed: {
                SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[CommandQueue][DrawIndexed]");

                const auto* cmd = commandIterator.nextCommand<snap::rhi::backend::opengl::DrawIndexedCmd>();
                renderPerformer.drawIndexed(*cmd);
            } break;

            case snap::rhi::backend::opengl::Command::SetDepthBias: {
                SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[CommandQueue][SetDepthBias]");

                const snap::rhi::backend::opengl::SetDepthBiasCmd* cmd =
                    commandIterator.nextCommand<snap::rhi::backend::opengl::SetDepthBiasCmd>();
                renderPerformer.setDepthBias(*cmd);
            } break;

            case snap::rhi::backend::opengl::Command::SetStencilReference: {
                SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[CommandQueue][SetStencilReference]");

                const snap::rhi::backend::opengl::SetStencilReferenceCmd* cmd =
                    commandIterator.nextCommand<snap::rhi::backend::opengl::SetStencilReferenceCmd>();
                renderPerformer.setStencilReference(*cmd);
            } break;

            case snap::rhi::backend::opengl::Command::SetBlendConstants: {
                SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[CommandQueue][SetBlendConstants]");

                const snap::rhi::backend::opengl::SetBlendConstantsCmd* cmd =
                    commandIterator.nextCommand<snap::rhi::backend::opengl::SetBlendConstantsCmd>();
                renderPerformer.setBlendConstants(*cmd);
            } break;

            case snap::rhi::backend::opengl::Command::InvokeCustomCallback: {
                SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[CommandQueue][InvokeCustomCallback]");

                const snap::rhi::backend::opengl::InvokeCustomCallbackCmd* cmd =
                    commandIterator.nextCommand<snap::rhi::backend::opengl::InvokeCustomCallbackCmd>();
                renderPerformer.invokeCustomCallback(*cmd);
            } break;

            case snap::rhi::backend::opengl::Command::EndRenderPass: {
                SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[CommandQueue][EndRenderPass]");

                const snap::rhi::backend::opengl::EndRenderPassCmd* cmd =
                    commandIterator.nextCommand<snap::rhi::backend::opengl::EndRenderPassCmd>();
                renderPerformer.endEncoding(*cmd);

                activeEncodingType = EncodingType::None;
            } break;

            // Blit commands
            case snap::rhi::backend::opengl::Command::BeginBlitPass: {
                SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[CommandQueue][BeginBlitPass]");

                activeEncodingType = EncodingType::Blit;

                commandIterator.nextCommand<snap::rhi::backend::opengl::BeginBlitPassCmd>();
            } break;

            case snap::rhi::backend::opengl::Command::CopyBufferToBuffer: {
                SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[CommandQueue][CopyBufferToBuffer]");

                const snap::rhi::backend::opengl::CopyBufferToBufferCmd* cmd =
                    commandIterator.nextCommand<snap::rhi::backend::opengl::CopyBufferToBufferCmd>();
                blitPerformer.copyBuffer(*cmd);
            } break;

            case snap::rhi::backend::opengl::Command::CopyBufferToTexture: {
                SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[CommandQueue][CopyBufferToTexture]");

                const snap::rhi::backend::opengl::CopyBufferToTextureCmd* cmd =
                    commandIterator.nextCommand<snap::rhi::backend::opengl::CopyBufferToTextureCmd>();
                blitPerformer.copyBufferToTexture(*cmd);
            } break;

            case snap::rhi::backend::opengl::Command::CopyTextureToBuffer: {
                SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[CommandQueue][CopyTextureToBuffer]");

                const snap::rhi::backend::opengl::CopyTextureToBufferCmd* cmd =
                    commandIterator.nextCommand<snap::rhi::backend::opengl::CopyTextureToBufferCmd>();
                blitPerformer.copyTextureToBuffer(*cmd);
            } break;

            case snap::rhi::backend::opengl::Command::CopyTextureToTexture: {
                SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[CommandQueue][CopyTextureToTexture]");

                const snap::rhi::backend::opengl::CopyTextureToTextureCmd* cmd =
                    commandIterator.nextCommand<snap::rhi::backend::opengl::CopyTextureToTextureCmd>();
                blitPerformer.copyTexture(*cmd);
            } break;

            case snap::rhi::backend::opengl::Command::GenerateMipmap: {
                SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[CommandQueue][GenerateMipmap]");

                const snap::rhi::backend::opengl::GenerateMipmapCmd* cmd =
                    commandIterator.nextCommand<snap::rhi::backend::opengl::GenerateMipmapCmd>();
                blitPerformer.generateMipmap(*cmd);
            } break;

            case snap::rhi::backend::opengl::Command::EndBlitPass: {
                SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[CommandQueue][EndBlitPass]");

                commandIterator.nextCommand<snap::rhi::backend::opengl::EndBlitPassCmd>();

                activeEncodingType = EncodingType::None;
            } break;

            // Compute commands
            case snap::rhi::backend::opengl::Command::BeginComputePass: {
                SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[CommandQueue][BeginComputePass]");

                activeEncodingType = EncodingType::Compute;
                const snap::rhi::backend::opengl::BeginComputePassCmd* cmd =
                    commandIterator.nextCommand<snap::rhi::backend::opengl::BeginComputePassCmd>();

                computePerformer.beginEncoding(*cmd);
            } break;

            case snap::rhi::backend::opengl::Command::BindComputePipeline: {
                SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[CommandQueue][BindComputePipeline]");

                const snap::rhi::backend::opengl::BindComputePipelineCmd* cmd =
                    commandIterator.nextCommand<snap::rhi::backend::opengl::BindComputePipelineCmd>();
                computePerformer.bindComputePipeline(*cmd);
            } break;

            case snap::rhi::backend::opengl::Command::Dispatch: {
                SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[CommandQueue][Dispatch]");

                const snap::rhi::backend::opengl::DispatchCmd* cmd =
                    commandIterator.nextCommand<snap::rhi::backend::opengl::DispatchCmd>();

                computePerformer.dispatch(*cmd);
            } break;

            case snap::rhi::backend::opengl::Command::EndComputePass: {
                SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[CommandQueue][EndComputePass]");

                const snap::rhi::backend::opengl::EndComputePassCmd* cmd =
                    commandIterator.nextCommand<snap::rhi::backend::opengl::EndComputePassCmd>();

                computePerformer.endEncoding(*cmd);
                activeEncodingType = EncodingType::None;
            } break;

            case snap::rhi::backend::opengl::Command::BeginDebugGroup: {
                SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[CommandQueue][BeginDebugGroup]");

                const auto* cmd = commandIterator.nextCommand<snap::rhi::backend::opengl::BeginDebugGroupCmd>();
#if SNAP_RHI_ENABLE_DEBUG_LABELS
                gl.pushDebugGroup(cmd->labelBuffer.data(), static_cast<GLsizei>(cmd->labelSize));
#endif
            } break;

            case snap::rhi::backend::opengl::Command::EndDebugGroup: {
                SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[CommandQueue][EndDebugGroup]");

                commandIterator.nextCommand<snap::rhi::backend::opengl::EndDebugGroupCmd>();
#if SNAP_RHI_ENABLE_DEBUG_LABELS
                gl.popDebugGroup();
#endif
            } break;

            default: {
                snap::rhi::common::throwException<UnexpectedCommandException>(snap::rhi::common::stringFormat(
                    "[CommandRecorder] unsupported command: 0x%x", static_cast<uint32_t>(command)));
            }
        }
    }
}

void CommandQueue::waitUntilScheduled() {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[CommandQueue][waitUntilScheduled]");

    gl.flush();
}

void CommandQueue::waitIdle() {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[CommandQueue][waitIdle]");

    gl.finish();

    auto& submissionTracker = glesDevice->getSubmissionTracker();
    submissionTracker.tryReclaim();
}
} // namespace snap::rhi::backend::opengl
