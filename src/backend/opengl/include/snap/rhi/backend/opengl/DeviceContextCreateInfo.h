#pragma once

#include "snap/rhi/DeviceContextCreateInfo.h"
#include "snap/rhi/EnumOps.h"

namespace snap::rhi::backend::opengl {
struct DeviceContextCreateInfo : snap::rhi::DeviceContextCreateInfo {
    /**
     * If isUserTheOwnerOfGLContext is true user should guarantee the following condition
     * - the user is obliged to ensure that the lifespan of the platform context is not less than the lifespan of the
     *DeviceContext
     * - when creating a DeviceContext, the platform context must be bound to the thread
     * - when calling the setCurrent(context) method and before calling the setCurrent(nullptr) method,
     * the same platform context must be bound as when creating the DeviceContext
     * - when deleting a DeviceContext, the same platform context must be bound as when it was created
     *
     * Allowed value depend on GLContextInitMode value.
     **/
    bool isUserTheOwnerOfGLContext = false;

    /**
     * @brief Controls whether the engine automatically submits commands to the GPU when creating a fence.
     *
     * @details
     * **Default Behavior (false):**
     * When `snap::rhi::Fence` is created or reset, the engine automatically calls `glFlush()` immediately
     * after `glFenceSync`. This ensures that the sync object is pushed to the GPU command queue
     * instantly, preventing deadlocks when other threads wait on it.
     *
     * **Explicit Behavior (true):**
     * The engine will **NOT** call `glFlush()` during fence creation. The fence will remain in the
     * CPU-side driver buffer until you manually call `context->flush()` or `glFlush()`.
     *
     * **Use Case:**
     * Use this if you are issuing a large batch of work involving multiple fences and want to
     * perform a single `glFlush()` at the very end to minimize driver overhead (kernel context switching).
     *
     * @warning **Deadlock Risk:** If you enable this, you **MUST** ensure `glFlush()` is called
     * on this context before any other thread attempts to wait on the generated fence.
     * Failing to do so will cause the waiting thread to timeout or hang.
     */
    bool explicitFenceFlush = false;
};
} // namespace snap::rhi::backend::opengl
