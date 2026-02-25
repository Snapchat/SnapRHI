#pragma once

#include <functional>
#include <memory>

#include "snap/rhi/Compare.hpp"
#include "snap/rhi/DeviceChild.hpp"
#include "snap/rhi/backend/common/Device.hpp"
#include "snap/rhi/backend/common/DeviceContextless.h"
#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/metal/CommandQueue.h"
#include "snap/rhi/backend/metal/DeviceCreateInfo.h"
#include "snap/rhi/backend/metal/ResourceBatcher.h"
#include "snap/rhi/backend/metal/Sampler.h"
#include <Metal/Metal.h>
#include <queue>

namespace snap::rhi::backend::metal {
class Device final : public snap::rhi::backend::common::DeviceContextless {
public:
    explicit Device(const snap::rhi::backend::metal::DeviceCreateInfo& info);
    ~Device() noexcept(false) override;

    std::shared_ptr<snap::rhi::CommandBuffer> createCommandBuffer(
        const snap::rhi::CommandBufferCreateInfo& info) override;
    std::shared_ptr<snap::rhi::Sampler> createSampler(const snap::rhi::SamplerCreateInfo& info) override;
    std::shared_ptr<snap::rhi::Semaphore> createSemaphore(
        const snap::rhi::SemaphoreCreateInfo& info,
        const std::shared_ptr<snap::rhi::PlatformSyncHandle>& platformSyncHandle) override;
    std::shared_ptr<snap::rhi::Fence> createFence(const snap::rhi::FenceCreateInfo& info) override;
    std::shared_ptr<snap::rhi::Buffer> createBuffer(const snap::rhi::BufferCreateInfo& info) override;
    std::shared_ptr<snap::rhi::Texture> createTexture(const id<MTLTexture>& mtlTexture,
                                                      const snap::rhi::TextureCreateInfo& info);
    std::shared_ptr<snap::rhi::Texture> createTexture(
        const std::shared_ptr<snap::rhi::TextureInterop>& textureInterop) final;
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
    std::shared_ptr<snap::rhi::PipelineLayout> createPipelineLayout(
        const snap::rhi::PipelineLayoutCreateInfo& info) override;
    std::shared_ptr<snap::rhi::DescriptorSet> createDescriptorSet(
        const snap::rhi::DescriptorSetCreateInfo& info) override;
    std::shared_ptr<snap::rhi::PipelineCache> createPipelineCache(
        const snap::rhi::PipelineCacheCreateInfo& info) override;
    std::shared_ptr<snap::rhi::RenderPipeline> createRenderPipeline(
        const snap::rhi::RenderPipelineCreateInfo& info) override;
    std::shared_ptr<snap::rhi::ComputePipeline> createComputePipeline(
        const snap::rhi::ComputePipelineCreateInfo& info) override;
    std::shared_ptr<snap::rhi::DebugMessenger> createDebugMessenger(
        snap::rhi::DebugMessengerCreateInfo&& info) override;
    TextureFormatProperties getTextureFormatProperties(const TextureFormatInfo& info) override;
    std::shared_ptr<snap::rhi::QueryPool> createQueryPool(const snap::rhi::QueryPoolCreateInfo& info) override;

    uint64_t getGPUMemoryUsage() const override;

    const id<MTLDevice>& getMtlDevice() const;

    ResourceBatcher acquireResourceBatcher() {
        std::lock_guard lock(resourceBatcherPoolMutex);
        if (resourceBatcherPool.empty()) {
            return ResourceBatcher();
        }

        ResourceBatcher batcher = std::move(resourceBatcherPool.front());
        resourceBatcherPool.pop();
        return batcher;
    }

    void releaseResourceBatcher(ResourceBatcher&& resourceBatcher) {
        std::lock_guard lock(resourceBatcherPoolMutex);
        resourceBatcherPool.push(std::move(resourceBatcher));
    }

private:
    id<MTLDevice> mtlDevice = nil;

    std::mutex resourceBatcherPoolMutex;
    std::queue<ResourceBatcher> resourceBatcherPool;
};
} // namespace snap::rhi::backend::metal
