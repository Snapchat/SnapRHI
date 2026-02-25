#include "snap/rhi/Semaphore.hpp"

namespace snap::rhi {
Semaphore::Semaphore(Device* device, const SemaphoreCreateInfo& info)
    : snap::rhi::DeviceChild(device, snap::rhi::ResourceType::Semaphore) {}
} // namespace snap::rhi
