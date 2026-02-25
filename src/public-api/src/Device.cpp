#include "snap/rhi/Device.hpp"
#include <iostream>
#include <thread>

namespace snap::rhi {
Device::Device(const DeviceCreateInfo& info) : createInfo(info) {}

Device::~Device() noexcept(false) {}

const std::string& Device::getPlatformDeviceString() const {
    return platformDeviceString;
}
} // namespace snap::rhi
