#pragma once

#include "snap/rhi/Device.hpp"
#include "snap/rhi/backend/vulkan/DeviceCreateInfo.h"
#include <memory>

namespace snap::rhi::backend::vulkan {
std::shared_ptr<snap::rhi::Device> createDevice(const snap::rhi::backend::vulkan::DeviceCreateInfo& info);
} // namespace snap::rhi::backend::vulkan
