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
