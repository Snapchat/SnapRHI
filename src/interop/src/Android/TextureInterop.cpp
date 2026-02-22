#include "TextureInterop.h"

#include "snap/rhi/Device.hpp"
#include "snap/rhi/backend/common/PlatformSyncHandle.h"
#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/common/platform/android/SyncHandle.h"

#if SNAP_RHI_ENABLE_BACKEND_OPENGL
#include "snap/rhi/backend/opengl/Device.hpp"
#include "snap/rhi/backend/opengl/Profile.hpp"
#include "snap/rhi/backend/opengl/TextureTarget.h"
#include <OpenGL/Context.h>
#endif

#include "snap/rhi/common/Throw.h"
#include <dlfcn.h>
#include <snap/rhi/common/Scope.h>

#if SNAP_RHI_ENABLE_BACKEND_VULKAN
#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/vulkan/Device.h"
#include "snap/rhi/backend/vulkan/Utils.hpp"
#endif

#if __ANDROID_API__ < 26
#define AHardwareBuffer_allocate AHardwareBuffer_allocate_t
#define AHardwareBuffer_release AHardwareBuffer_release_t
#define AHardwareBuffer_describe AHardwareBuffer_describe_t
#define AHardwareBuffer_lock AHardwareBuffer_lock_t
#define AHardwareBuffer_unlock AHardwareBuffer_unlock_t
#endif

#if __ANDROID_API__ < 29
#define AHardwareBuffer_lockPlanes AHardwareBuffer_lockPlanes_t
#endif

#include <android/hardware_buffer.h>

#if __ANDROID_API__ < 26
#undef AHardwareBuffer_allocate
#undef AHardwareBuffer_release
#undef AHardwareBuffer_describe
#undef AHardwareBuffer_lock
#undef AHardwareBuffer_unlock
#endif

#if __ANDROID_API__ < 29
#undef AHardwareBuffer_lockPlanes
#endif

extern "C" {
#if __ANDROID_API__ < 26
int __attribute__((weak)) AHardwareBuffer_allocate(const AHardwareBuffer_Desc* desc, AHardwareBuffer** outBuffer);
void __attribute__((weak)) AHardwareBuffer_release(AHardwareBuffer* buffer);
void __attribute__((weak)) AHardwareBuffer_describe(const AHardwareBuffer* buffer, AHardwareBuffer_Desc* outDesc);
int __attribute__((weak)) AHardwareBuffer_lock(
    AHardwareBuffer* buffer, uint64_t usage, int32_t fence, const ARect* rect, void** outVirtualAddress);
int __attribute__((weak)) AHardwareBuffer_unlock(AHardwareBuffer* buffer, int32_t* fence);
#endif
#if __ANDROID_API__ < 29
int __attribute__((weak)) AHardwareBuffer_lockPlanes(
    AHardwareBuffer* buffer, uint64_t usage, int32_t fence, const ARect* rect, AHardwareBuffer_Planes* outPlanes);
#endif
}

