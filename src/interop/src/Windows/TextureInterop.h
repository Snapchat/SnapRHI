#pragma once

#include "snap/rhi/backend/common/TextureInterop.h"

#include "snap/rhi/Texture.hpp"

#if SNAP_RHI_ENABLE_BACKEND_VULKAN
#include "snap/rhi/backend/vulkan/ImageWithMemory.h"
#include "snap/rhi/backend/vulkan/TextureInterop.h"
#endif

#if SNAP_RHI_ENABLE_BACKEND_OPENGL
#include "snap/rhi/backend/opengl/Device.hpp"
#include "snap/rhi/backend/opengl/TextureInterop.h"
#endif

#include <future>
#include <memory>

namespace snap::rhi {
class Semaphore;
}

namespace snap::rhi::interop::Windows {
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
    TextureInterop(const snap::rhi::TextureInteropCreateInfo& info);
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
    std::shared_ptr<snap::rhi::Texture> rhiTexture = nullptr;
    GLuint glTextureID = 0;

    void loadGLTexture(const snap::rhi::backend::opengl::Profile& gl);
#endif

#if SNAP_RHI_ENABLE_BACKEND_VULKAN
    std::unique_ptr<snap::rhi::backend::vulkan::ImageWithMemory> imageWithMemory = nullptr;
    snap::rhi::backend::vulkan::Device* cachedVulkanDevice = nullptr;

    void loadVkTexture(snap::rhi::Device* device);
#endif
};
} // namespace snap::rhi::interop::Windows
