#pragma once

#include "snap/rhi/backend/common/ValidationLayer.hpp"
#include "snap/rhi/backend/vulkan/vulkan.h"
#include "snap/rhi/common/NonCopyable.h"
#include <cassert>
#include <unordered_map>
#include <vector>

namespace snap::rhi::backend::vulkan {
class Device;
class Texture;
class CommandBuffer;
/**
 *
 * Renderpass:
 * - On Rendering(Renderpass case) we need to transit image into
 * VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL/VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL.
 * - After RenderPass we can update image layout into
 * VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL/VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL.
 *
 *
 * */
class ImageLayoutManager final : public snap::rhi::common::NonCopyable {
public:
    ImageLayoutManager(snap::rhi::backend::vulkan::Device* vkDevice,
                       snap::rhi::backend::vulkan::CommandBuffer* commandBuffer);
    ~ImageLayoutManager();

    void transferLayout(snap::rhi::backend::vulkan::Texture* texture,
                        const VkImageSubresourceRange& range,
                        const VkPipelineStageFlags targetStageMask,
                        const VkAccessFlags targetAccessMask,
                        const VkImageLayout targetLayout);
    void transferImagesIntoDefaultLayout();

private:
    snap::rhi::backend::vulkan::Device* vkDevice = nullptr;
    const snap::rhi::backend::common::ValidationLayer& validationLayer;
    snap::rhi::backend::vulkan::CommandBuffer* commandBuffer = nullptr;

    struct TextureSliceMipLayout {
        VkPipelineStageFlags stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        VkAccessFlags accessMask = 0;
        VkImageLayout layout = VK_IMAGE_LAYOUT_GENERAL;

        TextureSliceMipLayout(VkPipelineStageFlags stageMask, VkAccessFlags accessMask, VkImageLayout layout)
            : stageMask(stageMask), accessMask(accessMask), layout(layout) {}

        constexpr friend auto operator<=>(const TextureSliceMipLayout&,
                                          const TextureSliceMipLayout&) noexcept = default;
    };

    struct TextureLayout {
    public:
        TextureLayout(const uint32_t layers,
                      const uint32_t mipLevels,
                      VkPipelineStageFlags stageMask,
                      VkAccessFlags accessMask,
                      VkImageLayout layout)
            : layers(layers), mipLevels(mipLevels) {
            layouts.resize(mipLevels * layers, TextureSliceMipLayout(stageMask, accessMask, layout));
        }

        TextureSliceMipLayout& getLayout(const uint32_t layer, const uint32_t mipLevel) {
            assert(mipLevel < mipLevels);
            assert(layer < layers);

            return layouts[mipLevels * layer + mipLevel];
        }

    public:
        std::vector<TextureSliceMipLayout> layouts;
        const uint32_t layers = 0;
        const uint32_t mipLevels = 0;
    };

    struct ImageInfo {
        VkImageAspectFlags aspectMask = 0;
        VkImageLayout defaultLayout = VK_IMAGE_LAYOUT_GENERAL;
    };

    void transferLayout(TextureLayout& textureLayoutInfo,
                        const VkImage image,
                        const VkImageSubresourceRange& range,
                        const VkPipelineStageFlags targetStageMask,
                        const VkAccessFlags targetAccessMask,
                        const VkImageLayout targetLayout);

    std::unordered_map<VkImage, TextureLayout> imageToLayout;
    std::unordered_map<VkImage, ImageInfo> imageToInfo;
};
} // namespace snap::rhi::backend::vulkan
