#include "snap/rhi/backend/metal/DescriptorSet.h"
#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/metal/Buffer.h"
#include "snap/rhi/backend/metal/CommandBuffer.h"
#include "snap/rhi/backend/metal/DescriptorPool.h"
#include "snap/rhi/backend/metal/Device.h"
#include "snap/rhi/backend/metal/RenderPipeline.h"
#include "snap/rhi/backend/metal/Sampler.h"
#include "snap/rhi/backend/metal/Texture.h"
#include "snap/rhi/backend/metal/Utils.h"

#include <unordered_set>

namespace {
[[maybe_unused]] bool isArgumentBufferValid(snap::rhi::DescriptorSetLayout* layout) {
    std::unordered_set<uint32_t> bindings{};

    const auto& argBufferInfo = layout->getCreateInfo();
    for (const auto& uniformInfo : argBufferInfo.bindings) {
        if (bindings.count(uniformInfo.binding)) {
            return false;
        }

        bindings.insert(uniformInfo.binding);
    }

    return true;
}

/**
 * https://developer.apple.com/documentation/metal/improving-cpu-performance-by-using-argument-buffers?language=objc
 * To directly encode the argument buffer resources on these Tier 2 devices,
 * write the MTLBuffer.gpuAddress property — and for other resource types
 * (samplers, textures, and acceleration structures),
 * the gpuResourceID property — into the corresponding structure member.
 * To encode offsets, treat these property values as uint64 types and add the offset to them.
 */
constexpr uint32_t GPUAdressSize = sizeof(uint64_t);
} // unnamed namespace

namespace snap::rhi::backend::metal {
DescriptorSet::DescriptorSet(Device* device, const snap::rhi::DescriptorSetCreateInfo& info)
    : snap::rhi::backend::common::DescriptorSet<>(device, info) {
    const auto& validationLayer = device->getValidationLayer();
    SNAP_RHI_VALIDATE(validationLayer,
                      isArgumentBufferValid(info.descriptorSetLayout),
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::DescriptorSetOp,
                      "[DescriptorSet::DescriptorSet] invalid DescriptorSetLayout for Argument Buffer.");
    const auto& mtlDevice = device->getMtlDevice();
    auto* descriptorPool = snap::rhi::backend::common::smart_cast<DescriptorPool>(info.descriptorPool);

    bool canFillArgumentBufferDirectly = false;
    if (@available(macOS 13.00, ios 16.0, *)) {
        canFillArgumentBufferDirectly = mtlDevice.argumentBuffersSupport == MTLArgumentBuffersTier2;
    }

    uint32_t encodedLength = 0;
    uint32_t alignment = 0;

    if (canFillArgumentBufferDirectly) {
        const auto bindingCount = info.descriptorSetLayout->getCreateInfo().bindings.size();
        encodedLength = static_cast<uint32_t>(bindingCount * GPUAdressSize);
        alignment = static_cast<uint32_t>(GPUAdressSize);
    } else {
        argumentEncoder = createArgumentEncoder(device, info.descriptorSetLayout->getCreateInfo());
        encodedLength = static_cast<uint32_t>([argumentEncoder encodedLength]);
        alignment = static_cast<uint32_t>([argumentEncoder alignment]);
    }

    argumentBufferSubRange = descriptorPool->allocateBuffer(encodedLength, alignment);
    if (!argumentBufferSubRange) {
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Warning,
                        snap::rhi::ValidationTag::DescriptorSetOp,
                        "[DescriptorSet::DescriptorSet] failed to allocate argument buffer from DescriptorPool.");
        return;
    }

    if (canFillArgumentBufferDirectly) {
        // Resolve raw pointer for direct writing
        auto* baseAddress = static_cast<std::byte*>([argumentBufferSubRange->buffer contents]);
        argumentBufferData = reinterpret_cast<uint64_t*>(baseAddress + argumentBufferSubRange->offset);
    } else {
        // Bind buffer to encoder
        [argumentEncoder setArgumentBuffer:argumentBufferSubRange->buffer offset:argumentBufferSubRange->offset];
    }

