// OpenGL passthrough render shader (combined VS/FS)

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
