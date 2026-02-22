#include "snap/rhi/backend/opengl/platform/android/Semaphore.h"

#include "snap/rhi/backend/opengl/Context.h"
#include "snap/rhi/backend/opengl/Device.hpp"
#include "snap/rhi/backend/opengl/OpenGL.h"

#if SNAP_RHI_OS_ANDROID()
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <mutex>
#include <unistd.h>

namespace snap::rhi::backend::opengl::platform::android {
Semaphore::Semaphore(Device* device, const SemaphoreCreateInfo& info, int32_t fd)
    : snap::rhi::backend::opengl::Semaphore(device, info) {
    EGLDisplay eglDpy = eglGetCurrentDisplay();
    EGLint attrs[] = {EGL_SYNC_NATIVE_FENCE_FD_ANDROID, fd, EGL_NONE};
    fence = eglCreateSyncKHR(eglDpy, EGL_SYNC_NATIVE_FENCE_ANDROID, attrs);
}
} // namespace snap::rhi::backend::opengl::platform::android
#endif
