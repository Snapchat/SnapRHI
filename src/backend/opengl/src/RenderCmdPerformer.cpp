#include "snap/rhi/backend/opengl/RenderCmdPerformer.hpp"
#include "snap/rhi/backend/opengl/Buffer.hpp"
#include "snap/rhi/backend/opengl/DescriptorSet.hpp"
#include "snap/rhi/backend/opengl/Device.hpp"
#include "snap/rhi/backend/opengl/DeviceContext.hpp"
#include "snap/rhi/backend/opengl/FramebufferDiscardFlag.h"
#include "snap/rhi/backend/opengl/OpenGL.h"
#include "snap/rhi/backend/opengl/Profile.hpp"
#include "snap/rhi/backend/opengl/RenderPipeline.hpp"
#include "snap/rhi/backend/opengl/Sampler.hpp"
#include "snap/rhi/backend/opengl/Texture.hpp"
#include "snap/rhi/backend/opengl/Utils.hpp"

#include <snap/rhi/common/Scope.h>

#include <iostream>

namespace {
constexpr uint32_t numColorTargets = uint32_t(snap::rhi::MaxColorAttachments);
constexpr uint32_t numDepthStencilTargets = 2; // DepthStencil (ES3 only) or Depth and Stencil.
constexpr uint32_t numTotalTargets = numColorTargets + numDepthStencilTargets;

bool shouldClear(const snap::rhi::AttachmentLoadOp loadOp, const bool UseClearOnDontCare) {
    switch (loadOp) {
        case snap::rhi::AttachmentLoadOp::Load:
            // do nothing
            return false;

        case snap::rhi::AttachmentLoadOp::Clear:
            return true;

        case snap::rhi::AttachmentLoadOp::DontCare:
            return UseClearOnDontCare;

        default:
            snap::rhi::common::throwException("invalid load operation");
    }
}

snap::rhi::RenderingInfo buildRenderingInfo(snap::rhi::Framebuffer* framebuffer,
                                            snap::rhi::RenderPass* renderPass,
                                            std::span<const snap::rhi::ClearValue> clearValues) {
    const auto& framebufferCreateInfo = framebuffer->getCreateInfo();
    const auto& attachments = framebufferCreateInfo.attachments;
    const auto& renderPassInfo = renderPass->getCreateInfo();

    snap::rhi::RenderingInfo renderingInfo{};

    if (renderPassInfo.subpassCount > 1) {
        snap::rhi::common::throwException("[SnapRHI Metal] support only 1 subpasss");
    }
    const auto& subpassDescription = renderPassInfo.subpasses[0];
    uint32_t numLayers = 0;

    // If viewMask is not 0, multiview is enabled.
    uint32_t viewMask = subpassDescription.viewMask;

    renderingInfo.colorAttachments.resize(subpassDescription.colorAttachmentCount);
    for (uint32_t i = 0; i < subpassDescription.colorAttachmentCount; ++i) {
        const uint32_t attachmentIdx = subpassDescription.colorAttachments[i].attachment;
        assert(attachmentIdx < renderPassInfo.attachmentCount);
        auto* texture = attachments[attachmentIdx];
        numLayers = std::max(texture->getCreateInfo().size.depth, numLayers);

        const auto& attachmentInfo = renderPassInfo.attachments[attachmentIdx];

        renderingInfo.colorAttachments[i].storeOp = attachmentInfo.storeOp;
        renderingInfo.colorAttachments[i].loadOp = attachmentInfo.loadOp;
        if (attachmentInfo.loadOp == snap::rhi::AttachmentLoadOp::Clear) {
            assert(attachmentIdx < clearValues.size());
            renderingInfo.colorAttachments[i].clearValue = clearValues[attachmentIdx];
        }

        renderingInfo.colorAttachments[i].attachment.texture = texture;
        renderingInfo.colorAttachments[i].attachment.mipLevel = attachmentInfo.mipLevel;
        renderingInfo.colorAttachments[i].attachment.layer = attachmentInfo.layer;

        if (subpassDescription.resolveAttachmentCount) {
            const uint32_t resolveAttachmentIdx = subpassDescription.resolveAttachments[i].attachment;
            if (resolveAttachmentIdx != snap::rhi::AttachmentUnused) {
                assert(resolveAttachmentIdx < renderPassInfo.attachmentCount);
                auto* resolveTexture = attachments[resolveAttachmentIdx];
                const auto& resolveAttachmentInfo = renderPassInfo.attachments[resolveAttachmentIdx];

                renderingInfo.colorAttachments[i].resolveAttachment.texture = resolveTexture;
                renderingInfo.colorAttachments[i].resolveAttachment.mipLevel = resolveAttachmentInfo.mipLevel;
                renderingInfo.colorAttachments[i].resolveAttachment.layer = resolveAttachmentInfo.layer;
            } else if (attachmentInfo.autoresolvedAttachment.has_value()) {
                renderingInfo.colorAttachments[i].autoresolvedAttachment.emplace(
                    *attachmentInfo.autoresolvedAttachment);
            }
        }
    }

    const uint32_t depthAttachmentIdx = subpassDescription.depthStencilAttachment.attachment;
    if (depthAttachmentIdx != snap::rhi::AttachmentUnused) {
        assert(depthAttachmentIdx < renderPassInfo.attachmentCount);
        auto* texture = attachments[depthAttachmentIdx];
        numLayers = std::max(texture->getCreateInfo().size.depth, numLayers);
        assert(numLayers == 1 || viewMask == 0);
        const auto& textureInfo = texture->getCreateInfo();
        const auto& attachmentInfo = renderPassInfo.attachments[depthAttachmentIdx];

        renderingInfo.depthAttachment.attachment.texture = texture;
        renderingInfo.depthAttachment.attachment.mipLevel = attachmentInfo.mipLevel;
        renderingInfo.depthAttachment.attachment.layer = attachmentInfo.layer;

        renderingInfo.depthAttachment.storeOp = attachmentInfo.storeOp;
        renderingInfo.depthAttachment.loadOp = attachmentInfo.loadOp;
        if (attachmentInfo.loadOp == snap::rhi::AttachmentLoadOp::Clear) {
            assert(depthAttachmentIdx < clearValues.size());
            renderingInfo.depthAttachment.clearValue = clearValues[depthAttachmentIdx];
        }

        if (hasStencilAspect(textureInfo.format)) {
            renderingInfo.stencilAttachment.attachment = renderingInfo.depthAttachment.attachment;
            renderingInfo.stencilAttachment.storeOp = attachmentInfo.stencilStoreOp;
            renderingInfo.stencilAttachment.loadOp = attachmentInfo.stencilLoadOp;
            if (attachmentInfo.stencilLoadOp == snap::rhi::AttachmentLoadOp::Clear) {
                assert(depthAttachmentIdx < clearValues.size());
                renderingInfo.stencilAttachment.clearValue = clearValues[depthAttachmentIdx];
            }
        }

        const uint32_t resolveDepthAttachmentIdx = subpassDescription.resolveDepthStencilAttachment.attachment;
        if (resolveDepthAttachmentIdx != snap::rhi::AttachmentUnused) {
            assert(resolveDepthAttachmentIdx < renderPassInfo.attachmentCount);
            auto* resolveTexture = attachments[resolveDepthAttachmentIdx];

            const auto& resolveTextureInfo = resolveTexture->getCreateInfo();
            const auto& resolveAttachmentInfo = renderPassInfo.attachments[resolveDepthAttachmentIdx];

            renderingInfo.depthAttachment.resolveAttachment.texture = resolveTexture;
            renderingInfo.depthAttachment.resolveAttachment.mipLevel = resolveAttachmentInfo.mipLevel;
            renderingInfo.depthAttachment.resolveAttachment.layer = resolveAttachmentInfo.layer;

            if (hasStencilAspect(resolveTextureInfo.format)) {
                renderingInfo.stencilAttachment.resolveAttachment = renderingInfo.depthAttachment.resolveAttachment;
            }
        } else if (attachmentInfo.autoresolvedAttachment.has_value()) {
            renderingInfo.depthAttachment.autoresolvedAttachment.emplace(*attachmentInfo.autoresolvedAttachment);
            if (hasStencilAspect(textureInfo.format)) {
                renderingInfo.stencilAttachment.autoresolvedAttachment.emplace(*attachmentInfo.autoresolvedAttachment);
            }
        }
    }

    renderingInfo.layers = numLayers;
    renderingInfo.viewMask = viewMask;
    return renderingInfo;
}

auto prepareDiscardTargetsOnStore(const snap::rhi::RenderingInfo& renderingInfo, uint32_t& numCurrTargets) {
    numCurrTargets = 0;
    std::array<snap::rhi::backend::opengl::FramebufferAttachmentTarget, numTotalTargets> targets = {};
    std::fill(targets.begin(), targets.end(), snap::rhi::backend::opengl::FramebufferAttachmentTarget::Detached);

    for (uint32_t attachmentID = 0; attachmentID < renderingInfo.colorAttachments.size(); ++attachmentID) {
        const auto& colorInfo = renderingInfo.colorAttachments[attachmentID];
        assert(colorInfo.attachment.texture);

        switch (colorInfo.storeOp) {
            case snap::rhi::AttachmentStoreOp::DontCare: {
                targets[numCurrTargets++] =
                    snap::rhi::backend::opengl::getFramebufferColorAttachmentTarget(attachmentID);
            } break;

            default:
                assert(colorInfo.storeOp == snap::rhi::AttachmentStoreOp::Store);
                break;
        }
    }

    if (renderingInfo.depthAttachment.attachment.texture) {
        const auto& depthAttachmentInfo = renderingInfo.depthAttachment;
        const auto& texCreateInfo = depthAttachmentInfo.attachment.texture->getCreateInfo();

        // iPhone 7+, glInvalidateFramebuffer causes the GL_INVALID_ENUM error.
        // https://github.com/bkaradzic/bgfx/pull/949
        // https://stackoverflow.com/questions/37950562/glinvalidateframebuffer-generate-invalid-enum-opengl-es-3-0

        if (snap::rhi::hasDepthAspect(texCreateInfo.format) &&
            (depthAttachmentInfo.storeOp == snap::rhi::AttachmentStoreOp::DontCare)) {
            targets[numCurrTargets++] = snap::rhi::backend::opengl::FramebufferAttachmentTarget::DepthAttachment;
        }
    }

    if (renderingInfo.stencilAttachment.attachment.texture) {
        const auto& stencilAttachmentInfo = renderingInfo.stencilAttachment;
        const auto& texCreateInfo = stencilAttachmentInfo.attachment.texture->getCreateInfo();

        // iPhone 7+, glInvalidateFramebuffer causes the GL_INVALID_ENUM error.
        // https://github.com/bkaradzic/bgfx/pull/949
        // https://stackoverflow.com/questions/37950562/glinvalidateframebuffer-generate-invalid-enum-opengl-es-3-0

        if (snap::rhi::hasStencilAspect(texCreateInfo.format) &&
            (stencilAttachmentInfo.storeOp == snap::rhi::AttachmentStoreOp::DontCare)) {
            targets[numCurrTargets++] = snap::rhi::backend::opengl::FramebufferAttachmentTarget::StencilAttachment;
        }
    }

    return targets;
}

auto prepareDiscardTargetsOnLoad(const snap::rhi::RenderingInfo& renderingInfo, uint32_t& numCurrTargets) {
    numCurrTargets = 0;
    std::array<snap::rhi::backend::opengl::FramebufferAttachmentTarget, numTotalTargets> targets = {};
    std::fill(targets.begin(), targets.end(), snap::rhi::backend::opengl::FramebufferAttachmentTarget::Detached);

    for (uint32_t attachmentID = 0; attachmentID < renderingInfo.colorAttachments.size(); ++attachmentID) {
        const auto& colorInfo = renderingInfo.colorAttachments[attachmentID];
        assert(colorInfo.attachment.texture);

        switch (colorInfo.loadOp) {
            case snap::rhi::AttachmentLoadOp::DontCare: {
                targets[numCurrTargets++] =
                    snap::rhi::backend::opengl::getFramebufferColorAttachmentTarget(attachmentID);
            } break;

            default:
                // Do nothin for other load operations
                break;
        }
    }

    if (renderingInfo.depthAttachment.attachment.texture) {
        const auto& depthAttachmentInfo = renderingInfo.depthAttachment;
        const auto& texCreateInfo = depthAttachmentInfo.attachment.texture->getCreateInfo();

        if (snap::rhi::hasDepthAspect(texCreateInfo.format) &&
            (depthAttachmentInfo.loadOp == snap::rhi::AttachmentLoadOp::DontCare)) {
            targets[numCurrTargets++] = snap::rhi::backend::opengl::FramebufferAttachmentTarget::DepthAttachment;
        }
    }

    if (renderingInfo.stencilAttachment.attachment.texture) {
        const auto& stencilAttachmentInfo = renderingInfo.stencilAttachment;
        const auto& texCreateInfo = stencilAttachmentInfo.attachment.texture->getCreateInfo();

        if (snap::rhi::hasStencilAspect(texCreateInfo.format) &&
            (stencilAttachmentInfo.loadOp == snap::rhi::AttachmentLoadOp::DontCare)) {
            targets[numCurrTargets++] = snap::rhi::backend::opengl::FramebufferAttachmentTarget::StencilAttachment;
        }
    }

    return targets;
}

/// @see AttachmentDescription::samples
/// @see snap::rhi::Capabilities::isMultiviewMSAAImplicitResolveEnabled
snap::rhi::SampleCount selectSampleCountForAttachment(const snap::rhi::Capabilities& caps,
                                                      const snap::rhi::RenderingAttachmentInfo& att,
                                                      const snap::rhi::TextureCreateInfo& textureCreateInfo) {
    bool isAutoresolvedSamplesDefined =
        caps.isMultiviewMSAAImplicitResolveEnabled && textureCreateInfo.size.depth > 1 &&
        att.autoresolvedAttachment.has_value() &&
        att.autoresolvedAttachment->autoresolvedSamples > snap::rhi::SampleCount::Count1;
    return isAutoresolvedSamplesDefined ? att.autoresolvedAttachment->autoresolvedSamples :
                                          textureCreateInfo.sampleCount;
}

snap::rhi::backend::opengl::FramebufferDescription buildFBODescription(snap::rhi::backend::opengl::DeviceContext* dc,
                                                                       const snap::rhi::RenderingInfo& renderingInfo) {
    assert(dc);
    assert(dc->getDevice());
    auto& caps = dc->getDevice()->getCapabilities();

    snap::rhi::backend::opengl::FramebufferDescription result{};

    const uint32_t numLayers = renderingInfo.layers ? renderingInfo.layers : 1;

    uint32_t viewMask = renderingInfo.viewMask;

    // Assert that stereoscopic multiview uses more than one layer.
    // If viewMask is not 0, multiview is enabled.
    assert(viewMask == 0 || numLayers > 1);

    for (uint32_t i = 0; i < renderingInfo.colorAttachments.size(); ++i) {
        auto& attDesc = renderingInfo.colorAttachments[i];
        auto* att = attDesc.attachment.texture;
        auto& createInfo = att->getCreateInfo();
        const snap::rhi::SampleCount numSamples = selectSampleCountForAttachment(caps, attDesc, createInfo);

        auto* glTexture = snap::rhi::backend::common::smart_cast<const snap::rhi::backend::opengl::Texture>(att);
        if (!glTexture) {
            snap::rhi::common::throwException<snap::rhi::InvalidArgumentException>("Unexpected texture type");
        }

        snap::rhi::TextureSubresourceRange subresRange = {};
        subresRange.baseArrayLayer = attDesc.attachment.layer;
        subresRange.baseMipLevel = attDesc.attachment.mipLevel;
        subresRange.layerCount = numLayers;
        subresRange.levelCount = 1;

        result.colorAttachments[result.numColorAttachments++] =
            snap::rhi::backend::opengl::buildFramebufferAttachment(dc, glTexture, subresRange, numSamples, viewMask);
    }

    if (renderingInfo.depthAttachment.attachment.texture) {
        auto& attDesc = renderingInfo.depthAttachment;
        auto* att = attDesc.attachment.texture;
        auto& createInfo = att->getCreateInfo();
        const snap::rhi::SampleCount numSamples = selectSampleCountForAttachment(caps, attDesc, createInfo);

        auto* glTexture = snap::rhi::backend::common::smart_cast<const snap::rhi::backend::opengl::Texture>(att);
        if (!glTexture) {
            snap::rhi::common::throwException<snap::rhi::InvalidArgumentException>("Unexpected texture type");
        }

        snap::rhi::TextureSubresourceRange subresRange = {};
        subresRange.baseArrayLayer = attDesc.attachment.layer;
        subresRange.baseMipLevel = attDesc.attachment.mipLevel;
        subresRange.layerCount = numLayers;
        subresRange.levelCount = 1;

        auto dsTraits = snap::rhi::getDepthStencilFormatTraits(createInfo.format);
        if (dsTraits != snap::rhi::DepthStencilFormatTraits::None) {
            result.depthStencilAttachment = snap::rhi::backend::opengl::buildFramebufferAttachment(
                dc, glTexture, subresRange, numSamples, viewMask);
        } else {
            snap::rhi::common::throwException<snap::rhi::InvalidArgumentException>(snap::rhi::common::stringFormat(
                "[bindFramebuffer] Unexpected depth stencil format: 0x%x", static_cast<uint32_t>(createInfo.format)));
        }
    }

    if (result.depthStencilAttachment.texId == snap::rhi::backend::opengl::TextureId::Null &&
        renderingInfo.stencilAttachment.attachment.texture != nullptr) {
        auto& attDesc = renderingInfo.stencilAttachment;
        auto* att = attDesc.attachment.texture;
        auto& createInfo = att->getCreateInfo();
        const snap::rhi::SampleCount numSamples = createInfo.sampleCount;

        auto* glTexture = snap::rhi::backend::common::smart_cast<const snap::rhi::backend::opengl::Texture>(att);
        if (!glTexture) {
            snap::rhi::common::throwException<snap::rhi::InvalidArgumentException>("Unexpected texture type");
        }

        snap::rhi::TextureSubresourceRange subresRange = {};
        subresRange.baseArrayLayer = attDesc.attachment.layer;
        subresRange.baseMipLevel = attDesc.attachment.mipLevel;
        subresRange.layerCount = numLayers;
        subresRange.levelCount = 1;

        auto dsTraits = snap::rhi::getDepthStencilFormatTraits(createInfo.format);
        if (dsTraits != snap::rhi::DepthStencilFormatTraits::None) {
            result.depthStencilAttachment = snap::rhi::backend::opengl::buildFramebufferAttachment(
                dc, glTexture, subresRange, numSamples, viewMask);
        } else {
            snap::rhi::common::throwException<snap::rhi::InvalidArgumentException>(snap::rhi::common::stringFormat(
                "[bindFramebuffer] Unexpected depth stencil format: 0x%x", static_cast<uint32_t>(createInfo.format)));
        }
    }

    return result;
}
} // unnamed namespace

