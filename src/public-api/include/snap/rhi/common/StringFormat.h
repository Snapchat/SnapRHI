#pragma once

#include <cstdarg>
#include <cstdio>
#include <snap/rhi/common/zstring_view.h>
#include <string>

// Compiler attribute to enable static analysis of format strings (GCC/Clang)
#ifdef __GNUC__
#define SNAP_RHI_PRINTF_FORMAT(fmt_idx, args_idx) __attribute__((__format__(__printf__, fmt_idx, args_idx)))
#else
#define SNAP_RHI_PRINTF_FORMAT(fmt_idx, args_idx)
#endif

namespace snap::rhi::common {

namespace detail {

/**
 * @brief Internal formatting with compile-time format string validation.
 */
SNAP_RHI_PRINTF_FORMAT(1, 2)
inline std::string stringFormatChecked(const char* fmt, ...) noexcept {
    va_list args;
    va_start(args, fmt);
    const int len = std::vsnprintf(nullptr, 0, fmt, args);
    va_end(args);

    if (len < 0) {
        return {};
    }

    std::string result(static_cast<size_t>(len), '\0');
    va_start(args, fmt);
    std::vsnprintf(result.data(), static_cast<size_t>(len) + 1, fmt, args);
    va_end(args);
    return result;
}

} // namespace detail

/**
 * @brief High-performance, noexcept std::string formatter.
 *
 * Uses zstring_view to guarantee null-terminated format string.
 * Accepts const char*, std::string, and zstring_view.
 * Enables compile-time format string validation on GCC/Clang.
 *
 * @param fmt Null-terminated format string.
 * @param args Printf-style format arguments.
 * @return Formatted string, or empty string on error.
 */
template<typename... Args>
inline std::string stringFormat(zstring_view fmt, Args&&... args) noexcept {
    return detail::stringFormatChecked(fmt.c_str(), std::forward<Args>(args)...);
}

} // namespace snap::rhi::common
