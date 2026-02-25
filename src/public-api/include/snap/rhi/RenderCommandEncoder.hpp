//
//  RenderCommandEncoder.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 21.09.2020.
//  Copyright © 2021 Snapchat. All rights reserved.
//

#pragma once

#include <cstdint>

#include "snap/rhi/ClearValue.h"
#include "snap/rhi/DeviceChild.hpp"
#include "snap/rhi/Structs.h"
#include <span>

#include "snap/rhi/CommandEncoder.h"
#include <functional>
#include <string_view>

namespace snap::rhi {
class Device;
class CommandBuffer;
class RenderPipeline;
class Buffer;
class Framebuffer;
class Texture;
class Sampler;
class DescriptorSet;

/**
 * @brief Encoder used to record graphics (render) commands into a `snap::rhi::CommandBuffer`.
 *
 * A RenderCommandEncoder is created by a `snap::rhi::CommandBuffer` and is used to record render pass / dynamic
 * rendering commands (binding a render pipeline, binding resources, setting dynamic state, and issuing draw calls).
 *
 * The encoder is backend-agnostic: depending on the active backend it maps to concepts such as:
 * - Vulkan render pass commands (`vkCmdBeginRenderPass`) or dynamic rendering (`vkCmdBeginRendering`).
 * - Metal render command encoder (`MTLRenderCommandEncoder`).
 * - OpenGL command recording performed by the backend command queue.
 *
 * @note The exact validation behavior depends on the backend and enabled validation tags.
 */
class RenderCommandEncoder : public snap::rhi::CommandEncoder {
public:
    RenderCommandEncoder(Device* device, CommandBuffer* commandBuffer);
    ~RenderCommandEncoder() override = default;

    /**
     * @brief Begins an encoding scope using a traditional render pass + framebuffer.
     *
     * @param renderPassBeginInfo Render pass begin parameters (render pass, framebuffer, and clear values).
     *
     * @note If the render pass specifies a load operation of `AttachmentLoadOp::Clear` for an attachment, a
     * corresponding clear value must be provided.
     */
    virtual void beginEncoding(const RenderPassBeginInfo& renderPassBeginInfo) = 0;

    /**
     * @brief Begins an encoding scope using dynamic rendering-style attachments.
     *
     * @param renderingInfo Description of color/depth/stencil attachments, load/store operations, and clear values.
     */
    virtual void beginEncoding(const RenderingInfo& renderingInfo) = 0;

    /**
     * @brief Sets the viewport (and scissor, when applicable) for subsequent draw calls.
     *
     * Backends may also update the scissor rectangle to match the viewport extents.
     *
     * @param viewport Viewport rectangle and depth range.
     */
    virtual void setViewport(const Viewport& viewport) = 0;

    /**
     * @brief Binds a render pipeline for subsequent draw calls.
     *
     * The pipeline defines shader stages and fixed-function render state.
     *
     * @param pipeline Render pipeline to bind. Must not be null.
     */
    virtual void bindRenderPipeline(RenderPipeline* pipeline) = 0;

    /**
     * @brief Binds a descriptor set (resource group) to a specific slot for the current pipeline.
     *
     * @details
     * This binds a collection of resources (textures, samplers, UBOs, SSBOs) to the pipeline
     * so they can be accessed by shaders.
     *
     * ## Handling Dynamic Buffers
     * If the `descriptorSet` layout contains **Dynamic Buffers** (e.g., UniformBufferDynamic),
     * you must provide the specific byte offsets for this draw call via `dynamicOffsets`.
     *
     * ### Offset Ordering Rule
     * The values in `dynamicOffsets` must be strictly ordered as follows:
     * 1. **By Binding Index** (ascending order) defined in the layout.
     * 2. **By Array Element** (ascending order) if the binding is an array.
     *
     * **Example:**
     * - Binding 0: UniformBufferDynamic (Single)
     * - Binding 1: StorageBuffer (Static)
     * - Binding 2: UniformBufferDynamic (Array[2])
     *
     * The `dynamicOffsets` span must contain 3 values in this exact order:
     * `[Offset_Bind0, Offset_Bind2_Elem0, Offset_Bind2_Elem1]`
     *
     * ## Platform Specifics
     * - **Vulkan:** Maps directly to `pDynamicOffsets` in `vkCmdBindDescriptorSets`. The effective address
     * is `BufferAddress + DescriptorBaseOffset + dynamicOffset`.
     * - **Metal:** The engine writes these values into the **Auxiliary Dynamic Offset Buffer** associated
     * with this pipeline. The shader uses these values to offset the pointers stored in the Argument Buffer.
     * - **OpenGL:** The engine updates `glBindBufferRange` for the specific binding points associated with
     * the dynamic resources.
     *
     * @param binding The logical slot index to bind this set to.
     * @note Must be less than `snap::rhi::SupportedLimit::MaxBoundDescriptorSets`.
     *
     * @param descriptorSet Pointer to the descriptor set to bind.
     * @warning Must not be null. The set's layout must be compatible with the bound pipeline's layout.
     *
     * @param dynamicOffsets A span of byte offsets for all dynamic buffers in this set.
     * @warning The size of this span **must exactly match** the total count of dynamic descriptors
     * in the set layout. If the set has 0 dynamic buffers, pass an empty span.
     */
    virtual void bindDescriptorSet(uint32_t binding,
                                   DescriptorSet* descriptorSet,
                                   std::span<const uint32_t> dynamicOffsets) = 0;

