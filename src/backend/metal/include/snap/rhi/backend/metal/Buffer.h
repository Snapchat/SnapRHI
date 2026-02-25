//
//  Buffer.h
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 1/17/20.
//  Copyright © 2020 Snapchat. All rights reserved.
//

#pragma once

#include "snap/rhi/Buffer.hpp"
#include <Metal/Metal.h>

namespace snap::rhi::backend::metal {
class Device;

class Buffer final : public snap::rhi::Buffer {
public:
    explicit Buffer(Device* mtlDevice, const snap::rhi::BufferCreateInfo& info);
    ~Buffer() override = default;

    uint64_t getGPUAddress() const {
        return gpuAddress;
    }

    const id<MTLBuffer>& getBuffer() const;

    std::byte* map(const snap::rhi::MemoryAccess access, const uint64_t offset, const uint64_t size) override;
    void unmap() override;

    void flushMappedMemoryRanges(std::span<const snap::rhi::MappedMemoryRange> ranges) override;
    void invalidateMappedMemoryRanges(std::span<const snap::rhi::MappedMemoryRange> ranges) override;

    void setDebugLabel(std::string_view label) override;

private:
    uint64_t gpuAddress = 0;
    std::byte* dataPtr = nullptr;
    id<MTLBuffer> buffer = nil;
};
} // namespace snap::rhi::backend::metal
