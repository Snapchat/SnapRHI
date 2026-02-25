#include "snap/rhi/backend/opengl/RenderCommandEncoder.hpp"
#include "snap/rhi/backend/common/CommandBuffer.h"
#include "snap/rhi/backend/common/Utils.hpp"

#include "snap/rhi/Buffer.hpp"
#include "snap/rhi/DescriptorSet.hpp"
#include "snap/rhi/Framebuffer.hpp"
#include "snap/rhi/RenderPipeline.hpp"
#include "snap/rhi/Sampler.hpp"
#include "snap/rhi/Texture.hpp"
#include "snap/rhi/backend/opengl/DescriptorSet.hpp"
#include "snap/rhi/backend/opengl/Device.hpp"

#include "snap/rhi/Limits.h"
#include "snap/rhi/backend/opengl/CommandBuffer.hpp"
#include "snap/rhi/backend/opengl/Commands.h"
#include <algorithm>
#include <sstream>

namespace snap::rhi::backend::opengl {
RenderCommandEncoder::RenderCommandEncoder(snap::rhi::backend::opengl::Device* device,
                                           snap::rhi::backend::opengl::CommandBuffer* commandBuffer)
    : RenderCommandEncoderBase(device, commandBuffer) {}

void RenderCommandEncoder::beginEncoding() {
    RenderCommandEncoderBase::onBeginEncoding();
}

void RenderCommandEncoder::beginEncoding(const RenderPassBeginInfo& renderPassBeginInfo) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[RenderCommandEncoder][beginEncoding]");

    assert(renderPassBeginInfo.framebuffer);
    assert(renderPassBeginInfo.renderPass);

    BeginRenderPassCmd* cmd = commandAllocator.allocateCommand<BeginRenderPassCmd>();
    cmd->framebuffer = renderPassBeginInfo.framebuffer;
    cmd->renderPass = renderPassBeginInfo.renderPass;

    const auto& framebufferCreateInfo = renderPassBeginInfo.framebuffer->getCreateInfo();

    const auto& attachments = framebufferCreateInfo.attachments;
    for (const auto& attachment : attachments) {
        if (attachment) {
            resourceResidencySet.track(attachment);
        }
    }

    cmd->clearValueCount = static_cast<uint32_t>(renderPassBeginInfo.clearValues.size());
    std::copy(renderPassBeginInfo.clearValues.begin(), renderPassBeginInfo.clearValues.end(), cmd->clearValues.begin());

    resourceResidencySet.track(renderPassBeginInfo.framebuffer);
    resourceResidencySet.track(renderPassBeginInfo.renderPass);
    resourceResidencySet.track(framebufferCreateInfo.renderPass);

    beginEncoding();

    if (device->getDeviceCreateInfo().enabledReportLevel == snap::rhi::ReportLevel::Debug) {
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        "******Executing BeginRenderPassCmd******");
        SNAP_RHI_REPORT(
            validationLayer, snap::rhi::ReportLevel::Debug, snap::rhi::ValidationTag::CommandBufferOp, "Parameters: ");
        std::stringstream framebufferStream;
        framebufferStream << "\tframebuffer: " << cmd->framebuffer;
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        framebufferStream.str());
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        "\tclearValueCount: " + std::to_string(cmd->clearValueCount));
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        "\tclearValues: ");
        int idx = 0;
        for (auto& element : renderPassBeginInfo.clearValues) {
            SNAP_RHI_REPORT(validationLayer,
                            snap::rhi::ReportLevel::Debug,
                            snap::rhi::ValidationTag::CommandBufferOp,
                            "\tClearValue element " + std::to_string(idx));
            SNAP_RHI_REPORT(validationLayer,
                            snap::rhi::ReportLevel::Debug,
                            snap::rhi::ValidationTag::CommandBufferOp,
                            "\t\tcolor:");
            SNAP_RHI_REPORT(validationLayer,
                            snap::rhi::ReportLevel::Debug,
                            snap::rhi::ValidationTag::CommandBufferOp,
                            "\t\t\t(r, g, b, a): (" + std::to_string(element.color.float32[0]) + ", " +
                                std::to_string(element.color.float32[1]) + ", " +
                                std::to_string(element.color.float32[2]) + ", " +
                                std::to_string(element.color.float32[3]) + ")");
            SNAP_RHI_REPORT(validationLayer,
                            snap::rhi::ReportLevel::Debug,
                            snap::rhi::ValidationTag::CommandBufferOp,
                            "\t\tdepthStencil: ");
            SNAP_RHI_REPORT(validationLayer,
                            snap::rhi::ReportLevel::Debug,
                            snap::rhi::ValidationTag::CommandBufferOp,
                            "\t\t\tdepth: " + std::to_string(element.depthStencil.depth));
            SNAP_RHI_REPORT(validationLayer,
                            snap::rhi::ReportLevel::Debug,
                            snap::rhi::ValidationTag::CommandBufferOp,
                            "\t\t\tstencil: " + std::to_string(element.depthStencil.stencil));
            idx++;
        }
    }
}

