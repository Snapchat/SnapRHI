//
// TriangleRenderer.h
// SnapRHI Demo Application
//
// Copyright (c) 2026 Snap Inc. All rights reserved.
//
// This file demonstrates how to use the SnapRHI API to render
// a simple rotating triangle. It serves as a reference implementation for
// engine integrators and graphics programmers.
//

#pragma once

#include <snap/rhi/Buffer.hpp>
#include <snap/rhi/CommandBuffer.hpp>
#include <snap/rhi/DescriptorPool.hpp>
#include <snap/rhi/DescriptorSet.hpp>
#include <snap/rhi/DescriptorSetLayout.hpp>
#include <snap/rhi/Device.hpp>
#include <snap/rhi/Framebuffer.hpp>
#include <snap/rhi/PipelineLayout.hpp>
#include <snap/rhi/RenderPass.hpp>
#include <snap/rhi/RenderPipeline.hpp>
#include <snap/rhi/ShaderLibrary.hpp>
#include <snap/rhi/ShaderModule.hpp>
#include <snap/rhi/Texture.hpp>

#include <chrono>
#include <memory>

#include <glm/gtc/matrix_transform.hpp>

namespace snap::rhi::demo {

/**
 * @class TriangleRenderer
 * @brief A demonstration renderer that draws a rotating, colored triangle.
 *
 * @details
 * This class showcases the core SnapRHI rendering workflow:
 *
 * 1. **Shader Loading**: Platform-specific shader sources are loaded from assets
 *    and compiled into shader modules (Metal MSL / OpenGL GLSL).
 *
 * 2. **Resource Creation**: Vertex buffers, uniform buffers, and descriptor sets
 *    are created to supply data to the GPU.
 *
 * 3. **Pipeline Setup**: A render pipeline is configured with vertex input layout,
 *    rasterization state, blend state, and shader stages.
 *
 * 4. **Rendering Paths**: Supports both modern "Dynamic Rendering" (VK_KHR_dynamic_rendering,
 *    Metal) and traditional "RenderPass/Framebuffer" paths for maximum compatibility.
 *
 * ## Thread Safety
 * This class is **not thread-safe**. All methods must be called from the thread
 * that owns the associated `snap::rhi::Device` context.
 *
 * ## Memory Model
 * - GPU resources are created lazily on first render and cached for reuse.
 * - Per-frame uniform buffers and descriptor sets are allocated from a pool
 *   to avoid recreating them every frame (simple ring-buffer pattern).
 *
 * ## Shader Binding Model
 * The demo uses descriptor set index 3 with binding 2 for the uniform buffer.
 * This deliberately exercises a non-zero descriptor set to demonstrate proper
 * multi-set pipeline layout configuration.
 *
 * @note This is a reference implementation intended for learning. Production code
 *       should implement proper resource lifetime management, frame synchronization,
 *       and error handling.
 *
 * @see snap::rhi::Device, snap::rhi::RenderPipeline, snap::rhi::CommandBuffer
 */
class TriangleRenderer final {
public:
    // =========================================================================
    // Construction / Destruction
    // =========================================================================

    /**
     * @brief Constructs a TriangleRenderer bound to the given device.
     *
     * @param device The SnapRHI device to use for resource creation.
     *               Must remain valid for the lifetime of this renderer.
     *
     * @throws std::runtime_error If shader loading or resource creation fails.
     *
     * @details
     * Construction performs the following initialization:
     * - Loads platform-appropriate shaders (Metal MSL or OpenGL GLSL)
     * - Creates shader modules for vertex and fragment stages
     * - Creates the descriptor set layout describing our uniform buffer binding
     * - Creates the pipeline layout with proper descriptor set arrangement
     * - Allocates a descriptor pool for per-frame descriptor set allocation
     * - Creates and uploads the static vertex buffer data
     */
    explicit TriangleRenderer(std::shared_ptr<snap::rhi::Device> device);

    /**
     * @brief Destroys the renderer and releases all GPU resources.
     *
     * @note All GPU resources are released via shared_ptr ref-counting.
     *       The actual GPU memory is freed when the device processes pending deletions.
     */
    ~TriangleRenderer();

    // Non-copyable, non-movable (contains device-bound resources)
    TriangleRenderer(const TriangleRenderer&) = delete;
    TriangleRenderer& operator=(const TriangleRenderer&) = delete;
    TriangleRenderer(TriangleRenderer&&) = delete;
    TriangleRenderer& operator=(TriangleRenderer&&) = delete;

    // =========================================================================
    // Rendering
    // =========================================================================

