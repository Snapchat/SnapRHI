//
//  ValidationLayer.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 27.04.2021.
//

#pragma once

#include "snap/rhi/Enums.h"
#include "snap/rhi/common/Inlining.h"
#include "snap/rhi/common/StringFormat.h"
#include "snap/rhi/common/zstring_view.h"

#include <string_view>

namespace snap::rhi::backend::common {

/**
 * @brief Validation and reporting layer for the graphics backend.
 *
 * Provides compile-time filtered validation with printf-style message formatting.
 * Uses zstring_view to guarantee null-terminated format strings.
 */
class ValidationLayer final {
public:
    ValidationLayer(snap::rhi::ValidationTag tags, snap::rhi::ReportLevel reportLevel);
    ~ValidationLayer() = default;

    ValidationLayer(const ValidationLayer&) = delete;
    ValidationLayer& operator=(const ValidationLayer&) = delete;
    ValidationLayer(ValidationLayer&&) = default;
    ValidationLayer& operator=(ValidationLayer&&) = default;

#define SNAP_RHI_VALIDATE(layer, condition, reportLevel, tags, ...)                        \
    if constexpr ((static_cast<uint64_t>(tags) & (snap::rhi::EnabledValidationTags)) &&    \
                  (static_cast<uint64_t>(reportLevel) >= SNAP_RHI_ENABLED_REPORT_LEVEL)) { \
        (layer).validate((condition), (reportLevel), (tags), __VA_ARGS__);                 \
    }

#define SNAP_RHI_REPORT(layer, reportLevel, tags, ...)                                     \
    if constexpr ((static_cast<uint64_t>(tags) & (snap::rhi::EnabledValidationTags)) &&    \
                  (static_cast<uint64_t>(reportLevel) >= SNAP_RHI_ENABLED_REPORT_LEVEL)) { \
        (layer).report((reportLevel), (tags), __VA_ARGS__);                                \
    }

#define SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(reportLevel, tags, customValidateFunc)          \
    if constexpr ((static_cast<uint64_t>(tags) & (snap::rhi::EnabledValidationTags)) &&    \
                  (static_cast<uint64_t>(reportLevel) >= SNAP_RHI_ENABLED_REPORT_LEVEL)) { \
        customValidateFunc();                                                              \
    }

    /**
     * @brief Validates a condition; reports a formatted message if the condition is false.
     *
     * @param condition If false, triggers the report.
     * @param msgReportLevel Severity level of the report.
     * @param msgValidationTags Tags categorizing the validation.
     * @param fmt Printf-style format string (null-terminated via zstring_view).
     * @param args Format arguments.
     */
    template<typename... Args>
    SNAP_RHI_ALWAYS_INLINE void validate(bool condition,
                                         snap::rhi::ReportLevel msgReportLevel,
                                         snap::rhi::ValidationTag msgValidationTags,
                                         snap::rhi::common::zstring_view fmt,
                                         Args&&... args) const {
        if (!condition) {
            report(msgReportLevel, msgValidationTags, fmt, std::forward<Args>(args)...);
        }
    }

    /**
     * @brief Reports a formatted validation message.
     *
     * @param msgReportLevel Severity level of the report.
     * @param msgValidationTags Tags categorizing the validation.
     * @param fmt Printf-style format string (null-terminated via zstring_view).
     * @param args Format arguments.
     */
    template<typename... Args>
    SNAP_RHI_ALWAYS_INLINE void report(snap::rhi::ReportLevel msgReportLevel,
                                       snap::rhi::ValidationTag msgValidationTags,
                                       snap::rhi::common::zstring_view fmt,
                                       Args&&... args) const {
        if (((msgValidationTags & enabledTags) != snap::rhi::ValidationTag::None) &&
            (this->reportLevel <= msgReportLevel)) {
            reportMessage(msgReportLevel, msgValidationTags, fmt, std::forward<Args>(args)...);
        }
    }

    [[nodiscard]] snap::rhi::ValidationTag getValidationTags() const noexcept {
        return enabledTags;
    }
    [[nodiscard]] snap::rhi::ReportLevel getReportLevel() const noexcept {
        return reportLevel;
    }

private:
    /**
     * @brief Formats and dispatches the message to reportImpl.
     */
    template<typename... Args>
    void reportMessage(snap::rhi::ReportLevel msgReportLevel,
                       snap::rhi::ValidationTag msgValidationTags,
                       snap::rhi::common::zstring_view fmt,
                       Args&&... args) const {
        if constexpr (sizeof...(Args) == 0) {
            // No formatting needed — zstring_view is convertible to string_view
            reportImpl(msgReportLevel, msgValidationTags, fmt);
        } else {
            const std::string message = snap::rhi::common::stringFormat(fmt, std::forward<Args>(args)...);
            reportImpl(msgReportLevel, msgValidationTags, message);
        }
    }

    void reportImpl(snap::rhi::ReportLevel msgReportLevel,
                    snap::rhi::ValidationTag msgValidationTags,
                    snap::rhi::common::zstring_view message) const;

    snap::rhi::ValidationTag enabledTags = snap::rhi::ValidationTag::None;
    snap::rhi::ReportLevel reportLevel = snap::rhi::ReportLevel::All;
};

} // namespace snap::rhi::backend::common
