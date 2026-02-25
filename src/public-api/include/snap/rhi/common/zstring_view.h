#pragma once

#include <snap/rhi/common/deduce_noexcept.hpp>
#include <string_view>

namespace snap::rhi::common {
namespace Detail {
template<class T, class = void>
struct CanConvertToZstring : std::false_type {};

template<class T>
struct CanConvertToZstring<T, std::void_t<decltype(std::declval<T>().c_str(), std::declval<T>().size())>>
    : std::true_type {};
} // namespace Detail

/// basic_zstring_view is an analogue of std::basic_string_view with an additional guarantee of being zero-terminated,
/// which is especially important for interaction with C iterfaces.
///
/// Unlike std::basic_string_view, constructor form (size, pointer) and remove_suffix aren't supported.
/// c_str method is added to return C-style string.
template<class CharT, class Traits = std::char_traits<CharT>>
class basic_zstring_view : public std::basic_string_view<CharT, Traits> {
private:
    using base_type = std::basic_string_view<CharT, Traits>;

    using base_type::remove_suffix; // Hidden because it doesn't result in zero-terminated string.

public:
    constexpr basic_zstring_view() noexcept = default;

    constexpr basic_zstring_view(const CharT* s) : base_type{s} {}

    template<class T, class = std::enable_if_t<Detail::CanConvertToZstring<const T>::value>>
    constexpr basic_zstring_view(const T& s) SNAP_RHI_DEDUCE_NOEXCEPT_AND_INITIALIZE(base_type{s.c_str(), s.size()}) {}

    constexpr void swap(basic_zstring_view& other) noexcept {
        base_type::swap(other);
    }

    constexpr const CharT* c_str() const noexcept {
        return base_type::data();
    }
};

using zstring_view = basic_zstring_view<char>;
using wzstring_view = basic_zstring_view<wchar_t>;
using u16zstring_view = basic_zstring_view<char16_t>;
using u32zstring_view = basic_zstring_view<char32_t>;
} // namespace snap::rhi::common

namespace std {
template<class CharT, class Traits>
struct hash<snap::rhi::common::basic_zstring_view<CharT, Traits>> : hash<basic_string_view<CharT, Traits>> {};
} // namespace std
