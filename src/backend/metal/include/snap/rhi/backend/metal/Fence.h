//
//  Fence.h
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 1/17/20.
//  Copyright © 2020 Snapchat. All rights reserved.
//

#pragma once

#include "snap/rhi/Fence.hpp"
#include <Metal/Metal.h>

namespace snap::rhi::backend::metal {
class Device;

class Fence final : public snap::rhi::Fence {
public:
    Fence(Device* mtlDevice, const snap::rhi::FenceCreateInfo& info);
    ~Fence() override = default;

    std::unique_ptr<snap::rhi::PlatformSyncHandle> exportPlatformSyncHandle() override;

    snap::rhi::FenceStatus getStatus(uint64_t generationID) override;
    void waitForComplete() override;
    void waitForScheduled() override;
    void reset() override;

    void setWaitCommandBuffer(const id<MTLCommandBuffer>& commandBuffer);

private:
    id<MTLCommandBuffer> commandBuffer = nil;
    snap::rhi::FenceStatus result = snap::rhi::FenceStatus::NotReady;
};
} // namespace snap::rhi::backend::metal