namespace snap::rhi::backend::opengl {
RenderCmdPerformer::RenderCmdPerformer(snap::rhi::backend::opengl::Device* device)
    : CmdPerformer(device), renderInfo(gl), pipelineResourceState(device) {}

void RenderCmdPerformer::setViewport(const snap::rhi::backend::opengl::SetViewportCmd& cmd) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[RenderCmdPerformer][setViewport]");

    gl.viewport(cmd.x, cmd.y, cmd.width, cmd.height, dc);
    gl.depthRangef(cmd.znear, cmd.zfar);
}

void RenderCmdPerformer::setDepthBias(const snap::rhi::backend::opengl::SetDepthBiasCmd& cmd) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[RenderCmdPerformer][setDepthBias]");
    if (cmd.depthBiasClamp != 0.0f) {
        // https://www.khronos.org/registry/OpenGL/extensions/EXT/EXT_polygon_offset_clamp.txt
        snap::rhi::common::throwException<UnsupportedOperationException>(
            "[SetDepthBias] OpenGL ES 3.0 doesn't support depthBiasClamp, "
            "please use the default value - 0.0f");
    }

    gl.polygonOffset(cmd.depthBiasSlopeFactor, cmd.depthBiasConstantFactor, dc);
}

void RenderCmdPerformer::setStencilReference(const snap::rhi::backend::opengl::SetStencilReferenceCmd& cmd) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[RenderCmdPerformer][setStencilReference]");

    snap::rhi::backend::opengl::setStencilReference(gl, renderInfo, cmd.face, cmd.reference, dc);
}

