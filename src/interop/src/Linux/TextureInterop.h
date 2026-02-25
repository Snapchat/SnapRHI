#pragma once

#if SNAP_RHI_ENABLE_BACKEND_VULKAN
#include "snap/rhi/backend/vulkan/ImageWithMemory.h"
#include "snap/rhi/backend/vulkan/TextureInterop.h"
#include "snap/rhi/interop/platform/linux/TextureInteropFactory.h"
#endif

#if SNAP_RHI_ENABLE_BACKEND_OPENGL
#include "snap/rhi/backend/opengl/TextureInterop.h"
#endif

#include "snap/rhi/backend/common/TextureInterop.h"

#include <future>
#include <memory>

namespace snap::rhi {
class Semaphore;
}

namespace snap::rhi::interop::Linux {
class TextureInterop : public virtual snap::rhi::backend::common::TextureInterop
#if SNAP_RHI_ENABLE_BACKEND_OPENGL
    ,
                       public virtual snap::rhi::backend::opengl::TextureInterop
#endif
#if SNAP_RHI_ENABLE_BACKEND_VULKAN
    ,
                       public virtual snap::rhi::backend::vulkan::TextureInterop
#endif
{
public:
    TextureInterop(const TextureInteropCreateInfo& info);

#if SNAP_RHI_ENABLE_BACKEND_VULKAN
    TextureInterop(const TextureInteropCreateInfo& info, std::shared_ptr<ExternalVulkanResourceInfo> externalResources);
#endif

    ~TextureInterop() override;

#if SNAP_RHI_ENABLE_BACKEND_OPENGL
    GLuint getOpenGLTexture(const snap::rhi::backend::opengl::Profile& gl) final;
    snap::rhi::backend::opengl::TextureTarget getOpenGLTextureTarget(
        const snap::rhi::backend::opengl::Profile& gl) final;
#endif

#if SNAP_RHI_ENABLE_BACKEND_VULKAN
    VkImage getVulkanTexture(snap::rhi::Device* device) final;
    VkDeviceMemory getVulkanTextureMemory(snap::rhi::Device* device) final;
#endif

private:
#if SNAP_RHI_ENABLE_BACKEND_OPENGL
    GLuint glTextureID = 0;
    GLuint glMemoryObject = 0;
    void loadGLTexture(const snap::rhi::backend::opengl::Profile& gl);
#endif

#if SNAP_RHI_ENABLE_BACKEND_VULKAN
    // For internally-created Vulkan resources
    std::unique_ptr<snap::rhi::backend::vulkan::ImageWithMemory> imageWithMemory = nullptr;
    VkDevice cachedVulkanDevice = VK_NULL_HANDLE;

    // For externally-provided Vulkan resources (no ImageWithMemory wrapper needed)
    std::shared_ptr<ExternalVulkanResourceInfo> externalResources = nullptr;

    void loadVkTexture(snap::rhi::Device* device);
#endif
};
} // namespace snap::rhi::interop::Linux
