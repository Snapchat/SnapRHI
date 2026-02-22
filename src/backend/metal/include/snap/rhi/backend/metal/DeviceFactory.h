#pragma once

#include <memory>

#include "snap/rhi/Device.hpp"

#include "snap/rhi/backend/metal/DeviceCreateInfo.h"

namespace snap::rhi::backend::metal {
std::shared_ptr<snap::rhi::Device> createDevice(const snap::rhi::backend::metal::DeviceCreateInfo& info);
} // namespace snap::rhi::backend::metal
