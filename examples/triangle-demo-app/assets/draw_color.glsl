//
// draw_color.glsl
// SnapRHI Demo Application - OpenGL Shader
//
// Copyright (c) 2026 Snap Inc. All rights reserved.
//
// This combined shader file contains both vertex and fragment stages.
// The SnapRHI OpenGL backend compiles each stage separately by
// defining the appropriate preprocessor macro (snap_rhi_demo_triangle_vs or snap_rhi_demo_triangle_fs).
//
// Binding Model:
// - Uniform buffer "TriangleSettings" at binding 2 (maps to descriptor set index 3, binding 2)
// - Vertex attribute "aPos" at location 0
// - Vertex attribute "aColor" at location 1
//

// =============================================================================
// VERTEX SHADER
// =============================================================================

#ifdef snap_rhi_demo_triangle_vs

// --- Vertex Inputs ---
// These must match the VertexInputState configuration in TriangleRenderer.
layout(location = 0) in vec2 aPos;    // 2D position in NDC
layout(location = 1) in vec4 aColor;  // RGBA color

// --- Interpolated Outputs ---
// Passed to the fragment shader for interpolation across the triangle.
out vec4 vColor;

// --- Uniform Buffer ---
// Contains transformation matrix and color multiplier.
layout(std140) uniform TriangleSettings {
    mat4 transform;  // 4x4 transformation matrix (currently rotation around Y)
    vec4 color;      // Color multiplier/tint
} ubo;

void main() {
    // Apply transformation to vertex position
    // Note: We use vec4(aPos, 0.0, 1.0) since our positions are 2D
    gl_Position = ubo.transform * vec4(aPos, 0.0, 1.0);

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

// --- Interpolated Inputs ---
// Received from vertex shader, interpolated across the triangle face.
in vec4 vColor;

// --- Fragment Output ---
// Final color written to the render target.
layout(location = 0) out vec4 oColor;

void main() {
    // Output the interpolated color directly
    oColor = vColor;
}

#endif // snap_rhi_demo_triangle_fs
