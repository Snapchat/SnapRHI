//
// Created by Vladyslav Deviatkov on 10/31/25.
//

#pragma once

#include <cstdint>

#include "snap/rhi/DeviceChild.hpp"
#include "snap/rhi/EncoderDebugMarkers.h"
#include "snap/rhi/Structs.h"
#include <span>

namespace snap::rhi {
class Device;
class CommandBuffer;
class ComputePipeline;
class Buffer;
class Texture;
class DescriptorSet;
class QueryPool;

/**
 * @brief Base interface for all command encoders.
 *
 * A command encoder records commands of a specific type (render/blit/compute) into an owning
 * `snap::rhi::CommandBuffer`. Encoders are obtained from a command buffer (e.g.,
 * `CommandBuffer::getRenderCommandEncoder()`) and are command-buffer-owned.
 *
 * ## Call contract
 * - Begin encoding using the concrete encoder API (e.g., `beginEncoding()` on render/blit/compute encoders).
 * - Record commands.
 * - Call `endEncoding()` to close the encoding scope.
 *
 * ## Thread-safety
 * Not thread-safe. Recording must be externally synchronized.
 */
class CommandEncoder : public snap::rhi::DeviceChild, public EncoderDebugMarkers {
public:
    CommandEncoder(Device* device, CommandBuffer* commandBuffer, const snap::rhi::ResourceType resourceType);
    ~CommandEncoder() override = default;

    /**
     * @brief Records a timestamp at the current point in the command stream.
     *
     * Timestamp queries are used for GPU profiling.
     *
     * @param queryPool Pool that owns the timestamp queries.
     * @param query Query index within the pool.
     * @param location Whether this timestamp marks the start or end of a measurement region.
     */
    virtual void writeTimestamp(snap::rhi::QueryPool* queryPool,
                                uint32_t query,
                                snap::rhi::TimestampLocation location) = 0;

    /**
     * @brief Inserts a synchronization barrier into the command stream.
     *
     * A pipeline barrier establishes execution and/or memory dependencies between commands recorded before and after
     * the barrier. Barriers are used to prevent hazards such as read-after-write and to perform texture layout
     * transitions where applicable.
     *
     * @param srcStageMask Source pipeline stages to wait for.
     * @param dstStageMask Destination pipeline stages that will wait.
     * @param dependencyFlags Additional dependency modifiers (e.g., by-region dependencies on tile GPUs).
     * @param memoryBarriers Global memory barriers.
     * @param bufferMemoryBarriers Barriers for specific buffers.
     * @param textureMemoryBarriers Barriers for specific textures (may include layout transitions).
     *
     * @note Backend behavior differs:
     * - OpenGL converts this to `glMemoryBarrier` / `glMemoryBarrierByRegion` and tracks referenced resources.
     * - Metal generally manages hazards automatically; backend implementations may treat this as a no-op.
     * - Vulkan converts this to `vkCmdPipelineBarrier`.
     */
    virtual void pipelineBarrier(snap::rhi::PipelineStageBits srcStageMask,
                                 snap::rhi::PipelineStageBits dstStageMask,
                                 snap::rhi::DependencyFlags dependencyFlags,
                                 std::span<snap::rhi::MemoryBarrierInfo> memoryBarriers,
                                 std::span<snap::rhi::BufferMemoryBarrierInfo> bufferMemoryBarriers,
                                 std::span<snap::rhi::TextureMemoryBarrierInfo> textureMemoryBarriers) = 0;

    /**
     * @brief Ends the current encoding scope.
     *
     * After calling this, no further commands may be recorded with this encoder until a new encoding scope is started
     * (using the concrete encoder's `beginEncoding()` method).
     */
    virtual void endEncoding() = 0;

    /**
     * @brief Returns the command buffer this encoder records into.
     */
    CommandBuffer* getCommandBuffer() const;

    /**
     * @brief Returns CPU memory usage attributed to this encoder.
     */
    uint64_t getCPUMemoryUsage() const override {
        return 0;
    }

    /**
     * @brief Returns GPU memory usage attributed to this encoder.
     */
    uint64_t getGPUMemoryUsage() const override {
        return 0;
    }

protected:
    /** @brief Owning command buffer. */
    CommandBuffer* const commandBuffer = nullptr;
};
} // namespace snap::rhi
