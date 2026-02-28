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
