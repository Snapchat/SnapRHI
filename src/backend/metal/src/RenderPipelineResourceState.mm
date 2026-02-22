#include "snap/rhi/backend/metal/RenderPipelineResourceState.h"
#include "snap/rhi/backend/metal/Buffer.h"
#include "snap/rhi/backend/metal/CommandBuffer.h"
#include "snap/rhi/backend/metal/RenderPipeline.h"
#include <algorithm>
#include <snap/rhi/common/Throw.h>

namespace {
MTLCullMode convertToMtlCullMode(const snap::rhi::CullMode cullMode) {
    switch (cullMode) {
        case snap::rhi::CullMode::Back:
            return MTLCullModeBack;

        case snap::rhi::CullMode::Front:
            return MTLCullModeFront;

        case snap::rhi::CullMode::None:
            return MTLCullModeNone;

        default:
            snap::rhi::common::throwException("invalid cull mode");
    }
}

MTLTriangleFillMode convertToMtlFillMode(const snap::rhi::PolygonMode polygonMode) {
    switch (polygonMode) {
        case snap::rhi::PolygonMode::Fill:
            return MTLTriangleFillModeFill;

        case snap::rhi::PolygonMode::Line:
            return MTLTriangleFillModeLines;

        default:
            snap::rhi::common::throwException("invalid polygon mode");
    }
}

MTLWinding convertToMtlWinding(const snap::rhi::Winding windingMode) {
    switch (windingMode) {
        case snap::rhi::Winding::CCW:
            return MTLWindingCounterClockwise;

        case snap::rhi::Winding::CW:
            return MTLWindingClockwise;

        default:
            snap::rhi::common::throwException("invalid winding mode");
    }
}
} // namespace

namespace snap::rhi::backend::metal {
RenderPipelineResourceState::RenderPipelineResourceState(CommandBuffer* commandBuffer)
    : PipelineResourceState(commandBuffer) {
    clearStates();
}

void RenderPipelineResourceState::bindVertexBuffer(const uint32_t binding, Buffer* buffer, const uint32_t offset) {
    vertexBuffers[binding] = BufferWithOffset{buffer, offset};
    if (vertexBuffers[binding] != activeVertexBuffers[binding]) {
        vertexBufferBits[binding] = true;
    }
}

void RenderPipelineResourceState::bindRenderPipeline(RenderPipeline* pipeline) {
    this->pipeline = pipeline;

    const auto& pipelineInfo = pipeline->getCreateInfo();
    vertexBufferBindingBase = pipelineInfo.mtlRenderPipelineInfo->vertexBufferBindingBase;
    setAuxiliaryDynamicOffsetsBinding(pipelineInfo.mtlRenderPipelineInfo->auxiliaryDynamicOffsetsBinding);
}

void RenderPipelineResourceState::setAllStates() {
    auto& context = commandBuffer->getContext();
    const auto& renderEncoder = context.getRenderEncoder();

    if (pipeline != activePipeline) {
        [renderEncoder setRenderPipelineState:pipeline->getRenderPipeline()];
        [renderEncoder setDepthStencilState:pipeline->getDepthStencilState()];

        { // setup additional pipeline settings
            const auto& info = pipeline->getCreateInfo();

            [renderEncoder setTriangleFillMode:convertToMtlFillMode(info.rasterizationState.polygonMode)];
            [renderEncoder setCullMode:convertToMtlCullMode(info.rasterizationState.cullMode)];
            [renderEncoder setFrontFacingWinding:convertToMtlWinding(info.rasterizationState.windingMode)];
        }
        activePipeline = pipeline;
    }

    if (vertexBufferBindingBase != activeVertexBufferBindingBase) {
        activeVertexBufferBindingBase = vertexBufferBindingBase;
        vertexBufferBits.set();
    }

    if (vertexBufferBits.any()) {
        for (uint32_t i = 0; i < vertexBuffers.size(); ++i) {
            if (vertexBufferBits[i]) {
                const auto& bufferWithOffset = vertexBuffers[i];
                id<MTLBuffer> mtlBuffer = nullptr;
                NSUInteger mtlOffset = 0;

                if (bufferWithOffset.buffer) {
                    auto mtlBufferObj = snap::rhi::backend::common::smart_cast<snap::rhi::backend::metal::Buffer>(
                        bufferWithOffset.buffer);
                    mtlBuffer = mtlBufferObj->getBuffer();
                    mtlOffset = bufferWithOffset.offset;
                }

                const auto bindingIndex = activeVertexBufferBindingBase + i;
                auto& context = commandBuffer->getContext();
                const auto& renderEncoder = context.getRenderEncoder();
                [renderEncoder setVertexBuffer:mtlBuffer offset:mtlOffset atIndex:bindingIndex];

                activeVertexBuffers[i] = bufferWithOffset;
            }
        }
        vertexBufferBits.reset();
    }

    PipelineResourceState::setAllStates();
}

void RenderPipelineResourceState::clearStates() {
    vertexBufferBindingBase = 0;
    activeVertexBufferBindingBase = 0;

    activePipeline = nullptr;
    std::ranges::fill(activeVertexBuffers, BufferWithOffset{nullptr, 0});

    pipeline = nullptr;
    std::ranges::fill(vertexBuffers, BufferWithOffset{nullptr, 0});

    vertexBufferBits.reset();

    PipelineResourceState::clearStates();
}
} // namespace snap::rhi::backend::metal
