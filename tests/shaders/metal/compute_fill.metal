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
