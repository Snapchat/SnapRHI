#pragma once

#include "DebugMessengerCreateInfo.h"
#include "snap/rhi/DeviceChild.hpp"

namespace snap::rhi {
class Device;

/**
 * @brief Debug callback registration object.
 *
 * A `DebugMessenger` owns a user-supplied callback (see `DebugMessengerCreateInfo`) and registers it with the active
 * backend so that runtime debug messages can be forwarded to the application.
 *
 * Instances are created via `Device::createDebugMessenger()`.
 *
 * @note Backend behavior
 * - OpenGL uses the KHR_debug/Debug Output mechanism. Since OpenGL allows only a single global debug callback, the
 *   backend aggregates multiple `DebugMessenger` instances and fans out messages to them.
 * - If the device was not created with `DeviceCreateFlags::EnableDebugCallback`, backends may create the object but not
 *   register any callback.
 */
class DebugMessenger : public snap::rhi::DeviceChild {
public:
    /**
     * @brief Constructs a debug messenger.
     *
     * Applications typically do not call this directly; use `Device::createDebugMessenger()`.
     *
     * @param device Owning device.
     * @param info Creation info containing the callback to invoke.
     */
    DebugMessenger(Device* device, snap::rhi::DebugMessengerCreateInfo&& info);

    /**
     * @brief Destroys the debug messenger and unregisters the callback (backend-dependent).
     */
    virtual ~DebugMessenger() = default;

    /**
     * @brief Returns the creation info used to create this debug messenger.
     *
     * This includes the callback functor that will receive debug messages.
     */
    const DebugMessengerCreateInfo& getCreateInfo() const;

    /**
     * @brief Returns CPU memory usage attributed to this object.
     *
     * The base implementation returns 0; backends may override if they track allocations.
     */
    uint64_t getCPUMemoryUsage() const override {
        return 0;
    }

    /**
     * @brief Returns GPU memory usage attributed to this object.
     *
     * Debug messengers typically do not allocate GPU resources; the base implementation returns 0.
     */
    uint64_t getGPUMemoryUsage() const override {
        return 0;
    }

protected:
    /** @brief Stored creation info (including the callback). */
    DebugMessengerCreateInfo info;
};
} // namespace snap::rhi
