#pragma once

#include "snap/rhi/Enums.h"
#include "snap/rhi/Limits.h"
#include "snap/rhi/Structs.h"

#include <vector>

namespace snap::rhi::opengl {
/**
 * @brief Identifies a specific resource slot within the logical binding model.
 *
 * @details
 * Represents a unique key defined by the combination of a Descriptor Set index
 * and a Binding index (e.g., `layout(set = 0, binding = 1)` in Vulkan/HLSL).
 *
 * Since standard OpenGL GLSL does not natively support the concept of "Descriptor Sets"
 * (outside of SPIR-V usage), this structure is used as a key to look up the
 * corresponding OpenGL uniform location or binding point.
 */
struct ResourceBinding {
    /**
     * @brief The logical Descriptor Set index.
     * @note Maps to `layout(set = X)` in modern shading languages.
     */
    uint32_t descriptorSet = 0;

    /**
     * @brief The binding slot index within the specific Descriptor Set.
     * @note Maps to `layout(binding = Y)`.
     */
    uint32_t binding = 0;

    constexpr friend auto operator<=>(const ResourceBinding&, const ResourceBinding&) noexcept = default;
};

/**
 * @brief Describes the mapping between a logical engine resource and a physical OpenGL program resource.
 *
 * @details
 * This structure serves as the "Translation Layer" for the OpenGL backend.
 * It maps a high-level logical binding (Set/Binding) to the actual string name
 * required by the OpenGL driver to locate the uniform or resource block.
 *
 * Usage Cycle:
 * 1. Engine defines resources using `ResourceBinding` (Set 0, Binding 1).
 * 2. This struct links that binding to the string "u_CameraData".
 * 3. OpenGL backend calls `glGetUniformLocation(prog, "u_CameraData")` or
 * `glGetUniformBlockIndex(prog, "u_CameraData")` to perform the actual bind.
 */
struct PipelineResourceInfo {
    /**
     * @brief The raw name of the uniform or interface block as defined in the GLSL source code.
     *
     * @details
     * This string is critical for introspection. It is passed directly to functions like:
     * - `glGetUniformLocation` (for Samplers, Images, basic Uniforms)
     * - `glGetUniformBlockIndex` (for Uniform Buffer Objects)
     * - `glGetProgramResourceIndex` (for SSBOs)
     *
     * @warning This must match the shader source exactly (case-sensitive).
     */
    std::string name;

    /**
     * @brief The type of the descriptor (e.g., UniformBuffer, Sampler, Image).
     *
     * @note This hints to the backend which GL introspection API to use.
     * For example, if type is `Sampler`, the engine knows to query a `GLint` location.
     * If type is `UniformBuffer`, the engine knows to query a `GLuint` block index.
     */
    snap::rhi::DescriptorType descriptorType = DescriptorType::Undefined;

    /**
     * @brief The logical binding key associated with this named resource.
     */
    ResourceBinding binding{};

    constexpr friend auto operator<=>(const PipelineResourceInfo&, const PipelineResourceInfo&) noexcept = default;
};

/**
 * @brief Aggregates all active resource mappings for a specific OpenGL Pipeline (Program).
 *
 * @details
 * This structure is typically generated during Shader Reflection or Pipeline Creation.
 * It provides a complete lookup table that allows the `DescriptorSet` mechanism to
 * function on top of OpenGL.
 *
 */
struct PipelineInfo {
    /**
     * @brief A flat list of all active resources referenced by the shader program.
     */
    std::vector<PipelineResourceInfo> resources;

    constexpr friend auto operator<=>(const PipelineInfo&, const PipelineInfo&) noexcept = default;
};
} // namespace snap::rhi::opengl
