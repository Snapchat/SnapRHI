#pragma once

#include "snap/rhi/Enums.h"
#include "snap/rhi/Limits.h"
#include "snap/rhi/opengl/PipelineInfo.h"

#include <vector>

namespace snap::rhi::opengl {
/**
 * @brief Defines a pairing between a distinct Texture resource and a Sampler resource.
 *
 * @details
 * Modern graphics APIs (Vulkan, Metal, DX12) often treat Textures (Sampled Images)
 * and Samplers as completely separate objects that can be mixed and matched freely.
 *
 * In OpenGL, these concepts are tied together at the "Texture Unit" level. To emulate
 * separate descriptors, the engine must know which logical Texture binding pairs with
 * which logical Sampler binding for a specific shader operation.
 *
 * During bind time:
 * 1. The backend reads the texture handle from `textureBinding`.
 * 2. The backend reads the sampler object handle from `samplerBinding`.
 * 3. Both are bound to the *same* OpenGL Texture Unit index (e.g., `GL_TEXTURE0`).
 */
struct CombinedTextureSamplerInfo {
    /**
     * @brief The logical binding location of the Texture (Image) resource.
     */
    ResourceBinding textureBinding;

    /**
     * @brief The logical binding location of the Sampler resource.
     */
    ResourceBinding samplerBinding;

    constexpr friend auto operator<=>(const CombinedTextureSamplerInfo&,
                                      const CombinedTextureSamplerInfo&) noexcept = default;
};

/**
 * @brief Maps a vertex shader input attribute name to its generic attribute index location.
 *
 * @details
 * In OpenGL, vertex data flows from buffers into the shader via "Generic Vertex Attributes."
 * These connections are established using an integer index (Location).
 *
 * This structure links the GLSL variable name (e.g., "in_Position") to that integer (e.g., 0),
 * allowing the engine to correctly configure `glVertexAttribPointer` and `glEnableVertexAttribArray`
 * inside the Vertex Array Object (VAO).
 */
struct VertexAttributeInfo {
    /**
     * @brief The name of the input variable in the Vertex Shader source code.
     * @note Used with `glGetAttribLocation` if the location is not explicitly set in the shader.
     */
    std::string name;

    /**
     * @brief The Generic Vertex Attribute Index.
     * @note Corresponds to `layout(location = X)` in GLSL.
     */
    uint32_t location = Undefined;

    friend auto operator<=>(const VertexAttributeInfo&, const VertexAttributeInfo&) noexcept = default;
};

/**
 * @brief Extended pipeline configuration specifically for Graphics (Render) pipelines.
 *
 * @details
 * Inherits the base resource reflection (UBOs, SSBOs) from `PipelineInfo` and adds
 * metadata specific to the vertex processing and rasterization stages.
 *
 * This structure provides the complete blueprint required to link an OpenGL `GLProgram`
 * with the engine's `VertexInputState` and descriptor management systems.
 */
struct RenderPipelineInfo : public PipelineInfo {
    /**
     * @brief A list of Texture-Sampler pairs that need to be combined onto texture units.
     *
     * @note This is typically populated via shader reflection. If the shader uses
     * `combined image samplers` (standard GLSL), this list might track which discrete
     * engine resources satisfy that single combined requirement.
     */
    std::vector<CombinedTextureSamplerInfo> combinedTextureSamplerInfos;

    /**
     * @brief The active vertex attributes discovered in the shader program.
     */
    std::vector<VertexAttributeInfo> vertexAttributes;

    friend auto operator<=>(const RenderPipelineInfo&, const RenderPipelineInfo&) noexcept = default;
};
} // namespace snap::rhi::opengl
