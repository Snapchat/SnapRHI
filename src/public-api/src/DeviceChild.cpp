#include "snap/rhi/DeviceChild.hpp"

namespace snap::rhi {
DeviceChild::DeviceChild(Device* device, const snap::rhi::ResourceType resourceType)
    : device(device), resourceType(resourceType) {}

Device* DeviceChild::getDevice() {
    return device;
}

const Device* DeviceChild::getDevice() const {
    return device;
}
} // namespace snap::rhi
