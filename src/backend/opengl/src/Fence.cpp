#include "snap/rhi/backend/opengl/Fence.hpp"
#include "snap/rhi/backend/common/Logging.hpp"
#include "snap/rhi/backend/common/PlatformSyncHandle.h"
#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/opengl/Context.h"
#include "snap/rhi/backend/opengl/Device.hpp"
#include "snap/rhi/backend/opengl/OpenGL.h"
#include "snap/rhi/backend/opengl/Profile.hpp"

#if SNAP_RHI_OS_ANDROID()
#include "snap/rhi/backend/common/platform/android/SyncHandle.h"
#endif

namespace snap::rhi::backend::opengl {
Fence::Fence(Device* device, const snap::rhi::FenceCreateInfo& info)
    : snap::rhi::Fence(device, info), gl(device->getOpenGL()) {}

Fence::~Fence() {
    try {
        auto* glDevice = snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Device>(device);

        SNAP_RHI_REPORT(glDevice->getValidationLayer(),
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::DestroyOp,
                        "[snap::rhi::backend::opengl::~Fence] start destruction");
        SNAP_RHI_VALIDATE(glDevice->getValidationLayer(),
                          device->getCurrentDeviceContext(),
                          snap::rhi::ReportLevel::Warning,
                          snap::rhi::ValidationTag::DestroyOp,
                          "[snap::rhi::backend::opengl::~Fence] DeviceContext isn't attached to thread");
        SNAP_RHI_VALIDATE(glDevice->getValidationLayer(),
                          glDevice->isNativeContextAttached(),
                          snap::rhi::ReportLevel::CriticalError,
                          snap::rhi::ValidationTag::DestroyOp,
                          "[snap::rhi::backend::opengl::~Fence] OpenGL context isn't attached to thread");
        if (fence != nullptr) {
            gl.deleteSync(fence);
            fence = nullptr;
        }
        SNAP_RHI_REPORT(glDevice->getValidationLayer(),
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::DestroyOp,
                        "[snap::rhi::backend::opengl::~Fence] end of destruction");
    } catch (const snap::rhi::common::Exception& e) {
        SNAP_RHI_LOGE("[snap::rhi::backend::opengl::Fence::~Fence] Caught: %s, (possible resource leak).", e.what());
    } catch (...) {
        SNAP_RHI_LOGE("[snap::rhi::backend::opengl::Fence::~Fence] Caught unexpected error (possible resource leak).");
    }
}

void Fence::init() {
    std::lock_guard lock(accessMutex);
    fence = gl.fenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    status = snap::rhi::FenceStatus::NotReady;
}

snap::rhi::FenceStatus Fence::getStatus(uint64_t generationID) {
    std::lock_guard lock(accessMutex);
    if (generationID && this->generationID > generationID) {
        return snap::rhi::FenceStatus::Completed;
    }

    if (fence) {
        GLenum waitRes = gl.clientWaitSync(fence, 0, 0); // GL_SYNC_FLUSH_COMMANDS_BIT
        if (waitRes == GL_CONDITION_SATISFIED || waitRes == GL_ALREADY_SIGNALED) {
            status = snap::rhi::FenceStatus::Completed;
            gl.deleteSync(fence);
            fence = nullptr;
        }
    }
    return status;
}

/**
 * @brief Blocks CPU execution until the GPU has completed all commands associated with this fence.
 *
 * This function enforces a synchronization point, ensuring that all GPU commands submitted
 * prior to this fence's creation have finished execution before the CPU proceeds.
 *
 * @details The implementation differs significantly by platform to accommodate driver capabilities
 * and browser restrictions:
 *
 * - **Native:**
 * Checks the fence status with `glClientWaitSync` (timeout maxClientWaitTimeout).
 * If the fence is not already signaled (`GL_TIMEOUT_EXPIRED`), it will try `glClientWaitSync` until finished or failed.
 * This guarantees completion but performs a hard block on the calling thread.
 *
 * - **WebAssembly (WebGL):**
 * The blocking fallback (`glFinish`) is typically avoided or handled differently because
 * blocking the main thread is disallowed by browsers (causing context loss or UI freezes).
 *
 * @warning Using `glFinish()` explicitly forces a pipeline flush and CPU wait, which is
 * extremely expensive and can cause significant frame-rate stutters. Use only when
 * strict synchronization is required (e.g., resource destruction, reading back pixels).
 */
void Fence::waitForComplete() {
    std::lock_guard lock(accessMutex);
    if (fence) {
#if SNAP_RHI_PLATFORM_WEBASSEMBLY()
        const GLenum waitRes = gl.clientWaitSync(fence, 0, 0);
        if (waitRes == GL_TIMEOUT_EXPIRED) {
            gl.finish();
        }
#else
        GLenum waitRes = 0;
        do {
            const auto& features = gl.getFeatures();
            waitRes = gl.clientWaitSync(fence, 0, features.maxClientWaitTimeout); // GL_SYNC_FLUSH_COMMANDS_BIT
        } while (waitRes == GL_TIMEOUT_EXPIRED);
        assert(waitRes != GL_WAIT_FAILED);
#endif
        gl.deleteSync(fence);
        fence = nullptr;
    }
    status = snap::rhi::FenceStatus::Completed;
}

void Fence::waitForScheduled() {
    std::lock_guard lock(accessMutex);
    if (fence) {
        // In OpenGL, once the sync object is created with glFenceSync,
        // it is considered scheduled as soon as the command that created it is submitted.
        // Therefore, we can assume that the fence is scheduled immediately after creation.
        // No additional action is needed here.
    }
}

void Fence::reset() {
    std::lock_guard lock(accessMutex);
    if (fence != nullptr) {
        gl.deleteSync(fence);
    }
    fence = nullptr;
    status = snap::rhi::FenceStatus::NotReady;
    snap::rhi::Fence::reset();
}

std::unique_ptr<snap::rhi::PlatformSyncHandle> Fence::exportPlatformSyncHandle() {
    auto* glDevice = snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Device>(device);
    auto retainable = glDevice->resolveResource(this);
    auto sharedFence = std::static_pointer_cast<snap::rhi::Fence>(retainable);
    std::lock_guard lock(accessMutex);

#if SNAP_RHI_OS_ANDROID()
    if (fence == nullptr || eglDupNativeFenceFDANDROID == nullptr) {
        return std::make_unique<snap::rhi::backend::common::PlatformSyncHandle>(sharedFence);
    }

    EGLDisplay display = eglGetCurrentDisplay();
    int fenceFd = eglDupNativeFenceFDANDROID(display, reinterpret_cast<EGLSyncKHR>(fence));

    if (fenceFd < 0) {
        return std::make_unique<snap::rhi::backend::common::PlatformSyncHandle>(sharedFence);
    }

    return std::make_unique<snap::rhi::backend::common::platform::android::SyncHandle>(sharedFence, fenceFd);
#else
    return std::make_unique<snap::rhi::backend::common::PlatformSyncHandle>(sharedFence);
#endif
}

void Fence::setDebugLabel(std::string_view label) {
#if SNAP_RHI_ENABLE_DEBUG_LABELS
    ObjectDebugMarkers::setDebugLabel(label);
    gl.objectPtrLabel(fence, label);
#endif
}
} // namespace snap::rhi::backend::opengl
