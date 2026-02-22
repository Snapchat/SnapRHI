#pragma once

#include "snap/rhi/common/OS.h"

#include "snap/rhi/DescriptorSetLayoutCreateInfo.h"
#include "snap/rhi/Enums.h"
#include "snap/rhi/PixelFormat.h"
#include "snap/rhi/RenderPassCreateInfo.h"
#include "snap/rhi/RenderPipelineCreateInfo.h"
#include "snap/rhi/ShaderModule.hpp"
#include "snap/rhi/reflection/ComputePipelineInfo.h"
#include "snap/rhi/reflection/RenderPipelineInfo.h"

#include <span>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include <Metal/Metal.h>

namespace snap::rhi::backend::metal {
snap::rhi::VertexAttributeFormat convertToVtxFormat(MTLDataType dataType);
MTLPixelFormat convertToMtlPixelFormat(const snap::rhi::PixelFormat pixelFormat);
snap::rhi::PixelFormat convertMtlPixelFormatToRHI(const MTLPixelFormat pixelFormat);
MTLPrimitiveType convertToMtlPrimitiveType(const snap::rhi::Topology topology);

MTLCompareFunction convertToMtlCompareFunc(const snap::rhi::CompareFunction depthCompareFunction);

std::string getString(NSString* str);

NSString* getString(const std::string_view& str);

Boolean getBoolean(const std::string& value);

NSInteger getInt(const std::string& value);

float getFloat(const std::string& value);

API_AVAILABLE(macos(10.11), ios(12.0))
MTLPrimitiveTopologyClass convertToMtlPrimitiveTopologyClass(const snap::rhi::Topology topology);

MTLDataType getDataType(const snap::rhi::DescriptorType descriptorType);

snap::rhi::reflection::TextureType convertToReflectionTexType(const MTLTextureType type);
snap::rhi::reflection::ImageAccess convertToImageAccess(const MTLArgumentAccess access);

std::vector<snap::rhi::reflection::VertexAttributeInfo> buildVertexReflection(const id<MTLFunction>& vertexFunction);

std::vector<snap::rhi::reflection::DescriptorSetInfo> buildDescriptorSetReflection(
    MTLRenderPipelineReflection* reflection, const snap::rhi::PipelineLayout* pipelineLayout);
std::vector<snap::rhi::reflection::DescriptorSetInfo> buildDescriptorSetReflection(
    MTLComputePipelineReflection* reflection, const snap::rhi::PipelineLayout* pipelineLayout);

MTLRenderPipelineDescriptor* createRenderPipelineDescriptor(const snap::rhi::RenderPipelineCreateInfo& info,
                                                            const id<MTLFunction>& vertex,
                                                            const id<MTLFunction>& fragment);

id<MTLFunction> getFunction(std::span<snap::rhi::ShaderModule* const> shaders, const snap::rhi::ShaderStage stage);

template<typename T>
std::unordered_map<std::string, size_t> buildNameToId(const T& info) {
    std::unordered_map<std::string, size_t> nameToId;
    for (size_t i = 0; i < info.size(); ++i) {
        const auto& obj = info[i];
        nameToId[obj.name] = i;
    }
    return nameToId;
}

#if SNAP_RHI_OS_MACOS()
MTLRenderStages convertTo(snap::rhi::PipelineStageBits stage);
#endif

MTLRenderPassDescriptor* createFramebuffer(const snap::rhi::RenderPassCreateInfo& renderPassInfo,
                                           std::span<snap::rhi::Texture* const> attachments,
                                           std::span<const ClearValue> clearValues);
MTLRenderPassDescriptor* createFramebuffer(const snap::rhi::RenderingInfo& renderingInfo);

MTLResourceUsage convertToResourceUsage(snap::rhi::ShaderStageBits stageBits, snap::rhi::DescriptorType descriptorType);
MTLRenderStages convertToRenderStages(snap::rhi::ShaderStageBits stageBits);
MTLArgumentAccess getArgumentAccess(const snap::rhi::DescriptorType descriptorType);
id<MTLArgumentEncoder> createArgumentEncoder(Device* device, const snap::rhi::DescriptorSetLayoutCreateInfo& info);
MTLTextureType toMtlTextureType(const snap::rhi::TextureType textureType, const uint32_t sampleCount);
MTLTextureSwizzle toMtlSwizzle(const snap::rhi::ComponentSwizzle s);
} // namespace snap::rhi::backend::metal
