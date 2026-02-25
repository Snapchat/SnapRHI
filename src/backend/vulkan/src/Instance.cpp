#include "snap/rhi/backend/vulkan/Instance.h"
#include "snap/rhi/backend/vulkan/Device.h"

namespace snap::rhi::backend::vulkan {
std::unique_ptr<snap::rhi::Device> createVulkanDevice(const snap::rhi::backend::vulkan::DeviceCreateInfo& info) {
    return std::make_unique<snap::rhi::backend::vulkan::Device>(info);
}
} // namespace snap::rhi::backend::vulkan