namespace {

uint64_t getAHardwareBufferUsageFlags(uint32_t width,
                                      uint32_t height,
                                      snap::rhi::PixelFormat format,
                                      snap::rhi::TextureUsage textureUsage) {
    uint64_t usageFlags = 0;
    uint32_t usage = static_cast<uint32_t>(textureUsage);

    if (usage & static_cast<uint32_t>(snap::rhi::TextureUsage::Sampled)) {
        usageFlags |= AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE;
    }

    if (usage & static_cast<uint32_t>(snap::rhi::TextureUsage::ColorAttachment |
                                      snap::rhi::TextureUsage::DepthStencilAttachment)) {
        usageFlags |= AHARDWAREBUFFER_USAGE_GPU_FRAMEBUFFER;
    }

    if (usage & static_cast<uint32_t>(snap::rhi::TextureUsage::Storage)) {
        usageFlags |= AHARDWAREBUFFER_USAGE_GPU_DATA_BUFFER;
    }

    if (usage & static_cast<uint32_t>(snap::rhi::TextureUsage::TransferSrc)) {
        usageFlags |= AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN;
        usageFlags |= AHARDWAREBUFFER_USAGE_CPU_WRITE_NEVER;
    } else if (usage & static_cast<uint32_t>(snap::rhi::TextureUsage::TransferDst)) {
        usageFlags |= AHARDWAREBUFFER_USAGE_CPU_READ_RARELY;
        usageFlags |= AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN;
    }

    return usageFlags;
}

uint32_t getAHardwareBufferFormat(snap::rhi::PixelFormat format) {
    // We use a switch statement instead of a map, as most formats
    // do not have a direct equivalent.
    switch (format) {
        // Standard formats with direct equivalents
        case snap::rhi::PixelFormat::R8Unorm:
            return AHARDWAREBUFFER_FORMAT_R8_UNORM;
        case snap::rhi::PixelFormat::R8G8B8Unorm:
            return AHARDWAREBUFFER_FORMAT_R8G8B8_UNORM;
        case snap::rhi::PixelFormat::R8G8B8A8Unorm:
            return AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM;
        case snap::rhi::PixelFormat::R16Uint:
            return AHARDWAREBUFFER_FORMAT_R16_UINT;
        case snap::rhi::PixelFormat::R16G16Uint:
            return AHARDWAREBUFFER_FORMAT_R16G16_UINT;
        case snap::rhi::PixelFormat::R16G16B16A16Float:
            return AHARDWAREBUFFER_FORMAT_R16G16B16A16_FLOAT;
        case snap::rhi::PixelFormat::R10G10B10A2Unorm:
            return AHARDWAREBUFFER_FORMAT_R10G10B10A2_UNORM;
        case snap::rhi::PixelFormat::Grayscale:
            return AHARDWAREBUFFER_FORMAT_R8_UNORM;

        // Depth/stencil formats
        case snap::rhi::PixelFormat::Depth16Unorm:
            return AHARDWAREBUFFER_FORMAT_D16_UNORM;
        case snap::rhi::PixelFormat::DepthFloat:
            return AHARDWAREBUFFER_FORMAT_D32_FLOAT;
        case snap::rhi::PixelFormat::DepthStencil:
            return AHARDWAREBUFFER_FORMAT_D24_UNORM_S8_UINT;
        default:
            snap::rhi::common::throwException("Unsupported PixelFormat for texture interop");
    }
}

uint32_t getBytesPerPixelFromFormat(uint32_t format) {
    switch (format) {
        case AHARDWAREBUFFER_FORMAT_R8_UNORM:
            return 1;
        case AHARDWAREBUFFER_FORMAT_R16_UINT:
        case AHARDWAREBUFFER_FORMAT_D16_UNORM:
            return 2;
        case AHARDWAREBUFFER_FORMAT_R8G8B8_UNORM:
            return 3;
        case AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM:
        case AHARDWAREBUFFER_FORMAT_R10G10B10A2_UNORM:
        case AHARDWAREBUFFER_FORMAT_R16G16_UINT:
        case AHARDWAREBUFFER_FORMAT_D24_UNORM_S8_UINT:
        case AHARDWAREBUFFER_FORMAT_D32_FLOAT:
            return 4;
        case AHARDWAREBUFFER_FORMAT_R16G16B16A16_FLOAT:
            return 8;
        default:
            return 4;
    }
}

} // namespace