    const auto& layoutInfo = info.descriptorSetLayout->getCreateInfo();
    cachedInfos.resize(layoutInfo.bindings.size());
    for (size_t i = 0; i < layoutInfo.bindings.size(); ++i) {
        cachedInfos[i].usage =
            convertToResourceUsage(layoutInfo.bindings[i].stageBits, layoutInfo.bindings[i].descriptorType);
        cachedInfos[i].stages = convertToRenderStages(layoutInfo.bindings[i].stageBits);
    }
}

void DescriptorSet::reset() {
    snap::rhi::backend::common::DescriptorSet<>::reset();

    if constexpr (snap::rhi::backend::common::enableSlowSafetyChecks()) {
        std::byte* argumentBufferPtr = static_cast<std::byte*>([argumentBufferSubRange->buffer contents]);
        memset(argumentBufferPtr + argumentBufferSubRange->offset, 0, argumentBufferSubRange->size);
    }
}

void DescriptorSet::bindUniformBuffer(const uint32_t binding,
                                      snap::rhi::Buffer* buffer,
                                      const uint64_t offset,
                                      const uint64_t range) {
    SNAP_RHI_VALIDATE(validationLayer,
                      offset % device->getCapabilities().minUniformBufferOffsetAlignment == 0,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::DescriptorSetOp,
                      "[RenderCommandEncoder::bindUniformBuffer] wrong @offset alignment.");

    // FAST PATH: Bindless (Direct Pointer Write)
    if (argumentBufferData) {
        // No function calls = No stack frame = Super fast.
        auto mtlBuffer = snap::rhi::backend::common::smart_cast<snap::rhi::backend::metal::Buffer>(buffer);
        const auto gpuAddress = mtlBuffer->getGPUAddress() + offset;
        setResource(binding, buffer, gpuAddress);
        return;
    }

    // SLOW PATH => Encoder
    // We only jump to the heavy function if absolutely necessary.
    bindBufferSlow(argumentEncoder, binding, buffer, offset);
}

void DescriptorSet::bindStorageBuffer(const uint32_t binding,
                                      snap::rhi::Buffer* buffer,
                                      const uint64_t offset,
                                      const uint64_t range) {
    // FAST PATH: Bindless (Direct Pointer Write)
    if (argumentBufferData) {
        // No function calls = No stack frame = Super fast.
        auto mtlBuffer = snap::rhi::backend::common::smart_cast<snap::rhi::backend::metal::Buffer>(buffer);
        const auto gpuAddress = mtlBuffer->getGPUAddress() + offset;
        setResource(binding, buffer, gpuAddress);
        return;
    }

    // SLOW PATH => Encoder
    // We only jump to the heavy function if absolutely necessary.
    bindBufferSlow(argumentEncoder, binding, buffer, offset);
}

void DescriptorSet::bindTexture(const uint32_t binding, snap::rhi::Texture* texture) {
    tryPreserveInteropTexture(texture);
    SNAP_RHI_VALIDATE(validationLayer,
                      (texture->getCreateInfo().textureUsage & snap::rhi::TextureUsage::Sampled) ==
                          snap::rhi::TextureUsage::Sampled,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::DescriptorSetOp,
                      "[DescriptorSet::bindTexture] textureUsage should contain TextureUsage::Sampled bit.");

    // FAST PATH: Bindless (Direct Pointer Write)
    if (argumentBufferData) {
        // No function calls = No stack frame = Super fast.
        auto mtlTexture = snap::rhi::backend::common::smart_cast<snap::rhi::backend::metal::Texture>(texture);
        const auto gpuResourceID = mtlTexture->getGPUResourceID();
        setResource(binding, texture, gpuResourceID);
        return;
    }

    // SLOW PATH => Encoder
    // We only jump to the heavy function if absolutely necessary.
    bindTextureSlow(argumentEncoder, binding, texture);
}

