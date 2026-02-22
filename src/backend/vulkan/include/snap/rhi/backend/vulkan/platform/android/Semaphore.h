#pragma once

#include "snap/rhi/backend/vulkan/Semaphore.h"
#include <snap/rhi/common/Platform.h>

#if SNAP_RHI_OS_ANDROID()
namespace snap::rhi::backend::vulkan::platform::android {
class Semaphore final : public snap::rhi::backend::vulkan::Semaphore {
public:
    explicit Semaphore(Device* device, const snap::rhi::SemaphoreCreateInfo& info, int32_t fd);
};
} // namespace snap::rhi::backend::vulkan::platform::android
#endif