namespace snap::rhi::interop::Android {
TextureInterop::TextureInterop(const TextureInteropCreateInfo& info)
    : snap::rhi::backend::common::TextureInterop(info) {
#if SNAP_RHI_ENABLE_BACKEND_OPENGL
#if !SNAP_RHI_ENABLE_EGL_GLAD
    gladLoadEGLExtensionsSafe(gl::getDefaultDisplay());
#endif
    compatibleAPIs.push_back(snap::rhi::API::OpenGL);
#endif

#if SNAP_RHI_ENABLE_BACKEND_VULKAN
    compatibleAPIs.push_back(snap::rhi::API::Vulkan);
#endif

    AHardwareBuffer_Desc bufferDesc = {};
    bufferDesc.width = info.size.width;
    bufferDesc.height = info.size.height;
    bufferDesc.layers = 1;
    bufferDesc.format = getAHardwareBufferFormat(info.format);
    bufferDesc.usage = getAHardwareBufferUsageFlags(info.size.width, info.size.height, info.format, info.textureUsage);
    bufferDesc.stride = 0;

    AHardwareBuffer* buffer = nullptr;
    int result = AHardwareBuffer_allocate(&bufferDesc, &buffer);
    if (result != 0) {
        snap::rhi::common::throwException("Failed to allocate AHardwareBuffer");
    }

    androidBuffer = buffer;
}

TextureInterop::~TextureInterop() {
#if SNAP_RHI_ENABLE_BACKEND_OPENGL
    if (eglImage != nullptr) {
        eglDestroyImageKHR(gl::getDefaultDisplay(), eglImage);
        eglImage = nullptr;
    }
#endif

    if (androidBuffer != nullptr) {
        AHardwareBuffer_release(androidBuffer);
        androidBuffer = nullptr;
    }
}

#if SNAP_RHI_ENABLE_BACKEND_OPENGL
GLuint TextureInterop::getOpenGLTexture(const snap::rhi::backend::opengl::Profile& gl) {
    if (glTextureID != 0) {
        return glTextureID;
    }

    glTextureID = loadGLTexture(gl);
    return glTextureID;
}

GLuint TextureInterop::loadGLTexture(const snap::rhi::backend::opengl::Profile& gl) {
    if (!androidBuffer) {
        snap::rhi::common::throwException("Invalid Android hardware buffer");
    }

    EGLint eglImgAttrs[] = {EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE, EGL_NONE};

    EGLClientBuffer clientBuffer = eglGetNativeClientBufferANDROID(androidBuffer);
    if (!clientBuffer) {
        snap::rhi::common::throwException("Failed to get native client buffer from AHardwareBuffer");
    }

    eglImage = eglCreateImageKHR(
        gl::getDefaultDisplay(), EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, clientBuffer, eglImgAttrs);

    if (eglImage == nullptr) {
        snap::rhi::common::throwException("Failed to create EGL image from AHardwareBuffer");
    }

    const auto& device = gl.getDevice();

    rhiTexture = device->createTexture(snap::rhi::TextureInterop::getTextureCreateInfo());
    snap::rhi::backend::opengl::Texture* nativeTexture =
        snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Texture>(rhiTexture.get());
    glTextureID = static_cast<GLuint>(nativeTexture->getTextureID(nullptr));

    SNAP_RHI_ON_SCOPE_EXIT {
        gl.bindTexture(snap::rhi::backend::opengl::TextureTarget::Texture2D,
                       static_cast<snap::rhi::backend::opengl::TextureId>(0),
                       nullptr);
    };
    gl.bindTexture(snap::rhi::backend::opengl::TextureTarget::Texture2D,
                   static_cast<snap::rhi::backend::opengl::TextureId>(glTextureID),
                   nullptr);
    glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, eglImage);

    return glTextureID;
}

snap::rhi::backend::opengl::TextureTarget TextureInterop::getOpenGLTextureTarget(
    const snap::rhi::backend::opengl::Profile& gl) {
    return snap::rhi::backend::opengl::TextureTarget::Texture2D;
}
#endif

#if SNAP_RHI_ENABLE_BACKEND_VULKAN
VkImage TextureInterop::loadVkTexture(snap::rhi::Device* device) {
    if (!androidBuffer) {
        snap::rhi::common::throwException("Invalid Android hardware buffer");
    }

    auto* vkDevice = snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::Device>(device);
    if (!vkDevice) {
        snap::rhi::common::throwException("Device is not a Vulkan device");
    }

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
    VkExternalMemoryImageCreateInfo externalImageInfo{};
    externalImageInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
    externalImageInfo.pNext = nullptr;
    externalImageInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID;

    VkImportAndroidHardwareBufferInfoANDROID importInfo{};
    importInfo.sType = VK_STRUCTURE_TYPE_IMPORT_ANDROID_HARDWARE_BUFFER_INFO_ANDROID;
    importInfo.pNext = nullptr;
    importInfo.buffer = androidBuffer;

    imageWithMemory =
        std::make_shared<snap::rhi::backend::vulkan::ImageWithMemory>(vkDevice,
                                                                      snap::rhi::TextureInterop::getTextureCreateInfo(),
                                                                      &externalImageInfo, // pImageNext
                                                                      &importInfo         // pMemoryNext
        );
#else
    snap::rhi::common::throwException("Android hardware buffer support not compiled in");
#endif

    return imageWithMemory->getImage();
}

