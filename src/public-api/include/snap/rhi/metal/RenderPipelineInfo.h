#pragma once

#include "snap/rhi/Limits.h"
#include "snap/rhi/metal/PipelineInfo.h"

#include <array>
#include <cstdint>

namespace snap::rhi::metal {
/**
 * @brief Configuration for creating a Render Pipeline State Object (PSO).
 *
 * @details
 * This structure defines the pipeline layout and specifically handles the impedance mismatch
 * between Vulkan-style binding models and Metal's linear binding slots.
 */
struct RenderPipelineInfo final : public PipelineInfo {
    /**
     * @brief Default binding slot for Vertex Buffers to avoid collision with Argument Buffers.
     * @note Metal reserves slots 0..30 for both resources and vertex buffers.
     */
    static constexpr uint32_t VertexBufferBindingBase = 23;

    /**
     * @brief The starting binding slot index for vertex buffers in the Metal Vertex Shader.
     *
     * @details
     * Metal shares the same binding table (slots 0-30) for both Buffer Resources (Argument Buffers, UBOs)
     * and Vertex Attributes (Vertex Buffers).
     *
     * To prevent collisions, we reserve the lower slots (e.g., 0-22) for Descriptor Sets/Argument Buffers
     * and shift all Vertex Buffers to the upper slots (e.g., 23+).
     *
     * Example:
     * - `vertexBufferBindingBase` = 23.
     * - Input Layout defines a vertex stream at binding 0.
     * - Engine binds the actual `MTLBuffer` to `vertex_buffer(23)`.
     */
    uint32_t vertexBufferBindingBase = VertexBufferBindingBase;

    friend auto operator<=>(const RenderPipelineInfo&, const RenderPipelineInfo&) noexcept = default;
};
} // namespace snap::rhi::metal
