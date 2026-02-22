//
//  CommandQueue.h
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 07.10.2020.
//  Copyright © 2021 Snapchat. All rights reserved.
//

#pragma once

#include "snap/rhi/CommandQueue.hpp"

#include <Metal/Metal.h>
#include <mutex>

namespace snap::rhi::backend::metal {
class Device;
class CommandBuffer;

/**
 * MTLCommandQueue only has submitCommands as thread safe operation, creating command buffer is unsafe operation.
 * https://developer.apple.com/documentation/metal/mtlcommandqueue
 *
 * The command queues you create with this method allow up to 64 uncompleted command buffers at time.
 * https://developer.apple.com/documentation/metal/mtldevice/makecommandqueue()?language=objc
 */
class CommandQueue final : public snap::rhi::CommandQueue {
public:
    CommandQueue(Device* mtlDevice, const id<MTLCommandQueue>& commandQueue);
    ~CommandQueue() override;

    void setDebugLabel(std::string_view label) override;

    void submitCommands(std::span<snap::rhi::Semaphore*> waitSemaphores,
                        std::span<const snap::rhi::PipelineStageBits> waitDstStageMask,
                        std::span<snap::rhi::CommandBuffer*> buffers,
                        std::span<snap::rhi::Semaphore*> signalSemaphores,
                        snap::rhi::CommandBufferWaitType waitType,
                        snap::rhi::Fence* fence) override;
    void waitUntilScheduled() override;
    void waitIdle() override;

    const id<MTLCommandQueue>& getMtlCommandQueue() const;

private:
    void signalQueue(std::span<snap::rhi::Semaphore*> semaphores, CommandBuffer* commandBuffer);
    void waitQueue(std::span<snap::rhi::Semaphore*> semaphores);
    void wait(const snap::rhi::CommandBufferWaitType waitType);

    id<MTLCommandQueue> commandQueue = nil;

    std::mutex accessLock;
    id<MTLCommandBuffer> lastSubmitCommandBuffer = nil;
};
} // namespace snap::rhi::backend::metal
