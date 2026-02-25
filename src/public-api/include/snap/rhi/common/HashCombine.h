#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>

namespace snap::rhi::common {
template<typename T>
constexpr void hashCombinePair(T& h1, T h2) {
    // hashCombinePair based on: https://www.open-std.org/JTC1/SC22/WG21/docs/papers/2005/n1756.pdf#page=57
    h1 ^= (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
}

template<typename... Ts>
constexpr size_t hash_combine(const Ts&... args) {
    size_t seed = 0;
    (..., hashCombinePair(seed, std::hash<Ts>{}(args)));
    return seed;
}
} // namespace snap::rhi::common
