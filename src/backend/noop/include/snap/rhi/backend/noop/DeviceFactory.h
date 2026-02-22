#pragma once

#include <memory>

#include "snap/rhi/Device.hpp"

#include "snap/rhi/backend/noop/DeviceCreateInfo.h"

namespace snap::rhi::backend::noop {
std::shared_ptr<snap::rhi::Device> createDevice(const snap::rhi::backend::noop::DeviceCreateInfo& info);
} // namespace snap::rhi::backend::noop
