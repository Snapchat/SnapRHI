#include "TextureInterop.h"
#include "snap/rhi/CommandBuffer.hpp"
#include "snap/rhi/Device.hpp"
#include "snap/rhi/backend/common/PlatformSyncHandle.h"
#include "snap/rhi/backend/common/Utils.hpp"

#if SNAP_RHI_ENABLE_BACKEND_OPENGL
#include "snap/rhi/backend/opengl/Profile.hpp"
#include "snap/rhi/backend/opengl/TextureTarget.h"
#endif

#if SNAP_RHI_ENABLE_BACKEND_METAL
#include "snap/rhi/backend/metal/CommandBuffer.h"
#include "snap/rhi/backend/metal/Utils.h"
#endif

#if SNAP_RHI_ENABLE_BACKEND_VULKAN
#include "snap/rhi/backend/vulkan/CommandBuffer.h"
#include "snap/rhi/backend/vulkan/Device.h"
#endif

#include "snap/rhi/Semaphore.hpp"

#include <CoreVideo/CVPixelBuffer.h>
#include <CoreVideo/CVReturn.h>

#include <array>
#include <condition_variable>
#include <future>
#include <mutex>

namespace {
constexpr std::array<OSType, static_cast<uint32_t>(snap::rhi::PixelFormat::Count)> pixelFormatToCVPixelFormat = []() {
    std::array<OSType, static_cast<uint32_t>(snap::rhi::PixelFormat::Count)> result{};
    result[static_cast<uint32_t>(snap::rhi::PixelFormat::Undefined)] = kCVPixelFormatType_32BGRA;
    result[static_cast<uint32_t>(snap::rhi::PixelFormat::R8Unorm)] = kCVPixelFormatType_OneComponent8;
    result[static_cast<uint32_t>(snap::rhi::PixelFormat::R8G8Unorm)] = kCVPixelFormatType_TwoComponent8;
    result[static_cast<uint32_t>(snap::rhi::PixelFormat::R8G8B8Unorm)] = kCVPixelFormatType_24RGB;
    result[static_cast<uint32_t>(snap::rhi::PixelFormat::R16Unorm)] = kCVPixelFormatType_OneComponent16;
    result[static_cast<uint32_t>(snap::rhi::PixelFormat::R16G16Unorm)] = kCVPixelFormatType_TwoComponent16;
    result[static_cast<uint32_t>(snap::rhi::PixelFormat::R16G16B16A16Unorm)] = kCVPixelFormatType_64ARGB;
    result[static_cast<uint32_t>(snap::rhi::PixelFormat::R32Uint)] = kCVPixelFormatType_OneComponent32Float;
    result[static_cast<uint32_t>(snap::rhi::PixelFormat::R32G32Uint)] = kCVPixelFormatType_TwoComponent32Float;
    result[static_cast<uint32_t>(snap::rhi::PixelFormat::R32G32B32A32Uint)] = kCVPixelFormatType_128RGBAFloat;
    result[static_cast<uint32_t>(snap::rhi::PixelFormat::R32Sint)] = kCVPixelFormatType_OneComponent32Float;
    result[static_cast<uint32_t>(snap::rhi::PixelFormat::R32G32Sint)] = kCVPixelFormatType_TwoComponent32Float;
    result[static_cast<uint32_t>(snap::rhi::PixelFormat::R32G32B32A32Sint)] = kCVPixelFormatType_128RGBAFloat;
    result[static_cast<uint32_t>(snap::rhi::PixelFormat::R16Float)] = kCVPixelFormatType_DepthFloat16;
    result[static_cast<uint32_t>(snap::rhi::PixelFormat::R16G16Float)] = kCVPixelFormatType_TwoComponent16Half;
    result[static_cast<uint32_t>(snap::rhi::PixelFormat::R16G16B16A16Float)] = kCVPixelFormatType_64RGBAHalf;
    result[static_cast<uint32_t>(snap::rhi::PixelFormat::R32Float)] = kCVPixelFormatType_OneComponent32Float;
    result[static_cast<uint32_t>(snap::rhi::PixelFormat::R32G32Float)] = kCVPixelFormatType_TwoComponent32Float;
    result[static_cast<uint32_t>(snap::rhi::PixelFormat::R32G32B32A32Float)] = kCVPixelFormatType_128RGBAFloat;
    result[static_cast<uint32_t>(snap::rhi::PixelFormat::Grayscale)] = kCVPixelFormatType_OneComponent8;
    result[static_cast<uint32_t>(snap::rhi::PixelFormat::B8G8R8A8Unorm)] = kCVPixelFormatType_32BGRA;
    result[static_cast<uint32_t>(snap::rhi::PixelFormat::R10G10B10A2Unorm)] = kCVPixelFormatType_ARGB2101010LEPacked;
    result[static_cast<uint32_t>(snap::rhi::PixelFormat::R11G11B10Float)] = kCVPixelFormatType_30RGBLEPackedWideGamut;
    result[static_cast<uint32_t>(snap::rhi::PixelFormat::Depth16Unorm)] = kCVPixelFormatType_DepthFloat16;
    result[static_cast<uint32_t>(snap::rhi::PixelFormat::DepthFloat)] = kCVPixelFormatType_DepthFloat32;

    // not exact equivalent (should rotate channels), but keeping it for unit tests
    result[static_cast<uint32_t>(snap::rhi::PixelFormat::R8G8B8A8Unorm)] = kCVPixelFormatType_32BGRA;
    return result;
}();

CVPixelBufferRef createPixelBuffer(uint32_t width, uint32_t height, snap::rhi::PixelFormat format) {
    auto emptyCFDictionary = CFDictionaryCreate(
        kCFAllocatorDefault, NULL, NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

    auto cvPixelBufferAttrs = CFDictionaryCreateMutable(
        kCFAllocatorDefault, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

#if SNAP_RHI_OS_IOS()
    CFDictionarySetValue(cvPixelBufferAttrs, kCVPixelBufferOpenGLESCompatibilityKey, kCFBooleanTrue);
#else
    CFDictionarySetValue(cvPixelBufferAttrs, kCVPixelBufferOpenGLCompatibilityKey, kCFBooleanTrue);
#endif

    CFDictionarySetValue(cvPixelBufferAttrs, kCVPixelBufferIOSurfacePropertiesKey, emptyCFDictionary);
    CFDictionarySetValue(cvPixelBufferAttrs, kCVPixelBufferMetalCompatibilityKey, kCFBooleanTrue);

    uint32_t formatIndex = static_cast<uint32_t>(format);
    OSType cvPixelFormat = pixelFormatToCVPixelFormat[formatIndex];

    CVPixelBufferRef pixelBufferRef = nullptr;
    CVReturn err =
        CVPixelBufferCreate(kCFAllocatorDefault, width, height, cvPixelFormat, cvPixelBufferAttrs, &pixelBufferRef);

    CFRelease(cvPixelBufferAttrs);
    CFRelease(emptyCFDictionary);

    if (err != kCVReturnSuccess) {
        snap::rhi::common::throwException("Failed to create CVPixelBuffer: " + std::to_string(err));
    }

    return pixelBufferRef;
}
} // unnamed namespace

namespace snap::rhi::interop::Apple {
TextureInterop::TextureInterop(const TextureInteropCreateInfo& info)
    : snap::rhi::backend::common::TextureInterop(info) {
    pixelBuffer = ::createPixelBuffer(info.size.width, info.size.height, info.format);

#if SNAP_RHI_ENABLE_BACKEND_OPENGL
    compatibleAPIs.push_back(snap::rhi::API::OpenGL);
#endif

#if SNAP_RHI_ENABLE_BACKEND_METAL
    compatibleAPIs.push_back(snap::rhi::API::Metal);
#endif

#if SNAP_RHI_ENABLE_BACKEND_VULKAN
    compatibleAPIs.push_back(snap::rhi::API::Vulkan);
#endif
}

TextureInterop::~TextureInterop() {
    if (pixelBuffer) {
        CFRelease(pixelBuffer);
        pixelBuffer = nil;
    }

#if SNAP_RHI_ENABLE_BACKEND_OPENGL
    if (cvGLTexture) {
        CFRelease(cvGLTexture);
    }
    if (glTextureCache) {
        CFRelease(glTextureCache);
    }
#endif

#if SNAP_RHI_ENABLE_BACKEND_METAL
    if (cvMetalTexture) {
        CFRelease(cvMetalTexture);
    }
    if (metalTextureCache) {
        CFRelease(metalTextureCache);
    }
    metalTexture = nil;
    lastMetalCommandBuffer = nil;
#endif

#if SNAP_RHI_ENABLE_BACKEND_VULKAN
    vkImageWithMemory.reset();
#endif
}

#if SNAP_RHI_ENABLE_BACKEND_OPENGL
GLuint TextureInterop::getOpenGLTexture(const snap::rhi::backend::opengl::Profile& gl) {
    if (glTextureID == 0) {
        loadGLTexture(gl);
    }
    return glTextureID;
}

snap::rhi::backend::opengl::TextureTarget TextureInterop::getOpenGLTextureTarget(
    const snap::rhi::backend::opengl::Profile& gl) {
    if (!cvGLTexture) {
        loadGLTexture(gl);
    }

#if SNAP_RHI_OS_IOS()
    GLenum target = CVOpenGLESTextureGetTarget(cvGLTexture);
#else
    GLenum target = CVOpenGLTextureGetTarget(cvGLTexture);
#endif

    return ::snap::rhi::backend::opengl::TextureTarget(target);
}

void TextureInterop::loadGLTexture(const snap::rhi::backend::opengl::Profile& gl) {
    if (glTextureID != 0) {
        return;
    }

    if (!pixelBuffer) {
        snap::rhi::common::throwException("Invalid pixel buffer");
    }

#if SNAP_RHI_OS_IOS()
    if (!glTextureCache) {
        CVOpenGLESTextureCacheRef textureCacheRef = nullptr;
        CVReturn err = CVOpenGLESTextureCacheCreate(
            kCFAllocatorDefault, nullptr, (__bridge CVEAGLContext)gl::getActiveContext(), nullptr, &textureCacheRef);
        if (err != kCVReturnSuccess) {
            snap::rhi::common::throwException("Failed to create OpenGL ES texture cache");
        }
        glTextureCache = textureCacheRef;
    }

    const auto& formatInfo = gl.getTextureFormat(textureInteropCreateInfo.format);

    GLint internalFormat = static_cast<GLint>(formatInfo.internalFormat);
    GLenum format = static_cast<GLenum>(formatInfo.format);
    GLenum dataType = static_cast<GLenum>(formatInfo.dataType);

    // Special case for unit tests: R8G8B8A8Unorm is mapped to kCVPixelFormatType_32BGRA
    // This is done only for R8G8B8A8Unorm to address unit tests
    if (textureInteropCreateInfo.format == snap::rhi::PixelFormat::R8G8B8A8Unorm) {
        internalFormat = GL_RGBA;
        format = GL_BGRA;
        dataType = GL_UNSIGNED_BYTE;
    }

    CVOpenGLESTextureRef textureRef = nullptr;
    CVReturn err = CVOpenGLESTextureCacheCreateTextureFromImage(kCFAllocatorDefault,
                                                                glTextureCache,
                                                                pixelBuffer,
                                                                nullptr,
                                                                GL_TEXTURE_2D,
                                                                internalFormat,
                                                                textureInteropCreateInfo.size.width,
                                                                textureInteropCreateInfo.size.height,
                                                                format,
                                                                dataType,
                                                                0,
                                                                &textureRef);
#else
    if (!glTextureCache) {
        auto nsOpenGLContext = (__bridge NSOpenGLContext*)(gl::getActiveContext());

        CGLContextObj cglContext = [nsOpenGLContext CGLContextObj];
        CGLPixelFormatObj cglPixelFormat = CGLGetPixelFormat(cglContext);

        CVOpenGLTextureCacheRef textureCacheRef = nullptr;
        CVReturn err = CVOpenGLTextureCacheCreate(
            kCFAllocatorDefault, nullptr, cglContext, cglPixelFormat, nullptr, &textureCacheRef);
        if (err != kCVReturnSuccess) {
            snap::rhi::common::throwException("Failed to create CVGLTexture texture cache");
        }
        glTextureCache = textureCacheRef;
    }

    CVOpenGLTextureRef textureRef = nullptr;
    CVReturn err = CVOpenGLTextureCacheCreateTextureFromImage(
        kCFAllocatorDefault, glTextureCache, pixelBuffer, nullptr, &textureRef);
#endif

    if (err != kCVReturnSuccess) {
        snap::rhi::common::throwException("Failed to create OpenGL texture from image: " + std::to_string(err));
    }

    cvGLTexture = textureRef;

#if SNAP_RHI_OS_IOS()
    glTextureID = CVOpenGLESTextureGetName(cvGLTexture);
#else
    glTextureID = CVOpenGLTextureGetName(cvGLTexture);
#endif
}
#endif

#if SNAP_RHI_ENABLE_BACKEND_VULKAN
VkImage TextureInterop::getVulkanTexture(snap::rhi::Device* device) {
    if (vkImageWithMemory) {
        return vkImageWithMemory->getImage();
    }

    auto* vkDeviceImpl = static_cast<snap::rhi::backend::vulkan::Device*>(device);

    if (!pixelBuffer) {
        snap::rhi::common::throwException("Invalid pixel buffer for Vulkan interop");
    }

    IOSurfaceRef ioSurface = CVPixelBufferGetIOSurface(pixelBuffer);
    if (!ioSurface) {
        snap::rhi::common::throwException("Failed to get IOSurface from CVPixelBuffer");
    }

    // VK_EXT_metal_objects
    // https://docs.vulkan.org/features/latest/features/proposals/VK_EXT_metal_objects.html
    VkImportMetalIOSurfaceInfoEXT importSurfaceInfo = {};
    importSurfaceInfo.sType = VK_STRUCTURE_TYPE_IMPORT_METAL_IO_SURFACE_INFO_EXT;
    importSurfaceInfo.pNext = nullptr;
    importSurfaceInfo.ioSurface = ioSurface;

    snap::rhi::TextureCreateInfo textureCreateInfo = getTextureCreateInfo();
    vkImageWithMemory = std::make_unique<snap::rhi::backend::vulkan::ImageWithMemory>(
        vkDeviceImpl, textureCreateInfo, &importSurfaceInfo);

    return vkImageWithMemory->getImage();
}

VkDeviceMemory TextureInterop::getVulkanTextureMemory(snap::rhi::Device* device) {
    getVulkanTexture(device);
    return vkImageWithMemory->getImageMemory();
}
#endif

#if SNAP_RHI_ENABLE_BACKEND_METAL
void TextureInterop::loadMetalTexture(const id<MTLDevice>& mtlDevice) {
    if (!pixelBuffer) {
        snap::rhi::common::throwException("Invalid pixel buffer");
    }

    if (!metalTextureCache) {
        CVMetalTextureCacheRef textureCacheRef = nullptr;
        CVReturn err = CVMetalTextureCacheCreate(kCFAllocatorDefault, nullptr, mtlDevice, nullptr, &textureCacheRef);
        if (err != kCVReturnSuccess) {
            snap::rhi::common::throwException("Failed to create Metal texture cache: " + std::to_string(err));
        }
        metalTextureCache = textureCacheRef;
    }

    MTLPixelFormat mtlPixelFormat = snap::rhi::backend::metal::convertToMtlPixelFormat(textureInteropCreateInfo.format);

    if (mtlPixelFormat == MTLPixelFormatInvalid) {
        snap::rhi::common::throwException("Unsupported pixel format for Metal: " +
                                          std::to_string(static_cast<uint32_t>(textureInteropCreateInfo.format)));
    }

    CVMetalTextureRef textureRef = nullptr;
    CVReturn err = CVMetalTextureCacheCreateTextureFromImage(kCFAllocatorDefault,
                                                             metalTextureCache,
                                                             pixelBuffer,
                                                             nullptr,
                                                             mtlPixelFormat,
                                                             textureInteropCreateInfo.size.width,
                                                             textureInteropCreateInfo.size.height,
                                                             0,
                                                             &textureRef);

    if (err != kCVReturnSuccess) {
        snap::rhi::common::throwException("Failed to create CVMetalTexture texture from image: " + std::to_string(err));
    }

    cvMetalTexture = textureRef;
    metalTexture = CVMetalTextureGetTexture(cvMetalTexture);
}

const id<MTLTexture>& TextureInterop::getMetalTexture(const id<MTLDevice>& mtlDevice) {
    if (metalTexture == nil) {
        loadMetalTexture(mtlDevice);
    }
    return metalTexture;
}
#endif

std::span<const snap::rhi::TextureInterop::ImagePlane> TextureInterop::map(
    const snap::rhi::MemoryAccess access, snap::rhi::PlatformSyncHandle* platformSyncHandle) {
    if (!pixelBuffer) {
        snap::rhi::common::throwException("[TextureInterop::map] Invalid CVPixelBuffer");
    }

    if (platformSyncHandle) {
        auto* commonHandle =
            snap::rhi::backend::common::smart_cast<snap::rhi::backend::common::PlatformSyncHandle>(platformSyncHandle);
        if (const auto& fence = commonHandle->getFence(); fence) {
            fence->waitForComplete();
        }
    }

    // CoreVideo only supports two lock modes:
    // - kCVPixelBufferLock_ReadOnly for read-only access
    // - 0 (no flag) for write or read-write access (no separate write-only flag exists)
    currentLockFlags = (access == snap::rhi::MemoryAccess::Read) ? kCVPixelBufferLock_ReadOnly : 0;
    CVReturn result = CVPixelBufferLockBaseAddress(pixelBuffer, currentLockFlags);

    if (result != kCVReturnSuccess) {
        snap::rhi::common::throwException("[TextureInterop::map] Failed to lock CVPixelBuffer");
    }

    imagePlanes.clear();
    size_t planeCount = CVPixelBufferGetPlaneCount(pixelBuffer);

    if (planeCount == 0) {
        ImagePlane plane;
        plane.bytesPerRow = CVPixelBufferGetBytesPerRow(pixelBuffer);
        plane.bytesPerPixel = static_cast<uint32_t>(plane.bytesPerRow / CVPixelBufferGetWidth(pixelBuffer));
        plane.pixels = CVPixelBufferGetBaseAddress(pixelBuffer);
        imagePlanes.push_back(plane);
    } else {
        for (size_t i = 0; i < planeCount; ++i) {
            ImagePlane plane;
            plane.bytesPerRow = CVPixelBufferGetBytesPerRowOfPlane(pixelBuffer, i);
            plane.bytesPerPixel =
                static_cast<uint32_t>(plane.bytesPerRow / CVPixelBufferGetWidthOfPlane(pixelBuffer, i));
            plane.pixels = CVPixelBufferGetBaseAddressOfPlane(pixelBuffer, i);
            imagePlanes.push_back(plane);
        }
    }

    return std::span<const snap::rhi::TextureInterop::ImagePlane>(imagePlanes);
}

std::unique_ptr<snap::rhi::PlatformSyncHandle> TextureInterop::unmap() {
    if (!pixelBuffer) {
        snap::rhi::common::throwException("[TextureInterop::unmap] Invalid CVPixelBuffer");
    }

    CVPixelBufferUnlockBaseAddress(pixelBuffer, currentLockFlags);

    return std::make_unique<snap::rhi::backend::common::PlatformSyncHandle>(nullptr);
}
} // namespace snap::rhi::interop::Apple
