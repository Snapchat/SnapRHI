#pragma once

#include "snap/rhi/Structs.h"
#include <functional>

namespace snap::rhi {
/**
 * @brief Debug message callback function type.
 *
 * The callback is invoked by the active backend when a debug message is produced (validation warnings/errors,
 * driver/runtime messages, etc.).
 *
 * @param info Message payload describing source/severity and textual description.
 *
 * @note Backend behavior
 * - OpenGL uses the KHR_debug/Debug Output mechanism and fans out messages to all active `DebugMessenger` instances.
 * - Some backends may invoke this callback from an internal thread or from within command submission; implementations
 *   should therefore avoid blocking for long periods.
 */
using DebugMessengerCallback = std::function<void(const snap::rhi::DebugCallbackInfo&)>;

/**
 * @brief Describes how to create a @ref snap::rhi::DebugMessenger.
 */
struct DebugMessengerCreateInfo {
    /**
     * @brief Application-provided callback to receive debug messages.
     *
     * @note Lifetime
     * The callback is stored by the created `DebugMessenger`, so any captured state must remain valid for the lifetime
     * of the messenger.
     */
    DebugMessengerCallback debugMessengerCallback;
};
} // namespace snap::rhi
