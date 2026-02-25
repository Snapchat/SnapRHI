#pragma once

#include "snap/rhi/Buffer.hpp"
#include <memory>

namespace snap::rhi::backend::common {
class Device;
} // namespace snap::rhi::backend::common

namespace snap::rhi::backend::noop {
class Buffer final : public snap::rhi::Buffer {
public:
    explicit Buffer(snap::rhi::backend::common::Device* device, const snap::rhi::BufferCreateInfo& info);
    ~Buffer() override = default;

    std::byte* map(const snap::rhi::MemoryAccess access, const uint64_t offset, const uint64_t size) override;
    void unmap() override;

    void flushMappedMemoryRanges(std::span<const snap::rhi::MappedMemoryRange> ranges) override {}

    void invalidateMappedMemoryRanges(std::span<const snap::rhi::MappedMemoryRange> ranges) override {}

private:
    std::unique_ptr<std::byte[]> bufferData;
};
} // namespace snap::rhi::backend::noop
