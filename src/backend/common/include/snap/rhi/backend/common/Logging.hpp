#pragma once

#include <cstdarg>

namespace snap::rhi::logging {
enum class Level : int { Verbose = 0, Debug = 1, Info = 2, Warn = 3, Error = 4 };

inline const char* level_to_str(Level lv) {
    switch (lv) {
        case Level::Verbose:
            return "V";
        case Level::Debug:
            return "D";
        case Level::Info:
            return "I";
        case Level::Warn:
            return "W";
        case Level::Error:
            return "E";
        default:
            return "?";
    }
}

// Optional: user-provided callback (e.g., to pipe logs elsewhere)
using LogCallback = void (*)(Level, const char* msg);
inline LogCallback& global_callback() {
    static LogCallback cb = nullptr;
    return cb;
}
inline void set_log_callback(LogCallback cb) {
    global_callback() = cb;
}

void vlog(Level level, const char* fmt, va_list ap);

inline void log(Level level, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vlog(level, fmt, ap);
    va_end(ap);
}
} // namespace snap::rhi::logging

#define SNAP_RHI_LOG_TAG "[SnapRHI]"
#define SNAP_RHI_LOGV(format, ...) \
    ::snap::rhi::logging::log(::snap::rhi::logging::Level::Verbose, SNAP_RHI_LOG_TAG " " format, ##__VA_ARGS__)
#define SNAP_RHI_LOGD(format, ...) \
    ::snap::rhi::logging::log(::snap::rhi::logging::Level::Debug, SNAP_RHI_LOG_TAG " " format, ##__VA_ARGS__)
#define SNAP_RHI_LOGI(format, ...) \
    ::snap::rhi::logging::log(::snap::rhi::logging::Level::Info, SNAP_RHI_LOG_TAG " " format, ##__VA_ARGS__)
#define SNAP_RHI_LOGW(format, ...) \
    ::snap::rhi::logging::log(::snap::rhi::logging::Level::Warn, SNAP_RHI_LOG_TAG " " format, ##__VA_ARGS__)
#define SNAP_RHI_LOGE(format, ...) \
    ::snap::rhi::logging::log(::snap::rhi::logging::Level::Error, SNAP_RHI_LOG_TAG " " format, ##__VA_ARGS__)
