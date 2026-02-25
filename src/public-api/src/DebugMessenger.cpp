#include "snap/rhi/DebugMessenger.h"

namespace snap::rhi {
DebugMessenger::DebugMessenger(Device* device, snap::rhi::DebugMessengerCreateInfo&& info)
    : snap::rhi::DeviceChild(device, snap::rhi::ResourceType::DebugMessenger), info(std::move(info)) {}

const DebugMessengerCreateInfo& DebugMessenger::getCreateInfo() const {
    return info;
}
} // namespace snap::rhi
