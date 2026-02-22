#pragma once

#include "snap/rhi/Enums.h"
#include "snap/rhi/Limits.h"

#include <array>
#include <cstdint>

namespace snap::rhi {
/**
 * @brief Defines a specific range of memory within a mapped resource.
 *
 * @details
 * This structure is used to specify regions of memory for explicit cache management operations.
 * When working with non-coherent memory (i.e., memory without the `MemoryProperties::HostCoherent` flag),
 * the host must manually flush writes or invalidate reads to ensure data visibility between the CPU and GPU.
 *
 * @see snap::rhi::Buffer::flushMappedMemoryRanges
 * @see snap::rhi::Buffer::invalidateMappedMemoryRanges
 */
struct MappedMemoryRange {
    /**
     * @brief The offset in bytes from the start of the memory object.
     * @note For non-coherent memory operations, this must be a multiple of `nonCoherentAtomSize`.
     */
    uint64_t offset = 0;

    /**
     * @brief The size in bytes of the memory range.
     * @note For non-coherent memory operations, this must be a multiple of `nonCoherentAtomSize`,
     * or equal to `WholeSize` (or the remaining size from the offset).
     */
    uint64_t size = 0;

    constexpr friend auto operator<=>(const MappedMemoryRange&, const MappedMemoryRange&) noexcept = default;
};

/**
 * @brief Describes the properties of a specific memory type supported by the physical device.
 *
 * @details
 * A memory type is a specific classification of memory offered by the hardware, characterized
 * by a set of property flags (e.g., DeviceLocal, HostVisible).
 */
struct MemoryType {
    /**
     * @brief The bitmask of properties for this memory type.
     */
    snap::rhi::MemoryProperties memoryProperties = snap::rhi::MemoryProperties::None;

    constexpr friend auto operator<=>(const MemoryType&, const MemoryType&) noexcept = default;
};

/**
 * @brief Container for the memory properties of a physical device.
 *
 * @details
 * This structure aggregates all available memory types supported by the hardware.
 * It is typically populated during device initialization or capability querying.
 * Applications can iterate through `memoryTypes` to find the index of a memory type
 * that satisfies their specific requirements (e.g., finding a HostVisible | HostCoherent type).
 *
 * @see snap::rhi::Capabilities::physicalDeviceMemoryProperties
 */
struct PhysicalDeviceMemoryProperties {
    /**
     * @brief An array of available memory types.
     * @note The number of valid entries is determined by the implementation specific count,
     * but will not exceed `MaxMemoryTypes`.
     */
    std::array<MemoryType, MaxMemoryTypes> memoryTypes{};
    uint32_t memoryTypeCount = 0;

    constexpr friend auto operator<=>(const PhysicalDeviceMemoryProperties&,
                                      const PhysicalDeviceMemoryProperties&) noexcept = default;
};
} // namespace snap::rhi
