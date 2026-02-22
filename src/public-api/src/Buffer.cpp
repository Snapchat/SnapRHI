#include "snap/rhi/Buffer.hpp"

namespace snap::rhi {
Buffer::Buffer(Device* device, const BufferCreateInfo& info)
    : snap::rhi::DeviceChild(device, snap::rhi::ResourceType::Buffer), info(info) {}

uint64_t Buffer::getGPUMemoryUsage() const {
    return info.size;
}
} // namespace snap::rhi
