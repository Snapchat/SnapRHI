//
//  Math.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 22.09.2020.
//  Copyright © 2021 Snapchat. All rights reserved.
//

#pragma once

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <type_traits>

namespace snap::rhi::backend::common {
constexpr float Epsilon = float(1e-6);

template<class T>
inline bool epsilonEqual(const T& val1, const T& val2, const T& eps = T(Epsilon)) {
    static_assert(std::is_floating_point_v<T>, "only for floating point types");

    return std::abs(val1 - val2) <= eps;
}

bool isPowerOfTwo(uint64_t n);
bool isPtrAligned(const void* ptr, size_t alignment);
bool isAligned(uint32_t value, size_t alignment);
uint32_t align(uint32_t value, size_t alignment);

template<typename T>
constexpr T* alignPtr(T* ptr, size_t alignment) noexcept {
    assert(isPowerOfTwo(alignment));
    assert(alignment != 0);
    return reinterpret_cast<T*>((reinterpret_cast<size_t>(ptr) + (alignment - 1)) & ~(alignment - 1));
}

template<typename T>
constexpr const T* alignPtr(const T* ptr, size_t alignment) noexcept {
    assert(isPowerOfTwo(alignment));
    assert(alignment != 0);
    return reinterpret_cast<const T*>((reinterpret_cast<size_t>(ptr) + (alignment - 1)) & ~(alignment - 1));
}

constexpr uint64_t log2(uint64_t value) noexcept {
    constexpr uint64_t log2_tab64[64] = {63, 0,  58, 1,  59, 47, 53, 2,  60, 39, 48, 27, 54, 33, 42, 3,
                                         61, 51, 37, 40, 49, 18, 28, 20, 55, 30, 34, 11, 43, 14, 22, 4,
                                         62, 57, 46, 52, 38, 26, 32, 41, 50, 36, 17, 19, 29, 10, 13, 21,
                                         56, 45, 25, 31, 35, 16, 9,  12, 44, 24, 15, 8,  23, 7,  6,  5};

    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    value |= value >> 32;
    return log2_tab64[((uint64_t)((value - (value >> 1)) * 0x07EDD5E59A4E28C2)) >> 58];
}

template<typename T>
constexpr T divCeil(const T numerator, const T denominator) {
    static_assert(std::is_integral_v<T>, "only for integral types");
    return (numerator + (denominator - static_cast<T>(1))) / denominator;
}

} // namespace snap::rhi::backend::common
