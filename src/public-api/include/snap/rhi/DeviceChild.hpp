#pragma once

#include "snap/rhi/Enums.h"
#include "snap/rhi/ObjectDebugMarkers.h"

namespace snap::rhi {
class Device;

/**
 * @brief Base class for objects created by (and logically owned by) an `snap::rhi::Device`.
 *
 * `DeviceChild` is the common base for most SnapRHI resources (buffers, textures, pipelines, etc.). It provides:
 * - access to the creating device;
 * - a resource type tag for diagnostics;
 * - optional capture of a creation stacktrace;
 * - a uniform interface for reporting CPU and GPU/driver memory usage.
 */
class DeviceChild : public ObjectDebugMarkers {
public:
    DeviceChild(Device* device, snap::rhi::ResourceType resourceType);
    ~DeviceChild() override = default;

    DeviceChild(DeviceChild&&) noexcept = delete;
    DeviceChild& operator=(DeviceChild&&) noexcept = delete;

    /**
     * @brief Returns the device that created this object.
     */
    [[nodiscard]] Device* getDevice();

    /**
     * @brief Returns the device that created this object (const overload).
     */
    [[nodiscard]] const Device* getDevice() const;

    /**
     * @brief Returns estimated CPU memory usage for this object.
     *
     * The exact meaning is resource-specific. In general, this should estimate CPU-side bookkeeping and exclude
     * backend driver allocations.
     */
    [[nodiscard]] virtual uint64_t getCPUMemoryUsage() const = 0;

    /**
     * @brief Returns estimated GPU/driver memory usage for this object.
     *
     * The exact meaning is resource- and backend-specific. Some resources/backends may return 0 when this information
     * is not tracked.
     */
    [[nodiscard]] virtual uint64_t getGPUMemoryUsage() const = 0;

    /**
     * @brief Returns the resource type tag for this object.
     *
     * This tag is used for diagnostics, validation reporting, and debug tooling.
     */
    [[nodiscard]] snap::rhi::ResourceType getResourceType() const {
        return resourceType;
    }

protected:
    Device* device = nullptr;

    snap::rhi::ResourceType resourceType = snap::rhi::ResourceType::Undefined;
};
} // namespace snap::rhi
