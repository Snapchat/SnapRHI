#include "snap/rhi/backend/noop/Device.hpp"

#include "snap/rhi/Device.hpp"
#include "snap/rhi/PipelineCache.hpp"
#include "snap/rhi/Sampler.hpp"
#include "snap/rhi/Semaphore.hpp"
#include "snap/rhi/Texture.hpp"
#include "snap/rhi/backend/common/Utils.hpp"

#include "snap/rhi/backend/common/PipelineLayout.h"
#include "snap/rhi/backend/noop/Buffer.hpp"
#include "snap/rhi/backend/noop/CommandBuffer.hpp"
#include "snap/rhi/backend/noop/CommandQueue.hpp"
#include "snap/rhi/backend/noop/ComputePipeline.hpp"
#include "snap/rhi/backend/noop/DescriptorPool.hpp"
#include "snap/rhi/backend/noop/DescriptorSet.hpp"
#include "snap/rhi/backend/noop/DescriptorSetLayout.hpp"
#include "snap/rhi/backend/noop/Fence.hpp"
#include "snap/rhi/backend/noop/Framebuffer.hpp"
#include "snap/rhi/backend/noop/PipelineCache.hpp"
#include "snap/rhi/backend/noop/RenderPipeline.hpp"
#include "snap/rhi/backend/noop/Sampler.hpp"
#include "snap/rhi/backend/noop/Semaphore.hpp"
#include "snap/rhi/backend/noop/ShaderLibrary.hpp"
#include "snap/rhi/backend/noop/ShaderModule.hpp"
#include "snap/rhi/backend/noop/Texture.hpp"

#include <span>

