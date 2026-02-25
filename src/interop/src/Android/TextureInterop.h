#pragma once

#if SNAP_RHI_ENABLE_BACKEND_VULKAN
#include "snap/rhi/backend/vulkan/ImageWithMemory.h"
#include "snap/rhi/backend/vulkan/TextureInterop.h"
#endif

#if SNAP_RHI_ENABLE_BACKEND_OPENGL
#include "snap/rhi/backend/opengl/TextureInterop.h"
#endif

#include "snap/rhi/backend/common/TextureInterop.h"

#include <future>
#include <memory>

struct AHardwareBuffer;

namespace snap::rhi {
class Semaphore;
}

namespace snap::rhi::interop::Android {
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

    std::span<const ImagePlane> map(const snap::rhi::MemoryAccess access,
                                    snap::rhi::PlatformSyncHandle* platformSyncHandle) final;
    std::unique_ptr<snap::rhi::PlatformSyncHandle> unmap() final;

private:
    struct AHardwareBuffer* androidBuffer = nullptr;

#if SNAP_RHI_ENABLE_BACKEND_OPENGL
    GLuint loadGLTexture(const snap::rhi::backend::opengl::Profile& gl);

    std::shared_ptr<snap::rhi::Texture> rhiTexture = nullptr;
    GLuint glTextureID = 0;
    EGLImageKHR eglImage = nullptr;
#endif

#if SNAP_RHI_ENABLE_BACKEND_VULKAN
    VkImage loadVkTexture(snap::rhi::Device* device);
    std::shared_ptr<snap::rhi::backend::vulkan::ImageWithMemory> imageWithMemory = nullptr;
#endif
};
} // namespace snap::rhi::interop::Android
