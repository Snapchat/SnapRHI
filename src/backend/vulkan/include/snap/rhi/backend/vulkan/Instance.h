#pragma once

#include "snap/rhi/Device.hpp"
#include "snap/rhi/backend/vulkan/DeviceCreateInfo.h"

namespace snap::rhi::backend::vulkan {
std::unique_ptr<snap::rhi::Device> createVulkanDevice(const snap::rhi::backend::vulkan::DeviceCreateInfo& info);
} // namespace snap::rhi::backend::vulkan
