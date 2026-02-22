#pragma once

#include "snap/rhi/Sampler.hpp"
#include "snap/rhi/Texture.hpp"
#include "snap/rhi/backend/common/Device.hpp"
#include "snap/rhi/backend/common/DeviceContextless.h"
#include "snap/rhi/backend/noop/CommandQueue.hpp"
#include "snap/rhi/backend/noop/DeviceCreateInfo.h"

#include <span>

namespace snap::rhi::backend::noop {
class Device final : public snap::rhi::backend::common::DeviceContextless {
public:
    explicit Device(const snap::rhi::backend::noop::DeviceCreateInfo& info);
    ~Device() noexcept(false) override = default;

    snap::rhi::CommandQueue* getCommandQueue(const uint32_t queueFamilyIndex, const uint32_t queueIndex) override;

    std::shared_ptr<snap::rhi::CommandBuffer> createCommandBuffer(
        const snap::rhi::CommandBufferCreateInfo& info) override;

    std::shared_ptr<snap::rhi::Sampler> createSampler(const snap::rhi::SamplerCreateInfo& info) override;

    std::shared_ptr<snap::rhi::Semaphore> createSemaphore(
        const snap::rhi::SemaphoreCreateInfo& info,
        const std::shared_ptr<snap::rhi::PlatformSyncHandle>& platformSyncHandle) override;

    std::shared_ptr<snap::rhi::Fence> createFence(const snap::rhi::FenceCreateInfo& info) override;

    std::shared_ptr<snap::rhi::Buffer> createBuffer(const snap::rhi::BufferCreateInfo& info) override;

    std::shared_ptr<snap::rhi::Texture> createTexture(const std::shared_ptr<TextureInterop>& textureInterop) final;
    std::shared_ptr<snap::rhi::Texture> createTexture(const snap::rhi::TextureCreateInfo& info) override;

    std::shared_ptr<snap::rhi::Texture> createTextureView(const snap::rhi::TextureViewCreateInfo& info) override;

    std::shared_ptr<RenderPass> createRenderPass(const RenderPassCreateInfo& info) override;

    std::shared_ptr<snap::rhi::Framebuffer> createFramebuffer(const snap::rhi::FramebufferCreateInfo& info) override;

    std::shared_ptr<snap::rhi::ShaderLibrary> createShaderLibrary(
        const snap::rhi::ShaderLibraryCreateInfo& info) override;

    std::shared_ptr<snap::rhi::ShaderModule> createShaderModule(const snap::rhi::ShaderModuleCreateInfo& info) override;

    std::shared_ptr<snap::rhi::DescriptorSetLayout> createDescriptorSetLayout(
        const snap::rhi::DescriptorSetLayoutCreateInfo& info) override;

    std::shared_ptr<snap::rhi::DescriptorPool> createDescriptorPool(
        const snap::rhi::DescriptorPoolCreateInfo& info) override;

    std::shared_ptr<snap::rhi::DescriptorSet> createDescriptorSet(
        const snap::rhi::DescriptorSetCreateInfo& info) override;

    std::shared_ptr<snap::rhi::PipelineLayout> createPipelineLayout(
        const snap::rhi::PipelineLayoutCreateInfo& info) override;

    std::shared_ptr<snap::rhi::PipelineCache> createPipelineCache(
        const snap::rhi::PipelineCacheCreateInfo& info) override;

    std::shared_ptr<snap::rhi::RenderPipeline> createRenderPipeline(
        const snap::rhi::RenderPipelineCreateInfo& info) override;

    std::shared_ptr<snap::rhi::ComputePipeline> createComputePipeline(
        const snap::rhi::ComputePipelineCreateInfo& info) override;

    std::shared_ptr<snap::rhi::DebugMessenger> createDebugMessenger(
        snap::rhi::DebugMessengerCreateInfo&& info) override;

    std::shared_ptr<snap::rhi::QueryPool> createQueryPool(const snap::rhi::QueryPoolCreateInfo& info) override;

    TextureFormatProperties getTextureFormatProperties(const TextureFormatInfo& info) override;

private:
    snap::rhi::backend::noop::CommandQueue mainQueue;
};
} // namespace snap::rhi::backend::noop