namespace snap::rhi::backend::noop {
Device::Device(const snap::rhi::backend::noop::DeviceCreateInfo& info)
    : snap::rhi::backend::common::DeviceContextless(info), mainQueue(this) {
    caps.apiDescription = {snap::rhi::API::NoOp, snap::rhi::APIVersionNone};

    caps.formatProperties.fill({});
    std::vector<snap::rhi::PixelFormat> formats = {
        snap::rhi::PixelFormat::R8Unorm,
        snap::rhi::PixelFormat::R8G8Unorm,
        snap::rhi::PixelFormat::R8G8B8A8Unorm,
        snap::rhi::PixelFormat::B8G8R8A8Unorm,
        snap::rhi::PixelFormat::R16Float,
    };
    for (auto format : formats) {
        auto& formatProperties = caps.formatProperties[static_cast<uint8_t>(format)];
        formatProperties.textureFeatures = static_cast<snap::rhi::FormatFeatures>(
            snap::rhi::FormatFeatures::SampledFilterLinear | snap::rhi::FormatFeatures::TransferDst |
            snap::rhi::FormatFeatures::TransferSrc | snap::rhi::FormatFeatures::BlitDst |
            snap::rhi::FormatFeatures::BlitSrc | snap::rhi::FormatFeatures::ColorRenderableBlend |
            snap::rhi::FormatFeatures::Resolve);
    }
}

snap::rhi::CommandQueue* Device::getCommandQueue(const uint32_t queueFamilyIndex, const uint32_t queueIndex) {
    SNAP_RHI_VALIDATE(validationLayer,
                      (queueFamilyIndex == 0) && (queueIndex == 0),
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::CommandQueueOp,
                      "[NoOp::Device::getCommandQueue] queueFamilyIndex{%d}, queueIndex{%d}",
                      queueFamilyIndex,
                      queueIndex);
    return &mainQueue;
}

std::shared_ptr<snap::rhi::CommandBuffer> Device::createCommandBuffer(const snap::rhi::CommandBufferCreateInfo& info) {
    return createResource<snap::rhi::CommandBuffer, snap::rhi::backend::noop::CommandBuffer>(this, info);
}

std::shared_ptr<snap::rhi::Sampler> Device::createSampler(const snap::rhi::SamplerCreateInfo& info) {
    return createResource<snap::rhi::Sampler, snap::rhi::backend::noop::Sampler>(this, info);
}

std::shared_ptr<snap::rhi::Semaphore> Device::createSemaphore(
    const snap::rhi::SemaphoreCreateInfo& info,
    const std::shared_ptr<snap::rhi::PlatformSyncHandle>& platformSyncHandle) {
    return createResource<snap::rhi::Semaphore, snap::rhi::backend::noop::Semaphore>(this, info);
}

std::shared_ptr<snap::rhi::Fence> Device::createFence(const snap::rhi::FenceCreateInfo& info) {
    return createResource<snap::rhi::Fence, snap::rhi::backend::noop::Fence>(this, info);
}

std::shared_ptr<snap::rhi::Buffer> Device::createBuffer(const snap::rhi::BufferCreateInfo& info) {
    return createResource<snap::rhi::Buffer, snap::rhi::backend::noop::Buffer>(this, info);
}

std::shared_ptr<snap::rhi::Texture> Device::createTexture(const snap::rhi::TextureCreateInfo& info) {
    return createResource<snap::rhi::Texture, snap::rhi::backend::noop::Texture>(this, info);
}

std::shared_ptr<snap::rhi::Texture> Device::createTexture(const std::shared_ptr<TextureInterop>& textureInterop) {
    return createResource<snap::rhi::Texture, snap::rhi::backend::noop::Texture>(this, snap::rhi::TextureCreateInfo{});
}

std::shared_ptr<snap::rhi::Texture> Device::createTextureView(const snap::rhi::TextureViewCreateInfo& info) {
    return createResource<snap::rhi::Texture, snap::rhi::backend::noop::Texture>(this, info);
}

std::shared_ptr<RenderPass> Device::createRenderPass(const RenderPassCreateInfo& info) {
    return createResource<snap::rhi::RenderPass, snap::rhi::RenderPass>(this, info);
}

std::shared_ptr<snap::rhi::Framebuffer> Device::createFramebuffer(const snap::rhi::FramebufferCreateInfo& info) {
    return createResource<snap::rhi::Framebuffer, snap::rhi::backend::noop::Framebuffer>(this, info);
}

std::shared_ptr<snap::rhi::ShaderLibrary> Device::createShaderLibrary(const snap::rhi::ShaderLibraryCreateInfo& info) {
    return createResource<snap::rhi::ShaderLibrary, snap::rhi::backend::noop::ShaderLibrary>(this, info);
}

std::shared_ptr<snap::rhi::ShaderModule> Device::createShaderModule(const snap::rhi::ShaderModuleCreateInfo& info) {
    return createResource<snap::rhi::ShaderModule, snap::rhi::backend::noop::ShaderModule>(this, info);
}

std::shared_ptr<snap::rhi::DescriptorSetLayout> Device::createDescriptorSetLayout(
    const snap::rhi::DescriptorSetLayoutCreateInfo& info) {
    return createResource<snap::rhi::DescriptorSetLayout, snap::rhi::backend::noop::DescriptorSetLayout>(this, info);
}

std::shared_ptr<snap::rhi::DescriptorPool> Device::createDescriptorPool(
    const snap::rhi::DescriptorPoolCreateInfo& info) {
    return createResource<snap::rhi::DescriptorPool, snap::rhi::backend::noop::DescriptorPool>(this, info);
}

std::shared_ptr<snap::rhi::DescriptorSet> Device::createDescriptorSet(const snap::rhi::DescriptorSetCreateInfo& info) {
    return createResource<snap::rhi::DescriptorSet, snap::rhi::backend::noop::DescriptorSet>(this, info);
}

std::shared_ptr<snap::rhi::PipelineLayout> Device::createPipelineLayout(
    const snap::rhi::PipelineLayoutCreateInfo& info) {
    return createResource<snap::rhi::PipelineLayout, snap::rhi::backend::common::PipelineLayout>(this, info);
}

std::shared_ptr<snap::rhi::PipelineCache> Device::createPipelineCache(const snap::rhi::PipelineCacheCreateInfo& info) {
    return createResource<snap::rhi::PipelineCache, snap::rhi::backend::noop::PipelineCache>(this, info);
}

std::shared_ptr<snap::rhi::RenderPipeline> Device::createRenderPipeline(
    const snap::rhi::RenderPipelineCreateInfo& info) {
    return createResource<snap::rhi::RenderPipeline, snap::rhi::backend::noop::RenderPipeline>(this, info);
}

std::shared_ptr<snap::rhi::ComputePipeline> Device::createComputePipeline(
    const snap::rhi::ComputePipelineCreateInfo& info) {
    return createResource<snap::rhi::ComputePipeline, snap::rhi::backend::noop::ComputePipeline>(this, info);
}

std::shared_ptr<snap::rhi::DebugMessenger> Device::createDebugMessenger(snap::rhi::DebugMessengerCreateInfo&& info) {
    return nullptr;
}

std::shared_ptr<snap::rhi::QueryPool> Device::createQueryPool(const snap::rhi::QueryPoolCreateInfo& info) {
    return nullptr;
}

TextureFormatProperties Device::getTextureFormatProperties(const TextureFormatInfo& info) {
    return {};
}
} // namespace snap::rhi::backend::noop
