#pragma once

#include <memory>

#include "snap/rhi/TextureInterop.h"

#if SNAP_RHI_ENABLE_BACKEND_VULKAN
#include "snap/rhi/backend/vulkan/vulkan.h"
#endif

namespace snap::rhi::interop::Linux {

#if SNAP_RHI_ENABLE_BACKEND_VULKAN

/**
 * External Vulkan resources for texture interop. The caller retains ownership
 * of device and image. memoryFD ownership is transferred to GL on import.
 */
struct ExternalVulkanResourceInfo {
    VkDevice device = VK_NULL_HANDLE;
    VkImage image = VK_NULL_HANDLE;
    int memoryFD = -1;
    VkDeviceSize memorySize = 0;
};

/**
 * @brief Creates a TextureInterop from externally-owned Vulkan resources.
 *
 * This function is specifically for Linux platforms where Vulkan memory can be shared
 * via file descriptors. The caller retains ownership of the Vulkan resources and is
 * responsible for ensuring they remain valid for the lifetime of the TextureInterop.
 *
 * @param info The texture interop creation info (size, format, usage).
 * @param externalResources Shared pointer to external Vulkan resource info.
 * @return A unique_ptr to the created TextureInterop.
 */
std::unique_ptr<snap::rhi::TextureInterop> createTextureInteropFromExternalFD(
    const snap::rhi::TextureInteropCreateInfo& info, std::shared_ptr<ExternalVulkanResourceInfo> externalResources);

#endif // SNAP_RHI_ENABLE_BACKEND_VULKAN

} // namespace snap::rhi::interop::Linux
