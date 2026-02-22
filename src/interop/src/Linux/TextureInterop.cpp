#include "TextureInterop.h"

#include "snap/rhi/Device.hpp"
#include "snap/rhi/backend/common/Utils.hpp"

#if SNAP_RHI_ENABLE_BACKEND_OPENGL
#include "snap/rhi/backend/opengl/Device.hpp"
#include "snap/rhi/backend/opengl/Profile.hpp"
#include <EGL/egl.h>

#endif // SNAP_RHI_ENABLE_BACKEND_OPENGL

#if SNAP_RHI_ENABLE_BACKEND_VULKAN
#include "snap/rhi/backend/vulkan/Device.h"
#endif
#include "snap/rhi/common/Throw.h"
#include <snap/rhi/common/Scope.h>

namespace snap::rhi::interop::Linux {
TextureInterop::TextureInterop(const TextureInteropCreateInfo& info)
    : snap::rhi::backend::common::TextureInterop(info) {
#if SNAP_RHI_ENABLE_BACKEND_OPENGL
    compatibleAPIs.push_back(snap::rhi::API::OpenGL);
#endif

#if SNAP_RHI_ENABLE_BACKEND_VULKAN
    compatibleAPIs.push_back(snap::rhi::API::Vulkan);
#endif
}

#if SNAP_RHI_ENABLE_BACKEND_VULKAN
TextureInterop::TextureInterop(const TextureInteropCreateInfo& info,
                               std::shared_ptr<ExternalVulkanResourceInfo> extResources)
    : snap::rhi::backend::common::TextureInterop(info), externalResources(std::move(extResources)) {
#if SNAP_RHI_ENABLE_BACKEND_OPENGL
    compatibleAPIs.push_back(snap::rhi::API::OpenGL);
#endif
    compatibleAPIs.push_back(snap::rhi::API::Vulkan);

    if (!externalResources) {
        snap::rhi::common::throwException("External resources cannot be null");
    }
    if (externalResources->device == VK_NULL_HANDLE) {
        snap::rhi::common::throwException("External Vulkan device cannot be VK_NULL_HANDLE");
    }
    if (externalResources->image == VK_NULL_HANDLE) {
        snap::rhi::common::throwException("External Vulkan image cannot be VK_NULL_HANDLE");
    }
    if (externalResources->memoryFD < 0) {
        snap::rhi::common::throwException("External memory FD must be a valid file descriptor");
    }

    cachedVulkanDevice = externalResources->device;
}
#endif

TextureInterop::~TextureInterop() {}

#if SNAP_RHI_ENABLE_BACKEND_OPENGL
GLuint TextureInterop::getOpenGLTexture(const snap::rhi::backend::opengl::Profile& gl) {
    if (glTextureID == 0) {
        loadGLTexture(gl);
    }
    return glTextureID;
}

void TextureInterop::loadGLTexture(const snap::rhi::backend::opengl::Profile& gl) {
#if SNAP_RHI_ENABLE_BACKEND_VULKAN
    if (!imageWithMemory && !externalResources) {
        snap::rhi::common::throwException("Vulkan external memory not available - call getVulkanTexture first");
    }

    int memoryFd = -1;
    VkDeviceSize memorySize = 0;

    if (externalResources && externalResources->memoryFD >= 0) {
        // Use pre-exported FD directly from external resources
        memoryFd = externalResources->memoryFD;
        memorySize = externalResources->memorySize;
        // Mark FD as consumed (ownership transferred to GL)
        externalResources->memoryFD = -1;
    } else {
        VkMemoryGetFdInfoKHR getFdInfo{};
        getFdInfo.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
        getFdInfo.memory = imageWithMemory->getImageMemory();
        getFdInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;

        if (vkGetMemoryFdKHR(cachedVulkanDevice, &getFdInfo, &memoryFd) != VK_SUCCESS) {
            snap::rhi::common::throwException("Failed to get file descriptor from Vulkan memory");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(cachedVulkanDevice, imageWithMemory->getImage(), &memRequirements);
        memorySize = memRequirements.size;
    }

    snap::rhi::TextureCreateInfo texCreateInfo = getTextureCreateInfo();

    glCreateMemoryObjectsEXT(1, &glMemoryObject);
    glImportMemoryFdEXT(glMemoryObject, memorySize, GL_HANDLE_TYPE_OPAQUE_FD_EXT, memoryFd);

    glGenTextures(1, &glTextureID);

    const bool isArrayTexture = texCreateInfo.textureType == snap::rhi::TextureType::Texture2DArray;
    const bool is3DTexture = texCreateInfo.textureType == snap::rhi::TextureType::Texture3D;
    const GLenum glTextureTarget = isArrayTexture ? GL_TEXTURE_2D_ARRAY : (is3DTexture ? GL_TEXTURE_3D : GL_TEXTURE_2D);

    glBindTexture(glTextureTarget, glTextureID);

    const auto& formatInfo = gl.getTextureFormat(texCreateInfo.format);
    GLenum glInternalFormat = static_cast<GLenum>(formatInfo.internalFormat);

    if (isArrayTexture || is3DTexture) {
        glTexStorageMem3DEXT(glTextureTarget,
                             1,
                             glInternalFormat,
                             texCreateInfo.size.width,
                             texCreateInfo.size.height,
                             texCreateInfo.size.depth,
                             glMemoryObject,
                             0);
    } else {
        glTexStorageMem2DEXT(glTextureTarget,
                             1,
                             glInternalFormat,
                             texCreateInfo.size.width,
                             texCreateInfo.size.height,
                             glMemoryObject,
                             0);
    }

    glBindTexture(glTextureTarget, 0);
#else
    snap::rhi::common::throwException("OpenGL texture interop requires Vulkan support to be enabled");
#endif
}

snap::rhi::backend::opengl::TextureTarget TextureInterop::getOpenGLTextureTarget(
    const snap::rhi::backend::opengl::Profile& gl) {
    switch (textureCreateInfo.textureType) {
        case snap::rhi::TextureType::Texture2DArray:
            return snap::rhi::backend::opengl::TextureTarget::Texture2DArray;
        case snap::rhi::TextureType::Texture3D:
            return snap::rhi::backend::opengl::TextureTarget::Texture3D;
        default:
            return snap::rhi::backend::opengl::TextureTarget::Texture2D;
    }
}
#endif

#if SNAP_RHI_ENABLE_BACKEND_VULKAN
VkImage TextureInterop::getVulkanTexture(snap::rhi::Device* device) {
    if (externalResources) {
        return externalResources->image;
    }
    if (!imageWithMemory) {
        loadVkTexture(device);
    }
    return imageWithMemory->getImage();
}

VkDeviceMemory TextureInterop::getVulkanTextureMemory(snap::rhi::Device* device) {
    if (externalResources) {
        return VK_NULL_HANDLE; // External resources manage their own memory
    }
    if (!imageWithMemory) {
        loadVkTexture(device);
    }
    return imageWithMemory->getImageMemory();
}

void TextureInterop::loadVkTexture(snap::rhi::Device* rhiDevice) {
    auto vkDeviceImpl = snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::Device>(rhiDevice);

    if (!vkDeviceImpl) {
        snap::rhi::common::throwException("Failed to get Vulkan device from SnapRHI device");
    }

    cachedVulkanDevice = vkDeviceImpl->getVkLogicalDevice();

    VkExternalMemoryImageCreateInfo externalMemoryImageInfo{};
    externalMemoryImageInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
    externalMemoryImageInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;

    VkExportMemoryAllocateInfo exportAllocInfo{};
    exportAllocInfo.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
    exportAllocInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;

    snap::rhi::TextureCreateInfo textureCreateInfo = getTextureCreateInfo();

    imageWithMemory = std::make_unique<snap::rhi::backend::vulkan::ImageWithMemory>(
        vkDeviceImpl, textureCreateInfo, &externalMemoryImageInfo, &exportAllocInfo);
}
#endif // SNAP_RHI_ENABLE_BACKEND_VULKAN

} // namespace snap::rhi::interop::Linux