    /**
     * @brief Binds multiple vertex buffers starting at the given binding index.
     *
     * Each entry in `buffers` is bound at `firstBinding + i` with the corresponding byte `offsets[i]`.
     *
     * @param firstBinding Index of the first vertex buffer binding slot.
     * @param buffers Vertex buffers to bind.
     * @param offsets Byte offsets into each buffer.
     *
     * @note `offsets.size()` must be greater than or equal to `buffers.size()`.
     * @note Each buffer should be created with `snap::rhi::BufferUsage::VertexBuffer`.
     */
    virtual void bindVertexBuffers(uint32_t firstBinding,
                                   std::span<snap::rhi::Buffer*> buffers,
                                   std::span<uint32_t> offsets) = 0;

    /**
     * @brief Convenience overload to bind a single vertex buffer.
     *
     * @param binding Vertex buffer binding slot.
     * @param buffer Vertex buffer to bind. Must not be null.
     * @param offset Byte offset into `buffer`.
     *
     * @note The buffer should be created with `snap::rhi::BufferUsage::VertexBuffer`.
     */
    virtual void bindVertexBuffer(uint32_t binding, snap::rhi::Buffer* buffer, uint32_t offset) = 0;

    /**
     * @brief Binds an index buffer for indexed drawing.
     *
     * @param indexBuffer Index buffer to bind. Must not be null.
     * @param offset Byte offset into the index buffer.
     * @param indexType Index element type.
     *
     * Backend notes / common restrictions:
     * - Many backends require `offset` to be aligned to the index element size (at least 2 or 4 bytes).
     * - Some platforms require 4-byte alignment for the index buffer offset.
     */
    virtual void bindIndexBuffer(Buffer* indexBuffer, uint32_t offset, IndexType indexType) = 0;

    /**
     * @brief Issues a non-indexed draw call.
     *
     * @param vertexCount Number of vertices to draw.
     * @param firstVertex First vertex index within the bound vertex buffers.
     * @param instanceCount Number of instances to draw.
     */
    virtual void draw(uint32_t vertexCount, uint32_t firstVertex, uint32_t instanceCount) = 0;

    /**
     * @brief Issues an indexed draw call.
     *
     * `bindIndexBuffer()` must be called beforehand.
     *
     * @param indexCount Number of indices to draw.
     * @param firstIndex First index within the bound index buffer.
     * @param instanceCount Number of instances to draw.
     */
    virtual void drawIndexed(uint32_t indexCount, uint32_t firstIndex, uint32_t instanceCount) = 0;

    /**
     * @brief Sets depth bias parameters used by rasterization.
     *
     * The effect depends on whether the currently bound pipeline enables depth bias.
     *
     * @param depthBiasConstantFactor Constant depth bias factor.
     * @param depthBiasSlopeFactor Slope-scaled depth bias factor.
     * @param depthBiasClamp Optional clamp for the depth bias.
     *
     * @note Support for depth-bias clamp may be device-dependent (see device capabilities).
     */
    virtual void setDepthBias(float depthBiasConstantFactor, float depthBiasSlopeFactor, float depthBiasClamp) = 0;

    /**
     * @brief Sets the stencil reference value used for stencil testing.
     *
     * @param face Faces affected by the reference value.
     * @param reference Stencil reference value.
     */
    virtual void setStencilReference(StencilFace face, uint32_t reference) = 0;

    /**
     * @brief Sets the constant blend color.
     *
     * The blend constant is used by blend factors that reference a constant color.
     *
     * @param r Red component.
     * @param g Green component.
     * @param b Blue component.
     * @param a Alpha component.
     */
    virtual void setBlendConstants(float r, float g, float b, float a) = 0;

    /**
     * @brief Schedules a custom callback to be executed when the command buffer is executed.
     *
     * This is intended for legacy/debug use. Backends may execute the callback immediately at encode time or later
     * during command buffer playback.
     *
     * @warning **LIFETIME HAZARD**: The user is responsible for keeping the
     * 'callback' object alive until the command buffer has finished execution.
     * SnapRHI DOES NOT copy or own this object. Passing a stack-allocated
     * variable that goes out of scope before GPU execution completes will cause a crash.
     */
    [[deprecated]] virtual void invokeCustomCallback(std::function<void()>* callback) = 0;

protected:
};
} // namespace snap::rhi
