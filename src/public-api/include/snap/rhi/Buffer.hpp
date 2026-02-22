//
//  Buffer.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 21.09.2020.
//  Copyright © 2021 Snapchat. All rights reserved.
//

#pragma once

#include "snap/rhi/BufferCreateInfo.h"
#include "snap/rhi/DeviceChild.hpp"
#include "snap/rhi/Memory.h"
#include <cstddef>
#include <span>

namespace snap::rhi {
class Device;

/**
 * @brief GPU buffer resource.
 *
 * A buffer is a linear memory resource used for vertex/index/uniform/storage data as well as transfer (copy/blit)
 * operations.
 *
 * Instances are created by `snap::rhi::Device::createBuffer()` and are owned by the device. The buffer’s properties are
 * described by `snap::rhi::BufferCreateInfo`.
 *
 * ## Thread-safety
 * Buffer objects are not inherently thread-safe. The application must externally synchronize:
 * - concurrent calls to `map()` / `unmap()`
 * - CPU writes/reads to the returned mapped pointer
 * - GPU usage across command buffers/queues (barriers, semaphores, fences as appropriate)
 *
 */
class Buffer : public snap::rhi::DeviceChild {
public:
    Buffer(Device* device, const BufferCreateInfo& info);
    ~Buffer() override = default;

    /**
     * @brief Returns the immutable creation parameters for this buffer.
     *
     * @return Reference to the creation info used to derive size, usage flags, and memory placement.
     */
    [[nodiscard]] const BufferCreateInfo& getCreateInfo() const {
        return info;
    }

    /**
     * @brief Maps a specific range of the buffer into host address space for CPU access.
     *
     * @details
     * This function obtains a pointer to the buffer's underlying memory, allowing the CPU to read or write data.
     * This is only valid for buffers created with the `MemoryProperties::HostVisible` property.
     *
     * **Synchronization & Persistence:**
     * - **Standard Behavior:** You must call `unmap()` before submitting any commands to the GPU that reference this
     * buffer.
     * - **Persistent Mapping:** If `Capabilities::supportsPersistentMapping` is true, you may keep the pointer mapped
     * indefinitely. However, you must still ensure correct synchronization (barriers/fences) and handle
     * cache coherency (flush/invalidate) manually if the memory is not `HostCoherent`.
     *
     * @param access Specifies the intended CPU access pattern (Read/Write).
     * Providing accurate flags allows the backend to optimize performance (e.g., using `Write`
     * without `Read` may allow the driver to invalidate the cache or orphan the previous buffer).
     * @param offset The zero-based byte offset into the buffer to start the mapping.
     * @param size   The number of bytes to map. Use `WholeSize` to map from `offset` to the end of the buffer.
     *
     * @return A valid pointer to the mapped host memory, or `nullptr` if the operation fails
     * (e.g., if the memory is not host-visible or system limits are exceeded).
     *
     * Multiple `map()` calls without an intermediate `unmap()` are not allowed and will result in an error.
     */
    virtual std::byte* map(const snap::rhi::MemoryAccess access, const uint64_t offset, const uint64_t size) = 0;

    /**
     * @brief Unmaps the buffer, invalidating the pointer previously returned by `map()`.
     *
     * @details
     * This function signals the end of a CPU access sequence.
     * - **Standard Behavior:** You must call this before submitting commands that use this buffer.
     * After this call, the pointer returned by `map()` is invalid and accessing it results in undefined behavior.
     * - **Persistent Mapping:** If persistent mapping is supported and used, you typically do not call this
     * until the buffer is destroyed or you genuinely no longer need CPU access.
     *
     * @note On some backends (e.g., WebGPU or mobile OpenGL with shadow buffers), this call may trigger
     * an expensive upload of the modified data to the GPU.
     */
    virtual void unmap() = 0;

    /**
     * @brief Flushes CPU writes to the GPU (CPU -> GPU synchronization).
     *
     * @details
     * This ensures that data written by the CPU into `HostVisible` memory is made visible to the GPU.
     * This operation is strictly required for memory types that are **not** `HostCoherent`.
     * If the memory is already `HostCoherent`, this function is a no-op.
     *
     * @param ranges A list of memory ranges to flush.
     * @note **Alignment:** The `offset` and `size` of each range must be a multiple of
     * `Capabilities::nonCoherentAtomSize` (unless `size` is `WholeSize` or reaches the end of the buffer).
     * Failure to align correctly may result in API validation errors or data corruption.
     */
    virtual void flushMappedMemoryRanges(std::span<const snap::rhi::MappedMemoryRange> ranges) = 0;

    /**
     * @brief Invalidates CPU caches to read fresh data from the GPU (GPU -> CPU synchronization).
     *
     * @details
     * This ensures that the CPU sees the most recent data written by the GPU. It effectively
     * forces the CPU to discard its cached copy of the memory and fetch fresh values from VRAM/RAM.
     * This operation is strictly required for memory types that are **not** `HostCoherent`.
     *
     * @param ranges A list of memory ranges to invalidate.
     * @note **Alignment:** The `offset` and `size` of each range must be a multiple of
     * `Capabilities::nonCoherentAtomSize`.
     *
     * @warning **Execution Sync Required:** This function only handles *memory* visibility.
     * It does **not** wait for the GPU to finish writing. You must ensure the GPU has completed
     * its work (via Fences, `queueWaitIdle`, or `deviceWaitIdle`) *before* calling this function.
     * Calling this while the GPU is still writing is a race condition.
     */
    virtual void invalidateMappedMemoryRanges(std::span<const snap::rhi::MappedMemoryRange> ranges) = 0;

    /**
     * @brief Returns CPU memory usage attributed to this buffer.
     *
     * @return Estimated bytes of CPU memory.
     *
     * @note The base `snap::rhi::Buffer` implementation returns 0 and backends may override to provide a more accurate
     * value.
     */
    [[nodiscard]] uint64_t getCPUMemoryUsage() const override {
        return 0;
    }

    /**
     * @brief Returns GPU memory usage attributed to this buffer.
     *
     * @return Estimated bytes of GPU memory.
     *
     * @note The value is backend-dependent and may be approximate.
     */
    [[nodiscard]] uint64_t getGPUMemoryUsage() const override;

protected:
    BufferCreateInfo info{};
};
} // namespace snap::rhi
