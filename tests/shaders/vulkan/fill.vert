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
