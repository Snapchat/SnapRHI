//
//  ShaderSources.h
//  unitest-catch2
//
//  Embedded shader source code for all backends (GLSL, MSL, SPIR-V).
//  Shaders are stored as constexpr byte arrays to avoid file I/O in tests.
//  Copyright © 2026 Snapchat. All rights reserved.
//

#pragma once

#include <cstdint>
#include <span>
#include <string_view>

namespace test_shaders {

// =============================================================================
// OpenGL GLSL — Passthrough Render Shader (combined VS/FS)
// =============================================================================
// Uses #ifdef guards for entry point selection (same pattern as demo app).
// Binding model: layout(std140) uniform TestUBO at binding 0, set 0
// Vertex attributes: location 0 = position (vec2), location 1 = color (vec4)

constexpr std::string_view kGLSLPassthrough = R"GLSL(
#ifdef test_passthrough_vs

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec4 aColor;

out vec4 vColor;

layout(std140) uniform TestUBO {
    vec4 colorMultiplier;
} ubo;

void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    vColor = aColor * ubo.colorMultiplier;
}

#endif // test_passthrough_vs

#ifdef test_passthrough_fs
#ifdef GL_ES
    precision highp float;
#endif

in vec4 vColor;
layout(location = 0) out vec4 oColor;

void main() {
    oColor = vColor;
}

#endif // test_passthrough_fs
)GLSL";

// =============================================================================
// OpenGL GLSL — Fullscreen Fill Shader (no vertex input, outputs solid color)
// =============================================================================
// This shader draws a fullscreen triangle outputting a solid color from a UBO.

constexpr std::string_view kGLSLFill = R"GLSL(
#ifdef test_fill_vs

out vec4 vColor;

layout(std140) uniform FillUBO {
    vec4 fillColor;
} ubo;

void main() {
    // Fullscreen triangle trick: 3 vertices cover the screen
    vec2 positions[3] = vec2[3](
        vec2(-1.0, -1.0),
        vec2( 3.0, -1.0),
        vec2(-1.0,  3.0)
    );
    gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
    vColor = ubo.fillColor;
}

#endif // test_fill_vs

#ifdef test_fill_fs
#ifdef GL_ES
    precision highp float;
#endif

in vec4 vColor;
layout(location = 0) out vec4 oColor;

void main() {
    oColor = vColor;
}

#endif // test_fill_fs
)GLSL";

// =============================================================================
// OpenGL GLSL — Compute Shader (fill SSBO with a pattern)
// =============================================================================
constexpr std::string_view kGLSLCompute = R"GLSL(
layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

layout(std430) buffer OutputBuffer {
    uint data[];
} outputBuffer;

void main() {
    uint idx = gl_GlobalInvocationID.x;
    outputBuffer.data[idx] = idx * 42u + 7u;
}
)GLSL";

// =============================================================================
// Metal MSL — Passthrough Render Shader
// =============================================================================
// Binding: argument buffer at buffer(0) with UBO pointer at id(0)
// Vertex attributes: attribute(0) = position, attribute(1) = color

constexpr std::string_view kMSLPassthrough = R"MSL(
#include <metal_stdlib>
using namespace metal;

struct VertexInput {
    float2 position [[attribute(0)]];
    float4 color    [[attribute(1)]];
};

struct VertexOutput {
    float4 position [[position]];
    float4 color;
};

struct TestUBO {
    float4 colorMultiplier;
};

struct ArgumentBuffer {
    constant TestUBO* ubo [[id(0)]];
};

vertex VertexOutput test_passthrough_vs(
    VertexInput in [[stage_in]],
    constant ArgumentBuffer& argBuf [[buffer(0)]]
) {
    VertexOutput out;
    out.position = float4(in.position, 0.0, 1.0);
    out.color = in.color * argBuf.ubo->colorMultiplier;
    return out;
}

fragment float4 test_passthrough_fs(VertexOutput in [[stage_in]]) {
    return in.color;
}
)MSL";

// =============================================================================
// Metal MSL — Fullscreen Fill Shader
// =============================================================================

constexpr std::string_view kMSLFill = R"MSL(
#include <metal_stdlib>
using namespace metal;

struct VertexOutput {
    float4 position [[position]];
    float4 color;
};

struct FillUBO {
    float4 fillColor;
};

