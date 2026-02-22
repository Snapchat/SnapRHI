#include "snap/rhi/Fence.hpp"

namespace snap::rhi {
Fence::Fence(Device* device, const FenceCreateInfo& info)
    : snap::rhi::DeviceChild(device, snap::rhi::ResourceType::Fence) {}

void Fence::reset() {
    ++generationID;
}
} // namespace snap::rhi
