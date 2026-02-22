#include "snap/rhi/backend/noop/Fence.hpp"

#include "snap/rhi/backend/noop/Device.hpp"

namespace snap::rhi::backend::noop {
Fence::Fence(snap::rhi::backend::common::Device* device, const snap::rhi::FenceCreateInfo& info)
    : snap::rhi::Fence(device, info) {}

snap::rhi::FenceStatus Fence::getStatus(uint64_t generationID) {
    return snap::rhi::FenceStatus::NotReady;
}

void Fence::waitForComplete() {}

void Fence::reset() {}

std::unique_ptr<snap::rhi::PlatformSyncHandle> Fence::exportPlatformSyncHandle() {
    return nullptr;
}
} // namespace snap::rhi::backend::noop