    /**
     * @brief Records rendering commands into the given command buffer.
     *
     * @param commandBuffer The command buffer to record into. Must be in a valid
     *                      state for recording (not submitted, not finalized).
     * @param renderTarget  The texture to render into. Must have ColorAttachment usage.
     *
     * @details
     * This method:
     * 1. Allocates a per-frame descriptor set and uniform buffer
     * 2. Updates the uniform buffer with the current rotation transform
     * 3. Begins a render pass (dynamic or traditional based on device capabilities)
     * 4. Binds the pipeline, vertex buffer, and descriptor set
     * 5. Issues a draw call for 3 vertices (one triangle)
     * 6. Ends the render pass
     *
     * The triangle rotates around the Y-axis based on elapsed time since construction.
     *
     * @note The caller is responsible for submitting the command buffer to a queue
     *       after this method returns.
     */
    void render(const std::shared_ptr<snap::rhi::CommandBuffer>& commandBuffer,
                const std::shared_ptr<snap::rhi::Texture>& renderTarget);

private:
    // =========================================================================
    // Internal Types
    // =========================================================================

    /**
     * @brief CPU-side vertex data structure.
     *
     * @details
     * Layout must match the vertex shader input:
     * - location 0: vec2 position (aPos)
     * - location 1: vec4 color (aColor)
     *
     * This struct is tightly packed for efficient GPU upload.
     */
    struct Vertex {
        glm::vec2 position; ///< 2D position in normalized device coordinates
        glm::vec4 color;    ///< RGBA color (will be modulated by uniform tint)
    };

    /**
     * @brief Uniform buffer data for the triangle transform and color.
     *
     * @details
     * Layout must match the shader's uniform block (std140/Metal aligned):
     * - mat4 transform: Model-view-projection matrix
     * - vec4 color: Color multiplier/tint
     *
     * The 16-byte alignment ensures compatibility with all backends.
     */
    struct alignas(16) UniformData {
        glm::mat4 transformMatrix{1.0f}; ///< 4x4 transformation matrix
        glm::vec4 colorMultiplier{1.0f}; ///< RGBA color tint (multiplied with vertex color)
    };
    static_assert(sizeof(UniformData) == 80, "UniformData must be 80 bytes (mat4 + vec4)");
    static_assert(alignof(UniformData) == 16, "UniformData must be 16-byte aligned");

    // =========================================================================
    // Initialization Helpers
    // =========================================================================

    /**
     * @brief Loads and compiles shaders for the current graphics backend.
     *
     * @details
     * Detects the active backend (Metal/OpenGL) via device capabilities and loads
     * the appropriate shader source file from the assets directory.
     *
     * Metal: assets/draw_color.msl
     * OpenGL: assets/draw_color.glsl
     */
    void initializeShaders();

    /**
     * @brief Creates the descriptor set layout describing resource bindings.
     *
     * @details
     * Our shader uses a single uniform buffer at binding 2.
     * The layout is created with vertex+fragment stage visibility.
     */
    void initializeDescriptorSetLayout();

    /**
     * @brief Creates the pipeline layout with descriptor set arrangements.
     *
     * @details
     * We use descriptor set index 3 to demonstrate non-trivial set indexing.
     * Sets 0-2 use an empty descriptor set layout (zero bindings), and set 3 contains our actual layout.
     * All entries must be non-null; backends like Vulkan require valid descriptor set layouts for all indices.
     */
    void initializePipelineLayout();

    /**
     * @brief Creates the descriptor pool for allocating per-frame descriptor sets.
     *
     * @details
     * Pool is sized for multiple frames in flight to avoid stalls.
     */
    void initializeDescriptorPool();

    /**
     * @brief Creates and uploads the static vertex buffer.
     *
     * @details
     * The triangle vertices are defined in NDC with per-vertex colors:
     * - Bottom-left: Red
     * - Bottom-right: Green
     * - Top-center: Blue
     */
    void initializeVertexBuffer();

    // =========================================================================
    // Per-Frame Helpers
    // =========================================================================

    /**
     * @brief Allocates a descriptor set and uniform buffer for the current frame.
     *
     * @param[out] outDescriptorSet The allocated descriptor set.
     * @param[out] outUniformBuffer The allocated uniform buffer.
     * @return true if allocation succeeded, false otherwise.
     *
     * @details
     * Creates a new uniform buffer and descriptor set, then binds the buffer
     * to the descriptor set at our UBO binding slot.
     */
    [[nodiscard]] bool allocatePerFrameResources(std::shared_ptr<snap::rhi::DescriptorSet>& outDescriptorSet,
                                                 std::shared_ptr<snap::rhi::Buffer>& outUniformBuffer);

    /**
     * @brief Updates the uniform buffer with the current transformation.
     *
     * @param uniformBuffer The buffer to update.
     *
     * @details
     * Computes a rotation matrix based on elapsed time and writes it to the buffer.
     * Uses coherent host-visible memory for simplicity (no explicit flush needed).
     */
    void updateUniformBuffer(snap::rhi::Buffer* uniformBuffer);

    /**
     * @brief Creates or retrieves a cached render pipeline for the given format.
     *
     * @param colorFormat The render target format.
     * @param useRenderPass Whether to use the traditional render pass path.
     * @return true if a valid pipeline is available, false otherwise.
     *
     * @details
     * Pipelines are lazily created and cached. For dynamic rendering, the pipeline
     * is format-specific. For render pass mode, we also cache the render pass and
     * framebuffer.
     */
    [[nodiscard]] bool ensurePipelineForFormat(snap::rhi::PixelFormat colorFormat, bool useRenderPass);

