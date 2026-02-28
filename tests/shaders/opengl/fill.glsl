// OpenGL fullscreen fill shader (combined VS/FS)

#ifdef test_fill_vs

out vec4 vColor;

layout(std140) uniform FillUBO {
    vec4 fillColor;
} ubo;

void main() {
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
