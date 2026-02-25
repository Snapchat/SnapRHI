#pragma once

#include <functional>
#include <memory>

#include "snap/rhi/Compare.hpp"
#include "snap/rhi/DeviceChild.hpp"
#include "snap/rhi/backend/common/Device.hpp"
#include "snap/rhi/backend/common/DeviceContextless.h"
#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/vulkan/CommandQueue.h"
#include "snap/rhi/backend/vulkan/DeviceCreateInfo.h"

#include "snap/rhi/backend/vulkan/vulkan.h"

namespace snap::rhi::backend::vulkan {
class Device final : public snap::rhi::backend::common::DeviceContextless {
public:
    explicit Device(const snap::rhi::backend::vulkan::DeviceCreateInfo& info);
    ~Device() noexcept(false) override;

    std::shared_ptr<snap::rhi::CommandBuffer> createCommandBuffer(
        const snap::rhi::CommandBufferCreateInfo& info) override;
    std::shared_ptr<snap::rhi::Sampler> createSampler(const snap::rhi::SamplerCreateInfo& info) override;
    std::shared_ptr<snap::rhi::Semaphore> createSemaphore(
        const snap::rhi::SemaphoreCreateInfo& info,
        const std::shared_ptr<snap::rhi::PlatformSyncHandle>& platformSyncHandle) override;
    std::shared_ptr<snap::rhi::Fence> createFence(const snap::rhi::FenceCreateInfo& info) override;
    std::shared_ptr<snap::rhi::Buffer> createBuffer(const snap::rhi::BufferCreateInfo& info) override;
    std::shared_ptr<snap::rhi::Texture> createTexture(const snap::rhi::TextureCreateInfo& info) override;
    std::shared_ptr<snap::rhi::Texture> createTexture(const snap::rhi::TextureCreateInfo& info,
                                                      void* imageCreateInfoNext);
    std::shared_ptr<snap::rhi::Texture> createTexture(
        const std::shared_ptr<snap::rhi::TextureInterop>& textureInterop) override;
    std::shared_ptr<snap::rhi::Texture> createTexture(const snap::rhi::TextureCreateInfo& info,
                                                      VkImage image,
                                                      bool isOwner,
                                                      VkImageLayout defaultLayout);
    std::shared_ptr<snap::rhi::Texture> createTextureView(const snap::rhi::TextureViewCreateInfo& info) override;
    std::shared_ptr<snap::rhi::RenderPass> createRenderPass(const RenderPassCreateInfo& info) override;
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
    std::shared_ptr<snap::rhi::QueryPool> createQueryPool(const snap::rhi::QueryPoolCreateInfo& info) override;
    TextureFormatProperties getTextureFormatProperties(const TextureFormatInfo& info) override;

    uint64_t getCPUMemoryUsage() const override;
    uint64_t getGPUMemoryUsage() const override;

    VkInstance getVkInstance() const;
    VkPhysicalDevice getVkPhysicalDevice() const;
    VkDevice getVkLogicalDevice() const;

    bool isExternalSemaphoreSupported() const;

    uint32_t chooseMemoryTypeIndex(const uint32_t memoryTypeBits, const VkMemoryPropertyFlags memoryProperty) const;

    const VkPhysicalDeviceProperties& getPhysicalDeviceProperties() const {
        return physicalDeviceProperties;
    }

    const VkPhysicalDeviceFeatures& getPhysicalDeviceFeatures() const {
        return physicalDeviceFeatures;
    }

    bool supportsImageView2DOn3DImage() const {
#if SNAP_RHI_OS_APPLE()
        return portabilitySubsetFeatures.imageView2DOn3DImage == VK_TRUE;
#else
        return true;
#endif
    }

private:
    void logPhysicalDeviceFeatures() const;
    void initInstance(std::span<const char* const> enabledInstanceExtensions);
    void initValidationLayerMessages();
    void choosePhysicalDevice();
    void initLogicalDevice(std::span<const char* const> enabledDeviceExtensions);
    void initCapabilities();
    void initCommandQueues();
    void initMemoryProperties();

    VkExternalSemaphoreProperties externalSemaphoreProperties{.sType = VK_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_PROPERTIES};
    snap::rhi::backend::vulkan::DeviceCreateInfo vkDeviceCreateinfo{};
    uint32_t preferredAPIVersion = 0;

    // https://docs.vulkan.org/spec/latest/chapters/limits.html => Table 2. Required Limits
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties physicalDeviceProperties{};
    VkPhysicalDeviceFeatures physicalDeviceFeatures{};
    VkPhysicalDeviceFeatures2 physicalDevice_1_2_Features{};
    VkPhysicalDeviceVulkan13Features physicalDevice_1_3_Features{};

#if SNAP_RHI_OS_APPLE()
    VkPhysicalDevicePortabilitySubsetFeaturesKHR portabilitySubsetFeatures{};
#endif

    VkPhysicalDeviceMemoryProperties memoryProperties{};
    std::vector<VkQueueFamilyProperties> queueFamilyProperties{};
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};
    VkDevice device = VK_NULL_HANDLE;

    std::vector<const char*> enabledInstanceExtensions{};
    std::vector<const char*> enabledDeviceExtensions{};
    std::shared_ptr<VkDebugUtilsMessengerEXT> mainMessenger;
};
} // namespace snap::rhi::backend::vulkan