void RenderCmdPerformer::setBlendConstants(const snap::rhi::backend::opengl::SetBlendConstantsCmd& cmd) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[RenderCmdPerformer][setBlendConstants]");

    gl.blendColor(cmd.r, cmd.g, cmd.b, cmd.a, dc);
}

void RenderCmdPerformer::invokeCustomCallback(const snap::rhi::backend::opengl::InvokeCustomCallbackCmd& cmd) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[RenderCmdPerformer][invokeCustomCallback]");
    tryBindFBO();

    DeviceContext* curDC = dc;
    disableCache();

    SNAP_RHI_ON_SCOPE_EXIT {
        if (curDC) {
            GLStateCache& cache = curDC->getGLStateCache();

            cache.reset();
            enableCache(curDC);
        }
    };

    (*cmd.callback)();
}

void RenderCmdPerformer::pipelineBarrier(const snap::rhi::backend::opengl::PipelineBarrierCmd& cmd) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[RenderCmdPerformer][pipelineBarrier]");

    barrier(gl, cmd);
}

void RenderCmdPerformer::attachRenderPipeline() const {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[RenderCmdPerformer][attachRenderPipeline]");
    renderInfo.renderPipeline->useProgram(dc);

    const auto& info = renderInfo.renderPipeline->getCreateInfo();

    const auto& srcStates = renderInfo.renderPipeline->getStatesUUID();
    auto& dstStates = renderInfo.renderPipelineStatesUUID;

    {
        { // States that effected by clear FBO attachements
            uint32_t colorBlendStatesUpdateCnt = 0;
            for (uint32_t i = 0; i < info.colorBlendState.colorAttachmentsCount; ++i) {
                if (dstStates.colorAttachmentsBlendStates[i] != srcStates.colorAttachmentsBlendStates[i]) {
                    setupColorBlendState(gl, info.colorBlendState, dc);
                    ++colorBlendStatesUpdateCnt;
                }
                dstStates.colorAttachmentsBlendStates[i] = srcStates.colorAttachmentsBlendStates[i];
            }

            if (colorBlendStatesUpdateCnt == info.colorBlendState.colorAttachmentsCount) {
                if (renderInfo.restoreMask & GL_COLOR_BUFFER_BIT) {
                    renderInfo.restoreMask ^= GL_COLOR_BUFFER_BIT;
                }
            }

            if (dstStates.depthStencilState != srcStates.depthStencilState) {
                setupDepthSettings(gl, info.depthStencilState, dc);
                dstStates.depthStencilState = srcStates.depthStencilState;
                if (renderInfo.restoreMask & GL_DEPTH_BUFFER_BIT) {
                    renderInfo.restoreMask ^= GL_DEPTH_BUFFER_BIT;
                }
            }

            { // Stencil settings
                setupStencilSettings(gl, info.depthStencilState, renderInfo.stencilReference, dc);
                if (renderInfo.restoreMask & GL_STENCIL_BUFFER_BIT) {
                    renderInfo.restoreMask ^= GL_STENCIL_BUFFER_BIT;
                }
            }
            restoreAttachmentsMask(dc, renderInfo.renderPipeline, renderInfo.restoreMask);
        }

        if (dstStates.rasterizationState != srcStates.rasterizationState) {
            setupRasterizationState(gl, info.rasterizationState, dc);
            dstStates.rasterizationState = srcStates.rasterizationState;
        }

        if (dstStates.multisampleState != srcStates.multisampleState) {
            setupMultisampleState(gl, info.multisampleState, dc);
            dstStates.multisampleState = srcStates.multisampleState;
        }

        if (dstStates.inputAssemblyState != srcStates.inputAssemblyState) {
            setupInputAssemblyState(gl, info.inputAssemblyState, dc);
            dstStates.inputAssemblyState = srcStates.inputAssemblyState;
        }
    }
}

