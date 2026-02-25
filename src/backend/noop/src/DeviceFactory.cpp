#include "snap/rhi/backend/noop/DeviceFactory.h"
#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/noop/Device.hpp"

namespace snap::rhi::backend::noop {
std::shared_ptr<snap::rhi::Device> createDevice(const snap::rhi::backend::noop::DeviceCreateInfo& info) {
    return snap::rhi::backend::common::createDeviceSafe(std::make_shared<snap::rhi::backend::noop::Device>(info));
}
} // namespace snap::rhi::backend::noop
