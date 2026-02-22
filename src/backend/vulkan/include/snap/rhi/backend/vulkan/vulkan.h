#pragma once

#include "snap/rhi/Enums.h"
#include "snap/rhi/common/OS.h"
#include <cstdint>
#include <snap/rhi/common/Platform.h>
#include <string_view>

#if SNAP_RHI_OS_WINDOWS()
#define VK_USE_PLATFORM_WIN32_KHR 1
#endif

#if SNAP_RHI_OS_APPLE()
#define VK_USE_PLATFORM_METAL_EXT
#endif
#include <GLAD/vulkan.h>
#define SNAP_RHI_VULKAN_DYNAMIC_LOADING() (1)

namespace snap::rhi::backend::vulkan {
enum class APIVersion : snap::rhi::APIVersionType {
    None = 0,

    Vulkan_1_0 = 10,
    Vulkan_1_1 = 11,
    Vulkan_1_2 = 12,
    Vulkan_1_3 = 13,
    Vulkan_1_4 = 14,
};
} // namespace snap::rhi::backend::vulkan
