#pragma once

#include <CoreFoundation/CoreFoundation.h>
#include <CoreVideo/CoreVideo.h>

#if SNAP_RHI_ENABLE_BACKEND_OPENGL
#include "snap/rhi/backend/opengl/TextureInterop.h"

#if SNAP_RHI_OS_IOS()
#include <CoreVideo/CVOpenGLESTextureCache.h>

using CVOpenGLTextureCacheRef = CVOpenGLESTextureCacheRef;
using CVOpenGLTextureRef = CVOpenGLESTextureRef;
#else
#include <AppKit/NSOpenGL.h>
#include <CoreVideo/CVOpenGLTextureCache.h>
#endif

#endif

#if SNAP_RHI_ENABLE_BACKEND_METAL
#include "snap/rhi/backend/metal/TextureInterop.h"
#include <CoreVideo/CVMetalTextureCache.h>
#endif

#if SNAP_RHI_ENABLE_BACKEND_VULKAN
#include "snap/rhi/backend/vulkan/ImageWithMemory.h"
#include "snap/rhi/backend/vulkan/TextureInterop.h"
#endif

#include "snap/rhi/backend/common/TextureInterop.h"

#include <condition_variable>
#include <future>
#include <memory>
#include <mutex>

namespace snap::rhi::interop::Apple {
class TextureInterop : public virtual snap::rhi::backend::common::TextureInterop
#if SNAP_RHI_ENABLE_BACKEND_OPENGL
    ,
                       public virtual snap::rhi::backend::opengl::TextureInterop
#endif
#if SNAP_RHI_ENABLE_BACKEND_VULKAN
    ,
                       public virtual snap::rhi::backend::vulkan::TextureInterop
#endif
#if SNAP_RHI_ENABLE_BACKEND_METAL
    ,
                       public virtual snap::rhi::backend::metal::TextureInterop
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

#if SNAP_RHI_ENABLE_BACKEND_METAL
    const id<MTLTexture>& getMetalTexture(const id<MTLDevice>& mtlDevice) final;
#endif

#if SNAP_RHI_ENABLE_BACKEND_VULKAN
    VkImage getVulkanTexture(snap::rhi::Device* device) final;
    VkDeviceMemory getVulkanTextureMemory(snap::rhi::Device* device) final;
#endif

    std::span<const ImagePlane> map(const snap::rhi::MemoryAccess access,
                                    snap::rhi::PlatformSyncHandle* platformSyncHandle) final;
    std::unique_ptr<snap::rhi::PlatformSyncHandle> unmap() final;

private:
    CVPixelBufferRef pixelBuffer = nullptr;

#if SNAP_RHI_ENABLE_BACKEND_OPENGL
    CVOpenGLTextureCacheRef glTextureCache = nullptr;
    CVOpenGLTextureRef cvGLTexture = nullptr;
    GLuint glTextureID = 0;

    void loadGLTexture(const snap::rhi::backend::opengl::Profile& gl);
#endif

#if SNAP_RHI_ENABLE_BACKEND_METAL
    CVMetalTextureCacheRef metalTextureCache = nullptr;
    CVMetalTextureRef cvMetalTexture = nullptr;
    id<MTLTexture> metalTexture = nil;
    id<MTLCommandBuffer> lastMetalCommandBuffer = nil;

    void loadMetalTexture(const id<MTLDevice>& mtlDevice);
#endif

#if SNAP_RHI_ENABLE_BACKEND_VULKAN
    std::unique_ptr<snap::rhi::backend::vulkan::ImageWithMemory> vkImageWithMemory;
    snap::rhi::Device* _lastVulkanDevice = nullptr;
#endif

    CVPixelBufferLockFlags currentLockFlags = 0;
};
} // namespace snap::rhi::interop::Apple
