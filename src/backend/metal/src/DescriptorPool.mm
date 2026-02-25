#include "snap/rhi/backend/metal/DescriptorPool.h"
#include "snap/rhi/backend/metal/Device.h"
#include "snap/rhi/backend/metal/Utils.h"
#include <algorithm>
#include <numeric>
#include <ranges>

namespace {
constexpr uint32_t DefaultArgBufferAlignment = 256;
constexpr uint32_t DefaultDescriptorSize = 8;

uint32_t alignTo(uint32_t value, uint32_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

uint32_t calculateArgBufferSize(const snap::rhi::DescriptorPoolCreateInfo& info) {
    const uint32_t setsBasedSize = info.maxSets * DefaultArgBufferAlignment;
    const uint32_t descriptorsBasedSize =
        alignTo(std::accumulate(info.descriptorCount.begin(), info.descriptorCount.end(), 0u) * DefaultDescriptorSize,
                DefaultArgBufferAlignment);

    const uint32_t totalSize = std::max(setsBasedSize, descriptorsBasedSize);
    return totalSize;
}

id<MTLBuffer> createArgBuffer(const snap::rhi::backend::metal::Device* device, const uint32_t argBufferSize) {
    const auto& mtlDevice = device->getMtlDevice();
    const MTLResourceOptions options = static_cast<MTLResourceOptions>(MTLResourceStorageModeShared) |
                                       static_cast<MTLResourceOptions>(MTLCPUCacheModeWriteCombined);
    return [mtlDevice newBufferWithLength:argBufferSize options:options];
}
} // unnamed namespace

namespace snap::rhi::backend::metal {
DescriptorPool::DescriptorPool(Device* device, const DescriptorPoolCreateInfo& info)
    : snap::rhi::DescriptorPool(device, info),
      bufferSize(calculateArgBufferSize(info)),
      buffer(createArgBuffer(device, bufferSize)),
      numBlocks(bufferSize / DefaultArgBufferAlignment) {
    used = std::unique_ptr<std::atomic<bool>[]>(new std::atomic<bool>[numBlocks]);
}

DescriptorPool::BufferSubRange DescriptorPool::allocateBuffer(const uint32_t size, const uint32_t alignment) {
    const uint32_t step = std::max(1u, alignment / DefaultArgBufferAlignment);
    const uint32_t alignedSize = alignTo(size, DefaultArgBufferAlignment);
    const uint32_t requiredBlocks = alignedSize / DefaultArgBufferAlignment;

    uint32_t foundIndex = std::numeric_limits<uint32_t>::max();
    for (uint32_t i = 0; i + requiredBlocks <= numBlocks; i += step) {
        bool blockFree = true;
        for (uint32_t j = 0; j < requiredBlocks; ++j) {
            if (used[i + j].load(std::memory_order_acquire)) {
                blockFree = false;
                break;
            }
        }

        if (blockFree) {
            for (uint32_t j = 0; j < requiredBlocks; ++j) {
                used[i + j].store(true, std::memory_order_release);
            }
            foundIndex = i;
            break;
        }
    }

    if (foundIndex == std::numeric_limits<uint32_t>::max()) {
        return {};
    }

    const uint32_t offset = foundIndex * DefaultArgBufferAlignment;
    auto subRange = std::make_shared<DescriptorPool::BufferSubRangeInfo>(
        DescriptorPool::BufferSubRangeInfo{.buffer = buffer, .offset = offset, .size = alignedSize});

    auto deleter = [this, subRange](DescriptorPool::BufferSubRangeInfo* subRangeInfo) {
        /*
         * SnapRHI requires that DescriptorPool have a lifetime at least as long as any DescriptorSet allocated from it.
         * So it is safe to use @this and  not synchronize here.
         **/
        const uint32_t base = subRangeInfo->offset / DefaultArgBufferAlignment;
        const uint32_t count = subRangeInfo->size / DefaultArgBufferAlignment;
        for (uint32_t i = 0; i < count; ++i) {
            used[base + i].store(false, std::memory_order_release);
        }
    };

    return {subRange.get(), deleter};
}
} // namespace snap::rhi::backend::metal
