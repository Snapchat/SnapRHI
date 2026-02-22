#include "snap/rhi/backend/metal/DeviceFactory.h"
#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/metal/Device.h"

namespace snap::rhi::backend::metal {
std::shared_ptr<snap::rhi::Device> createDevice(const snap::rhi::backend::metal::DeviceCreateInfo& info) {
    return snap::rhi::backend::common::createDeviceSafe(std::make_shared<snap::rhi::backend::metal::Device>(info));
}
} // namespace snap::rhi::backend::metal
