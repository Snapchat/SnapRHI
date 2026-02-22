//
// Created by Vladyslav Deviatkov on 1/20/26.
//

#include "snap/rhi/backend/common/Logging.hpp"
#include "snap/rhi/common/OS.h"
#include <algorithm>
#include <cstdio>
#include <mutex>
#include <snap/rhi/common/Platform.h>
#include <string>
#include <thread>

#if SNAP_RHI_OS_ANDROID()
#include <android/log.h>
#elif SNAP_RHI_OS_WINDOWS()
#include <windows.h>
#endif

namespace snap::rhi::logging {
void vlog(Level level, const char* fmt, va_list ap) {
#if SNAP_RHI_ENABLE_LOGS
    // First, compute required size without truncation.
    va_list ap_copy;
    va_copy(ap_copy, ap);
    int needed = vsnprintf(nullptr, 0, fmt, ap_copy);
    va_end(ap_copy);
    if (needed < 0) {
        return; // formatting error
    }
    // Allocate exact size (+1 for null terminator)
    std::string msg;
    msg.resize(static_cast<size_t>(needed) + 1u, '\0');
    vsnprintf(msg.data(), msg.size(), fmt, ap);
    // Trim potential extra null for consistent length operations later.
    if (!msg.empty() && msg.back() == '\0') {
        msg.pop_back();
    }

    // If user callback installed, deliver whole message (no chunking) and return.
    if (auto cb = global_callback()) {
        cb(level, msg.c_str());
        return;
    }

    static std::mutex m; // protects stderr / platform logging sequence

#if SNAP_RHI_OS_ANDROID()
    // Android log buffer has per-line limits (~4K). Chunk long messages to avoid truncation.
    constexpr size_t kChunkSize = 3800; // leave headroom for tag & metadata
    size_t offset = 0;
    while (offset < msg.size()) {
        size_t len = std::min(kChunkSize, msg.size() - offset);
        std::string_view slice(msg.data() + offset, len);
        int prio = ANDROID_LOG_INFO;
        switch (level) {
            case Level::Verbose:
                prio = ANDROID_LOG_VERBOSE;
                break;
            case Level::Debug:
                prio = ANDROID_LOG_DEBUG;
                break;
            case Level::Info:
                prio = ANDROID_LOG_INFO;
                break;
            case Level::Warn:
                prio = ANDROID_LOG_WARN;
                break;
            case Level::Error:
                prio = ANDROID_LOG_ERROR;
                break;
            default:
                prio = ANDROID_LOG_INFO;
                break;
        }
        // Use SnapRHI tag for consistency.
        __android_log_print(prio, "SnapRHI", "%.*s", static_cast<int>(slice.size()), slice.data());
        offset += len;
    }
    // Mirror to stderr as one combined output while holding mutex (useful for test capture).
    {
        std::lock_guard<std::mutex> lock(m);
        std::fprintf(stderr, "[%s]%s\n", level_to_str(level), msg.c_str());
        std::fflush(stderr);
    }
#elif SNAP_RHI_OS_WINDOWS()
    {
        std::lock_guard<std::mutex> lock(m);
        FILE* out = (level == Level::Error || level == Level::Warn) ? stderr : stdout;
        std::fprintf(out, "[%s] %s\n", level_to_str(level), msg.c_str());
        std::fflush(out);
    }
    {
        // OutputDebugString has length limits too; chunk if extremely large.
        constexpr size_t kChunkSize = 16000; // generous; Windows typical limit ~32K
        for (size_t off = 0; off < msg.size(); off += kChunkSize) {
            size_t len = std::min(kChunkSize, msg.size() - off);
            std::string line =
                std::string("[") + level_to_str(level) + "] " + std::string(msg.data() + off, len) + "\r\n";
            OutputDebugStringA(line.c_str());
        }
    }
#else // POSIX
    {
        std::lock_guard<std::mutex> lock(m);
        FILE* out = (level == Level::Error || level == Level::Warn) ? stderr : stdout;
        std::fprintf(out, "[%s]%s\n", level_to_str(level), msg.c_str());
        std::fflush(out);
    }
#endif
#endif
}
} // namespace snap::rhi::logging
