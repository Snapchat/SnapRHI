//
//  DeviceCreateInfo.h
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 21.09.2020.
//  Copyright © 2021 Snapchat. All rights reserved.
//

#pragma once

#include "snap/rhi/DeviceContextCreateInfo.h"
#include "snap/rhi/Enums.h"
#include "snap/rhi/ProfilingCreateInfo.h"
#include "snap/rhi/Structs.h"
#include "snap/rhi/common/HashCombine.h"
#include <limits>

namespace snap::rhi {
/**
 * @brief Parameters controlling `snap::rhi::Device` creation.
 *
 * The values in this struct configure debug/validation features, reporting verbosity, optional profiling hooks, and
 * selected backend behaviors.
 */
struct DeviceCreateInfo {
    /**
     * @brief Device creation flags.
     *
     * Backends use these flags to enable optional features.
     *
     * Examples:
     * - `DeviceCreateFlags::EnableDebugCallback`: enables API debug callbacks where supported (see OpenGL debug
     * handler).
     * - `DeviceCreateFlags::ReclaimAsync`: allows running resource reclamation on a background thread.
     */
    snap::rhi::DeviceCreateFlags deviceCreateFlags = snap::rhi::DeviceCreateFlags::None;

    /**
     * @brief Validation categories enabled for the device.
     *
     * This bitmask is consumed by the backend validation layer to filter which validations/reports are emitted.
     */
    snap::rhi::ValidationTag enabledTags = snap::rhi::ValidationTag::None;

    /**
     * @brief Minimum severity level of validation reports to emit.
     *
     * The backend validation layer uses this as a threshold. Reports below this level may be suppressed.
     */
    snap::rhi::ReportLevel enabledReportLevel = snap::rhi::ReportLevel::All;

#if SNAP_RHI_ENABLE_CUSTOM_PROFILING_LABELS
    /**
     * @brief Optional profiling callbacks invoked when profiling scopes begin/end.
     *
     * When `SNAP_RHI_ENABLE_CUSTOM_PROFILING_LABELS` is enabled, backends may call these callbacks.
     */
    snap::rhi::ProfilingCreateInfo profilingCreateInfo{};
#endif

    constexpr friend auto operator<=>(const DeviceCreateInfo&, const DeviceCreateInfo&) noexcept = default;
};
} // namespace snap::rhi

namespace std {
/**
 * @brief Hash specialization for `snap::rhi::DeviceCreateInfo`.
 *
 * @note This currently hashes only a subset of fields (flags + validation config). If additional fields should
 * participate in cache keys, they must be added here.
 */
template<>
struct hash<snap::rhi::DeviceCreateInfo> {
    size_t operator()(const snap::rhi::DeviceCreateInfo& deviceInfo) const noexcept {
        return snap::rhi::common::hash_combine(
            deviceInfo.deviceCreateFlags, deviceInfo.enabledTags, deviceInfo.enabledReportLevel);
    }
};
} // namespace std
