#pragma once

#include "snap/rhi/Enums.h"
#include "snap/rhi/Limits.h"

#include <array>
#include <compare>
#include <cstdint>
#include <memory>
#include <optional>

namespace snap::rhi::metal {
/**
 * @brief Metal-specific configuration for creating a Render Pipeline State Object (PSO).
 *
 * @details
 * This structure configures how the engine maps abstract logic (Bindings, Dynamic Buffers)
 * to physical Metal hardware slots.
 */
struct PipelineInfo {
    /**
     * @brief The global binding slot index where the "Auxiliary Dynamic Offsets Buffer" is bound.
     *
     * @details
     * **The Implicit Packing Contract:**
     * The engine assumes dynamic offsets are packed linearly in this buffer, strictly following
     * the order of Set Index -> Binding Index -> Array Element defined in the Pipeline Layout.
     *
     * ### Packing Example
     * **Logical Layout (Descriptor Sets):**
     * - **Set 0**: Binding 5 (DynUBO), Binding 8 (DynUBO[2] Array)
     * - **Set 1**: Binding 2 (DynSSBO)
     *
     * **Physical Layout (Auxiliary Buffer Contents):**
     * The buffer must contain exactly 4 `uint32_t` values in this specific order:
     *
     * | Index | Value Origin                  | Description                      |
     * | :---  | :---                          | :---                             |
     * | `0`   | Set 0, Binding 5              | Offset for the first UBO         |
     * | `1`   | Set 0, Binding 8, Element `0` | Offset for first item in array   |
     * | `2`   | Set 0, Binding 8, Element `1` | Offset for second item in array  |
     * | `3`   | Set 1, Binding 2              | Offset for the SSBO              |
     *
     * When calling `bindDescriptorSet`, the engine copies the user-provided offsets into
     * the corresponding slice of this array (e.g., Set 1 writes to index 3).
     *
     * @note If `snap::rhi::Undefined`, dynamic buffers support is disabled for this pipeline.
     */
    uint32_t auxiliaryDynamicOffsetsBinding = snap::rhi::Undefined;

    friend auto operator<=>(const PipelineInfo&, const PipelineInfo&) noexcept = default;
};
} // namespace snap::rhi::metal
