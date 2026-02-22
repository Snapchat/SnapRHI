//
//  RenderCommandEncoder.h
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 07.10.2020.
//  Copyright © 2021 Snapchat. All rights reserved.
//

#pragma once

#include "snap/rhi/RenderCommandEncoder.hpp"
#include "snap/rhi/backend/common/ValidationLayer.hpp"
#include "snap/rhi/backend/metal/CommandEncoder.hpp"
#include "snap/rhi/backend/metal/Context.h"
#include "snap/rhi/backend/metal/RenderPipelineResourceState.h"

#include <Metal/Metal.h>
#include <array>

namespace snap::rhi::backend::metal {
class Device;
class CommandBuffer;
class Buffer;
class Framebuffer;

using RenderCommandEncoderBase = snap::rhi::backend::metal::CommandEncoder<snap::rhi::RenderCommandEncoder>;
class RenderCommandEncoder final : public RenderCommandEncoderBase {
    struct IndexBufferInfo {
        Buffer* indexBuffer = nullptr;
        uint32_t offset = 0;

        void clear() {
            indexBuffer = nullptr;
            offset = 0;
        }
    };

    struct RenderPipelineInfo {
        bool depthBiasEnabled = false;

        float depthBiasConstantFactor = 0.0f;
        float depthBiasSlopeFactor = 0.0f;
        float depthBiasClamp = 0.0f;

        std::array<uint32_t, 2> stencilReference{0, 0}; //[0] ->StencilFace::Front; [1] -> StencilFace::Back
        MTLPrimitiveType primitiveType = MTLPrimitiveTypeTriangle;
        MTLIndexType indexType = MTLIndexTypeUInt16;

        void clear() {
            primitiveType = MTLPrimitiveTypeTriangle;
            indexType = MTLIndexTypeUInt16;
            stencilReference.fill(0);
            depthBiasConstantFactor = 0.0f;
            depthBiasSlopeFactor = 0.0f;
            depthBiasClamp = 0.0f;
            depthBiasEnabled = false;
        }
    };

public:
    RenderCommandEncoder(Device* mtlDevice, CommandBuffer* commandBuffer);
    ~RenderCommandEncoder() override = default;

    void beginEncoding(const RenderPassBeginInfo& renderPassBeginInfo) override;
    void beginEncoding(const RenderingInfo& renderingInfo) override;
    void setViewport(const snap::rhi::Viewport& viewport) override;
    void bindRenderPipeline(snap::rhi::RenderPipeline* pipeline) override;
    void bindDescriptorSet(uint32_t binding,
                           snap::rhi::DescriptorSet* descriptorSet,
                           std::span<const uint32_t> dynamicOffsets) override;
    void bindVertexBuffers(const uint32_t firstBinding,
                           std::span<snap::rhi::Buffer*> buffers,
                           std::span<uint32_t> offsets) override;
    void bindVertexBuffer(const uint32_t binding, snap::rhi::Buffer* buffer, const uint32_t offset) override;
    void bindIndexBuffer(snap::rhi::Buffer* indexBuffer, const uint32_t offset, const IndexType indexType) override;
    void draw(const uint32_t vertexCount, const uint32_t firstVertex, const uint32_t instanceCount) override;
    void drawIndexed(const uint32_t indexCount, const uint32_t firstIndex, const uint32_t instanceCount) override;
    void setDepthBias(float depthBiasConstantFactor, float depthBiasSlopeFactor, float depthBiasClamp) override;
    void setStencilReference(const StencilFace face, const uint32_t reference) override;
    void setBlendConstants(const float r, const float g, const float b, const float a) override;
    void invokeCustomCallback(std::function<void()>* callback) override;
    void pipelineBarrier(snap::rhi::PipelineStageBits srcStageMask,
                         snap::rhi::PipelineStageBits dstStageMask,
                         snap::rhi::DependencyFlags dependencyFlags,
                         std::span<snap::rhi::MemoryBarrierInfo> memoryBarriers,
                         std::span<snap::rhi::BufferMemoryBarrierInfo> bufferMemoryBarriers,
                         std::span<snap::rhi::TextureMemoryBarrierInfo> textureMemoryBarriers) override;
    void endEncoding() override;

private:
    void beginEncoding();

    Context& context;

    RenderPipelineInfo renderPipelineInfo{};
    IndexBufferInfo indexBufferInfo{};
    RenderPipelineResourceState pipelineState;
};
} // namespace snap::rhi::backend::metal
