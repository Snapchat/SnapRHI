//
//  CommandBufferPool.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 05.10.2021.
//

#pragma once

#include "snap/rhi/backend/opengl/CommandBuffer.hpp"

#include <memory>
#include <queue>
#include <unordered_set>
#include <vector>

namespace snap::rhi::backend::opengl {
class Device;
class CommandQueue;

class CommandBufferPool final {
public:
    CommandBufferPool(Device* device);
    ~CommandBufferPool() = default;

    snap::rhi::CommandBuffer* acquireCommandBuffer(const snap::rhi::CommandBufferCreateInfo& info);
    void releaseCommandBuffer(snap::rhi::CommandBuffer* commandBuffer);

    bool isAllCommandBufferReleased();
    void clear();

private:
    Device* device = nullptr;

    std::vector<std::unique_ptr<snap::rhi::backend::opengl::CommandBuffer>> buffers;

    std::vector<snap::rhi::backend::opengl::CommandBuffer*> freeBuffers;
    std::unordered_set<snap::rhi::backend::opengl::CommandBuffer*> usedBuffers;
    std::mutex accessMutex;
};
} // namespace snap::rhi::backend::opengl
