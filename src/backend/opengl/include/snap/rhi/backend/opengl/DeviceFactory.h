#pragma once

#include <memory>

#include "snap/rhi/Device.hpp"

#include "snap/rhi/backend/opengl/DeviceCreateInfo.h"

namespace snap::rhi::backend::opengl {
std::shared_ptr<snap::rhi::Device> createDevice(const snap::rhi::backend::opengl::DeviceCreateInfo& info);
} // namespace snap::rhi::backend::opengl
