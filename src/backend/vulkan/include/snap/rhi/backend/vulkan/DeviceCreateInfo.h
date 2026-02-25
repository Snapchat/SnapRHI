#pragma once

#include "snap/rhi/DeviceCreateInfo.h"
#include "snap/rhi/backend/vulkan/vulkan.h"

namespace snap::rhi::backend::vulkan {
struct DeviceCreateInfo : public snap::rhi::DeviceCreateInfo {
    snap::rhi::backend::vulkan::APIVersion apiVersion = snap::rhi::backend::vulkan::APIVersion::Vulkan_1_0;

    std::span<const char* const> enabledInstanceExtensions;
    std::span<const char* const> enabledDeviceExtensions;
};
} // namespace snap::rhi::backend::vulkan
