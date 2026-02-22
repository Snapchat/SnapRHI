//
// draw_color.frontend.glsl
// SnapRHI Demo Application - Vulkan/SPIR-V Shader
//
// Copyright (c) 2026 Snap Inc. All rights reserved.
//
// This combined shader file contains both vertex and fragment stages
// for compilation to SPIR-V. Each stage is compiled separately by
// defining the appropriate preprocessor macro.
//
// Build instructions:
//
// 1. Compile each stage to SPIR-V:
//    glslang -V -S vert -Dsnap_rhi_demo_triangle_vs draw_color.frontend.glsl --entry-point snap_rhi_demo_triangle_vs --source-entrypoint main -o draw_color_vs.spv
//    glslang -V -S frag -Dsnap_rhi_demo_triangle_fs draw_color.frontend.glsl --entry-point snap_rhi_demo_triangle_fs --source-entrypoint main -o draw_color_fs.spv
//
// 2. Link both stages into a single shader library:
//    spirv-link draw_color_vs.spv draw_color_fs.spv -o draw_color.spv
//
// One-liner:
//    glslang -V -S vert -Dsnap_rhi_demo_triangle_vs draw_color.frontend.glsl --entry-point snap_rhi_demo_triangle_vs --source-entrypoint main -o draw_color_vs.spv && glslang -V -S frag -Dsnap_rhi_demo_triangle_fs draw_color.frontend.glsl --entry-point snap_rhi_demo_triangle_fs --source-entrypoint main -o draw_color_fs.spv && spirv-link draw_color_vs.spv draw_color_fs.spv -o draw_color.spv
//
// Binding Model (matching Metal ArgumentBuffer):
//   - Descriptor set 3, binding 2: TriangleSettings uniform buffer
//   - Vertex attribute location 0: position (vec2)
//   - Vertex attribute location 1: color (vec4)
//

#version 450 core

// =============================================================================
// VERTEX SHADER
// =============================================================================

#ifdef snap_rhi_demo_triangle_vs

// --- Vertex Inputs ---
// These must match the VertexInputState configuration in TriangleRenderer.
layout(location = 0) in vec2 aPos;    // 2D position in NDC
layout(location = 1) in vec4 aColor;  // RGBA color

// --- Vertex Outputs ---
// Passed to the fragment shader for interpolation across the triangle.
layout(location = 0) out vec4 vColor;

// --- Uniform Buffer ---
// set = 3 corresponds to kDescriptorSetIndex
// binding = 2 corresponds to kUniformBufferBinding
layout(set = 3, binding = 2, std140) uniform TriangleSettings {
    mat4 transform;  // 4x4 transformation matrix (currently rotation around Y)
    vec4 color;      // Color multiplier/tint
} ubo;

/**
 * @brief Adjusts clip-space Z for Vulkan's depth range.
 *
 * Vulkan (like Metal) uses [0, 1] depth range while OpenGL uses [-1, 1].
 * This remaps the Z coordinate from [-w, w] to [0, w].
 *
 * @param position Clip-space position with OpenGL-style Z.
 * @return Position with Vulkan-compatible Z range.
 */
vec4 adjustDepthRangeForVulkan(vec4 position) {
    // Remap Z from [-1, 1] * w to [0, 1] * w
    position.z = position.z * 0.5 + 0.5 * position.w;
    return position;
}

void main() {
    // Apply transformation to vertex position
    // Note: We use vec4(aPos, 0.0, 1.0) since our positions are 2D
    vec4 clipPosition = ubo.transform * vec4(aPos, 0.0, 1.0);

    // Adjust depth for Vulkan's [0, 1] depth range (same as Metal)
    gl_Position = adjustDepthRangeForVulkan(clipPosition);

    // Pass through vertex color multiplied by uniform tint
    vColor = aColor * ubo.color;
}

#endif // snap_rhi_demo_triangle_vs

// =============================================================================
// FRAGMENT SHADER
// =============================================================================

#ifdef snap_rhi_demo_triangle_fs
#ifdef GL_ES
    precision highp float;
#endif

// --- Fragment Inputs ---
// Received from vertex shader, interpolated across the triangle face.
layout(location = 0) in vec4 vColor;

// --- Fragment Outputs ---
// Final color written to the render target.
layout(location = 0) out vec4 oColor;

void main() {
    // Output the interpolated color directly
    oColor = vColor;
}

#endif // snap_rhi_demo_triangle_fs
