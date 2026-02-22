#include "snap/rhi/backend/metal/RenderCommandEncoder.h"
#include "snap/rhi/backend/common/CommandBuffer.h"
#include "snap/rhi/backend/common/Logging.hpp"
#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/metal/Buffer.h"
#include "snap/rhi/backend/metal/CommandBuffer.h"
#include "snap/rhi/backend/metal/DescriptorSet.h"
#include "snap/rhi/backend/metal/Device.h"
#include "snap/rhi/backend/metal/Framebuffer.h"
#include "snap/rhi/backend/metal/RenderPipeline.h"
#include "snap/rhi/backend/metal/Sampler.h"
#include "snap/rhi/backend/metal/Texture.h"
#include "snap/rhi/backend/metal/Utils.h"
#include "snap/rhi/common/OS.h"
#include <iostream>
#include <snap/rhi/common/Throw.h>
#include <vector>

namespace {
MTLIndexType convertToMtlIndexType(const snap::rhi::IndexType indexType) {
    switch (indexType) {
        case snap::rhi::IndexType::UInt16:
            return MTLIndexTypeUInt16;

        case snap::rhi::IndexType::UInt32:
            return MTLIndexTypeUInt32;

        default:
            snap::rhi::common::throwException("invalid index type");
    }
}

uint32_t getIndexTypeByteSize(const MTLIndexType indexType) {
    switch (indexType) {
        case MTLIndexTypeUInt16:
            return 2;

        case MTLIndexTypeUInt32:
            return 4;

        default:
            snap::rhi::common::throwException("invalid index type");
    }
}
} // unnamed namespace