void RenderCmdPerformer::bindDescriptorSet(const snap::rhi::backend::opengl::BindDescriptorSetCmd& cmd) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[RenderCmdPerformer][bindDescriptorSet]");

    assert(cmd.binding < snap::rhi::SupportedLimit::MaxBoundDescriptorSets);
    pipelineResourceState.bindDescriptorSet(
        cmd.binding,
        snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::DescriptorSet>(cmd.descriptorSet),
        cmd.dynamicOffsets);
}

void RenderCmdPerformer::bindVertexBuffers(const snap::rhi::backend::opengl::SetVertexBuffersCmd& cmd) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[RenderCmdPerformer][bindVertexBuffers]");

    for (uint32_t i = 0; i < cmd.vertexBuffersCount; ++i) {
        const uint32_t bufferIdx = cmd.firstBinding + i;
        assert(bufferIdx < snap::rhi::MaxVertexBuffers);

        renderInfo.vertexBuffers.setBuffer(
            bufferIdx,
            snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Buffer>(cmd.vertexBuffers[i]),
            cmd.offsets[i]);
    }
}

void RenderCmdPerformer::bindVertexBuffer(const snap::rhi::backend::opengl::SetVertexBufferCmd& cmd) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[RenderCmdPerformer][bindVertexBuffer]");

    assert(cmd.binding < snap::rhi::MaxVertexBuffers);
    renderInfo.vertexBuffers.setBuffer(
        cmd.binding,
        snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Buffer>(cmd.vertexBuffer),
        cmd.offset);
}