void RenderCommandEncoder::beginEncoding(const RenderingInfo& renderingInfo) {
    BeginRenderPass1Cmd* cmd = commandAllocator.allocateCommand<BeginRenderPass1Cmd>();

    cmd->layers = renderingInfo.layers;
    cmd->viewMask = renderingInfo.viewMask;
    std::copy(
        renderingInfo.colorAttachments.begin(), renderingInfo.colorAttachments.end(), cmd->colorAttachments.begin());
    cmd->colorAttachmentCount = renderingInfo.colorAttachments.size();
    cmd->depthAttachment = renderingInfo.depthAttachment;
    cmd->stencilAttachment = renderingInfo.stencilAttachment;
    beginEncoding();
}

void RenderCommandEncoder::setViewport(const snap::rhi::Viewport& viewport) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[RenderCommandEncoder][setViewport]");

    SetViewportCmd* cmd = commandAllocator.allocateCommand<SetViewportCmd>();
    *cmd = viewport;

    if (device->getDeviceCreateInfo().enabledReportLevel == snap::rhi::ReportLevel::Debug) {
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        "******Executing SetViewportCmd******");
        SNAP_RHI_REPORT(
            validationLayer, snap::rhi::ReportLevel::Debug, snap::rhi::ValidationTag::CommandBufferOp, "No Parameters");
    }
}

void RenderCommandEncoder::bindRenderPipeline(snap::rhi::RenderPipeline* pipeline) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[RenderCommandEncoder][bindRenderPipeline]");

    assert(pipeline);

    SetRenderPipelineCmd* cmd = commandAllocator.allocateCommand<SetRenderPipelineCmd>();
    cmd->pipeline = pipeline;

    resourceResidencySet.track(pipeline);

    if (device->getDeviceCreateInfo().enabledReportLevel == snap::rhi::ReportLevel::Debug) {
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        "******Executing SetRenderPipelineCmd******");
        SNAP_RHI_REPORT(
            validationLayer, snap::rhi::ReportLevel::Debug, snap::rhi::ValidationTag::CommandBufferOp, "Parameters: ");
        std::stringstream pipelineStream;
        pipelineStream << "\tpipeline: " << static_cast<const void*>(cmd->pipeline);
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        pipelineStream.str());
    }
}

void RenderCommandEncoder::bindDescriptorSet(uint32_t binding,
                                             snap::rhi::DescriptorSet* descriptorSet,
                                             std::span<const uint32_t> dynamicOffsets) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[RenderCommandEncoder][bindDescriptorSet]");

    assert(descriptorSet);

    BindDescriptorSetCmd* cmd = commandAllocator.allocateCommand<BindDescriptorSetCmd>();
    cmd->binding = binding;
    cmd->descriptorSet = descriptorSet;
    cmd->dynamicOffsetCount = static_cast<uint32_t>(dynamicOffsets.size());
    assert(dynamicOffsets.size() <= cmd->dynamicOffsets.size());
    std::ranges::copy(dynamicOffsets, cmd->dynamicOffsets.begin());

    auto* glDescriptorSet = snap::rhi::backend::common::smart_cast<DescriptorSet>(descriptorSet);
    glDescriptorSet->collectReferences(commandBuffer);
    commandBuffer->preserveInteropTextures(glDescriptorSet->getInteropTextures());
}

