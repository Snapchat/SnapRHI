#pragma once

#include <compare>
#include <type_traits>

// clang-format off
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define SNAP_RHI_DEFINE_ENUM_OPS(E) \
    static_assert(std::is_enum_v<E>, "E should be an enum type."); \
    static constexpr E  operator  ~ (E a)       noexcept { return (E)  (~(std::underlying_type_t<E>)(a)); } \
    static constexpr E  operator  | (E a, E b)  noexcept { return (E)  ( (std::underlying_type_t<E>)(a)   |  (std::underlying_type_t<E>)(b)); } \
    static constexpr E  operator  & (E a, E b)  noexcept { return (E)  ( (std::underlying_type_t<E>)(a)   &  (std::underlying_type_t<E>)(b)); } \
    static constexpr E  operator  ^ (E a, E b)  noexcept { return (E)  ( (std::underlying_type_t<E>)(a)   ^  (std::underlying_type_t<E>)(b)); } \
    static inline    E& operator ^= (E &a, E b) noexcept { return (E&) ( (std::underlying_type_t<E> &)(a) ^= (std::underlying_type_t<E>)(b)); } \
    static inline    E& operator |= (E &a, E b) noexcept { return (E&) ( (std::underlying_type_t<E> &)(a) |= (std::underlying_type_t<E>)(b)); } \
    static inline    E& operator &= (E &a, E b) noexcept { return (E&) ( (std::underlying_type_t<E> &)(a) &= (std::underlying_type_t<E>)(b)); } \
    static constexpr inline std::strong_ordering operator <=> (E a, E b) noexcept { return ( (std::underlying_type_t<E>)(a) <=> (std::underlying_type<E>::type)(b)); } \
    static constexpr inline bool operator == (std::underlying_type_t<E> a, E b) noexcept { return (a == (std::underlying_type<E>::type)(b)); } \
    static constexpr inline bool operator == (E a, std::underlying_type_t<E> b) noexcept { return ((std::underlying_type<E>::type)(a)) == b; }
// clang-format on
