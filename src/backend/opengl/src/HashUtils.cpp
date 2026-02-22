#include "snap/rhi/backend/opengl/HashUtils.hpp"

#include <cstring>

namespace {
uint64_t hash(const void* data, const size_t length) {
    constexpr size_t step = sizeof(uint64_t);

    uint64_t result = 0;
    const uint8_t* const uint8Ptr = static_cast<const uint8_t*>(data);

    if ((reinterpret_cast<uintptr_t>(data) % step) != 0) {
        uint64_t value;
        for (size_t i = 0; i < length / step; ++i) {
            std::memcpy(&value, uint8Ptr + i * step, step);
            result = snap::rhi::backend::opengl::singleHashCombine(result, value);
        }
    } else {
        const uint64_t* const uint64Ptr = static_cast<const uint64_t*>(data);
        for (size_t i = 0; i < length / step; ++i) {
            result = snap::rhi::backend::opengl::singleHashCombine(result, uint64Ptr[i]);
        }
    }

    uint64_t last = 0;
    const size_t leftUnaligned = length % step;
    if (leftUnaligned) {
        std::memcpy(&last, uint8Ptr + length - leftUnaligned, leftUnaligned);
    }
    result = snap::rhi::backend::opengl::singleHashCombine(result, last);

    result = snap::rhi::backend::opengl::singleHashCombine(result, static_cast<uint64_t>(length));
    return result;
}
} // unnamed namespace

namespace snap::rhi::backend::opengl {
snap::rhi::backend::opengl::hash64 build_hash64(const std::string_view str) {
    return hash(str.data(), str.size());
}
} // namespace snap::rhi::backend::opengl
