#pragma once

#include <array>
#include <cassert>
#include <cstdint>
#include <string_view>
#include <utility>

namespace snap::rhi::backend::common {

using uuid_type = std::array<uint8_t, 16>;

static inline constexpr std::pair<bool, uint8_t> uuidCharToByte(char input) noexcept {
    if (input >= '0' && input <= '9') {
        return {true, input - '0'};
    }
    if (input >= 'A' && input <= 'F') {
        return {true, input - 'A' + 10};
    }
    if (input >= 'a' && input <= 'f') {
        return {true, input - 'a' + 10};
    }
    return {false, 0};
}

static inline constexpr uuid_type uuidStringToArr(std::string_view sv) noexcept {
    assert(sv.size() >= 32);
    uint8_t j = 0;
    uuid_type arr = {0};
    for (size_t i = 0; i < sv.size(); ++i) {
        const char c0 = sv[i];
        auto [hasValue0, value0] = uuidCharToByte(c0);
        if (hasValue0) {
            const char c1 = sv[++i];
            auto [hasValue1, value1] = uuidCharToByte(c1);
            assert(hasValue1);
            arr[j++] = (value0 << 4) | value1;
        }
    }
    assert(j == arr.size());
    return arr;
}

static inline constexpr bool uuidEqual(uuid_type lhs, uuid_type rhs) noexcept {
    bool same = true;
    for (size_t i = 0; i < lhs.size(); ++i) {
        same &= (lhs[i] == rhs[i]);
    }
    return same;
}

static_assert(uuidEqual(
    uuidStringToArr("A541BD2A-B7AF-4501-BC9F-62612FCBC80E"),
    uuid_type{0xA5, 0x41, 0xBD, 0x2A, 0xB7, 0xAF, 0x45, 0x01, 0xBC, 0x9F, 0x62, 0x61, 0x2F, 0xCB, 0xC8, 0x0E}));

} // namespace snap::rhi::backend::common
