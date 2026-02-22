#include "TextureInterop.h"

#include "snap/rhi/Device.hpp"
#include "snap/rhi/backend/common/Utils.hpp"

#if SNAP_RHI_ENABLE_BACKEND_OPENGL
#include "snap/rhi/backend/opengl/Device.hpp"
#include "snap/rhi/backend/opengl/Profile.hpp"
#endif

#if SNAP_RHI_ENABLE_BACKEND_VULKAN
#include "snap/rhi/backend/vulkan/Device.h"
#include "snap/rhi/backend/vulkan/Texture.h"
#endif

namespace snap::rhi::interop::Other {
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
    if (!glTexture) {
        createGLTexture(gl.getDevice());
    }

    snap::rhi::backend::opengl::Texture* nativeTexture =
        snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Texture>(glTexture.get());
    auto textureId = static_cast<GLuint>(nativeTexture->getTextureID(nullptr));
    return textureId;
}

snap::rhi::backend::opengl::TextureTarget TextureInterop::getOpenGLTextureTarget(
    const snap::rhi::backend::opengl::Profile& gl) {
    return snap::rhi::backend::opengl::TextureTarget::Texture2D;
}
#endif

#if SNAP_RHI_ENABLE_BACKEND_VULKAN
VkImage TextureInterop::getVulkanTexture(snap::rhi::Device* device) {
    if (!vkImageWithMemory) {
        createVkTexture(device);
    }
    return vkImageWithMemory->getImage();
}

VkDeviceMemory TextureInterop::getVulkanTextureMemory(snap::rhi::Device* device) {
    if (!vkImageWithMemory) {
        createVkTexture(device);
    }
    return vkImageWithMemory->getImageMemory();
}

void TextureInterop::createVkTexture(snap::rhi::Device* device) {
    if (vkImageWithMemory) {
        return;
    }

    auto* vkDevice = snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::Device>(device);
    if (!vkDevice) {
        return;
    }

    snap::rhi::TextureCreateInfo textureCreateInfo = getTextureCreateInfo();
    vkImageWithMemory = std::make_shared<snap::rhi::backend::vulkan::ImageWithMemory>(vkDevice, textureCreateInfo);
    vkTexture = std::make_shared<snap::rhi::backend::vulkan::Texture>(vkDevice, textureCreateInfo, vkImageWithMemory);
}
#endif

void TextureInterop::createGLTexture(snap::rhi::Device* device) {
#if SNAP_RHI_ENABLE_BACKEND_OPENGL
    if (glTexture) {
        return;
    }

    auto* glDevice = snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Device>(device);
    if (!glDevice) {
        return;
    }

    snap::rhi::TextureCreateInfo textureCreateInfo = getTextureCreateInfo();
    glTexture = glDevice->createTexture(textureCreateInfo);
#endif
}

void TextureInterop::copyBetweenTextures(snap::rhi::Texture* sourceTexture, snap::rhi::Texture* targetTexture) {
    if (!sourceTexture || !targetTexture) {
        return;
    }

    auto* device = sourceTexture->getDevice();
    auto* commandQueue = device->getCommandQueue(0, 0);
    auto commandBuffer = device->createCommandBuffer({.commandQueue = commandQueue});
    auto* blitEncoder = commandBuffer->getBlitCommandEncoder();

    blitEncoder->beginEncoding();

    snap::rhi::TextureCopy copyInfo{};
    copyInfo.srcSubresource.mipLevel = 0;
    copyInfo.dstSubresource.mipLevel = 0;
    copyInfo.extent = {
        .width = textureInteropCreateInfo.size.width, .height = textureInteropCreateInfo.size.height, .depth = 1};
    copyInfo.srcOffset = {0, 0, 0};
    copyInfo.dstOffset = {0, 0, 0};

    blitEncoder->copyTexture(sourceTexture, targetTexture, {&copyInfo, 1});
    blitEncoder->endEncoding();

    commandQueue->submitCommandBuffer(
        commandBuffer.get(), snap::rhi::CommandBufferWaitType::WaitUntilCompleted, nullptr);
}

} // namespace snap::rhi::interop::Other
