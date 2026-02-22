#pragma once

namespace snap::rhi::common {
class NonCopyable {
public:
    constexpr NonCopyable() noexcept = default;

    constexpr NonCopyable(NonCopyable&&) noexcept = default;
    constexpr NonCopyable& operator=(NonCopyable&&) noexcept = default;

    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
};
} // namespace snap::rhi::common
