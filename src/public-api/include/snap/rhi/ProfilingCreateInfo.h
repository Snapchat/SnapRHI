// Copyright © 2023 Snap, Inc. All rights reserved.

#pragma once

#include "functional"
#include "snap/rhi/Enums.h"
#include <cstdint>
#include <string_view>

namespace snap::rhi {
/**
 * @brief Configuration for optional custom profiling hooks.
 *
 * When enabled (see `SNAP_RHI_ENABLE_CUSTOM_PROFILING_LABELS`), SnapRHI may invoke these callbacks when entering
 * and leaving internal profiling scopes.
 *
 * The provided label is intended for integration with external profilers or application-level tracing systems.
 */
struct ProfilingCreateInfo {
    /**
     * @brief Callback invoked when a profiling scope begins.
     *
     * @param label Scope label.
     *
     * @note The label is passed as a `std::string_view` and is only guaranteed to be valid for the duration of the
     * callback.
     */
    std::function<void(std::string_view)> onStartScope;

    /**
     * @brief Callback invoked when a profiling scope ends.
     *
     * @param label Scope label corresponding to the scope that is ending.
     *
     * @note The label is passed as a `std::string_view` and is only guaranteed to be valid for the duration of the
     * callback.
     */
    std::function<void(std::string_view)> onEndScope;

    /**
     * @brief Compares profiling create info objects.
     *
     * Profiling callbacks are not compared for equality; this operator treats all instances as equal to keep the
     * structure trivially comparable for configuration/caching purposes.
     */
    constexpr friend auto operator<=>(const ProfilingCreateInfo&, const ProfilingCreateInfo&) noexcept {
        return std::strong_ordering::equal;
    }
};
} // namespace snap::rhi