void DescriptorSet::bindStorageTexture(const uint32_t binding, snap::rhi::Texture* texture, uint32_t mipLevel) {
    tryPreserveInteropTexture(texture);
    SNAP_RHI_VALIDATE(validationLayer,
                      (texture->getCreateInfo().textureUsage & snap::rhi::TextureUsage::Storage) ==
                          snap::rhi::TextureUsage::Storage,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::DescriptorSetOp,
                      "[DescriptorSet::bindStorageTexture] textureUsage should contain TextureUsage::Storage bit.");

    auto* mtlTexture = snap::rhi::backend::common::smart_cast<snap::rhi::backend::metal::Texture>(texture);
    const auto& viewCreateInfo = mtlTexture->getViewCreateInfo();

    snap::rhi::TextureViewInfo viewInfo = viewCreateInfo.viewInfo;
    viewInfo.range.baseMipLevel += mipLevel;
    viewInfo.range.levelCount = 1;

    const auto mipView = mtlTexture->getTextureViewCache()->acquire(viewInfo);

    // FAST PATH: Bindless (Direct Pointer Write)
    if (argumentBufferData) {
        // We must write the gpuResourceID of the *view* to bind a single-mip storage texture.
        uint64_t viewResourceID = mipView.resourceID;
        setResource(binding, texture, viewResourceID);
        return;
    }

    // SLOW PATH => Encoder
    if (argumentEncoder) {
        [argumentEncoder setTexture:mipView.texture atIndex:binding];
        setResource(binding, texture);
    }
}

void DescriptorSet::bindSampler(const uint32_t binding, snap::rhi::Sampler* sampler) {
    // FAST PATH: Bindless (Direct Pointer Write)
    if (argumentBufferData) {
        // No function calls = No stack frame = Super fast.
        auto mtlSampler = snap::rhi::backend::common::smart_cast<snap::rhi::backend::metal::Sampler>(sampler);
        const auto gpuResourceID = mtlSampler->getGPUResourceID();
        setResource(binding, sampler, gpuResourceID);
        return;
    }

    // SLOW PATH => Encoder
    // We only jump to the heavy function if absolutely necessary.
    bindSamplerSlow(argumentEncoder, binding, sampler);
}

SNAP_RHI_NO_INLINE
void DescriptorSet::bindTextureSlow(const id<MTLArgumentEncoder>& argumentEncoder,
                                    uint32_t binding,
                                    snap::rhi::Texture* texture) {
    // The compiler will put the heavy 'stp x29, x30...' prologue HERE,
    // keeping your main function clean.
    if (argumentEncoder) {
        auto* mtlTex = static_cast<snap::rhi::backend::metal::Texture*>(texture);
        [argumentEncoder setTexture:mtlTex->getTexture() atIndex:binding];

        setResource(binding, texture);
    }
}

SNAP_RHI_NO_INLINE
void DescriptorSet::bindSamplerSlow(const id<MTLArgumentEncoder>& argumentEncoder,
                                    uint32_t binding,
                                    snap::rhi::Sampler* sampler) {
    // The compiler will put the heavy 'stp x29, x30...' prologue HERE,
    // keeping your main function clean.
    if (argumentEncoder) {
        auto* mtlSampler = static_cast<snap::rhi::backend::metal::Sampler*>(sampler);
        [argumentEncoder setSamplerState:mtlSampler->getSampler() atIndex:binding];

        setResource(binding, sampler);
    }
}

SNAP_RHI_NO_INLINE
void DescriptorSet::bindBufferSlow(const id<MTLArgumentEncoder>& argumentEncoder,
                                   uint32_t binding,
                                   snap::rhi::Buffer* buffer,
                                   const uint32_t offset) {
    // The compiler will put the heavy 'stp x29, x30...' prologue HERE,
    // keeping your main function clean.
    if (argumentEncoder) {
        auto* mtlBuffer = static_cast<snap::rhi::backend::metal::Buffer*>(buffer);
        [argumentEncoder setBuffer:mtlBuffer->getBuffer() offset:offset atIndex:binding];

        setResource(binding, buffer);
    }
}