void RenderCommandEncoder::bindVertexBuffers(const uint32_t firstBinding,
                                             std::span<snap::rhi::Buffer*> buffers,
                                             std::span<uint32_t> offsets) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[RenderCommandEncoder][bindVertexBuffers]");

    auto* cmd = commandAllocator.allocateCommand<SetVertexBuffersCmd>();
    cmd->vertexBuffersCount = buffers.size();
    cmd->firstBinding = firstBinding;

    assert(buffers.size() <= offsets.size());
    for (uint32_t i = 0; i < buffers.size(); ++i) {
        assert(buffers[i]);
        const auto& bufferInfo = buffers[i]->getCreateInfo();
        SNAP_RHI_VALIDATE(validationLayer,
                          (bufferInfo.bufferUsage & snap::rhi::BufferUsage::VertexBuffer) ==
                              snap::rhi::BufferUsage::VertexBuffer,
                          snap::rhi::ReportLevel::Error,
                          snap::rhi::ValidationTag::CommandBufferOp,
                          "[RenderCommandEncoder::bindVertexBuffers] buffer must have buffer usage VertexBuffer");

        cmd->vertexBuffers[i] = buffers[i];
        cmd->offsets[i] = offsets[i];

        resourceResidencySet.track(buffers[i]);
    }

    if (device->getDeviceCreateInfo().enabledReportLevel == snap::rhi::ReportLevel::Debug) {
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        "******Executing SetVertexBuffersCmd******");
        SNAP_RHI_REPORT(
            validationLayer, snap::rhi::ReportLevel::Debug, snap::rhi::ValidationTag::CommandBufferOp, "Parameters: ");
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        "\tfirstBinding: " + std::to_string(cmd->firstBinding));
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        "\tvertexBuffersCount: " + std::to_string(cmd->vertexBuffersCount));
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        "\tvertexBuffers");
        int idx = 0;
        for (snap::rhi::Buffer* bufPtr : cmd->vertexBuffers) {
            std::stringstream bufPtrStream;
            bufPtrStream << "\tVertex Buffer " << std::to_string(idx) << ": " << static_cast<const void*>(bufPtr);
            SNAP_RHI_REPORT(validationLayer,
                            snap::rhi::ReportLevel::Debug,
                            snap::rhi::ValidationTag::CommandBufferOp,
                            bufPtrStream.str());
            idx++;
        }
        SNAP_RHI_REPORT(
            validationLayer, snap::rhi::ReportLevel::Debug, snap::rhi::ValidationTag::CommandBufferOp, "\toffsets");
        for (idx = 0; idx < cmd->offsets.size(); idx++) {
            SNAP_RHI_REPORT(validationLayer,
                            snap::rhi::ReportLevel::Debug,
                            snap::rhi::ValidationTag::CommandBufferOp,
                            "\tOffset " + std::to_string(idx) + ": " + std::to_string(cmd->offsets[idx]));
        }
    }
}

void RenderCommandEncoder::bindVertexBuffer(const uint32_t binding, snap::rhi::Buffer* buffer, const uint32_t offset) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[RenderCommandEncoder][bindVertexBuffer]");

    assert(buffer);
    const auto& bufferInfo = buffer->getCreateInfo();
    SNAP_RHI_VALIDATE(validationLayer,
                      (bufferInfo.bufferUsage & snap::rhi::BufferUsage::VertexBuffer) ==
                          snap::rhi::BufferUsage::VertexBuffer,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::CommandBufferOp,
                      "[RenderCommandEncoder::bindVertexBuffer] buffer must have buffer usage VertexBuffer");

    auto* cmd = commandAllocator.allocateCommand<SetVertexBufferCmd>();
    cmd->binding = binding;
    cmd->vertexBuffer = buffer;
    cmd->offset = offset;

    resourceResidencySet.track(buffer);

    if (device->getDeviceCreateInfo().enabledReportLevel == snap::rhi::ReportLevel::Debug) {
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        "******Executing SetVertexBufferCmd******");
        SNAP_RHI_REPORT(
            validationLayer, snap::rhi::ReportLevel::Debug, snap::rhi::ValidationTag::CommandBufferOp, "Parameters: ");
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        "\tbinding: " + std::to_string(cmd->binding));
        std::stringstream vertexBufferStream;
        vertexBufferStream << "\tvertexBuffer: " << static_cast<const void*>(cmd->vertexBuffer);
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        vertexBufferStream.str());
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        "\toffset: " + std::to_string(cmd->offset));
    }
}

