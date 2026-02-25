#include "snap/rhi/backend/metal/Texture.h"

#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/metal/Device.h"
#include "snap/rhi/backend/metal/TextureInterop.h"
#include "snap/rhi/backend/metal/Utils.h"
#include "snap/rhi/common/Throw.h"

namespace {
MTLTextureUsage getTextureUsage(const snap::rhi::TextureUsage& usage) {
    MTLTextureUsage mtlUsage = MTLTextureUsageUnknown;

    if ((usage & snap::rhi::TextureUsage::Sampled) != snap::rhi::TextureUsage::None) {
        mtlUsage |= MTLTextureUsageShaderRead;
    }

    if ((usage & snap::rhi::TextureUsage::ColorAttachment) != snap::rhi::TextureUsage::None) {
        mtlUsage |= MTLTextureUsageRenderTarget;
    }

    if ((usage & snap::rhi::TextureUsage::Storage) != snap::rhi::TextureUsage::None) {
        mtlUsage |= MTLTextureUsageShaderWrite;
    }

    if ((usage & snap::rhi::TextureUsage::DepthStencilAttachment) != snap::rhi::TextureUsage::None) {
        mtlUsage |= MTLTextureUsageRenderTarget;
    }

    return mtlUsage;
}

uint32_t getDepthSize(const snap::rhi::Extent3D& size, snap::rhi::TextureType textureType) {
    if (textureType == snap::rhi::TextureType::Texture2DArray) {
        return 1;
    }

    //    if (textureType == snap::rhi::TextureType::TextureCubemapArray) {
    //        assert(size.depth % 6 == 0);
    //        return 6;
    //    }

    return size.depth;
}
} // unnamed namespace

namespace snap::rhi::backend::metal {
Texture::Texture(Device* mtlDevice, const TextureViewCreateInfo& viewCreateInfo)
    : snap::rhi::Texture(
          mtlDevice,
          common::buildTextureCreateInfoFromView(viewCreateInfo),
          common::buildTextureViewCreateInfo(viewCreateInfo.texture->getViewCreateInfo(), viewCreateInfo)) {
    auto* srcTexture = snap::rhi::backend::common::smart_cast<Texture>(viewInfo.texture);
    textureViewCache = srcTexture->getTextureViewCache();
    auto texInfo = textureViewCache->acquire(this->viewInfo.viewInfo);
    mtlTexture = texInfo.texture;
    gpuResourceID = texInfo.resourceID;
}

Texture::Texture(Device* mtlDevice, const id<MTLTexture>& mtlTexture, const snap::rhi::TextureCreateInfo& info)
    : snap::rhi::Texture(mtlDevice, info), mtlTexture(mtlTexture) {
    textureViewCache = std::make_shared<TextureViewCache>(mtlDevice, mtlTexture);

    if (@available(macOS 13.00, ios 16.0, *)) {
        gpuResourceID = mtlTexture.gpuResourceID._impl;
    }
}

Texture::Texture(Device* mtlDevice, const snap::rhi::TextureCreateInfo& info) : snap::rhi::Texture(mtlDevice, info) {
    MTLTextureDescriptor* textureDescriptor = [[MTLTextureDescriptor alloc] init];

    /**
     * https://developer.apple.com/documentation/metal/mtldevice/1433355-supportstexturesamplecount
     * 1 All devices
     * 2 All iOS and tvOS devices; some macOS devices
     * 4 All devices
     * 8 Some macOS devices
     */
    NSUInteger sampleCount = static_cast<uint32_t>(info.sampleCount);
    BOOL support = [mtlDevice->getMtlDevice() supportsTextureSampleCount:sampleCount];
    if (support == NO) {
        snap::rhi::common::throwException("unsupported sample count");
    }

    textureDescriptor.pixelFormat = convertToMtlPixelFormat(info.format);
    textureDescriptor.textureType = toMtlTextureType(info.textureType, static_cast<uint32_t>(info.sampleCount));
    textureDescriptor.width = info.size.width;
    textureDescriptor.height = info.size.height;
    textureDescriptor.depth = getDepthSize(info.size, info.textureType);
    textureDescriptor.arrayLength = getArrayLevelCount(info);
    textureDescriptor.mipmapLevelCount = info.mipLevels;
    textureDescriptor.sampleCount = static_cast<uint32_t>(info.sampleCount);
    textureDescriptor.usage = getTextureUsage(info.textureUsage);
    textureDescriptor.resourceOptions = MTLResourceStorageModePrivate;

    if (info.format == snap::rhi::PixelFormat::Grayscale) {
        const auto& caps = device->getCapabilities();
        if (caps.isTextureFormatSwizzleSupported) {
            if (@available(macOS 10.15, ios 13.0, *)) {
                textureDescriptor.swizzle = MTLTextureSwizzleChannelsMake(
                    MTLTextureSwizzleRed, MTLTextureSwizzleRed, MTLTextureSwizzleRed, MTLTextureSwizzleOne);
            }
        } else {
            snap::rhi::common::throwException("[Texture::Texture()] PixelFormat::Grayscale doesn't supported");
        }
    }

    mtlTexture = [mtlDevice->getMtlDevice() newTextureWithDescriptor:textureDescriptor];
    textureViewCache = std::make_shared<TextureViewCache>(mtlDevice, mtlTexture);

    if (@available(macOS 13.00, ios 16.0, *)) {
        gpuResourceID = mtlTexture.gpuResourceID._impl;
    }
}

Texture::Texture(Device* mtlDevice, const std::shared_ptr<snap::rhi::TextureInterop>& textureInterop)
    : snap::rhi::Texture(mtlDevice, textureInterop) {
    auto* mtlTextureInterop =
        snap::rhi::backend::common::smart_dynamic_cast<snap::rhi::backend::metal::TextureInterop>(textureInterop.get());
    mtlTexture = mtlTextureInterop->getMetalTexture(mtlDevice->getMtlDevice());
    textureViewCache = std::make_shared<TextureViewCache>(mtlDevice, mtlTexture);

    if (@available(macOS 13.00, ios 16.0, *)) {
        gpuResourceID = mtlTexture.gpuResourceID._impl;
    }
}

void Texture::setDebugLabel(std::string_view label) {
#if SNAP_RHI_ENABLE_DEBUG_LABELS
    ObjectDebugMarkers::setDebugLabel(label);
    mtlTexture.label = [NSString stringWithUTF8String:label.data()];
#endif
}
} // namespace snap::rhi::backend::metal
