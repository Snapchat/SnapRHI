#include "snap/rhi/backend/metal/TextureViewCache.h"

#include "snap/rhi/backend/metal/Device.h"
#include "snap/rhi/backend/metal/Utils.h"

#include "snap/rhi/common/Throw.h"

namespace {
bool isIdentity(const snap::rhi::ComponentMapping& mapping) {
    return mapping == snap::rhi::DefaultComponentMapping;
}
} // namespace

namespace snap::rhi::backend::metal {
TextureViewCache::TextureViewCache(Device* mtlDevice, const id<MTLTexture>& texture)
    : common::TextureViewCache<id<MTLTexture>, TextureWithResourceID>(mtlDevice, texture) {}

TextureViewCache::~TextureViewCache() = default;

TextureWithResourceID TextureViewCache::createView(const snap::rhi::TextureViewInfo& key) const {
    id<MTLTexture> view = nil;

    const id<MTLTexture> base = getTexture();

    const MTLPixelFormat pixelFormat = convertToMtlPixelFormat(key.format);
    const MTLTextureType textureType = toMtlTextureType(key.textureType, static_cast<uint32_t>([base sampleCount]));

    const NSRange levelRange = NSMakeRange(key.range.baseMipLevel, key.range.levelCount);

    // For 3D textures, Metal uses depth planes (not array slices). Creating a view with slices != 1 is invalid.
    const bool is3D = ([base textureType] == MTLTextureType3D);
    const NSRange sliceRange = is3D ? NSMakeRange(0, 1) : NSMakeRange(key.range.baseArrayLayer, key.range.layerCount);

    const auto& caps = device->getCapabilities();
    const bool needsSwizzle = !isIdentity(key.components);

    if (needsSwizzle && caps.isTextureFormatSwizzleSupported) {
        if (@available(macOS 10.15, ios 13.0, *)) {
            MTLTextureSwizzleChannels swizzle = MTLTextureSwizzleChannelsMake(toMtlSwizzle(key.components.r),
                                                                              toMtlSwizzle(key.components.g),
                                                                              toMtlSwizzle(key.components.b),
                                                                              toMtlSwizzle(key.components.a));
            view = [base newTextureViewWithPixelFormat:pixelFormat
                                           textureType:textureType
                                                levels:levelRange
                                                slices:sliceRange
                                               swizzle:swizzle];
        } else {
            snap::rhi::common::throwException(
                "[Metal::TextureViewCache] texture swizzle requires macOS 10.15 / iOS 13.0+");
        }
    } else {
        view = [base newTextureViewWithPixelFormat:pixelFormat
                                       textureType:textureType
                                            levels:levelRange
                                            slices:sliceRange];
    }

    SNAP_RHI_VALIDATE(validationLayer,
                      view != nil,
                      snap::rhi::ReportLevel::CriticalError,
                      snap::rhi::ValidationTag::CreateOp,
                      "[Metal::TextureViewCache] failed to create texture view");

    uint64_t gpuResourceID = 0;
    if (@available(macOS 13.00, ios 16.0, *)) {
        gpuResourceID = view.gpuResourceID._impl;
    }

    return {
        .texture = view,
        .resourceID = gpuResourceID,
    };
}

} // namespace snap::rhi::backend::metal
