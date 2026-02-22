#pragma once

#include "snap/rhi/CommandBuffer.hpp"
#include "snap/rhi/backend/common/CommandEncoder.h"
#include "snap/rhi/backend/vulkan/BlitCommandEncoder.h"
#include "snap/rhi/backend/vulkan/ComputeCommandEncoder.h"
#include "snap/rhi/backend/vulkan/ImageLayoutManager.h"
#include "snap/rhi/backend/vulkan/RenderCommandEncoder.h"
#include "snap/rhi/backend/vulkan/ResourcesInitInfo.h"
#include "snap/rhi/backend/vulkan/vulkan.h"
#include "snap/rhi/common/NonCopyable.h"

namespace snap::rhi::backend::vulkan {
class Device;
class Texture;

class CommandBuffer final : public snap::rhi::backend::common::CommandBuffer {
public:
    CommandBuffer(snap::rhi::backend::vulkan::Device* vkDevice, const snap::rhi::CommandBufferCreateInfo& info);
    ~CommandBuffer() override = default;

    void setDebugLabel(std::string_view label) override;

    void resetQueryPool(snap::rhi::QueryPool* queryPool, uint32_t firstQuery, uint32_t queryCount) override;

    snap::rhi::RenderCommandEncoder* getRenderCommandEncoder() override;
    snap::rhi::BlitCommandEncoder* getBlitCommandEncoder() override;
    snap::rhi::ComputeCommandEncoder* getComputeCommandEncoder() override;

    void finishRecording();

    void beginRenderPass();
    void endRenderPass();

    VkCommandBuffer getActiveCommandBuffer() const {
        return commandBuffers[activeCommandBufferIndex];
    }

    VkCommandBuffer getSyncCommandBuffer() const {
        return commandBuffers[syncCommandBufferIndex];
    }

    const std::vector<VkCommandBuffer>& getCommandBuffers() const {
        return commandBuffers;
    }

    ImageLayoutManager& getImageLayoutManager() {
        return imageLayoutManager;
    }

    ResourcesInitInfo& getResourcesInitInfo() {
        return resourcesInitInfo;
    }

private:
    const snap::rhi::backend::common::ValidationLayer& validationLayer;
    VkDevice vkDevice = VK_NULL_HANDLE;

    std::unique_ptr<VkCommandPool, std::function<void(VkCommandPool*)>> pool;
    std::vector<VkCommandBuffer> commandBuffers;
    uint32_t activeCommandBufferIndex = 0;
    uint32_t syncCommandBufferIndex = 0;

    ImageLayoutManager imageLayoutManager;
    snap::rhi::backend::vulkan::BlitCommandEncoder blitCommandEncoder;
    snap::rhi::backend::vulkan::ComputeCommandEncoder computeCommandEncoder;
    snap::rhi::backend::vulkan::RenderCommandEncoder renderCommandEncoder;

    ResourcesInitInfo resourcesInitInfo{};
};
} // namespace snap::rhi::backend::vulkan
