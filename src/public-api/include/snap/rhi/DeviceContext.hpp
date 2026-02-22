//
//  DeviceContext.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 21.09.2020.
//  Copyright © 2021 Snapchat. All rights reserved.
//

#pragma once

#include "Structs.h"

#include <cstdint>

#include "snap/rhi/DeviceChild.hpp"
#include "snap/rhi/DeviceContextCreateInfo.h"
#include "snap/rhi/Guard.h"

namespace snap::rhi {
class Device;

/**
 * @brief A device-associated execution context used for backend-specific command submission/state.
 *
 * A `DeviceContext` typically represents an API context that must be made current on a thread before issuing work.
 *
 * Backend notes:
 * - OpenGL: wraps an underlying GL context; `validateCurrent()` checks that this context is current on the calling
 *   thread. `getNativeContext()` returns the backend's native context pointer.
 * - Contextless/noop devices: may provide a trivial implementation where `validateCurrent()` always succeeds and
 *   `getNativeContext()` returns `nullptr`.
 *
 * @thread_safety Device contexts are generally thread-affine. Call `makeCurrent()` / `Device::setCurrent()` on the
 * thread that will use the context.
 */
class DeviceContext : public snap::rhi::DeviceChild {
    friend class Device;

public:
    /// RAII guard used to restore the previously active context when it goes out of scope.
    using Guard = snap::rhi::Guard;

    DeviceContext(Device* device, const DeviceContextCreateInfo& info);
    ~DeviceContext() override = default;

    /**
     * @brief Returns the creation parameters used for this context.
     */
    [[nodiscard]] const DeviceContextCreateInfo& getCreateInfo() const {
        return info;
    }

    /**
     * @brief Returns whether this context is the current context on the calling thread.
     *
     * @note OpenGL checks both that the thread-local "current" context matches and that a native context is attached.
     */
    [[nodiscard]] virtual bool validateCurrent() const = 0;

    /**
     * @brief Returns a backend-specific native context handle.
     *
     * @warning The returned pointer is owned by the backend and must not be freed by the caller.
     */
    [[nodiscard]] virtual void* getNativeContext() const = 0;

    /**
     * @brief Releases context-dependent internal resources (optional).
     *
     * Backends may allocate caches and other resources that are scoped to a context (for example VAOs/FBO pools in
     * OpenGL). This call allows releasing those internal resources.
     *
     * @note The exact behavior is backend-specific. Some implementations may also notify the device to flush any
     * deferred-destruction queues that require a current context.
     */
    virtual void clearInternalResources() {}

    /**
     * @brief Returns estimated CPU memory usage for this object.
     */
    [[nodiscard]] uint64_t getCPUMemoryUsage() const override {
        return 0;
    }

    /**
     * @brief Returns estimated GPU/driver memory usage for this object.
     */
    [[nodiscard]] uint64_t getGPUMemoryUsage() const override {
        return 0;
    }

protected:
    DeviceContextCreateInfo info{};
};
} // namespace snap::rhi