void RenderCmdPerformer::loadFBO() const {
    GLbitfield clearMask = GL_NONE;
    renderInfo.restoreMask = GL_NONE;

    uint32_t numDiscardTargets = 0;
    std::array<snap::rhi::backend::opengl::FramebufferAttachmentTarget, numTotalTargets> discardTargets{};

    bool useClearOnDontCare = false;

    const auto loadOpBehavior = common::smart_cast<Device>(gl.getDevice())->getLoadOpDontCareBehavior();
    switch (loadOpBehavior) {
        case snap::rhi::backend::opengl::LoadOpDontCareBehavior::DoNothing: {
            useClearOnDontCare = false;
        } break;

        case snap::rhi::backend::opengl::LoadOpDontCareBehavior::Clear: {
            useClearOnDontCare = true;
        } break;

        case snap::rhi::backend::opengl::LoadOpDontCareBehavior::Discard: {
            useClearOnDontCare = false;
            discardTargets = prepareDiscardTargetsOnLoad(renderInfo.renderingInfo, numDiscardTargets);
        } break;
    }

    if (renderInfo.renderingInfo.depthAttachment.attachment.texture) {
        const auto& depthInfo = renderInfo.renderingInfo.depthAttachment;

        if (shouldClear(depthInfo.loadOp, useClearOnDontCare)) {
            clearMask |= GL_DEPTH_BUFFER_BIT;
            renderInfo.restoreMask |= GL_DEPTH_BUFFER_BIT;
            gl.clearDepth(depthInfo.clearValue.depthStencil.depth);
        }
    }

    if (renderInfo.renderingInfo.stencilAttachment.attachment.texture) {
        const auto& stencilInfo = renderInfo.renderingInfo.stencilAttachment;

        if (shouldClear(stencilInfo.loadOp, useClearOnDontCare)) {
            clearMask |= GL_STENCIL_BUFFER_BIT;
            renderInfo.restoreMask |= GL_STENCIL_BUFFER_BIT;
            gl.clearStencil(stencilInfo.clearValue.depthStencil.stencil);
        }
    }

    if (renderInfo.renderingInfo.colorAttachments.size() == 1) { // None MRT case
        const auto& colorInfo = renderInfo.renderingInfo.colorAttachments[0];
        const auto& textureInfo = colorInfo.attachment.texture->getCreateInfo();
        if (shouldClear(colorInfo.loadOp, useClearOnDontCare)) {
            const bool isNonFloatFormat = isIntFormat(textureInfo.format) || isUintFormat(textureInfo.format);
            if (!isNonFloatFormat) {
                clearMask |= GL_COLOR_BUFFER_BIT;
                renderInfo.restoreMask |= GL_COLOR_BUFFER_BIT;
                gl.clearColor(colorInfo.clearValue.color.float32[0],
                              colorInfo.clearValue.color.float32[1],
                              colorInfo.clearValue.color.float32[2],
                              colorInfo.clearValue.color.float32[3]);
            }
        }
    }

    if (!(clearMask & GL_COLOR_BUFFER_BIT)) { // MRT and/or non-float cases
        for (uint32_t attachmentID = 0; attachmentID < renderInfo.renderingInfo.colorAttachments.size();
             ++attachmentID) {
            const auto& colorInfo = renderInfo.renderingInfo.colorAttachments[attachmentID];
            const auto& textureInfo = colorInfo.attachment.texture->getCreateInfo();

            if (shouldClear(colorInfo.loadOp, useClearOnDontCare)) {
                if (!(renderInfo.restoreMask & GL_COLOR_BUFFER_BIT)) {
                    renderInfo.restoreMask |= GL_COLOR_BUFFER_BIT;
                    gl.colorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, dc);
                }

                if (isIntFormat(textureInfo.format)) {
                    gl.clearBufferiv(GL_COLOR, attachmentID, colorInfo.clearValue.color.int32.data());
                } else if (isUintFormat(textureInfo.format)) {
                    gl.clearBufferuiv(GL_COLOR, attachmentID, colorInfo.clearValue.color.uint32.data());
                } else {
                    gl.clearBufferfv(GL_COLOR, attachmentID, colorInfo.clearValue.color.float32.data());
                }
            }
        }
    }

    if (clearMask != GL_NONE) {
        { // SnapRHI must enable a color/depth/stencil mask to clear attachments
            if (clearMask & GL_DEPTH_BUFFER_BIT) {
                gl.depthMask(GL_TRUE);
            }

            if (clearMask & GL_STENCIL_BUFFER_BIT) {
                gl.stencilMaskSeparate(
                    GL_FRONT_AND_BACK, static_cast<uint32_t>(snap::rhi::StencilMask::AllBitsMask), dc);
            }

            if (clearMask & GL_COLOR_BUFFER_BIT) {
                gl.colorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, dc);
            }
        }

        gl.clear(clearMask);
    }

    if (numDiscardTargets > 0) {
        gl.discardFramebuffer(renderInfo.target, static_cast<GLsizei>(numDiscardTargets), discardTargets.data());
    }
}

