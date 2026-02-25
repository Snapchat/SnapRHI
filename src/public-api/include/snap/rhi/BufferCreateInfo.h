//
//  BufferCreateInfo.h
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 21.09.2020.
//  Copyright © 2021 Snapchat. All rights reserved.
//

#pragma once

#include "snap/rhi/Enums.h"
#include "snap/rhi/common/HashCombine.h"
#include <compare>

namespace snap::rhi {

/**
 * @brief Describes how to create an `snap::rhi::Buffer`.
 *
 * This struct is consumed by `snap::rhi::Device::createBuffer()`.
 *
 * @note Invalid combinations may be reported through the device validation layer.
 */
struct BufferCreateInfo {
    /**
     * @brief Buffer size in bytes.
     *
     * Must be non-zero.
     */
    uint64_t size = 0;

    /**
     * @brief Intended GPU usages for the buffer.
     *
     * This is a bitmask of `snap::rhi::BufferUsage` flags (vertex/index/uniform/storage and/or transfer/copy usage).
     * Backends use this to select appropriate native usage bits and may validate feature support.
     */
    snap::rhi::BufferUsage bufferUsage = snap::rhi::BufferUsage::None;

    /**
     * @brief Specifies the explicit memory property flags for the buffer allocation.
     *
     * @details  Use this to explicitly request memory traits such as:
     * - `MemoryProperties::HostVisible` for CPU-writable buffers (e.g., dynamic uniforms).
     * - `MemoryProperties::HostCached` for efficient CPU readback.
     * - `MemoryProperties::DeviceLocal` for high-performance GPU-only buffers.
     */
    snap::rhi::MemoryProperties memoryProperties = snap::rhi::MemoryProperties::None;

    constexpr friend auto operator<=>(const BufferCreateInfo&, const BufferCreateInfo&) noexcept = default;
};
} // namespace snap::rhi

namespace std {
template<>
struct hash<snap::rhi::BufferCreateInfo> {
    size_t operator()(const snap::rhi::BufferCreateInfo& bufferInfo) const noexcept {
        return snap::rhi::common::hash_combine(bufferInfo.size,
                                               static_cast<uint32_t>(bufferInfo.bufferUsage),
                                               static_cast<uint32_t>(bufferInfo.memoryProperties));
    }
};
} // namespace std
