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