namespace snap::rhi::backend::metal {
RenderCommandEncoder::RenderCommandEncoder(Device* mtlDevice, CommandBuffer* commandBuffer)
    : RenderCommandEncoderBase(mtlDevice, commandBuffer),
      context(commandBuffer->getContext()),
      pipelineState(commandBuffer) {}

void RenderCommandEncoder::beginEncoding() {
    renderPipelineInfo.clear();
    indexBufferInfo.clear();
    RenderCommandEncoderBase::onBeginEncoding(context.getRenderEncoder());
}

void RenderCommandEncoder::beginEncoding(const RenderPassBeginInfo& renderPassBeginInfo) {
    { // RenderPass preparation
        const auto& renderPassInfo = renderPassBeginInfo.renderPass->getCreateInfo();
        const auto& framebufferInfo = renderPassBeginInfo.framebuffer->getCreateInfo();

        MTLRenderPassDescriptor* renderPassDescriptor =
            createFramebuffer(renderPassInfo, framebufferInfo.attachments, renderPassBeginInfo.clearValues);

        context.beginRender(renderPassDescriptor);

        {
            for (auto* texture : framebufferInfo.attachments) {
                if (texture) {
                    resourceResidencySet.track(texture);
                }
            }

            resourceResidencySet.track(renderPassBeginInfo.framebuffer);
            resourceResidencySet.track(renderPassBeginInfo.renderPass);
            resourceResidencySet.track(framebufferInfo.renderPass);
        }
    }

    beginEncoding();
}

void RenderCommandEncoder::beginEncoding(const RenderingInfo& renderingInfo) {
    { // RenderPass preparation
        MTLRenderPassDescriptor* renderPassDescriptor = createFramebuffer(renderingInfo);
        context.beginRender(renderPassDescriptor);

        {
            auto retainAttachemntFunc = [this](const snap::rhi::RenderingAttachmentInfo& info) {
                if (info.attachment.texture) {
                    resourceResidencySet.track(info.attachment.texture);
                }

                if (info.resolveAttachment.texture) {
                    resourceResidencySet.track(info.resolveAttachment.texture);
                }
            };

            for (const auto& attachment : renderingInfo.colorAttachments) {
                retainAttachemntFunc(attachment);
            }
            retainAttachemntFunc(renderingInfo.depthAttachment);
            retainAttachemntFunc(renderingInfo.stencilAttachment);
        }
    }

    beginEncoding();
}

void RenderCommandEncoder::setViewport(const snap::rhi::Viewport& viewport) {
    MTLViewport mtlViewport;
    mtlViewport.originX = viewport.x;
    mtlViewport.originY = viewport.y;
    mtlViewport.width = viewport.width;
    mtlViewport.height = viewport.height;
    mtlViewport.znear = viewport.znear;
    mtlViewport.zfar = viewport.zfar;
    [context.getRenderEncoder() setViewport:mtlViewport];
}

void RenderCommandEncoder::bindRenderPipeline(snap::rhi::RenderPipeline* pipeline) {
    auto* mtlRenderPipeline = snap::rhi::backend::common::smart_cast<RenderPipeline>(pipeline);
    pipelineState.bindRenderPipeline(mtlRenderPipeline);
    {
        const auto& renderEncoder = context.getRenderEncoder();
        const auto& info = pipeline->getCreateInfo();
        renderPipelineInfo.primitiveType = convertToMtlPrimitiveType(info.inputAssemblyState.primitiveTopology);

        // If you don't explicitly call this method, the pipeline won't apply a scale or a bias to the calculated depth
        // value. https://developer.apple.com/documentation/metal/mtlrendercommandencoder/1516269-setdepthbias
        renderPipelineInfo.depthBiasEnabled = info.rasterizationState.depthBiasEnable;
        if (renderPipelineInfo.depthBiasEnabled) {
            [renderEncoder setDepthBias:renderPipelineInfo.depthBiasConstantFactor
                             slopeScale:renderPipelineInfo.depthBiasSlopeFactor
                                  clamp:renderPipelineInfo.depthBiasClamp];
        } else {
            [renderEncoder setDepthBias:0.0f slopeScale:0.0f clamp:0.0f];
        }
    }
    {
        resourceResidencySet.track(pipeline);
    }
}

void RenderCommandEncoder::bindDescriptorSet(uint32_t binding,
                                             snap::rhi::DescriptorSet* descriptorSet,
                                             std::span<const uint32_t> dynamicOffsets) {
    auto* mtlDescriptorSet = snap::rhi::backend::common::smart_cast<DescriptorSet>(descriptorSet);
    pipelineState.bindDescriptorSet(binding, mtlDescriptorSet, dynamicOffsets);
}

void RenderCommandEncoder::bindVertexBuffers(const uint32_t firstBinding,
                                             std::span<snap::rhi::Buffer*> buffers,
                                             std::span<uint32_t> offsets) {
    assert(buffers.size() <= offsets.size());
    for (uint32_t i = 0; i < buffers.size(); ++i) {
        auto* mtlBuffer = snap::rhi::backend::common::smart_cast<Buffer>(buffers[i]);
        pipelineState.bindVertexBuffer(firstBinding + i, mtlBuffer, offsets[i]);
        {
            resourceResidencySet.track(mtlBuffer);
        }
    }
}

void RenderCommandEncoder::bindVertexBuffer(const uint32_t binding, snap::rhi::Buffer* buffer, const uint32_t offset) {
    auto* mtlBuffer = snap::rhi::backend::common::smart_cast<Buffer>(buffer);
    pipelineState.bindVertexBuffer(binding, mtlBuffer, offset);
    {
        resourceResidencySet.track(mtlBuffer);
    }
}

void RenderCommandEncoder::bindIndexBuffer(snap::rhi::Buffer* indexBuffer,
                                           const uint32_t offset,
                                           const IndexType indexType) {
    indexBufferInfo.indexBuffer = snap::rhi::backend::common::smart_cast<Buffer>(indexBuffer);
    indexBufferInfo.offset = offset;
    assert(indexBuffer && indexType != snap::rhi::IndexType::None);
    renderPipelineInfo.indexType = convertToMtlIndexType(indexType);
    {
        resourceResidencySet.track(indexBuffer);
    }
}

void RenderCommandEncoder::draw(const uint32_t vertexCount, const uint32_t firstVertex, const uint32_t instanceCount) {
    pipelineState.setAllStates();

    [context.getRenderEncoder() drawPrimitives:renderPipelineInfo.primitiveType
                                   vertexStart:firstVertex
                                   vertexCount:vertexCount
                                 instanceCount:instanceCount];
}

void RenderCommandEncoder::drawIndexed(const uint32_t indexCount,
                                       const uint32_t firstIndex,
                                       const uint32_t instanceCount) {
    pipelineState.setAllStates();

    assert(indexBufferInfo.indexBuffer);
    const uint32_t indexBufferOffset =
        firstIndex * getIndexTypeByteSize(renderPipelineInfo.indexType) + indexBufferInfo.offset;

    // Don't use baseVertex because devices with processor lower than A9 doesn't support this feature.
    [context.getRenderEncoder() drawIndexedPrimitives:renderPipelineInfo.primitiveType
                                           indexCount:indexCount
                                            indexType:renderPipelineInfo.indexType
                                          indexBuffer:indexBufferInfo.indexBuffer->getBuffer()
                                    indexBufferOffset:indexBufferOffset
                                        instanceCount:instanceCount];
}

void RenderCommandEncoder::setDepthBias(float depthBiasConstantFactor,
                                        float depthBiasSlopeFactor,
                                        float depthBiasClamp) {
    renderPipelineInfo.depthBiasConstantFactor = depthBiasConstantFactor;
    renderPipelineInfo.depthBiasSlopeFactor = depthBiasSlopeFactor;
    renderPipelineInfo.depthBiasClamp = depthBiasClamp;

    if (renderPipelineInfo.depthBiasEnabled) {
        [context.getRenderEncoder() setDepthBias:renderPipelineInfo.depthBiasConstantFactor
                                      slopeScale:renderPipelineInfo.depthBiasSlopeFactor
                                           clamp:renderPipelineInfo.depthBiasClamp];
    } else {
        [context.getRenderEncoder() setDepthBias:0.0f slopeScale:0.0f clamp:0.0f];
    }
}

void RenderCommandEncoder::setStencilReference(const snap::rhi::StencilFace face, const uint32_t reference) {
    switch (face) {
        case snap::rhi::StencilFace::Front: {
            renderPipelineInfo.stencilReference[static_cast<uint32_t>(snap::rhi::StencilFace::Front)] = reference;
        } break;

        case snap::rhi::StencilFace::Back: {
            renderPipelineInfo.stencilReference[static_cast<uint32_t>(snap::rhi::StencilFace::Back)] = reference;
        } break;

        case snap::rhi::StencilFace::FrontAndBack: {
            renderPipelineInfo.stencilReference[static_cast<uint32_t>(snap::rhi::StencilFace::Front)] = reference;
            renderPipelineInfo.stencilReference[static_cast<uint32_t>(snap::rhi::StencilFace::Back)] = reference;
        } break;

        default:
            snap::rhi::common::throwException("unsupported StencilFace");
    }

    [context.getRenderEncoder()
        setStencilFrontReferenceValue:renderPipelineInfo
                                          .stencilReference[static_cast<uint32_t>(snap::rhi::StencilFace::Front)]
                   backReferenceValue:renderPipelineInfo
                                          .stencilReference[static_cast<uint32_t>(snap::rhi::StencilFace::Back)]];
}

void RenderCommandEncoder::setBlendConstants(const float r, const float g, const float b, const float a) {
    [context.getRenderEncoder() setBlendColorRed:r green:g blue:b alpha:a];
}

void RenderCommandEncoder::invokeCustomCallback(std::function<void()>* callback) {
    (*callback)();
}

void RenderCommandEncoder::pipelineBarrier(snap::rhi::PipelineStageBits srcStageMask,
                                           snap::rhi::PipelineStageBits dstStageMask,
                                           snap::rhi::DependencyFlags dependencyFlags,
                                           std::span<snap::rhi::MemoryBarrierInfo> memoryBarriers,
                                           std::span<snap::rhi::BufferMemoryBarrierInfo> bufferMemoryBarriers,
                                           std::span<snap::rhi::TextureMemoryBarrierInfo> textureMemoryBarriers) {
#if SNAP_RHI_OS_MACOS()
    if (@available(macOS 10.14, macCatalyst 13.0, *)) {
        const uint32_t objectsCount = static_cast<uint32_t>(bufferMemoryBarriers.size() + textureMemoryBarriers.size());
        std::vector<id> objects(objectsCount);

        uint32_t idx = 0;
        for (uint32_t i = 0; i < bufferMemoryBarriers.size(); ++i) {
            Buffer* mtlBuffer = snap::rhi::backend::common::smart_cast<Buffer>(bufferMemoryBarriers[i].buffer);
            objects[idx++] = mtlBuffer->getBuffer();
        }

        for (uint32_t i = 0; i < textureMemoryBarriers.size(); ++i) {
            Texture* mtlTextures = snap::rhi::backend::common::smart_cast<Texture>(textureMemoryBarriers[i].texture);
            objects[idx++] = mtlTextures->getTexture();

            commandBuffer->tryPreserveInteropTexture(textureMemoryBarriers[i].texture);
        }

        [context.getRenderEncoder() memoryBarrierWithResources:objects.data()
                                                         count:objectsCount
                                                   afterStages:convertTo(dstStageMask)
                                                  beforeStages:convertTo(srcStageMask)];

        MTLBarrierScope scope = MTLBarrierScopeBuffers | MTLBarrierScopeTextures;
        if (!memoryBarriers.empty()) {
            if (@available(macOS 10.14, macCatalyst 13.0, *)) {
                scope = scope | MTLBarrierScopeRenderTargets;
            }

            [context.getRenderEncoder() memoryBarrierWithScope:scope
                                                   afterStages:convertTo(dstStageMask)
                                                  beforeStages:convertTo(srcStageMask)];
        }
    }
#endif
}

void RenderCommandEncoder::endEncoding() {
    context.endRender();
    RenderCommandEncoderBase::onEndEncoding();
    pipelineState.clearStates();
}
} // namespace snap::rhi::backend::metal
