#pragma once

#include "snap/rhi/Limits.h"
#include "snap/rhi/RenderCommandEncoder.hpp"
#include "snap/rhi/backend/common/CommandEncoder.h"
#include "snap/rhi/backend/common/ResourceResidencySet.h"
#include "snap/rhi/backend/common/ValidationLayer.hpp"
#include "snap/rhi/backend/vulkan/CommandEncoder.hpp"
#include "snap/rhi/backend/vulkan/DescriptorSetEncoder.h"
#include "snap/rhi/backend/vulkan/vulkan.h"

namespace snap::rhi::backend::vulkan {
class Device;
class CommandBuffer;
class Buffer;
class Framebuffer;

using RenderCommandEncoderBase = CommandEncoder<snap::rhi::RenderCommandEncoder>;
class RenderCommandEncoder final : public RenderCommandEncoderBase, public DescriptorSetEncoder {
public:
    RenderCommandEncoder(snap::rhi::backend::vulkan::Device* vkDevice,
                         snap::rhi::backend::vulkan::CommandBuffer* commandBuffer);
    ~RenderCommandEncoder() override;

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
    void endEncoding() override;

private:
    void reset();

    bool isDynamicRenderingUsed = false;
    const uint32_t apiVersion = 0;
};
} // namespace snap::rhi::backend::vulkan