void RenderCommandEncoder::bindIndexBuffer(snap::rhi::Buffer* indexBuffer,
                                           const uint32_t offset,
                                           const snap::rhi::IndexType indexType) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[RenderCommandEncoder][bindIndexBuffer]");

    assert(indexBuffer);

    const auto& bufferInfo = indexBuffer->getCreateInfo();
    SNAP_RHI_VALIDATE(validationLayer,
                      (bufferInfo.bufferUsage & snap::rhi::BufferUsage::IndexBuffer) ==
                          snap::rhi::BufferUsage::IndexBuffer,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::CommandBufferOp,
                      "[RenderCommandEncoder::bindIndexBuffer] buffer must have buffer usage IndexBuffer");

    auto* cmd = commandAllocator.allocateCommand<SetIndexBufferCmd>();
    cmd->indexBuffer = indexBuffer;
    cmd->offset = offset;
    cmd->indexType = indexType;

    resourceResidencySet.track(indexBuffer);

    if (device->getDeviceCreateInfo().enabledReportLevel == snap::rhi::ReportLevel::Debug) {
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        "******Executing SetIndexBufferCmd******");
        SNAP_RHI_REPORT(
            validationLayer, snap::rhi::ReportLevel::Debug, snap::rhi::ValidationTag::CommandBufferOp, "Parameters: ");
        std::stringstream indexBufferStream;
        indexBufferStream << "\tindexBuffer: " << static_cast<const void*>(cmd->indexBuffer);
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        indexBufferStream.str());
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        "\toffset: " + std::to_string(cmd->offset));
        std::string indexTypeString;
        switch (cmd->indexType) {
            case snap::rhi::IndexType::None:
                indexTypeString = std::string("None");
                break;
            case snap::rhi::IndexType::UInt16:
                indexTypeString = std::string("UInt16");
                break;
            case snap::rhi::IndexType::UInt32:
                indexTypeString = std::string("UInt32");
                break;
            default:
                indexTypeString = std::string("Count");
        }
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        "\tindexType: " + indexTypeString);
    }
}

void RenderCommandEncoder::draw(const uint32_t vertexCount, const uint32_t firstVertex, const uint32_t instanceCount) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[RenderCommandEncoder][draw]");

    auto* cmd = commandAllocator.allocateCommand<DrawCmd>();
    cmd->vertexCount = vertexCount;
    cmd->firstVertex = firstVertex;
    cmd->instanceCount = instanceCount;

    if (device->getDeviceCreateInfo().enabledReportLevel == snap::rhi::ReportLevel::Debug) {
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        "******Executing DrawCmd******");
        SNAP_RHI_REPORT(
            validationLayer, snap::rhi::ReportLevel::Debug, snap::rhi::ValidationTag::CommandBufferOp, "Parameters: ");
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        "\tfirstVertex: " + std::to_string(cmd->firstVertex));
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        "\tvertexCount: " + std::to_string(cmd->vertexCount));
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        "\tinstanceCount: " + std::to_string(cmd->instanceCount));
    }
}

void RenderCommandEncoder::drawIndexed(const uint32_t indexCount,
                                       const uint32_t firstIndex,
                                       const uint32_t instanceCount) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[RenderCommandEncoder][drawIndexed]");

    auto* cmd = commandAllocator.allocateCommand<DrawIndexedCmd>();
    cmd->indexCount = indexCount;
    cmd->firstIndex = firstIndex;
    cmd->instanceCount = instanceCount;

    if (device->getDeviceCreateInfo().enabledReportLevel == snap::rhi::ReportLevel::Debug) {
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        "******Executing DrawIndexedCmd******");
        SNAP_RHI_REPORT(
            validationLayer, snap::rhi::ReportLevel::Debug, snap::rhi::ValidationTag::CommandBufferOp, "Parameters: ");
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        "\tfirstIndex: " + std::to_string(cmd->firstIndex));
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        "\tindexCount: " + std::to_string(cmd->indexCount));
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        "\tinstanceCount: " + std::to_string(cmd->instanceCount));
    }
}