    // =========================================================================
    // Render Path Implementations
    // =========================================================================

    /**
     * @brief Renders using the dynamic rendering path (VK_KHR_dynamic_rendering / Metal).
     *
     * @details
     * Modern GPUs support dynamic rendering which eliminates the need for
     * pre-declared render pass objects. Attachments are specified inline.
     */
    void renderWithDynamicRendering(snap::rhi::RenderCommandEncoder* encoder,
                                    snap::rhi::Texture* renderTarget,
                                    snap::rhi::DescriptorSet* descriptorSet);

    /**
     * @brief Renders using the traditional render pass / framebuffer path.
     *
     * @details
     * For compatibility with older APIs or when dynamic rendering is unavailable.
     * Uses cached RenderPass and Framebuffer objects.
     */
    void renderWithRenderPass(snap::rhi::RenderCommandEncoder* encoder,
                              snap::rhi::Texture* renderTarget,
                              snap::rhi::DescriptorSet* descriptorSet);

    // =========================================================================
    // Static Helpers
    // =========================================================================

    /**
     * @brief Builds the OpenGL-specific pipeline reflection info.
     *
     * @return Configuration describing UBO names and vertex attribute mappings.
     *
     * @details
     * OpenGL lacks native descriptor sets, so we provide explicit name-to-binding
     * mappings that the backend uses to configure uniform blocks and attributes.
     */
    [[nodiscard]] static snap::rhi::opengl::RenderPipelineInfo buildOpenGLPipelineInfo();

    /**
     * @brief Creates a viewport covering the entire render target.
     *
     * @param width  Render target width in pixels.
     * @param height Render target height in pixels.
     * @return Configured viewport structure.
     */
    [[nodiscard]] static snap::rhi::Viewport createFullscreenViewport(uint32_t width, uint32_t height);

    // =========================================================================
    // Member Data
    // =========================================================================

    /// The SnapRHI device used for all resource creation.
    std::shared_ptr<snap::rhi::Device> device_;

    // --- Shader Resources ---
    std::shared_ptr<snap::rhi::ShaderLibrary> shaderLibrary_; ///< Compiled shader library
    std::shared_ptr<snap::rhi::ShaderModule> vertexShader_;   ///< Vertex shader module
    std::shared_ptr<snap::rhi::ShaderModule> fragmentShader_; ///< Fragment shader module

    // --- Pipeline Resources ---
    std::shared_ptr<snap::rhi::DescriptorSetLayout> descriptorSetLayout_;      ///< UBO binding layout
    std::shared_ptr<snap::rhi::DescriptorSetLayout> emptyDescriptorSetLayout_; ///< Empty layout for unused set indices
    std::shared_ptr<snap::rhi::PipelineLayout> pipelineLayout_;                ///< Full pipeline layout
    std::shared_ptr<snap::rhi::RenderPipeline> renderPipeline_;                ///< Cached render pipeline

    // --- Descriptor Resources ---
    std::shared_ptr<snap::rhi::DescriptorPool> descriptorPool_; ///< Pool for per-frame allocations

    // --- Geometry Resources ---
    std::shared_ptr<snap::rhi::Buffer> vertexBuffer_; ///< Static triangle vertex data

    // --- Render Pass Resources (for non-dynamic rendering path) ---
    std::shared_ptr<snap::rhi::RenderPass> cachedRenderPass_;                             ///< Cached render pass object
    snap::rhi::PixelFormat cachedRenderTargetFormat_ = snap::rhi::PixelFormat::Undefined; ///< Format of cached objects

    // --- Animation State ---
    std::chrono::steady_clock::time_point animationStartTime_; ///< Time when animation started

    // =========================================================================
    // Constants
    // =========================================================================

    /// Descriptor set index where our uniform buffer is bound.
    /// Using set 3 to demonstrate non-trivial pipeline layout configuration.
    static constexpr uint32_t kDescriptorSetIndex = 3;

    /// Binding slot within the descriptor set for the uniform buffer.
    /// Matches the shader's `layout(binding = 2)` declaration.
    static constexpr uint32_t kUniformBufferBinding = 2;

    /// Vertex buffer binding slot in the vertex input layout.
    static constexpr uint32_t kVertexBufferBinding = 0;

    /// Number of vertices in our triangle.
    static constexpr uint32_t kTriangleVertexCount = 3;

    /// Maximum number of descriptor sets that can be allocated from the pool.
    static constexpr uint32_t kDescriptorPoolMaxSets = 8;

    /// Clear color for the render target (deep blue).
    static constexpr std::array<float, 4> kClearColor = {0.1f, 0.2f, 0.8f, 1.0f};
};

} // namespace snap::rhi::demo