snap::rhi::backend::opengl::FramebufferId RenderCmdPerformer::beginRenderPass() const {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[RenderCmdPerformer][beginRenderPass]");
    FramebufferDescription fboDesc = buildFBODescription(dc, renderInfo.renderingInfo);
    renderInfo.target = fboDesc.isMultiview()
                            // NOTE: That's a requirement for multiview attachments!
                            ?
                            snap::rhi::backend::opengl::FramebufferTarget::DrawFramebuffer :
                            snap::rhi::backend::opengl::FramebufferTarget::Framebuffer;
    FramebufferId fboID = dc->bindFramebuffer(renderInfo.target, fboDesc);
    {
        renderInfo.drawBuffers.resize(renderInfo.renderingInfo.colorAttachments.size());
        for (uint32_t i = 0; i < renderInfo.renderingInfo.colorAttachments.size(); ++i) {
            renderInfo.drawBuffers[i] = getFramebufferColorAttachmentTarget(i);
        }

        if (fboID != FramebufferId::CurrSurfaceBackbuffer) {
            gl.drawBuffers(static_cast<GLsizei>(renderInfo.drawBuffers.size()),
                           renderInfo.drawBuffers.empty() ? nullptr : renderInfo.drawBuffers.data());
            gl.readBuffer(snap::rhi::backend::opengl::FramebufferAttachmentTarget::Detached);
        }
        gl.validateFramebuffer(renderInfo.target, fboID, fboDesc);
    }

    loadFBO();

    return fboID;
}