void DescriptorSet::updateDescriptorSet(std::span<snap::rhi::Descriptor> descriptorWrites) {
    // FAST PATH: Bindless (Direct Pointer Write)
    if (argumentBufferData) {
        for (const auto& desc : descriptorWrites) {
            uint64_t gpuAddress = 0;
            snap::rhi::DeviceChild* resource = nullptr;
            switch (desc.descriptorType) {
                case snap::rhi::DescriptorType::UniformBuffer:
                case snap::rhi::DescriptorType::StorageBuffer: {
                    const auto& bufferInfo = desc.bufferInfo;
                    auto mtlBuffer =
                        snap::rhi::backend::common::smart_cast<snap::rhi::backend::metal::Buffer>(bufferInfo.buffer);
                    gpuAddress = mtlBuffer->getGPUAddress() + bufferInfo.offset;
                    resource = bufferInfo.buffer;
                } break;

                case snap::rhi::DescriptorType::SampledTexture: {
                    const auto& sampledTextureInfo = desc.sampledTextureInfo;
                    auto mtlTexture = snap::rhi::backend::common::smart_cast<snap::rhi::backend::metal::Texture>(
                        sampledTextureInfo.texture);
                    gpuAddress = mtlTexture->getGPUResourceID();
                    resource = sampledTextureInfo.texture;
                } break;

                case snap::rhi::DescriptorType::StorageTexture: {
                    const auto& storedTextureInfo = desc.storageTextureInfo;
                    auto mtlTexture = snap::rhi::backend::common::smart_cast<snap::rhi::backend::metal::Texture>(
                        storedTextureInfo.texture);
                    gpuAddress = mtlTexture->getGPUResourceID();
                    resource = storedTextureInfo.texture;
                } break;

                case snap::rhi::DescriptorType::Sampler: {
                    const auto& samplerInfo = desc.samplerInfo;
                    auto mtlSampler =
                        snap::rhi::backend::common::smart_cast<snap::rhi::backend::metal::Sampler>(samplerInfo.sampler);
                    gpuAddress = mtlSampler->getGPUResourceID();
                    resource = samplerInfo.sampler;
                } break;

                default:
                    break;
            }

            setResource(desc.binding, resource, gpuAddress);
        }
        return;
    }

    // SLOW PATH => Encoder
    snap::rhi::backend::common::DescriptorSet<>::updateDescriptorSet(descriptorWrites);
}

void DescriptorSet::useResources(Context& context) {
    const auto& layoutInfo = info.descriptorSetLayout->getCreateInfo();
    const auto& descriptorInfos = layoutInfo.bindings;

    for (int32_t i = 0; i < descriptors.size(); ++i) {
        const auto& desc = descriptors[i];
        if (desc.resourcePtr == nullptr) {
            continue;
        }

        const auto usage = cachedInfos[i].usage;
        const auto stages = cachedInfos[i].stages;

        switch (descriptorInfos[i].descriptorType) {
            case snap::rhi::DescriptorType::UniformBuffer:
            case snap::rhi::DescriptorType::StorageBuffer: {
                auto* buffer =
                    snap::rhi::backend::common::smart_cast<snap::rhi::backend::metal::Buffer>(desc.resourcePtr);
                context.useResource(buffer->getBuffer(), usage, stages);
            } break;

            case snap::rhi::DescriptorType::SampledTexture:
            case snap::rhi::DescriptorType::StorageTexture: {
                auto* texture =
                    snap::rhi::backend::common::smart_cast<snap::rhi::backend::metal::Texture>(desc.resourcePtr);
                context.useResource(texture->getTexture(), usage, stages);
            } break;

            default: {
                // Do nothing for samplers
            } break;
        }
    }
}

void DescriptorSet::setDebugLabel(std::string_view label) {
#if SNAP_RHI_ENABLE_DEBUG_LABELS
    ObjectDebugMarkers::setDebugLabel(label);
    argumentEncoder.label = [NSString stringWithUTF8String:label.data()];
#endif
}
} // namespace snap::rhi::backend::metal
