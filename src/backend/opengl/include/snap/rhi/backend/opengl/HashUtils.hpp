// Copyright © 2024 Snap, Inc. All rights reserved.

#pragma once

#include "snap/rhi/common/HashCombine.h"
#include <string>
#include <string_view>

namespace snap::rhi::backend::opengl {
using hash64 = uint64_t;

template<typename T>
[[nodiscard]] constexpr T singleHashCombine(const T h1, const T h2) {
    static_assert(std::is_same_v<T, size_t> || std::is_same_v<T, uint64_t> || std::is_same_v<T, uint32_t>);

    const T result = h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
    return result;
}

snap::rhi::backend::opengl::hash64 build_hash64(const std::string_view str);

template<size_t Size>
snap::rhi::backend::opengl::hash64 computeShaderSrcHash(const std::array<std::string, Size>& str) {
    snap::rhi::backend::opengl::hash64 result = 0;

    for (const auto& src : str) {
        result = singleHashCombine(result, build_hash64(src));
    }

    return result;
}
} // namespace snap::rhi::backend::opengl
