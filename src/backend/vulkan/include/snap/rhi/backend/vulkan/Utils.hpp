#pragma once

#include "snap/rhi/Enums.h"
#include "snap/rhi/PixelFormat.h"
#include "snap/rhi/SpecializationConstantFormat.h"
#include "snap/rhi/Structs.h"
#include "snap/rhi/TextureCreateInfo.h"
#include "snap/rhi/VertexAttributeFormat.h"
#include "snap/rhi/backend/vulkan/ImageViewCache.h"
#include "snap/rhi/backend/vulkan/vulkan.h"
#include "snap/rhi/reflection/ShaderLibraryInfo.h"
#include <optional>
#include <span>
#include <vector>

struct SpvReflectShaderModule;

namespace snap::rhi::backend::vulkan {
class Texture;

struct EntryPointReflection {
    snap::rhi::reflection::EntryPointInfo entryPointInfo;
    std::vector<snap::rhi::reflection::VertexAttributeInfo> vertexAttributes;
    std::vector<snap::rhi::reflection::DescriptorSetInfo> descriptorSetInfo;
};
std::vector<EntryPointReflection> buildEntryPointReflection(std::span<const uint32_t> code,
                                                            const SpvReflectShaderModule& spvModule);

VkFormat toVkFormat(const snap::rhi::PixelFormat format);
uint32_t getArraySize(const snap::rhi::TextureType textureType, const snap::rhi::Extent3D& extent3D);

uint32_t getBaseArrayLayer(const snap::rhi::TextureType textureType, const uint32_t z);
uint32_t getLayerCount(const snap::rhi::TextureType textureType, const uint32_t depth);
int32_t getImageOffsetZ(const snap::rhi::TextureType textureType, const uint32_t z);
uint32_t getImageExtentDepth(const snap::rhi::TextureType textureType, const uint32_t depth);
VkImageAspectFlags getImageAspectFlags(const snap::rhi::PixelFormat format);
VkDescriptorType getDescriptorType(const snap::rhi::DescriptorType type);
VkShaderStageFlags getShaderStageFlags(const snap::rhi::ShaderStageBits bits);
VkShaderStageFlagBits getShaderStageFlags(const snap::rhi::ShaderStage stage);
uint32_t getDataSize(const snap::rhi::SpecializationConstantFormat format);
VkFilter toVkFilter(const snap::rhi::SamplerMinMagFilter filter);
VkSamplerMipmapMode toVkSamplerMipmapMode(const snap::rhi::SamplerMipFilter filter);
VkSamplerAddressMode toVkSamplerAddressMode(const snap::rhi::WrapMode mode);
VkCompareOp toVkCompareOp(const snap::rhi::CompareFunction compareFunction);
VkBorderColor toVkBorderColor(const snap::rhi::SamplerBorderColor color);
VkPipelineStageFlags toVkPipelineStageFlags(const snap::rhi::PipelineStageBits bits);
VkSampleCountFlagBits toVkSampleCountFlagBits(const snap::rhi::SampleCount sampleCount);
VkAttachmentLoadOp toVkAttachmentLoadOp(const snap::rhi::AttachmentLoadOp loadOp);
VkAttachmentStoreOp toVkAttachmentStoreOp(const snap::rhi::AttachmentStoreOp storeOp);
VkAccessFlags toVkAccessFlags(const snap::rhi::AccessFlags bits);
VkDependencyFlags toVkDependencyFlags(const snap::rhi::DependencyFlags bits);
VkVertexInputRate toVkVertexInputRate(const snap::rhi::VertexInputRate rate);
VkFormat toVkFormat(const snap::rhi::VertexAttributeFormat format);
VkPrimitiveTopology toVkPrimitiveTopology(const snap::rhi::Topology topology);
bool isStripPrimitiveTopology(const snap::rhi::Topology topology);
VkPolygonMode toVkPolygonMode(const snap::rhi::PolygonMode mode);
VkCullModeFlags toVkCullModeFlags(const snap::rhi::CullMode mode);
VkFrontFace toVkFrontFace(const snap::rhi::Winding winding);
VkStencilOp toVkStencilOp(const snap::rhi::StencilOp op);
VkBlendFactor toVkBlendFactor(const snap::rhi::BlendFactor factor);
VkBlendOp toVkBlendOp(const snap::rhi::BlendOp operation);
VkStencilFaceFlags toVkStencilFaceFlags(const snap::rhi::StencilFace face);
VkIndexType toVkIndexType(const snap::rhi::IndexType indexType);
VkImageCreateFlags toVkImageCreateFlags(snap::rhi::TextureType textureType);
VkImageType toVkImageType(const snap::rhi::TextureType textureType);
VkImageUsageFlags toVkImageUsageFlags(const snap::rhi::TextureUsage usage);
VkComponentSwizzle toVkComponentSwizzle(const snap::rhi::ComponentSwizzle swizzle);
VkComponentMapping toVkComponentMapping(const snap::rhi::ComponentMapping& mapping,
                                        const snap::rhi::PixelFormat viewFormat);
VkImageViewType getImageViewType(const snap::rhi::TextureType textureType);
VkImageAspectFlags getImageAspect(const snap::rhi::PixelFormat format);
VkImageView resolveImageView(const snap::rhi::backend::vulkan::Texture* texture,
                             const snap::rhi::TextureSubresourceRange& range);
snap::rhi::TextureViewInfo buildTextureViewInfo(const snap::rhi::TextureCreateInfo& info,
                                                const snap::rhi::TextureSubresourceRange& range,
                                                snap::rhi::PixelFormat viewFormat,
                                                snap::rhi::TextureType viewType,
                                                const snap::rhi::ComponentMapping& components);
} // namespace snap::rhi::backend::vulkan
