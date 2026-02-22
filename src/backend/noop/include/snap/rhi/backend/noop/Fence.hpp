#pragma once

#include "snap/rhi/Fence.hpp"

namespace snap::rhi::backend::common {
class Device;
} // namespace snap::rhi::backend::common

namespace snap::rhi::backend::noop {
class Fence final : public snap::rhi::Fence {
public:
    Fence(snap::rhi::backend::common::Device* device, const snap::rhi::FenceCreateInfo& info);
    ~Fence() override = default;

    std::unique_ptr<snap::rhi::PlatformSyncHandle> exportPlatformSyncHandle() override;

    snap::rhi::FenceStatus getStatus(uint64_t generationID) override;
    void waitForComplete() override;
    void waitForScheduled() override {}
    void reset() override;
};
} // namespace snap::rhi::backend::noop
