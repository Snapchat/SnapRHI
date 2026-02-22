//
//  CommandQueue.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 18.10.2021.
//

#pragma once

#include "snap/rhi/CommandQueue.hpp"
#include "snap/rhi/backend/opengl/BlitCmdPerformer.hpp"
#include "snap/rhi/backend/opengl/CommandBuffer.hpp"
#include "snap/rhi/backend/opengl/ComputeCmdPerformer.hpp"
#include "snap/rhi/backend/opengl/RenderCmdPerformer.hpp"
#include <mutex>

namespace snap::rhi::backend::opengl {
class Device;
class Profile;

class CommandQueue final : public snap::rhi::CommandQueue {
public:
    CommandQueue(snap::rhi::backend::opengl::Device* device);
    ~CommandQueue() override;

    void submitCommands(std::span<snap::rhi::Semaphore*> waitSemaphores,
                        std::span<const snap::rhi::PipelineStageBits> waitDstStageMask,
                        std::span<snap::rhi::CommandBuffer*> buffers,
                        std::span<snap::rhi::Semaphore*> signalSemaphores,
                        CommandBufferWaitType waitType,
                        snap::rhi::Fence* fence) override;

    void waitUntilScheduled() override;
    void waitIdle() override;

private:
    void replayCommands(snap::rhi::backend::opengl::CommandAllocator& commandAllocator,
                        snap::rhi::backend::opengl::DeviceContext* dc);
    void resetStates();

    Device* glesDevice = nullptr;
    const Profile& gl;

    std::mutex accessMutex;

    BlitCmdPerformer blitPerformer;
    RenderCmdPerformer renderPerformer;
    ComputeCmdPerformer computePerformer;

    EncodingType activeEncodingType = EncodingType::None;
};
} // namespace snap::rhi::backend::opengl