bool RenderCmdPerformer::resolveAttachments() const {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[RenderCmdPerformer][resolveAttachments]");
    bool hasResolvedAttachments = false;

    /**
     * Some devices may have GPU issues that require glFlush before resolving MSAA
     * (without glFlush 'resolve' will essentially do nothing),
     * so SnapRHI provides an API to explicitly specify such devices.
     */
    bool isFlushed = false;
    auto tryFlushBeforeResolve = [this, &isFlushed]() {
        if (!isFlushed &&
            device->getResolveOpBehavior() == snap::rhi::backend::opengl::ResolveOpBehavior::FlushBeforeResolve) {
            gl.flush();
            isFlushed = true;
        }
    };

    uint32_t colorAttachmentId = 0;
    for (uint32_t i = 0; i < renderInfo.renderingInfo.colorAttachments.size(); ++i) {
        const auto& colorAttachment = renderInfo.renderingInfo.colorAttachments[i];
        if (colorAttachment.attachment.texture == nullptr || colorAttachment.resolveAttachment.texture == nullptr) {
            continue;
        }

        colorAttachmentId = i;
        hasResolvedAttachments = true;

        tryFlushBeforeResolve();
        blitTextures(dc,
                     renderInfo.renderPipeline,
                     colorAttachment.attachment,
                     colorAttachment.resolveAttachment,
                     GL_COLOR_BUFFER_BIT);
    }

    GLbitfield depthStencilMask = GL_DEPTH_BUFFER_BIT;
    if (renderInfo.renderingInfo.depthAttachment.attachment.texture != nullptr &&
        renderInfo.renderingInfo.depthAttachment.resolveAttachment.texture != nullptr) {
        SNAP_RHI_VALIDATE(
            gl.getDevice()->getValidationLayer(),
            hasResolvedAttachments,
            snap::rhi::ReportLevel::Error,
            snap::rhi::ValidationTag::RenderPassOp,
            "[resolveAttachments] Must provide at least one color attachment when performing an msaa depth resolve.");

        const auto& depthAttachment = renderInfo.renderingInfo.depthAttachment;
        const auto& colorAttachment = renderInfo.renderingInfo.colorAttachments[colorAttachmentId];

        const auto& depthTextureInfo = depthAttachment.attachment.texture->getCreateInfo();
        const auto& depthResolveTextureInfo = depthAttachment.resolveAttachment.texture->getCreateInfo();

        if (renderInfo.renderingInfo.stencilAttachment.attachment.texture == depthAttachment.attachment.texture &&
            renderInfo.renderingInfo.stencilAttachment.resolveAttachment.texture ==
                depthAttachment.resolveAttachment.texture &&
            snap::rhi::hasStencilAspect(depthTextureInfo.format) &&
            snap::rhi::hasStencilAspect(depthResolveTextureInfo.format)) {
            depthStencilMask |= GL_STENCIL_BUFFER_BIT;
        }

        tryFlushBeforeResolve();
        blitTextures(dc,
                     renderInfo.renderPipeline,
                     depthAttachment.attachment,
                     depthAttachment.resolveAttachment,
                     depthStencilMask,
                     colorAttachment.attachment.texture,
                     colorAttachment.resolveAttachment.texture);
    }

    if (renderInfo.renderingInfo.stencilAttachment.attachment.texture != nullptr &&
        renderInfo.renderingInfo.stencilAttachment.resolveAttachment.texture != nullptr &&
        (depthStencilMask & GL_STENCIL_BUFFER_BIT) == 0) {
        SNAP_RHI_VALIDATE(
            gl.getDevice()->getValidationLayer(),
            hasResolvedAttachments,
            snap::rhi::ReportLevel::Error,
            snap::rhi::ValidationTag::RenderPassOp,
            "[resolveAttachments] Must provide at least one color attachment when performing an msaa depth resolve.");

        const auto& stencilAttachment = renderInfo.renderingInfo.stencilAttachment;
        const auto& colorAttachment = renderInfo.renderingInfo.colorAttachments[colorAttachmentId];

        tryFlushBeforeResolve();
        blitTextures(dc,
                     renderInfo.renderPipeline,
                     stencilAttachment.attachment,
                     stencilAttachment.resolveAttachment,
                     GL_STENCIL_BUFFER_BIT,
                     colorAttachment.attachment.texture,
                     colorAttachment.resolveAttachment.texture);
    }

    return hasResolvedAttachments;
}

void RenderCmdPerformer::endRenderPass() const {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[RenderCmdPerformer][endRenderPass]");

    SNAP_RHI_ON_SCOPE_EXIT {
        renderInfo.target = snap::rhi::backend::opengl::FramebufferTarget::Detached;
    };
    resolveAttachments();

    {
        SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[RenderCmdPerformer][endRenderPass][discard]");

        uint32_t numCurrTargets = 0;
        std::array<snap::rhi::backend::opengl::FramebufferAttachmentTarget, numTotalTargets> targets =
            prepareDiscardTargetsOnStore(renderInfo.renderingInfo, numCurrTargets);

        if (numCurrTargets > 0) {
            gl.bindFramebuffer(renderInfo.target, renderInfo.fboID, dc);
            if (renderInfo.fboID != FramebufferId::CurrSurfaceBackbuffer) {
                gl.drawBuffers(static_cast<GLsizei>(renderInfo.drawBuffers.size()),
                               renderInfo.drawBuffers.empty() ? nullptr : renderInfo.drawBuffers.data());
                gl.readBuffer(snap::rhi::backend::opengl::FramebufferAttachmentTarget::Detached);
            }

            gl.discardFramebuffer(renderInfo.target, static_cast<GLsizei>(numCurrTargets), targets.data());
        }
    }

    if (device->shouldDetachFramebufferOnScopeExit()) {
        // We shouldn't make this call because OpenGL can waste a lot of time on it.
        SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[RenderCmdPerformer][endRenderPass][unbindFramebuffer]");
        gl.bindFramebuffer(renderInfo.target, FramebufferId::CurrSurfaceBackbuffer, dc);
    }
}

void RenderCmdPerformer::beginEncoding(const snap::rhi::backend::opengl::BeginRenderPassCmd& cmd) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[RenderCmdPerformer][beginEncoding]");
    assert(dc);

    isDirty = true;
    renderInfo.isFBOBound = false;
    renderInfo.renderingInfo = buildRenderingInfo(
        cmd.framebuffer, cmd.renderPass, {cmd.clearValues.begin(), cmd.clearValues.begin() + cmd.clearValueCount});
    renderInfo.fboID = snap::rhi::backend::opengl::FramebufferId::CurrSurfaceBackbuffer;
}

