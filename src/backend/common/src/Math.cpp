#include "snap/rhi/backend/common/Math.hpp"

namespace snap::rhi::backend::common {
bool isPowerOfTwo(uint64_t n) {
    assert(n != 0);
    return (n & (n - 1)) == 0;
}

bool isPtrAligned(const void* ptr, size_t alignment) {
    assert(isPowerOfTwo(alignment));
    assert(alignment != 0);
    return (reinterpret_cast<size_t>(ptr) & (alignment - 1)) == 0;
}

bool isAligned(uint32_t value, size_t alignment) {
    assert(alignment <= UINT32_MAX);
    assert(isPowerOfTwo(alignment));
    assert(alignment != 0);
    uint32_t alignment32 = static_cast<uint32_t>(alignment);
    return (value & (alignment32 - 1)) == 0;
}

uint32_t align(uint32_t value, size_t alignment) {
    assert(alignment <= UINT32_MAX);
    assert(isPowerOfTwo(alignment));
    assert(alignment != 0);
    uint32_t alignment32 = static_cast<uint32_t>(alignment);
    return (value + (alignment32 - 1)) & ~(alignment32 - 1);
}
} // namespace snap::rhi::backend::common
