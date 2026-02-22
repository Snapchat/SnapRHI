#pragma once

#include "snap/rhi/DeviceCreateInfo.h"

namespace snap::rhi::backend::metal {
struct DeviceCreateInfo : public snap::rhi::DeviceCreateInfo {
    // Currently no Metal-specific fields are needed.
};
} // namespace snap::rhi::backend::metal
