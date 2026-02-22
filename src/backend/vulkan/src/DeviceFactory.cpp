#include "snap/rhi/backend/vulkan/DeviceFactory.h"
#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/vulkan/Device.h"

namespace snap::rhi::backend::vulkan {
std::shared_ptr<snap::rhi::Device> createDevice(const snap::rhi::backend::vulkan::DeviceCreateInfo& info) {
    return snap::rhi::backend::common::createDeviceSafe(std::make_shared<snap::rhi::backend::vulkan::Device>(info));
}
} // namespace snap::rhi::backend::vulkan