struct FillArgumentBuffer {
    constant FillUBO* ubo [[id(0)]];
};

vertex VertexOutput test_fill_vs(
    uint vertexID [[vertex_id]],
    constant FillArgumentBuffer& argBuf [[buffer(0)]]
) {
    float2 positions[3] = {
        float2(-1.0, -1.0),
        float2( 3.0, -1.0),
        float2(-1.0,  3.0)
    };
    VertexOutput out;
    out.position = float4(positions[vertexID], 0.0, 1.0);
    out.color = argBuf.ubo->fillColor;
    return out;
}

fragment float4 test_fill_fs(VertexOutput in [[stage_in]]) {
    return in.color;
}
)MSL";

// =============================================================================
// Metal MSL — Compute Shader (fill buffer with pattern)
// =============================================================================

constexpr std::string_view kMSLCompute = R"MSL(
#include <metal_stdlib>
using namespace metal;

struct ComputeArgumentBuffer {
    device uint* data [[id(0)]];
};

kernel void test_compute_fill(
    constant ComputeArgumentBuffer& argBuf [[buffer(0)]],
    uint idx [[thread_position_in_grid]]
) {
    argBuf.data[idx] = idx * 42u + 7u;
}
)MSL";

// =============================================================================
// Vulkan GLSL (SPIR-V source) — Passthrough Render Shader
// =============================================================================
// This is compiled with glslang to SPIR-V.
// set = 0, binding = 0: TestUBO

constexpr std::string_view kVulkanGLSLPassthroughVS = R"GLSL(
#version 450 core

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec4 aColor;

layout(location = 0) out vec4 vColor;

layout(set = 0, binding = 0, std140) uniform TestUBO {
    vec4 colorMultiplier;
} ubo;

void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    vColor = aColor * ubo.colorMultiplier;
}
)GLSL";

constexpr std::string_view kVulkanGLSLPassthroughFS = R"GLSL(
#version 450 core

layout(location = 0) in vec4 vColor;
layout(location = 0) out vec4 oColor;

void main() {
    oColor = vColor;
}
)GLSL";

// =============================================================================
// Vulkan GLSL — Fullscreen Fill Shader
// =============================================================================

constexpr std::string_view kVulkanGLSLFillVS = R"GLSL(
#version 450 core

layout(location = 0) out vec4 vColor;

layout(set = 0, binding = 0, std140) uniform FillUBO {
    vec4 fillColor;
} ubo;

void main() {
    vec2 positions[3] = vec2[3](
        vec2(-1.0, -1.0),
        vec2( 3.0, -1.0),
        vec2(-1.0,  3.0)
    );
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    vColor = ubo.fillColor;
}
)GLSL";

constexpr std::string_view kVulkanGLSLFillFS = R"GLSL(
#version 450 core

layout(location = 0) in vec4 vColor;
layout(location = 0) out vec4 oColor;

void main() {
    oColor = vColor;
}
)GLSL";

// =============================================================================
// Vulkan GLSL — Compute Shader
// =============================================================================

constexpr std::string_view kVulkanGLSLCompute = R"GLSL(
#version 450 core

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

layout(set = 0, binding = 0, std430) buffer OutputBuffer {
    uint data[];
} outputBuffer;

void main() {
    uint idx = gl_GlobalInvocationID.x;
    outputBuffer.data[idx] = idx * 42u + 7u;
}
)GLSL";

// =============================================================================
// Pre-compiled SPIR-V binaries
// =============================================================================
// These are compiled from the Vulkan GLSL shaders above.
// To regenerate:
//   glslang -V -S vert passthrough_vs.glsl -o passthrough_vs.spv
//   glslang -V -S frag passthrough_fs.glsl -o passthrough_fs.spv
//   glslang -V -S comp compute_fill.glsl -o compute_fill.spv
//   spirv-link passthrough_vs.spv passthrough_fs.spv -o passthrough.spv
//
// For test execution, we compile at runtime using the Vulkan GLSL text above
// via glslang if available, or use the ShaderLibraryCreateFlag::CompileFromBinary
// path with pre-compiled SPIR-V.
//
// The SPIR-V binaries are generated by the CMake build step (see CMakeLists.txt).
// For simplicity in unit tests, Vulkan tests that require shaders will skip
// if SPIR-V compilation tooling is not available at runtime.

} // namespace test_shaders
