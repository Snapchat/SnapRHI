// OpenGL compute shader — fill SSBO with a pattern
layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

layout(std430) buffer OutputBuffer {
    uint data[];
} outputBuffer;

void main() {
    uint idx = gl_GlobalInvocationID.x;
    outputBuffer.data[idx] = idx * 42u + 7u;
}