void RenderCommandEncoder::setDepthBias(float depthBiasConstantFactor,
                                        float depthBiasSlopeFactor,
                                        float depthBiasClamp) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[RenderCommandEncoder][setDepthBias]");

    SetDepthBiasCmd* cmd = commandAllocator.allocateCommand<SetDepthBiasCmd>();
    cmd->depthBiasConstantFactor = depthBiasConstantFactor;
    cmd->depthBiasSlopeFactor = depthBiasSlopeFactor;
    cmd->depthBiasClamp = depthBiasClamp;

    if (device->getDeviceCreateInfo().enabledReportLevel == snap::rhi::ReportLevel::Debug) {
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        "******Executing SetDepthBiasCmd******");
        SNAP_RHI_REPORT(
            validationLayer, snap::rhi::ReportLevel::Debug, snap::rhi::ValidationTag::CommandBufferOp, "Parameters: ");
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        "\tdepthBiasConstantFactor: " + std::to_string(cmd->depthBiasConstantFactor));
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        "\tdepthBiasSlopeFactor: " + std::to_string(cmd->depthBiasSlopeFactor));
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        "\tdepthBiasClamp: " + std::to_string(cmd->depthBiasClamp));
    }
}

void RenderCommandEncoder::setStencilReference(const snap::rhi::StencilFace face, const uint32_t reference) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[RenderCommandEncoder][setStencilReference]");

    SetStencilReferenceCmd* cmd = commandAllocator.allocateCommand<SetStencilReferenceCmd>();
    cmd->face = face;
    cmd->reference = reference;

    if (device->getDeviceCreateInfo().enabledReportLevel == snap::rhi::ReportLevel::Debug) {
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        "******Executing SetStencilReferenceCmd******");
        SNAP_RHI_REPORT(
            validationLayer, snap::rhi::ReportLevel::Debug, snap::rhi::ValidationTag::CommandBufferOp, "Parameters: ");

        std::string stencilFaceString = "";
        switch (cmd->face) {
            case snap::rhi::StencilFace::Front:
                stencilFaceString = "Front";
                break;
            case snap::rhi::StencilFace::Back:
                stencilFaceString = "Back";
                break;
            case snap::rhi::StencilFace::FrontAndBack:
                stencilFaceString = "FrontAndBack";
                break;
        }

        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        "\tface: " + stencilFaceString);
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        "\treference: " + std::to_string(cmd->reference));
    }
}

void RenderCommandEncoder::setBlendConstants(const float r, const float g, const float b, const float a) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[RenderCommandEncoder][setBlendConstants]");

    SetBlendConstantsCmd* cmd = commandAllocator.allocateCommand<SetBlendConstantsCmd>();
    cmd->r = r;
    cmd->g = g;
    cmd->b = b;
    cmd->a = a;

    if (device->getDeviceCreateInfo().enabledReportLevel == snap::rhi::ReportLevel::Debug) {
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        "******Executing SetBlendConstantsCmd******");
        SNAP_RHI_REPORT(
            validationLayer, snap::rhi::ReportLevel::Debug, snap::rhi::ValidationTag::CommandBufferOp, "Parameters: ");
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        "\t(r, g, b a): (" + std::to_string(cmd->r) + ", " + std::to_string(cmd->g) + ", " +
                            std::to_string(cmd->b) + ", " + std::to_string(cmd->a) + ")");
    }
}

void RenderCommandEncoder::invokeCustomCallback(std::function<void()>* callback) {
    InvokeCustomCallbackCmd* cmd = commandAllocator.allocateCommand<InvokeCustomCallbackCmd>();
    cmd->callback = callback;
}

void RenderCommandEncoder::endEncoding() {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[RenderCommandEncoder][endEncoding]");

    commandAllocator.allocateCommand<EndRenderPassCmd>();
    RenderCommandEncoderBase::onEndEncoding();

    if (device->getDeviceCreateInfo().enabledReportLevel == snap::rhi::ReportLevel::Debug) {
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        "******Executing EndRenderPassCmd******");
        SNAP_RHI_REPORT(
            validationLayer, snap::rhi::ReportLevel::Debug, snap::rhi::ValidationTag::CommandBufferOp, "No Parameters");
    }
}
} // namespace snap::rhi::backend::opengl
