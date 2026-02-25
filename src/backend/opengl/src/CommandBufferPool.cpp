#include "snap/rhi/backend/opengl/CommandBufferPool.hpp"
#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/opengl/Device.hpp"

namespace snap::rhi::backend::opengl {
CommandBufferPool::CommandBufferPool(Device* device) : device(device) {}

snap::rhi::CommandBuffer* CommandBufferPool::acquireCommandBuffer(const snap::rhi::CommandBufferCreateInfo& info) {
    std::lock_guard<std::mutex> lock(accessMutex);
    if (freeBuffers.empty()) {
        buffers.push_back(std::make_unique<snap::rhi::backend::opengl::CommandBuffer>(device, info));
        freeBuffers.push_back(buffers.back().get());
    }

    snap::rhi::backend::opengl::CommandBuffer* bufferPtr = freeBuffers.back();
    bufferPtr->reset(info);
    freeBuffers.pop_back();
    usedBuffers.insert(bufferPtr);
    return bufferPtr;
}

void CommandBufferPool::releaseCommandBuffer(snap::rhi::CommandBuffer* commandBuffer) {
    auto* glesCommandBuffer =
        snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::CommandBuffer>(commandBuffer);

    static constexpr snap::rhi::CommandBufferCreateInfo emptyInfo{};
    glesCommandBuffer->reset(emptyInfo);

    {
        std::lock_guard<std::mutex> lock(accessMutex);
        auto it = usedBuffers.find(glesCommandBuffer);
        SNAP_RHI_VALIDATE(
            device->getValidationLayer(),
            it != usedBuffers.end(),
            snap::rhi::ReportLevel::Error,
            snap::rhi::ValidationTag::CommandBufferOp,
            "[snap::rhi::backend::opengl::CommandBufferPool::releaseCommandBuffer] attempt to destroy invalid ptr!");
        usedBuffers.erase(it);
        freeBuffers.push_back(glesCommandBuffer);
    }
}

bool CommandBufferPool::isAllCommandBufferReleased() {
    std::lock_guard<std::mutex> lock(accessMutex);
    return usedBuffers.empty();
}

void CommandBufferPool::clear() {
    std::lock_guard<std::mutex> lock(accessMutex);

    buffers.clear();
    freeBuffers.clear();
    usedBuffers.clear();
}
} // namespace snap::rhi::backend::opengl
