//
//  Semaphore.h
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 1/17/20.
//  Copyright © 2020 Snapchat. All rights reserved.
//

#pragma once

#include "snap/rhi/Semaphore.hpp"

#include <Foundation/Foundation.h>
#include <Metal/Metal.h>
#include <cstdint>
#include <mutex>

namespace snap::rhi::backend::metal {
class Device;

class Semaphore final : public snap::rhi::Semaphore {
public:
    Semaphore(Device* mtlDevice, const snap::rhi::SemaphoreCreateInfo& info);
    ~Semaphore() override = default;

    void setDebugLabel(std::string_view label) override;

    void wait(const id<MTLCommandBuffer>& commandBuffer);
    void signal(const id<MTLCommandBuffer>& commandBuffer);

private:
    id<MTLSharedEvent> event = nil;

    std::mutex stateMutex;
    uint64_t nextSignalValue = 1;
    uint64_t pendingSignalValue = 0;
    bool hasPendingSignal = false;
};
} // namespace snap::rhi::backend::metal
