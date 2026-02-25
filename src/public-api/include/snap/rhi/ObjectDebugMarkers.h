#pragma once

#include "snap/rhi/common/NonCopyable.h"
#include <snap/rhi/common/zstring_view.h>
#include <string>
#include <string_view>

namespace snap::rhi {
/**
 * @brief Optional debug label support for API objects.
 *
 * `ObjectDebugMarkers` provides a lightweight, API-agnostic interface for attaching a human-readable label to an
 * object (buffer/texture/pipeline/encoder/etc.) for debugging and profiling.
 *
 * Behavior is controlled at build time by `SNAP_RHI_ENABLE_DEBUG_LABELS`:
 * - When enabled, this base class stores the label string and backends may also forward it to the native API
 *   (for example, Metal assigns `label` to `MTLResource::label`).
 * - When disabled, labels are not stored and the methods become cheap no-ops (empty string on get, ignore set).
 */
class ObjectDebugMarkers : public snap::rhi::common::NonCopyable {
public:
    virtual ~ObjectDebugMarkers() = default;

    /**
     * @brief Returns the current debug label.
     *
     * @return The label previously set via @ref setDebugLabel when `SNAP_RHI_ENABLE_DEBUG_LABELS` is enabled; otherwise
     * an empty string.
     */
    [[nodiscard]] virtual std::string_view getDebugLabel() const {
#if SNAP_RHI_ENABLE_DEBUG_LABELS
        return label;
#else
        return "";
#endif
    }

    /**
     * @brief Sets a human-readable debug label for this object.
     *
     * Backends may use this label to annotate native objects and improve GPU capture readability.
     *
     * @param debugLabel UTF-8 label text.
     *
     * @note If `SNAP_RHI_ENABLE_DEBUG_LABELS` is disabled, this function does nothing.
     */
    virtual void setDebugLabel(std::string_view debugLabel) {
#if SNAP_RHI_ENABLE_DEBUG_LABELS
        label = debugLabel;
#endif
    }

private:
#if SNAP_RHI_ENABLE_DEBUG_LABELS
    std::string label = "";
#endif
};
} // namespace snap::rhi
