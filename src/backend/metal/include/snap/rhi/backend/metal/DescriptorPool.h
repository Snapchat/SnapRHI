//
//  DescriptorPool.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 9/1/22.
//

#pragma once

#include "snap/rhi/DescriptorPool.hpp"
#include <Metal/Metal.h>

#include <atomic>
#include <functional>
#include <memory>
#include <stack>
#include <unordered_set>

namespace snap::rhi::backend::metal {
class Device;
class Profile;

class DescriptorPool final : public snap::rhi::DescriptorPool {
public:
    struct BufferSubRangeInfo {
        id<MTLBuffer> buffer = nil;
        uint32_t offset = 0;
        uint32_t size = 0;
    };

    using BufferSubRange = std::unique_ptr<BufferSubRangeInfo, std::function<void(BufferSubRangeInfo*)>>;

public:
    DescriptorPool(Device* device, const DescriptorPoolCreateInfo& info);
    ~DescriptorPool() override = default;

    BufferSubRange allocateBuffer(const uint32_t size, const uint32_t alignment);

    uint64_t getGPUMemoryUsage() const override {
        return bufferSize;
    }

private:
    const uint32_t bufferSize = 0;
    id<MTLBuffer> buffer = nil;

    /**
     * Each index represents a chunk in the buffer
     *
     * Atomic bool will be enough as allocation from user side is single threaded
     * and deallocation is thread-safe
     */
    const uint32_t numBlocks;
    std::unique_ptr<std::atomic_bool[]> used;
};
} // namespace snap::rhi::backend::metal
