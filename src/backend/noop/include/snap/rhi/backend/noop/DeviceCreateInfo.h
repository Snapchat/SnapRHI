#pragma once

#include "snap/rhi/DeviceCreateInfo.h"

namespace snap::rhi::backend::noop {
struct DeviceCreateInfo : public snap::rhi::DeviceCreateInfo {
    // Currently no NoOp-specific fields are needed.
};
} // namespace snap::rhi::backend::noop
