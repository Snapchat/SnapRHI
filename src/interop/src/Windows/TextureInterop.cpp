#include "TextureInterop.h"
#include "snap/rhi/backend/common/Utils.hpp"

#if SNAP_RHI_ENABLE_BACKEND_OPENGL
#include "snap/rhi/backend/opengl/Device.hpp"
#include "snap/rhi/backend/opengl/Profile.hpp"
#include <OpenGL/Context.h>
#endif

#if SNAP_RHI_ENABLE_BACKEND_VULKAN
#include "snap/rhi/backend/vulkan/Device.h"
#endif
#include "snap/rhi/common/Throw.h"
#include <mutex>
#include <snap/rhi/common/Scope.h>

#include "snap/rhi/Device.hpp"

#if SNAP_RHI_ENABLE_BACKEND_VULKAN
static PFN_vkGetMemoryWin32HandleKHR vkGetMemoryWin32HandleKHR_internal = nullptr;

void initVulkanWin32Extensions(VkInstance instance) {
    static std::once_flag initFlag;
    std::call_once(initFlag, [instance]() {
        if (!vkGetMemoryWin32HandleKHR_internal && instance) {
            vkGetMemoryWin32HandleKHR_internal =
                (PFN_vkGetMemoryWin32HandleKHR)vkGetInstanceProcAddr(instance, "vkGetMemoryWin32HandleKHR");
        }
    });
}
#endif

namespace snap::rhi::interop::Windows {
TextureInterop::TextureInterop(const TextureInteropCreateInfo& info)
    : snap::rhi::backend::common::TextureInterop(info) {
#if SNAP_RHI_ENABLE_BACKEND_OPENGL
    compatibleAPIs.push_back(snap::rhi::API::OpenGL);
#endif

#if SNAP_RHI_ENABLE_BACKEND_VULKAN
    compatibleAPIs.push_back(snap::rhi::API::Vulkan);
#endif
}

TextureInterop::~TextureInterop() {}

#if SNAP_RHI_ENABLE_BACKEND_OPENGL
GLuint TextureInterop::getOpenGLTexture(const snap::rhi::backend::opengl::Profile& gl) {
    if (glTextureID == 0) {
        loadGLTexture(gl);
    }
    return glTextureID;
}

snap::rhi::backend::opengl::TextureTarget TextureInterop::getOpenGLTextureTarget(
    const snap::rhi::backend::opengl::Profile& gl) {
    return snap::rhi::backend::opengl::TextureTarget::Texture2D;
}

void TextureInterop::loadGLTexture(const snap::rhi::backend::opengl::Profile& gl) {
#if SNAP_RHI_ENABLE_BACKEND_VULKAN
    if (!imageWithMemory) {
        snap::rhi::common::throwException("Vulkan external memory not available - call getVulkanTexture first");
    }

    VkDevice vkDevice = cachedVulkanDevice->getVkLogicalDevice();
    VkDeviceMemory vkMemory = imageWithMemory->getImageMemory();

    VkMemoryGetWin32HandleInfoKHR getWin32HandleInfo{};
    getWin32HandleInfo.sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
    getWin32HandleInfo.memory = vkMemory;
    getWin32HandleInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;

    HANDLE memoryHandle = nullptr;
    VkResult result = vkGetMemoryWin32HandleKHR_internal(vkDevice, &getWin32HandleInfo, &memoryHandle);
    if (result != VK_SUCCESS) {
        snap::rhi::common::throwException("Failed to get Win32 handle from Vulkan memory");
    }

    snap::rhi::TextureCreateInfo textureCreateInfo = getTextureCreateInfo();

    const auto& device = gl.getDevice();
    rhiTexture = device->createTexture(textureCreateInfo);

    snap::rhi::backend::opengl::Texture* nativeTexture =
        snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Texture>(rhiTexture.get());
    glTextureID = static_cast<GLuint>(nativeTexture->getTextureID(nullptr));

    VkImage vkImage = imageWithMemory->getImage();
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(vkDevice, vkImage, &memRequirements);

    GLuint memoryObject;
    glCreateMemoryObjectsEXT(1, &memoryObject);
    SNAP_RHI_ON_SCOPE_EXIT {
        gl.bindTexture(snap::rhi::backend::opengl::TextureTarget::Texture2D,
                       static_cast<snap::rhi::backend::opengl::TextureId>(0),
                       nullptr);
        glDeleteMemoryObjectsEXT(1, &memoryObject);
    };

    glImportMemoryWin32HandleEXT(memoryObject, memRequirements.size, GL_HANDLE_TYPE_OPAQUE_WIN32_EXT, memoryHandle);

    gl.bindTexture(snap::rhi::backend::opengl::TextureTarget::Texture2D,
                   static_cast<snap::rhi::backend::opengl::TextureId>(glTextureID),
                   nullptr);

    const auto& formatInfo = gl.getTextureFormat(textureCreateInfo.format);
    GLenum glInternalFormat = static_cast<GLenum>(formatInfo.internalFormat);

    glTexStorageMem2DEXT(GL_TEXTURE_2D,
                         1,
                         glInternalFormat,
                         textureInteropCreateInfo.size.width,
                         textureInteropCreateInfo.size.height,
                         memoryObject,
                         0);

#else
    snap::rhi::common::throwException("OpenGL texture interop requires Vulkan support to be enabled");
#endif
}
#endif

#if SNAP_RHI_ENABLE_BACKEND_VULKAN
VkImage TextureInterop::getVulkanTexture(snap::rhi::Device* device) {
    if (!imageWithMemory) {
        loadVkTexture(device);
    }
    return imageWithMemory->getImage();
}

VkDeviceMemory TextureInterop::getVulkanTextureMemory(snap::rhi::Device* device) {
    if (!imageWithMemory) {
        loadVkTexture(device);
    }
    return imageWithMemory->getImageMemory();
}

void TextureInterop::loadVkTexture(snap::rhi::Device* rhiDevice) {
    auto* vkDeviceImpl = snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::Device>(rhiDevice);

    cachedVulkanDevice = vkDeviceImpl;

    initVulkanWin32Extensions(vkDeviceImpl->getVkInstance());

    VkExternalMemoryImageCreateInfo externalMemoryImageInfo{};
    externalMemoryImageInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
    externalMemoryImageInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;

    VkExportMemoryAllocateInfo exportAllocInfo{};
    exportAllocInfo.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
    exportAllocInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;

    snap::rhi::TextureCreateInfo textureCreateInfo = getTextureCreateInfo();

    imageWithMemory = std::make_unique<snap::rhi::backend::vulkan::ImageWithMemory>(
        vkDeviceImpl, textureCreateInfo, &externalMemoryImageInfo, &exportAllocInfo);
}

#endif // SNAP_RHI_ENABLE_BACKEND_VULKAN
} // namespace snap::rhi::interop::Windows
