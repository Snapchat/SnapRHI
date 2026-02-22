#include "snap/rhi/backend/noop/Buffer.hpp"

#include "snap/rhi/Buffer.hpp"
#include "snap/rhi/Device.hpp"
#include "snap/rhi/backend/noop/Device.hpp"

namespace snap::rhi::backend::noop {
Buffer::Buffer(snap::rhi::backend::common::Device* device, const snap::rhi::BufferCreateInfo& info)
    : snap::rhi::Buffer(device, info) {}

std::byte* Buffer::map(const snap::rhi::MemoryAccess access, const uint64_t offset, const uint64_t size) {
    if (!bufferData) {
        bufferData = std::make_unique<std::byte[]>(info.size);
    }
    return bufferData.get();
}

void Buffer::unmap() {}

} // namespace snap::rhi::backend::noop