VkImage TextureInterop::getVulkanTexture(snap::rhi::Device* device) {
    if (imageWithMemory) {
        return imageWithMemory->getImage();
    }

    return loadVkTexture(device);
}

VkDeviceMemory TextureInterop::getVulkanTextureMemory(snap::rhi::Device* device) {
    if (!imageWithMemory) {
        loadVkTexture(device);
    }

    return imageWithMemory->getImageMemory();
}
#endif

std::span<const snap::rhi::TextureInterop::ImagePlane> TextureInterop::map(
    const snap::rhi::MemoryAccess access, snap::rhi::PlatformSyncHandle* platformSyncHandle) {
    if (!androidBuffer) {
        snap::rhi::common::throwException("[TextureInterop::map] Invalid Android hardware buffer");
    }

    int acquireFd = -1;

    if (platformSyncHandle) {
        auto* androidHandle =
            dynamic_cast<snap::rhi::backend::common::platform::android::SyncHandle*>(platformSyncHandle);
        if (androidHandle) {
            acquireFd = androidHandle->releaseFenceFd();
        }

        if (acquireFd < 0) {
            auto* commonHandle = snap::rhi::backend::common::smart_cast<snap::rhi::backend::common::PlatformSyncHandle>(
                platformSyncHandle);
            if (const auto& fence = commonHandle->getFence(); fence) {
                fence->waitForComplete();
            }
        }
    }

    uint64_t usageFlags = 0;
    if (access == snap::rhi::MemoryAccess::Read) {
        usageFlags = AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN;
    } else if (access == snap::rhi::MemoryAccess::Write) {
        usageFlags = AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN;
    } else if (access == (snap::rhi::MemoryAccess::Read | snap::rhi::MemoryAccess::Write)) {
        usageFlags = AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN | AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN;
    }

    imagePlanes.clear();

    if (&AHardwareBuffer_lockPlanes != nullptr) {
        // Multi-plane lock (Android 29+)
        AHardwareBuffer_Planes planes{};
        int result = AHardwareBuffer_lockPlanes(androidBuffer, usageFlags, acquireFd, nullptr, &planes);

        if (result != 0) {
            snap::rhi::common::throwException("[TextureInterop::map] Failed to lock AHardwareBuffer");
        }

        for (uint32_t i = 0; i < planes.planeCount; ++i) {
            snap::rhi::TextureInterop::ImagePlane plane{};
            plane.bytesPerRow = planes.planes[i].rowStride;
            plane.bytesPerPixel = planes.planes[i].pixelStride;
            plane.pixels = planes.planes[i].data;
            imagePlanes.push_back(plane);
        }
    } else {
        // Single-plane lock fallback (Android 26+)
        void* data = nullptr;
        int result = AHardwareBuffer_lock(androidBuffer, usageFlags, acquireFd, nullptr, &data);

        if (result != 0) {
            snap::rhi::common::throwException("[TextureInterop::map] Failed to lock AHardwareBuffer");
        }

        AHardwareBuffer_Desc desc{};
        AHardwareBuffer_describe(androidBuffer, &desc);

        uint32_t bytesPerPixel = getBytesPerPixelFromFormat(desc.format);

        snap::rhi::TextureInterop::ImagePlane plane{};
        plane.bytesPerRow = desc.width * bytesPerPixel;
        plane.bytesPerPixel = bytesPerPixel;
        plane.pixels = data;
        imagePlanes.push_back(plane);
    }

    return std::span<const snap::rhi::TextureInterop::ImagePlane>(imagePlanes);
}

std::unique_ptr<snap::rhi::PlatformSyncHandle> TextureInterop::unmap() {
    if (!androidBuffer) {
        snap::rhi::common::throwException("[TextureInterop::unmap] Invalid Android hardware buffer");
    }

    int releaseFd = -1;
    int result = AHardwareBuffer_unlock(androidBuffer, &releaseFd);

    if (result != 0) {
        snap::rhi::common::throwException("[TextureInterop::unmap] Failed to unlock AHardwareBuffer");
    }

    if (releaseFd >= 0) {
        return std::make_unique<snap::rhi::backend::common::platform::android::SyncHandle>(nullptr, releaseFd);
    }

    return std::make_unique<snap::rhi::backend::common::PlatformSyncHandle>(nullptr);
}
} // namespace snap::rhi::interop::Android
