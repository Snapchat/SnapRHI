#include "snap/rhi/backend/opengl/Semaphore.hpp"
#include "snap/rhi/Device.hpp"
#include "snap/rhi/backend/common/Logging.hpp"
#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/opengl/Context.h"
#include "snap/rhi/backend/opengl/Device.hpp"
#include "snap/rhi/backend/opengl/ErrorCheckGuard.hpp"
#include "snap/rhi/backend/opengl/OpenGL.h"
#include "snap/rhi/backend/opengl/Profile.hpp"

namespace snap::rhi::backend::opengl {
Semaphore::Semaphore(Device* device, const SemaphoreCreateInfo& info)
    : snap::rhi::Semaphore(device, info), gl(device->getOpenGL()) {}

Semaphore::~Semaphore() {
    try {
        auto* glDevice = snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Device>(device);
        SNAP_RHI_REPORT(glDevice->getValidationLayer(),
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::DestroyOp,
                        "[snap::rhi::backend::opengl::~Semaphore] start destruction");
        SNAP_RHI_VALIDATE(glDevice->getValidationLayer(),
                          device->getCurrentDeviceContext(),
                          snap::rhi::ReportLevel::Warning,
                          snap::rhi::ValidationTag::DestroyOp,
                          "[snap::rhi::backend::opengl::~Semaphore] DeviceContext isn't attached to thread");
        SNAP_RHI_VALIDATE(glDevice->getValidationLayer(),
                          glDevice->isNativeContextAttached(),
                          snap::rhi::ReportLevel::CriticalError,
                          snap::rhi::ValidationTag::DestroyOp,
                          "[snap::rhi::backend::opengl::~Semaphore] GLES context isn't attached to thread");
        if (fence != nullptr) {
            gl.deleteSync(fence);
        }
        fence = nullptr;
        SNAP_RHI_REPORT(glDevice->getValidationLayer(),
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::DestroyOp,
                        "[snap::rhi::backend::opengl::~Semaphore] end of destruction");
    } catch (const snap::rhi::common::Exception& e) {
        SNAP_RHI_LOGE("[snap::rhi::backend::opengl::Semaphore::~Semaphore] Caught: %s, (possible resource leak).",
                      e.what());
    } catch (...) {
        SNAP_RHI_LOGE(
            "[snap::rhi::backend::opengl::Semaphore::~Semaphore] Caught unexpected error (possible resource leak).");
    }
}

void Semaphore::wait() {
    std::unique_lock<std::mutex> lock(signalMutex);
    auto isReady = [&fence = fence] { return fence; };
    if (!isReady()) {
        cv.wait(lock, isReady);
    }

    gl.waitSync(fence, 0, GL_TIMEOUT_IGNORED);
    gl.deleteSync(fence);
    fence = nullptr;
}

void Semaphore::signal() {
    {
        std::lock_guard<std::mutex> guard(signalMutex);
        assert(fence == nullptr);
        fence = gl.fenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

        // https://www.khronos.org/opengl/wiki/Sync_Object
        glFlush();
    }
    cv.notify_one();
}

void Semaphore::setDebugLabel(std::string_view label) {
#if SNAP_RHI_ENABLE_DEBUG_LABELS
    ObjectDebugMarkers::setDebugLabel(label);
    gl.objectPtrLabel(fence, label);
#endif
}
} // namespace snap::rhi::backend::opengl
