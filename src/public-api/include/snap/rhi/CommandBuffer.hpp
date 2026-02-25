//
//  CommandBuffer.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 21.09.2020.
//  Copyright © 2021 Snapchat. All rights reserved.
//

#pragma once

#include "snap/rhi/CommandBufferCreateInfo.h"
#include "snap/rhi/DeviceChild.hpp"

namespace snap::rhi {
class Device;
class CommandQueue;
class Framebuffer;
class BlitCommandEncoder;
class RenderCommandEncoder;
class ComputeCommandEncoder;
class QueryPool;

/**
 * @brief GPU command buffer used to record and later submit command encoders.
 *
 * A command buffer provides access to encoder objects (`RenderCommandEncoder`, `BlitCommandEncoder`,
 * `ComputeCommandEncoder`) that record commands into the buffer. Command buffers are created via
 * `snap::rhi::Device::createCommandBuffer()`.
 *
 * ## Thread-safety
 * Not thread-safe. Recording must be externally synchronized and typically happens from a single thread.
 *
 * ## Encoder lifetime
 * The encoder pointers returned by `get*CommandEncoder()` are owned by the command buffer and remain valid for the
 * lifetime of the command buffer. Backends typically reuse a single encoder instance per command buffer.
 *
 * @note The exact submission/recording lifecycle is backend-specific and is managed by the queue and encoder
 * implementations. Use `getStatus()` for a coarse state.
 */
class CommandBuffer : public snap::rhi::DeviceChild {
public:
    CommandBuffer(Device* device, const CommandBufferCreateInfo& info);

    /**
     * @warning The application must ensure the command buffer is no longer in-flight when it is destroyed.
     */
    ~CommandBuffer() override = default;

    /**
     * @brief Resets a range of queries in a query pool.
     *
     * This must be called before the queries are used in a new frame/submission.
     *
     * @param queryPool Pool containing the queries to reset.
     * @param firstQuery Index of the first query to reset.
     * @param queryCount Number of queries to reset.
     */
    virtual void resetQueryPool(snap::rhi::QueryPool* queryPool, uint32_t firstQuery, uint32_t queryCount) = 0;

    /**
     * @brief Returns the render command encoder associated with this command buffer.
     *
     * @note The returned pointer remains valid until the command buffer is destroyed.
     */
    virtual RenderCommandEncoder* getRenderCommandEncoder() = 0;

    /**
     * @brief Returns the blit (transfer) command encoder associated with this command buffer.
     *
     * @note The returned pointer remains valid until the command buffer is destroyed.
     */
    virtual BlitCommandEncoder* getBlitCommandEncoder() = 0;

    /**
     * @brief Returns the compute command encoder associated with this command buffer.
     *
     * @note The returned pointer remains valid until the command buffer is destroyed.
     */
    virtual ComputeCommandEncoder* getComputeCommandEncoder() = 0;

    /**
     * @brief Returns the command queue this command buffer is associated with.
     */
    CommandQueue* getCommandQueue();

    /**
     * @brief Returns the current status of this command buffer.
     *
     * @note Status transitions are backend-specific but generally follow:
     * `Initial -> Recording -> Submitted`.
     */
    [[nodiscard]] virtual CommandBufferStatus getStatus() const = 0;

    /**
     * @brief Returns estimated CPU memory usage attributed to this command buffer.
     *
     * @note The base `snap::rhi::CommandBuffer` implementation returns 0. Backends may override to include CPU-side
     * command recording buffers and bookkeeping.
     */
    [[nodiscard]] uint64_t getCPUMemoryUsage() const override {
        return 0;
    }

    /**
     * @brief Returns estimated GPU/driver memory usage attributed to this command buffer.
     *
     * @note The base `snap::rhi::CommandBuffer` implementation returns 0. Backends may override if they can attribute
     * transient GPU allocations (e.g., indirect buffers) to a command buffer.
     */
    [[nodiscard]] uint64_t getGPUMemoryUsage() const override {
        return 0;
    }

    /**
     * @brief Returns the immutable creation info used to create this command buffer.
     */
    [[nodiscard]] const CommandBufferCreateInfo& getCreateInfo() const {
        return info;
    }

protected:
    CommandBufferCreateInfo info{};

private:
};
} // namespace snap::rhi
