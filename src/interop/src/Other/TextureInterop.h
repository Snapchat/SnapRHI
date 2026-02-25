#pragma once

#include "snap/rhi/backend/common/TextureInterop.h"

#include "snap/rhi/Texture.hpp"

#if SNAP_RHI_ENABLE_BACKEND_VULKAN
#include "snap/rhi/backend/vulkan/ImageWithMemory.h"
#include "snap/rhi/backend/vulkan/Texture.h"
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

namespace snap::rhi::interop::Other {
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

private:
    std::shared_ptr<snap::rhi::Texture> glTexture = nullptr;

#if SNAP_RHI_ENABLE_BACKEND_VULKAN
    std::shared_ptr<snap::rhi::backend::vulkan::ImageWithMemory> vkImageWithMemory = nullptr;
    std::shared_ptr<snap::rhi::backend::vulkan::Texture> vkTexture = nullptr;
    void createVkTexture(snap::rhi::Device* device);
#endif

    void createGLTexture(snap::rhi::Device* device);
    void copyBetweenTextures(snap::rhi::Texture* sourceTexture, snap::rhi::Texture* targetTexture);
};
} // namespace snap::rhi::interop::Other
