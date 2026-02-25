#pragma once

#include "snap/rhi/Buffer.hpp"
#include "snap/rhi/backend/vulkan/vulkan.h"

namespace snap::rhi::backend::vulkan {
class Device;

class Buffer final : public snap::rhi::Buffer {
public:
    explicit Buffer(Device* vkDevice, const snap::rhi::BufferCreateInfo& info);
    ~Buffer() override;

    void setDebugLabel(std::string_view label) override;

    std::byte* map(const snap::rhi::MemoryAccess access, const uint64_t offset, const uint64_t size) override;
    void unmap() override;

    void flushMappedMemoryRanges(std::span<const snap::rhi::MappedMemoryRange> ranges) override;
    void invalidateMappedMemoryRanges(std::span<const snap::rhi::MappedMemoryRange> ranges) override;

    VkBuffer getVkBuffer() const {
        return buffer;
    }

private:
    VkDevice vkDevice = VK_NULL_HANDLE;

    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
};
} // namespace snap::rhi::backend::vulkan
