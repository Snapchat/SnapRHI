#include "snap/rhi/backend/vulkan/Buffer.h"
#include "snap/rhi/backend/vulkan/Device.h"
#include "snap/rhi/backend/vulkan/VkDebugUtils.h"

namespace {
VkBufferUsageFlags getVkBufferUsage(const snap::rhi::BufferUsage bufferUsage) {
    VkBufferUsageFlags result = 0;

    if (static_cast<bool>(bufferUsage & snap::rhi::BufferUsage::VertexBuffer)) {
        result |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    }

    if (static_cast<bool>(bufferUsage & snap::rhi::BufferUsage::IndexBuffer)) {
        result |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    }

    if (static_cast<bool>(bufferUsage & snap::rhi::BufferUsage::UniformBuffer)) {
        result |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    }

    if (static_cast<bool>(bufferUsage & snap::rhi::BufferUsage::StorageBuffer)) {
        result |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    }

    if (static_cast<bool>(bufferUsage & snap::rhi::BufferUsage::CopySrc)) {
        result |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    }

    if (static_cast<bool>(bufferUsage & snap::rhi::BufferUsage::CopyDst)) {
        result |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }

    if (static_cast<bool>(bufferUsage & snap::rhi::BufferUsage::TransferSrc)) {
        result |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    }

    if (static_cast<bool>(bufferUsage & snap::rhi::BufferUsage::TransferDst)) {
        result |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }

    return result;
}

VkMemoryPropertyFlags toVkMemoryPropertyFlags(const snap::rhi::MemoryProperties bits) {
    VkMemoryPropertyFlags flags = 0;
    if (static_cast<bool>(bits & snap::rhi::MemoryProperties::DeviceLocal)) {
        flags |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    }
    if (static_cast<bool>(bits & snap::rhi::MemoryProperties::HostVisible)) {
        flags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    }
    if (static_cast<bool>(bits & snap::rhi::MemoryProperties::HostCoherent)) {
        flags |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    }
    if (static_cast<bool>(bits & snap::rhi::MemoryProperties::HostCached)) {
        flags |= VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
    }
    return flags;
}
} // unnamed namespace

namespace snap::rhi::backend::vulkan {
Buffer::Buffer(Device* vkDevice, const snap::rhi::BufferCreateInfo& info)
    : snap::rhi::Buffer(vkDevice, info), vkDevice(vkDevice->getVkLogicalDevice()) {
    const auto& validationLayer = vkDevice->getValidationLayer();
    {
        /**
         * https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkBufferCreateFlagBits.html
         *
         * VK_SHARING_MODE_EXCLUSIVE specifies that access to any range or image subresource of the object will be
         * exclusive to a single queue family at a time
         *
         * SnapRHI always assume that subresource of the object will be exclusive to a single queue family at a
         * time
         * */
        const VkBufferCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .size = info.size,
            .usage = getVkBufferUsage(info.bufferUsage),
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr,
        };

        VkResult result = vkCreateBuffer(this->vkDevice, &createInfo, nullptr, &buffer);
        SNAP_RHI_VALIDATE(validationLayer,
                          result == VK_SUCCESS,
                          snap::rhi::ReportLevel::CriticalError,
                          snap::rhi::ValidationTag::CreateOp,
                          "[Vulkan::Buffer] cannot create buffer");
    }

    {
        VkMemoryRequirements memRequirements{};
        vkGetBufferMemoryRequirements(this->vkDevice, buffer, &memRequirements);

        const auto memoryProperties = toVkMemoryPropertyFlags(info.memoryProperties);

        const VkMemoryAllocateInfo allocInfo{
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext = nullptr,
            .allocationSize = memRequirements.size,
            .memoryTypeIndex = vkDevice->chooseMemoryTypeIndex(memRequirements.memoryTypeBits, memoryProperties),
        };

        const VkResult result = vkAllocateMemory(this->vkDevice, &allocInfo, nullptr, &memory);
        SNAP_RHI_VALIDATE(validationLayer,
                          result == VK_SUCCESS,
                          snap::rhi::ReportLevel::CriticalError,
                          snap::rhi::ValidationTag::CreateOp,
                          "[Vulkan::Buffer] cannot allocate memory for buffer");
    }

    vkBindBufferMemory(this->vkDevice, buffer, memory, 0);
}

Buffer::~Buffer() {
    vkDestroyBuffer(vkDevice, buffer, nullptr);
    vkFreeMemory(vkDevice, memory, nullptr);
}

std::byte* Buffer::map(const snap::rhi::MemoryAccess access, const uint64_t offset, const uint64_t size) {
    const uint64_t fixedSize = size == WholeSize ? info.size - offset : size;

    void* data = nullptr;
    vkMapMemory(vkDevice, memory, offset, fixedSize, 0, &data);

    assert(data);
    return static_cast<std::byte*>(data);
}

void Buffer::unmap() {
    vkUnmapMemory(vkDevice, memory);
}

void Buffer::flushMappedMemoryRanges(std::span<const snap::rhi::MappedMemoryRange> ranges) {
    // For coherent memory this call is safe but unnecessary. Vulkan ignores this for coherent memory.
    if (ranges.empty()) {
        return;
    }

    std::vector<VkMappedMemoryRange> vkRanges;
    vkRanges.reserve(ranges.size());
    for (const auto& r : ranges) {
        VkMappedMemoryRange range{
            .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
            .pNext = nullptr,
            .memory = memory,
            .offset = r.offset,
            .size = (r.size == WholeSize) ? VK_WHOLE_SIZE : r.size,
        };
        vkRanges.push_back(range);
    }

    vkFlushMappedMemoryRanges(vkDevice, static_cast<uint32_t>(vkRanges.size()), vkRanges.data());
}

void Buffer::invalidateMappedMemoryRanges(std::span<const snap::rhi::MappedMemoryRange> ranges) {
    if (ranges.empty()) {
        return;
    }

    std::vector<VkMappedMemoryRange> vkRanges;
    vkRanges.reserve(ranges.size());
    for (const auto& r : ranges) {
        VkMappedMemoryRange range{.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
                                  .pNext = nullptr,
                                  .memory = memory,
                                  .offset = r.offset,
                                  .size = (r.size == WholeSize) ? VK_WHOLE_SIZE : r.size};
        vkRanges.push_back(range);
    }

    vkInvalidateMappedMemoryRanges(vkDevice, static_cast<uint32_t>(vkRanges.size()), vkRanges.data());
}

void Buffer::setDebugLabel(std::string_view label) {
#if SNAP_RHI_ENABLE_DEBUG_LABELS
    ObjectDebugMarkers::setDebugLabel(label);
    setVkObjectDebugName(vkDevice, VK_OBJECT_TYPE_BUFFER, reinterpret_cast<uint64_t>(buffer), label.data());
#endif
}
} // namespace snap::rhi::backend::vulkan
