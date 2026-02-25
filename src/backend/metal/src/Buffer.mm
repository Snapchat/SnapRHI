#include "snap/rhi/backend/metal/Buffer.h"
#include "snap/rhi/backend/metal/Device.h"

namespace {
MTLResourceOptions getResourceOption(const snap::rhi::BufferCreateInfo& info) {
    const auto bits = info.memoryProperties;
    const bool hostVisible = static_cast<bool>(bits & snap::rhi::MemoryProperties::HostVisible);
    const bool hostCached = static_cast<bool>(bits & snap::rhi::MemoryProperties::HostCached);
    [[maybe_unused]] const bool hostCoherent = static_cast<bool>(bits & snap::rhi::MemoryProperties::HostCoherent);

    if (!hostVisible) {
        return MTLResourceStorageModePrivate;
    }

#if SNAP_RHI_OS_MACOS()
    // On macOS discrete GPUs, managed storage is the non-coherent model.
    if (!hostCoherent) {
        return MTLResourceStorageModeManaged;
    }
#endif

    MTLResourceOptions options = MTLResourceStorageModeShared;
    if (!hostCached) {
#if defined(__x86_64__)
        options |= MTLCPUCacheModeDefaultCache;
#else
        options |= MTLCPUCacheModeWriteCombined;
#endif
    }
    return options;
}
} // unnamed namespace

namespace snap::rhi::backend::metal {
Buffer::Buffer(Device* mtlDevice, const snap::rhi::BufferCreateInfo& info) : snap::rhi::Buffer(mtlDevice, info) {
    const MTLResourceOptions vbOpt = getResourceOption(info);
    buffer = [mtlDevice->getMtlDevice() newBufferWithLength:info.size options:vbOpt];
    if (static_cast<bool>(info.memoryProperties & snap::rhi::MemoryProperties::HostVisible)) {
        dataPtr = static_cast<std::byte*>([buffer contents]);
    }

    if (@available(macOS 13.00, ios 16.0, *)) {
        gpuAddress = buffer.gpuAddress;
    }
}

const id<MTLBuffer>& Buffer::getBuffer() const {
    return buffer;
}

std::byte* Buffer::map(const snap::rhi::MemoryAccess access, const uint64_t offset, const uint64_t size) {
    assert(dataPtr);
    return dataPtr + offset;
}

void Buffer::unmap() {}

void Buffer::setDebugLabel(std::string_view label) {
#if SNAP_RHI_ENABLE_DEBUG_LABELS
    ObjectDebugMarkers::setDebugLabel(label);
    buffer.label = [NSString stringWithUTF8String:label.data()];
#endif
}

void Buffer::flushMappedMemoryRanges(std::span<const snap::rhi::MappedMemoryRange> ranges) {
#if SNAP_RHI_OS_MACOS()
    if (buffer.storageMode != MTLStorageModeManaged) {
        return;
    }

    for (const auto& r : ranges) {
        const NSUInteger offset = r.offset;
        const NSUInteger size = (r.size == WholeSize) ? (info.size - r.offset) : r.size;
        [buffer didModifyRange:NSMakeRange(offset, size)];
    }
#endif
}

void Buffer::invalidateMappedMemoryRanges(std::span<const snap::rhi::MappedMemoryRange> ranges) {
#if SNAP_RHI_OS_MACOS()
    if (buffer.storageMode != MTLStorageModeManaged) {
        return;
    }

    auto* mtlDevice = common::smart_cast<Device>(getDevice());
    auto* commandQueue = mtlDevice->getCommandQueue(0, 0);
    auto* mtlCommandQueue = common::smart_cast<CommandQueue>(commandQueue);

    auto commandBuffer = [mtlCommandQueue->getMtlCommandQueue() commandBuffer];
    auto blitEncoder = [commandBuffer blitCommandEncoder];
    [blitEncoder synchronizeResource:buffer];
    [blitEncoder endEncoding];
    [commandBuffer commit];
    [commandBuffer waitUntilCompleted];
#endif
}
} // namespace snap::rhi::backend::metal
