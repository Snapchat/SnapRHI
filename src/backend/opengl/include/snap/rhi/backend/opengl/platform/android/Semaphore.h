#pragma once

#include "snap/rhi/backend/opengl/Semaphore.hpp"
#include <snap/rhi/common/Platform.h>

#if SNAP_RHI_OS_ANDROID()
namespace snap::rhi::backend::opengl::platform::android {
class Semaphore final : public snap::rhi::backend::opengl::Semaphore {
public:
    explicit Semaphore(Device* device, const SemaphoreCreateInfo& info, int32_t fd);
};
} // namespace snap::rhi::backend::opengl::platform::android
#endif
