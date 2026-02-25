#include "snap/rhi/backend/metal/Sampler.h"
#include "snap/rhi/backend/metal/Device.h"
#include "snap/rhi/backend/metal/Utils.h"
#include "snap/rhi/common/Throw.h"

namespace {
MTLSamplerMinMagFilter convertToMtlMinMagFilter(const snap::rhi::SamplerMinMagFilter filter) {
    switch (filter) {
        case snap::rhi::SamplerMinMagFilter::Nearest:
            return MTLSamplerMinMagFilter::MTLSamplerMinMagFilterNearest;

        case snap::rhi::SamplerMinMagFilter::Linear:
            return MTLSamplerMinMagFilter::MTLSamplerMinMagFilterLinear;

        default:
            snap::rhi::common::throwException("[convertToMtlMinMagFilter] invalid filter");
    }
}

MTLSamplerMipFilter convertToMtlMipFilter(const snap::rhi::SamplerMipFilter filter) {
    switch (filter) {
        case snap::rhi::SamplerMipFilter::NotMipmapped:
            return MTLSamplerMipFilter::MTLSamplerMipFilterNotMipmapped;

        case snap::rhi::SamplerMipFilter::Nearest:
            return MTLSamplerMipFilter::MTLSamplerMipFilterNearest;

        case snap::rhi::SamplerMipFilter::Linear:
            return MTLSamplerMipFilter::MTLSamplerMipFilterLinear;

        default:
            snap::rhi::common::throwException("[convertToMtlMipFilter] invalid filter");
    }
}

MTLSamplerAddressMode convertToMtlAddressMode(const snap::rhi::WrapMode wrapMode,
                                              const snap::rhi::SamplerBorderColor border) {
    switch (wrapMode) {
        case snap::rhi::WrapMode::ClampToEdge:
            return MTLSamplerAddressMode::MTLSamplerAddressModeClampToEdge;
        case snap::rhi::WrapMode::Repeat:
            return MTLSamplerAddressMode::MTLSamplerAddressModeRepeat;
        case snap::rhi::WrapMode::MirrorRepeat:
            return MTLSamplerAddressMode::MTLSamplerAddressModeMirrorRepeat;
        case snap::rhi::WrapMode::ClampToBorderColor: {
            if (border == snap::rhi::SamplerBorderColor::ClampToZero) {
                return MTLSamplerAddressMode::MTLSamplerAddressModeClampToZero;
            } else {
                if (@available(macOS 10.12, ios 14.0, *)) {
                    return MTLSamplerAddressMode::MTLSamplerAddressModeClampToBorderColor;
                }

                snap::rhi::common::throwException(
                    "[convertToMtlAddressMode][available(macOS 10.12, ios 14.0)] ClampToBorderColor "
                    "doesn't suported for snap::rhi::backend::metal");
            }
        }

        default:
            snap::rhi::common::throwException("invalid filter type");
            break;
    }
}

API_AVAILABLE(macos(10.12), ios(14.0))
MTLSamplerBorderColor convertToBorderColor(const snap::rhi::SamplerBorderColor color) {
    switch (color) {
        case snap::rhi::SamplerBorderColor::TransparentBlack:
            return MTLSamplerBorderColorTransparentBlack;

        case snap::rhi::SamplerBorderColor::OpaqueBlack:
            return MTLSamplerBorderColorOpaqueBlack;

        case snap::rhi::SamplerBorderColor::OpaqueWhite:
            return MTLSamplerBorderColorOpaqueWhite;

        default:
            snap::rhi::common::throwException("[convertToBorderColor] invalid border color");
    }
}
} // unnamed namespace

namespace snap::rhi::backend::metal {
Sampler::Sampler(Device* mtlDevice, const snap::rhi::SamplerCreateInfo& info) : snap::rhi::Sampler(mtlDevice, info) {
    auto samplerDesc = [[MTLSamplerDescriptor alloc] init];

    if (info.borderColor != snap::rhi::SamplerBorderColor::ClampToZero) {
        if (@available(macOS 10.12, ios 14.0, *)) {
            samplerDesc.borderColor = convertToBorderColor(info.borderColor);
        }
    }

    if (info.anisotropyEnable) {
        samplerDesc.maxAnisotropy = static_cast<uint32_t>(info.maxAnisotropy);
    }
    samplerDesc.magFilter = convertToMtlMinMagFilter(info.magFilter);
    samplerDesc.minFilter = convertToMtlMinMagFilter(info.minFilter);
    samplerDesc.mipFilter = convertToMtlMipFilter(info.mipFilter);
    samplerDesc.normalizedCoordinates = info.unnormalizedCoordinates == false ? YES : NO;
    samplerDesc.sAddressMode = convertToMtlAddressMode(info.wrapU, info.borderColor);
    samplerDesc.tAddressMode = convertToMtlAddressMode(info.wrapV, info.borderColor);
    samplerDesc.rAddressMode = convertToMtlAddressMode(info.wrapW, info.borderColor);

    if (info.compareEnable) {
        samplerDesc.compareFunction = convertToMtlCompareFunc(info.compareFunction);
    } else {
        samplerDesc.compareFunction = MTLCompareFunctionNever;
    }

    samplerDesc.lodMinClamp = info.lodMin;
    samplerDesc.lodMaxClamp = info.lodMax;

    samplerDesc.supportArgumentBuffers = YES;

    sampler = [mtlDevice->getMtlDevice() newSamplerStateWithDescriptor:samplerDesc];
    if (@available(macOS 13.00, ios 16.0, *)) {
        gpuResourceID = sampler.gpuResourceID._impl;
    }
}

const id<MTLSamplerState>& Sampler::getSampler() const {
    return sampler;
}
} // namespace snap::rhi::backend::metal
