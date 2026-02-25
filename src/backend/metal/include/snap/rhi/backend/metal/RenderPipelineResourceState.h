//
// Created by Vladyslav Deviatkov on 2/3/26.
//

#pragma once

#include "snap/rhi/backend/metal/PipelineResourceState.h"

namespace snap::rhi::backend::metal {
class RenderPipeline;
class Buffer;

class RenderPipelineResourceState final : public PipelineResourceState {
public:
    RenderPipelineResourceState(CommandBuffer* commandBuffer);
    ~RenderPipelineResourceState() override = default;

    void bindVertexBuffer(const uint32_t binding, Buffer* buffer, const uint32_t offset);
    void bindRenderPipeline(RenderPipeline* pipeline);

    void setAllStates() override;
    void clearStates() override;

private:
    struct BufferWithOffset {
        Buffer* buffer = nullptr;
        uint32_t offset = 0;

        friend auto operator<=>(const BufferWithOffset& lhs, const BufferWithOffset& rhs) noexcept = default;
    };

    RenderPipeline* activePipeline = nullptr;
    std::array<BufferWithOffset, snap::rhi::MaxVertexBuffers> activeVertexBuffers{};
    uint32_t activeVertexBufferBindingBase = 0;

    RenderPipeline* pipeline = nullptr;
    std::array<BufferWithOffset, snap::rhi::MaxVertexBuffers> vertexBuffers{};
    uint32_t vertexBufferBindingBase = 0;

    std::bitset<snap::rhi::MaxVertexBuffers> vertexBufferBits;
};
} // namespace snap::rhi::backend::metal