void RenderCmdPerformer::beginEncoding(const snap::rhi::backend::opengl::BeginRenderPass1Cmd& cmd) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[RenderCmdPerformer][beginEncoding1]");
    assert(dc);

    isDirty = true;
    renderInfo.isFBOBound = false;
    renderInfo.renderingInfo.colorAttachments = {cmd.colorAttachments.begin(),
                                                 cmd.colorAttachments.begin() + cmd.colorAttachmentCount};
    renderInfo.renderingInfo.depthAttachment = cmd.depthAttachment;
    renderInfo.renderingInfo.stencilAttachment = cmd.stencilAttachment;
    renderInfo.renderingInfo.layers = cmd.layers;
    renderInfo.renderingInfo.viewMask = cmd.viewMask;
    renderInfo.fboID = snap::rhi::backend::opengl::FramebufferId::CurrSurfaceBackbuffer;
}

void RenderCmdPerformer::endEncoding(const snap::rhi::backend::opengl::EndRenderPassCmd& cmd) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[RenderCmdPerformer][endEncoding]");

    assert(dc);

    tryBindFBO();
    endRenderPass();

    reset();
}

void RenderCmdPerformer::bindIndexBuffer(const snap::rhi::backend::opengl::SetIndexBufferCmd& cmd) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[RenderCmdPerformer][bindIndexBuffer]");

    auto* indexBuffer = snap::rhi::backend::common::smart_cast<Buffer>(cmd.indexBuffer);

    gl.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer->getGLBuffer(dc), dc);

    renderInfo.indexBuffer = indexBuffer;
    renderInfo.indexType = snap::rhi::backend::opengl::convertToGLIndexType(cmd.indexType);
    renderInfo.indexBufferOffset = cmd.offset;
}

void RenderCmdPerformer::tryBindFBO() {
    /**
     *  iIn order to improve performance SnapRHI have to postpone FBO bind as far as possible
     */
    if (!renderInfo.isFBOBound) {
        renderInfo.fboID = beginRenderPass();
        renderInfo.isFBOBound = true;
    }
}

void RenderCmdPerformer::applyResources() {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[RenderCmdPerformer][applyResources]");

    if (renderInfo.renderPipeline != activePipeline) {
        renderInfo.renderPipeline = activePipeline;

        attachRenderPipeline();

        const auto& info = renderInfo.renderPipeline->getCreateInfo();
        renderInfo.primitiveType =
            snap::rhi::backend::opengl::convertToGLPrimitiveType(info.inputAssemblyState.primitiveTopology);
        renderInfo.vertexBuffers.setPipeline(renderInfo.renderPipeline);

        pipelineResourceState.setPipeline(renderInfo.renderPipeline);
    }

    renderInfo.vertexBuffers.bind(dc);
    pipelineResourceState.setAllStates(dc);

    tryBindFBO();
    restoreAttachmentsMask(dc, renderInfo.renderPipeline, renderInfo.restoreMask);
}

void RenderCmdPerformer::bindRenderPipeline(const snap::rhi::backend::opengl::SetRenderPipelineCmd& cmd) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[RenderCmdPerformer][bindRenderPipeline]");

    activePipeline = snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::RenderPipeline>(cmd.pipeline);
}

void RenderCmdPerformer::draw(const snap::rhi::backend::opengl::DrawCmd& cmd) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[RenderCmdPerformer][draw]");

    applyResources();

    {
        SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[RenderCmdPerformer][execute native draw]");
        if (cmd.instanceCount > 1) {
            gl.drawArraysInstanced(renderInfo.primitiveType, cmd.firstVertex, cmd.vertexCount, cmd.instanceCount);
        } else {
            gl.drawArrays(renderInfo.primitiveType, cmd.firstVertex, cmd.vertexCount);
        }
    }
}

void RenderCmdPerformer::drawIndexed(const snap::rhi::backend::opengl::DrawIndexedCmd& cmd) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[RenderCmdPerformer][drawIndexed]");

    applyResources();

    assert(renderInfo.indexType != GL_NONE);
    const uint32_t indexBufferOffset =
        renderInfo.indexBufferOffset +
        cmd.firstIndex * snap::rhi::backend::opengl::getGLIndexTypeByteSize(renderInfo.indexType);
    const void* indices = nullptr;

    if (renderInfo.indexBuffer->getGLBuffer(dc) != GL_NONE) {
        indices = reinterpret_cast<void*>(indexBufferOffset);
    } else {
        indices = renderInfo.indexBuffer->map(snap::rhi::MemoryAccess::Read, 0, WholeSize, dc) + indexBufferOffset;
    }
    SNAP_RHI_ON_SCOPE_EXIT {
        if (renderInfo.indexBuffer->getGLBuffer(dc) == GL_NONE) {
            renderInfo.indexBuffer->unmap(dc);
        }
    };

    {
        SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[RenderCmdPerformer][execute native draw]");
        if (cmd.instanceCount > 1) {
            gl.drawElementsInstanced(
                renderInfo.primitiveType, cmd.indexCount, renderInfo.indexType, indices, cmd.instanceCount);
        } else {
            gl.drawElements(renderInfo.primitiveType, cmd.indexCount, renderInfo.indexType, indices);
        }
    }
}

void RenderCmdPerformer::reset() {
    /**
     * SnapRHI will avoid extra clear op by using isDirty flag
     */
    if (!isDirty) {
        return;
    }

    renderInfo.clear();
    pipelineResourceState.clearStates();
    activePipeline = nullptr;
    isDirty = false;
}
} // namespace snap::rhi::backend::opengl
