#pragma once

#include "snap/rhi/common/NonCopyable.h"
#include <snap/rhi/common/zstring_view.h>
#include <string>
#include <string_view>

namespace snap::rhi {
/**
 * @brief Interface for inserting hierarchical debug markers into command encoders.
 *
 * Debug groups are used by GPU debuggers and profilers to present a nested call hierarchy (for example RenderDoc,
 * Xcode GPU tools, vendor-specific profilers).
 *
 * Backend notes:
 * - OpenGL: records marker commands into the command stream when `SNAP_RHI_ENABLE_DEBUG_LABELS` is enabled.
 * - Metal: forwards to `-[MTLCommandEncoder pushDebugGroup:]` / `popDebugGroup` when enabled.
 * - Vulkan: currently implemented as a no-op (placeholders for `vkCmdBeginDebugUtilsLabelEXT`).
 * - noop: no-op.
 */
class EncoderDebugMarkers : public snap::rhi::common::NonCopyable {
public:
    virtual ~EncoderDebugMarkers() = default;

    /**
     * @brief Begins a new nested debug group.
     *
     * @param label Human-readable label for the group.
     *
     * @note Calls to `beginDebugGroup()` must be paired with a matching `endDebugGroup()`.
     *
     * @note If debug labels are disabled at build time (`SNAP_RHI_ENABLE_DEBUG_LABELS == 0`), implementations typically
     * compile to a no-op.
     */
    virtual void beginDebugGroup(std::string_view label) = 0;

    /**
     * @brief Ends the most recently started debug group.
     *
     * @note This must be called once for every `beginDebugGroup()` to keep the debug group stack balanced.
     */
    virtual void endDebugGroup() = 0;

private:
};
} // namespace snap::rhi
