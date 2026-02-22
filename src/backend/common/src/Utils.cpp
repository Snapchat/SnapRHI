#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/common/CommandBuffer.h"
#include "snap/rhi/backend/common/Device.hpp"
#include "snap/rhi/backend/common/Logging.hpp"
#include "snap/rhi/backend/common/PlatformSyncHandle.h"
#include "snap/rhi/backend/common/TextureInterop.h"
#include "snap/rhi/common/Scope.h"
#include "snap/rhi/common/Throw.h"
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <sstream>

#if SNAP_RHI_OS_WINDOWS()
#include <windows.h>
#elif SNAP_RHI_OS_POSIX()
#include <stdlib.h>
#endif

#if SNAP_RHI_OS_ANDROID()
#include "snap/rhi/backend/common/platform/android/SyncHandle.h"
#endif

namespace snap::rhi::backend::common {
bool hasEnding(const std::string_view fullString, const std::string_view ending) {
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

std::string getNonArrayName(const std::string_view name) {
    if (name.length() > 0 && name[name.length() - 1] != ']') {
        return std::string(name);
    }

    size_t pos = name.find_last_of('[');
    if (pos == std::string::npos) {
        return std::string(name);
    }

    return std::string(name.data(), pos);
}

int32_t getIdxByName(const std::string_view name) {
    if (name.length() > 0 && name[name.length() - 1] != ']') {
        return 0;
    }

    size_t posS = name.find_last_of('[');
    if (posS == std::string::npos) {
        return 0;
    }

    char* end = nullptr;
    return static_cast<int32_t>(strtol(name.data() + posS + 1, &end, 10));
}

std::string computeSharedPrefix(const std::string& a, const std::string& b) {
    size_t end = 0;
    while (end < a.size() && end < b.size() && a[end] == b[end])
        ++end;

    if (end == 0) {
        return "";
    }

    return a.substr(0, end);
}

bool haveSamePrefix(const std::string& src, const std::string& prefix) {
    if (src.size() < prefix.size())
        return false;

    const std::string pStr = src.substr(0, prefix.size());
    return pStr == prefix;
}

AttachmentFormatsCreateInfo convertToAttachmentFormatsCreateInfo(
    const snap::rhi::RenderPassCreateInfo& renderPassCreateInfo, const bool stencilEnable) {
    AttachmentFormatsCreateInfo result{};

    if (renderPassCreateInfo.subpassCount > 1) {
        snap::rhi::common::throwException("[SnapRHI Metal] supports only 1 subpasss");
    }
    const auto& subpassDescription = renderPassCreateInfo.subpasses[0];

    result.colorAttachmentFormats.resize(subpassDescription.colorAttachmentCount);
    for (uint32_t i = 0; i < subpassDescription.colorAttachmentCount; ++i) {
        const uint32_t attachmentIdx = subpassDescription.colorAttachments[i].attachment;
        assert(attachmentIdx < renderPassCreateInfo.attachmentCount);

        const auto& attachmentInfo = renderPassCreateInfo.attachments[attachmentIdx];
        result.colorAttachmentFormats[i] = attachmentInfo.format;
    }

    const uint32_t depthAttachmentIdx = subpassDescription.depthStencilAttachment.attachment;
    if (depthAttachmentIdx != snap::rhi::AttachmentUnused) {
        const auto& attachmentInfo = renderPassCreateInfo.attachments[depthAttachmentIdx];
        result.depthAttachmentFormat = attachmentInfo.format;
    }

    result.stencilAttachmentFormat = (stencilEnable || snap::rhi::hasStencilAspect(result.depthAttachmentFormat)) ?
                                         result.depthAttachmentFormat :
                                         snap::rhi::PixelFormat::Undefined;
    return result;
}

bool setEnvVar(const std::string& name, const std::string& value) {
#if SNAP_RHI_OS_WINDOWS()
    return SetEnvironmentVariableA(name.c_str(), value.c_str()) != 0;
#elif SNAP_RHI_OS_POSIX()
    return setenv(name.c_str(), value.c_str(), 1) == 0;
#else
    return false;
#endif
}

bool writeBinToFile(const std::filesystem::path& filePath, std::span<const std::byte> data) {
    std::ofstream file;

    try {
        if (filePath.has_parent_path()) {
            std::filesystem::create_directories(filePath.parent_path());
        }

        // We want to throw if opening fails (permissions) or writing fails (disk full)
        file.exceptions(std::ofstream::failbit | std::ofstream::badbit);

        // std::ios::binary is critical here to prevent line-ending translation (LF -> CRLF) on Windows
        file.open(filePath, std::ios::out | std::ios::binary | std::ios::trunc);

        if (!data.empty()) {
            file.write(reinterpret_cast<const char*>(data.data()), data.size_bytes());
        }

        file.close();
    } catch (const std::filesystem::filesystem_error& e) {
        SNAP_RHI_LOGE("[writeBinToFile] Filesystem Error: %s", e.what());
        return false;
    } catch (const std::ios_base::failure& e) {
        SNAP_RHI_LOGE("[writeBinToFile] I/O Error: %s. Path: %s", e.what(), filePath.string().c_str());
        // Only remove if we actually opened (and thus created/truncated) the file
        if (file.is_open()) {
            file.close();
            std::error_code ec;
            std::filesystem::remove(filePath, ec);
        }
        return false;
    } catch (const std::exception& e) {
        SNAP_RHI_LOGE("[writeBinToFile] Generic Error: %s", e.what());
        return false;
    }

    return true;
}

bool writeStringToFile(const std::filesystem::path& filePath, const std::string& fileData) {
    try {
        if (filePath.has_parent_path()) {
            std::filesystem::create_directories(filePath.parent_path());
        }

        std::ofstream file;

        // Enable Exceptions
        // By default, streams only set flags. We want them to THROW if:
        // - failbit: Logical error (e.g., file permissions, open failed)
        // - badbit:  Read/writing error (e.g., disk full, hardware failure)
        file.exceptions(std::ofstream::failbit | std::ofstream::badbit);
        file.open(filePath, std::ios_base::out | std::ios_base::trunc);

        file << fileData;

        file.close();

    } catch (const std::filesystem::filesystem_error& e) {
        // Specific handling for path/directory errors
        SNAP_RHI_LOGE("[writeToFile] Filesystem Error: %s", e.what());
        return false;
    } catch (const std::ios_base::failure& e) {
        // Specific handling for File I/O errors (open, write, flush)
        SNAP_RHI_LOGE("[writeToFile] I/O Error: %s. Path: %s", e.what(), filePath.c_str());

        // CRITICAL: Cleanup
        // If we failed halfway through writing (e.g., disk full), the file exists
        // but is corrupted/incomplete. We should delete it.
        std::error_code ec;
        std::filesystem::remove(filePath, ec);
        return false;
    } catch (const std::exception& e) {
        // Catch-all for other standard exceptions (e.g. std::bad_alloc)
        SNAP_RHI_LOGE("[writeToFile] Generic Error: %s", e.what());
        return false;
    } catch (...) {
        // Catch-all for non-standard exceptions
        SNAP_RHI_LOGE("[writeToFile] Unknown fatal error.");
        return false;
    }

    return true;
}

void reportCapabilities(const snap::rhi::Capabilities& caps,
                        const snap::rhi::backend::common::ValidationLayer& validationLayer) {
    auto boolToStr = [](bool v) { return v ? "true" : "false"; };

    // Helper to convert FormatFeatures bitmask
    auto formatFeaturesToStr = [](snap::rhi::FormatFeatures f) {
        if (f == snap::rhi::FormatFeatures::None) {
            return std::string("None");
        }
        struct FeatureName {
            snap::rhi::FormatFeatures flag;
            const char* name;
        };
        static const FeatureName features[] = {
            {snap::rhi::FormatFeatures::Sampled, "Sampled"},
            {snap::rhi::FormatFeatures::SampledFilterLinear, "SampledFilterLinear"},
            {snap::rhi::FormatFeatures::ColorRenderable, "ColorRenderable"},
            {snap::rhi::FormatFeatures::ColorRenderableBlend, "ColorRenderableBlend"},
            {snap::rhi::FormatFeatures::DepthStencilRenderable, "DepthStencilRenderable"},
            {snap::rhi::FormatFeatures::BlitSrc, "BlitSrc"},
            {snap::rhi::FormatFeatures::BlitDst, "BlitDst"},
            {snap::rhi::FormatFeatures::Resolve, "Resolve"},
            {snap::rhi::FormatFeatures::TransferSrc, "TransferSrc"},
            {snap::rhi::FormatFeatures::TransferDst, "TransferDst"},
            {snap::rhi::FormatFeatures::Storage, "Storage"},
        };
        std::string out;
        for (const auto& fn : features) {
            if ((static_cast<uint32_t>(f) & static_cast<uint32_t>(fn.flag)) == static_cast<uint32_t>(fn.flag)) {
                if (!out.empty())
                    out += ",";
                out += fn.name;
            }
        }
        return out.empty() ? std::string("None") : out;
    };

    auto sampleCountToStr = [](snap::rhi::SampleCount sc) { return std::to_string(static_cast<uint32_t>(sc)); };

    auto shaderStageIndexToName = [](uint32_t i) {
        switch (static_cast<snap::rhi::ShaderStage>(i)) {
            case snap::rhi::ShaderStage::Vertex:
                return "Vertex";
            case snap::rhi::ShaderStage::Fragment:
                return "Fragment";
            case snap::rhi::ShaderStage::Compute:
                return "Compute";
            default:
                return "Unknown";
        }
    };

    std::ostringstream oss;
    oss << "Device Capabilities Report:" << '\n';

    // General feature booleans
    oss << "isRasterizerDisableSupported=" << boolToStr(caps.isRasterizerDisableSupported) << '\n'
        << "isNPOTWrapModeSupported=" << boolToStr(caps.isNPOTWrapModeSupported) << '\n'
        << "isClampToBorderSupported=" << boolToStr(caps.isClampToBorderSupported) << '\n'
        << "isClampToZeroSupported=" << boolToStr(caps.isClampToZeroSupported) << '\n'
        << "isTextureArraySupported=" << boolToStr(caps.isTextureArraySupported) << '\n'
        << "isTexture3DSupported=" << boolToStr(caps.isTexture3DSupported) << '\n'
        << "isMinMaxBlendModeSupported=" << boolToStr(caps.isMinMaxBlendModeSupported) << '\n'
        << "isRenderToMipmapSupported=" << boolToStr(caps.isRenderToMipmapSupported) << '\n'
        << "isNPOTGenerateMipmapCmdSupported=" << boolToStr(caps.isNPOTGenerateMipmapCmdSupported) << '\n'
        << "isDepthCubeMapSupported=" << boolToStr(caps.isDepthCubeMapSupported) << '\n'
        << "isPolygonOffsetClampSupported=" << boolToStr(caps.isPolygonOffsetClampSupported) << '\n'
        << "isAlphaToCoverageSupported=" << boolToStr(caps.isAlphaToCoverageSupported) << '\n'
        << "isAlphaToOneEnableSupported=" << boolToStr(caps.isAlphaToOneEnableSupported) << '\n'
        << "isMRTBlendSettingsDifferent=" << boolToStr(caps.isMRTBlendSettingsDifferent) << '\n'
        << "isSamplerMinMaxLodSupported=" << boolToStr(caps.isSamplerMinMaxLodSupported) << '\n'
        << "isSamplerUnnormalizedCoordsSupported=" << boolToStr(caps.isSamplerUnnormalizedCoordsSupported) << '\n'
        << "isSamplerCompareFuncSupported=" << boolToStr(caps.isSamplerCompareFuncSupported) << '\n'
        << "isTextureViewSupported=" << boolToStr(caps.isTextureViewSupported) << '\n'
        << "isTextureFormatSwizzleSupported=" << boolToStr(caps.isTextureFormatSwizzleSupported) << '\n'
        << "isNonFillPolygonModeSupported=" << boolToStr(caps.isNonFillPolygonModeSupported) << '\n'
        << "isShaderStorageBufferSupported=" << boolToStr(caps.isShaderStorageBufferSupported) << '\n'
        << "isUInt32IndexSupported=" << boolToStr(caps.isUInt32IndexSupported) << '\n'
        << "isDynamicRenderingSupported=" << boolToStr(caps.isDynamicRenderingSupported) << '\n'
        << "isFramebufferFetchSupported=" << boolToStr(caps.isFramebufferFetchSupported) << '\n'
        << "isPrimitiveRestartIndexEnabled=" << boolToStr(caps.isPrimitiveRestartIndexEnabled) << '\n'
        << "isMultiviewMSAAImplicitResolveEnabled=" << boolToStr(caps.isMultiviewMSAAImplicitResolveEnabled) << '\n'
        << "isFragmentPrimitiveIDSupported=" << boolToStr(caps.isFragmentPrimitiveIDSupported) << '\n'
        << "isFragmentBarycentricCoordinatesSupported=" << boolToStr(caps.isFragmentBarycentricCoordinatesSupported)
        << '\n';

    // Numeric limits
    oss << "maxFramebufferLayers=" << caps.maxFramebufferLayers << '\n'
        << "layerRenderingTypeBits=" << static_cast<uint32_t>(caps.layerRenderingType) << '\n'
        << "maxTextureArrayLayers=" << caps.maxTextureArrayLayers << '\n'
        << "maxTextureDimension2D=" << caps.maxTextureDimension2D << '\n'
        << "maxTextureDimension3D=" << caps.maxTextureDimension3D << '\n'
        << "maxTextureDimensionCube=" << caps.maxTextureDimensionCube << '\n'
        << "maxMultiviewViewCount=" << caps.maxMultiviewViewCount << '\n'
        << "maxClipDistances=" << caps.maxClipDistances << '\n'
        << "maxCullDistances=" << caps.maxCullDistances << '\n'
        << "maxCombinedClipAndCullDistances=" << caps.maxCombinedClipAndCullDistances << '\n'
        << "maxPerStageUniformBuffers=" << caps.maxPerStageUniformBuffers << '\n'
        << "maxUniformBuffers=" << caps.maxUniformBuffers << '\n'
        << "maxSpecializationConstants=" << caps.maxSpecializationConstants << '\n'
        << "maxVertexInputAttributes=" << caps.maxVertexInputAttributes << '\n'
        << "maxVertexInputBindings=" << caps.maxVertexInputBindings << '\n'
        << "maxVertexInputAttributeOffset=" << caps.maxVertexInputAttributeOffset << '\n'
        << "maxVertexInputBindingStride=" << caps.maxVertexInputBindingStride << '\n'
        << "maxVertexAttribDivisor=" << caps.maxVertexAttribDivisor << '\n'
        << "maxBoundDescriptorSets=" << caps.maxBoundDescriptorSets << '\n'
        << "maxComputeWorkGroupInvocations=" << caps.maxComputeWorkGroupInvocations << '\n'
        << "threadExecutionWidth=" << caps.threadExecutionWidth << '\n'
        << "minUniformBufferOffsetAlignment=" << caps.minUniformBufferOffsetAlignment << '\n'
        << "maxFramebufferColorAttachmentCount=" << caps.maxFramebufferColorAttachmentCount << '\n'
        << "maxAnisotropic=" << static_cast<uint32_t>(caps.maxAnisotropic) << '\n'
        << "queueFamiliesCount=" << caps.queueFamiliesCount << '\n'
        << "ndcLayout.yAxis=" << static_cast<uint32_t>(caps.ndcLayout.yAxis) << '\n'
        << "ndcLayout.depthRange=" << static_cast<uint32_t>(caps.ndcLayout.depthRange) << '\n'
        << "textureConvention=" << static_cast<uint32_t>(caps.textureConvention) << '\n'
        << "apiDescription.api=" << static_cast<uint32_t>(caps.apiDescription.api) << '\n'
        << "apiDescription.version=" << static_cast<uint32_t>(caps.apiDescription.version) << '\n';

    // Arrays per stage
    oss << "PerStageTotalUniformBufferSize:" << '\n';
    for (uint32_t i = 0; i < static_cast<uint32_t>(snap::rhi::ShaderStage::Count); ++i) {
        oss << "  " << shaderStageIndexToName(i) << "=" << caps.maxPerStageTotalUniformBufferSize[i] << '\n';
    }
    oss << "PerStageUniformBufferSize:" << '\n';
    for (uint32_t i = 0; i < static_cast<uint32_t>(snap::rhi::ShaderStage::Count); ++i) {
        oss << "  " << shaderStageIndexToName(i) << "=" << caps.maxPerStageUniformBufferSize[i] << '\n';
    }
    oss << "PerStageTextures:" << '\n';
    for (uint32_t i = 0; i < static_cast<uint32_t>(snap::rhi::ShaderStage::Count); ++i) {
        oss << "  " << shaderStageIndexToName(i) << "=" << caps.maxPerStageTextures[i] << '\n';
    }
    oss << "PerStageSamplers:" << '\n';
    for (uint32_t i = 0; i < static_cast<uint32_t>(snap::rhi::ShaderStage::Count); ++i) {
        oss << "  " << shaderStageIndexToName(i) << "=" << caps.maxPerStageSamplers[i] << '\n';
    }

    // Compute work group arrays
    oss << "maxComputeWorkGroupCount=(" << caps.maxComputeWorkGroupCount[0] << "," << caps.maxComputeWorkGroupCount[1]
        << "," << caps.maxComputeWorkGroupCount[2] << ")" << '\n';
    oss << "maxComputeWorkGroupSize=(" << caps.maxComputeWorkGroupSize[0] << "," << caps.maxComputeWorkGroupSize[1]
        << "," << caps.maxComputeWorkGroupSize[2] << ")" << '\n';

    // Queue families
    oss << "QueueFamilies:" << '\n';
    for (uint32_t i = 0; i < caps.queueFamiliesCount && i < snap::rhi::SupportedLimit::MaxQueueFamilies; ++i) {
        const auto& q = caps.queueFamilyProperties[i];
        oss << "  Index=" << i << " flags=" << static_cast<uint32_t>(q.queueFlags) << " queueCount=" << q.queueCount
            << " timestampQuerySupported=" << boolToStr(q.isTimestampQuerySupported) << '\n';
    }

    // Pixel format properties
    // Added: name table for snap::rhi::PixelFormat so we can print symbolic names
    static const char* kPixelFormatNames[] = {"Undefined",
                                              // Unsigned normalized
                                              "R8Unorm",
                                              "R8G8Unorm",
                                              "R8G8B8Unorm",
                                              "R8G8B8A8Unorm",
                                              "R16Unorm",
                                              "R16G16Unorm",
                                              "R16G16B16A16Unorm",
                                              // Signed normalized
                                              "R8Snorm",
                                              "R8G8Snorm",
                                              "R8G8B8A8Snorm",
                                              "R16Snorm",
                                              "R16G16Snorm",
                                              "R16G16B16A16Snorm",
                                              // Unsigned integer
                                              "R8Uint",
                                              "R8G8Uint",
                                              "R8G8B8A8Uint",
                                              "R16Uint",
                                              "R16G16Uint",
                                              "R16G16B16A16Uint",
                                              "R32Uint",
                                              "R32G32Uint",
                                              "R32G32B32A32Uint",
                                              // Signed integer
                                              "R8Sint",
                                              "R8G8Sint",
                                              "R8G8B8A8Sint",
                                              "R16Sint",
                                              "R16G16Sint",
                                              "R16G16B16A16Sint",
                                              "R32Sint",
                                              "R32G32Sint",
                                              "R32G32B32A32Sint",
                                              // Float
                                              "R16Float",
                                              "R16G16Float",
                                              "R16G16B16A16Float",
                                              "R32Float",
                                              "R32G32Float",
                                              "R32G32B32A32Float",
                                              // Special
                                              "Grayscale",
                                              "B8G8R8A8Unorm",
                                              "R8G8B8A8Srgb",
                                              "R10G10B10A2Unorm",
                                              "R10G10B10A2Uint",
                                              "R11G11B10Float",
                                              // Depth
                                              "Depth16Unorm",
                                              "DepthFloat",
                                              "DepthStencil",
                                              // Compressed
                                              "ETC_R8G8B8_Unorm",
                                              "ETC2_R8G8B8_Unorm",
                                              "ETC2_R8G8B8A1_Unorm",
                                              "ETC2_R8G8B8A8_Unorm",
                                              "ETC2_R8G8B8_sRGB",
                                              "ETC2_R8G8B8A1_sRGB",
                                              "ETC2_R8G8B8A8_sRGB",
                                              "BC3_sRGBA",
                                              "BC3_RGBA",
                                              "BC7_sRGBA",
                                              "BC7_RGBA",
                                              "ASTC_4x4_Unorm",
                                              "ASTC_4x4_sRGB"};
    static_assert(sizeof(kPixelFormatNames) / sizeof(kPixelFormatNames[0]) ==
                      static_cast<size_t>(snap::rhi::PixelFormat::Count),
                  "PixelFormat names table size mismatch");

    oss << "PixelFormatProperties:" << '\n';
    for (uint32_t i = 0; i < static_cast<uint32_t>(snap::rhi::PixelFormat::Count); ++i) {
        const auto& fp = caps.formatProperties[i];
        const char* name =
            (i < (sizeof(kPixelFormatNames) / sizeof(kPixelFormatNames[0]))) ? kPixelFormatNames[i] : "Unknown";
        oss << "  FormatIndex=" << i << " (snap::rhi::PixelFormat::" << name
            << ") features=" << formatFeaturesToStr(fp.textureFeatures)
            << " framebufferSamples=" << sampleCountToStr(fp.framebufferSampleCounts)
            << " sampledColor2D=" << sampleCountToStr(fp.sampledTexture2DColorSampleCounts)
            << " sampledColor2DArray=" << sampleCountToStr(fp.sampledTexture2DArrayColorSampleCounts) << '\n';
    }

    // Vertex attribute format bitset
    // Replace previous compact index-only listing with detailed name mapping
    static const char* kVertexAttributeFormatNames[] = {"Undefined",
                                                        // 8-bit signed
                                                        "Byte2",
                                                        "Byte3",
                                                        "Byte4",
                                                        // 8-bit signed normalized
                                                        "Byte2Normalized",
                                                        "Byte3Normalized",
                                                        "Byte4Normalized",
                                                        // 8-bit unsigned
                                                        "UnsignedByte2",
                                                        "UnsignedByte3",
                                                        "UnsignedByte4",
                                                        // 8-bit unsigned normalized
                                                        "UnsignedByte2Normalized",
                                                        "UnsignedByte3Normalized",
                                                        "UnsignedByte4Normalized",
                                                        // 16-bit signed
                                                        "Short2",
                                                        "Short3",
                                                        "Short4",
                                                        // 16-bit signed normalized
                                                        "Short2Normalized",
                                                        "Short3Normalized",
                                                        "Short4Normalized",
                                                        // 16-bit unsigned
                                                        "UnsignedShort2",
                                                        "UnsignedShort3",
                                                        "UnsignedShort4",
                                                        // 16-bit unsigned normalized
                                                        "UnsignedShort2Normalized",
                                                        "UnsignedShort3Normalized",
                                                        "UnsignedShort4Normalized",
                                                        // 16-bit float
                                                        "HalfFloat2",
                                                        "HalfFloat3",
                                                        "HalfFloat4",
                                                        // 32-bit float
                                                        "Float",
                                                        "Float2",
                                                        "Float3",
                                                        "Float4",
                                                        // 32-bit int
                                                        "Int",
                                                        "Int2",
                                                        "Int3",
                                                        "Int4",
                                                        // 32-bit uint
                                                        "UInt",
                                                        "UInt2",
                                                        "UInt3",
                                                        "UInt4"};
    static_assert(sizeof(kVertexAttributeFormatNames) / sizeof(kVertexAttributeFormatNames[0]) ==
                      static_cast<size_t>(snap::rhi::VertexAttributeFormat::Count),
                  "VertexAttributeFormat names table size mismatch");

    oss << "VertexAttributeFormatProperties:" << '\n';
    bool anyVAF = false;
    for (uint32_t i = 0; i < static_cast<uint32_t>(snap::rhi::VertexAttributeFormat::Count); ++i) {
        if (caps.vertexAttributeFormatProperties[i]) {
            anyVAF = true;
            const char* name = kVertexAttributeFormatNames[i];
            oss << "  index=" << i << " (snap::rhi::VertexAttributeFormat::" << name << ")" << '\n';
        }
    }
    if (!anyVAF) {
        oss << "  None" << '\n';
    }

    // constantInputRateSupportingBits
    oss << "constantInputRateSupportingBits=" << static_cast<uint32_t>(caps.constantInputRateSupportingBits) << '\n';
    oss << "isVertexInputRatePerInstanceSupported=" << boolToStr(caps.isVertexInputRatePerInstanceSupported) << '\n';

    SNAP_RHI_REPORT(
        validationLayer, snap::rhi::ReportLevel::Info, snap::rhi::ValidationTag::CreateOp, "%s", oss.str().c_str());
}

snap::rhi::backend::common::TextureInteropSyncInfo processTextureInterop(std::span<snap::rhi::CommandBuffer*> buffers,
                                                                         snap::rhi::Fence*& fence) {
    if (buffers.empty()) {
        return {};
    }

    TextureInteropSyncInfo result{};

    auto* device =
        snap::rhi::backend::common::smart_cast<snap::rhi::backend::common::Device>(buffers.front()->getDevice());
    const auto targetDeviceAPI = device->getCapabilities().apiDescription.api;

    std::vector<snap::rhi::TextureInterop*> interopTextures;

    for (size_t i = 0; i < buffers.size(); ++i) {
        auto* commandBuffer =
            snap::rhi::backend::common::smart_cast<snap::rhi::backend::common::CommandBuffer>(buffers[i]);
        const auto& cmdBufferInteropTextures = commandBuffer->getInteropTextures();
        for (const auto& interopTexture : cmdBufferInteropTextures) {
            interopTextures.push_back(interopTexture);
        }
    }
    std::ranges::sort(interopTextures);
    interopTextures.resize(std::unique(interopTextures.begin(), interopTextures.end()) - interopTextures.begin());

    if (!fence && !interopTextures.empty()) {
        /**
         * Pool can optimase it, but fence has to retain device reference.
         */
        result.signalFence = device->createFence({});
        fence = result.signalFence.get();
    }

    if (interopTextures.empty()) {
        return {};
    }

    std::shared_ptr<PlatformSyncHandle> platformSyncHandle = nullptr;
    if (fence) {
        std::shared_ptr ptr = fence->exportPlatformSyncHandle();
        platformSyncHandle = std::static_pointer_cast<PlatformSyncHandle>(ptr);
    }

    std::unordered_set<PlatformSyncHandle*> uniqueSyncHandles;
    for (const auto& interopTexture : interopTextures) {
        auto* implTextureInterop =
            snap::rhi::backend::common::smart_cast<snap::rhi::backend::common::TextureInterop>(interopTexture);
        auto currentAPI = implTextureInterop->getCurrentAPI();

        auto waitSync = implTextureInterop->sync(device, platformSyncHandle);
        if (currentAPI == snap::rhi::API::Undefined) {
            continue;
        }
        assert(waitSync);

        if (currentAPI != targetDeviceAPI && !uniqueSyncHandles.contains(waitSync.get())) {
#if SNAP_RHI_OS_APPLE()
            /**
             * On Apple platform wait until scheduled will be enought for sync between different APIs.
             */
            if (const auto& signalFence = waitSync->getFence(); signalFence) {
                signalFence->waitForScheduled();
            }
#else
#if SNAP_RHI_OS_ANDROID()
            auto* androidHandle =
                dynamic_cast<snap::rhi::backend::common::platform::android::SyncHandle*>(waitSync.get());
            if (androidHandle) {
                auto waitSemaphore = device->createSemaphore(snap::rhi::SemaphoreCreateInfo{}, platformSyncHandle);
                result.waitSemaphores.push_back(std::move(waitSemaphore));
                continue;
            }
#endif
            /**
             * Before using interop texture used on another API, we need to ensure that the
             * operations on that API are completed. The only way to do that here is to wait
             * on the fence associated with the interop texture.
             */
            if (const auto& signalFence = waitSync->getFence(); signalFence) {
                signalFence->waitForComplete();
            }
#endif
        }
    }

    return result;
}

snap::rhi::TextureCreateInfo buildTextureCreateInfoFromView(const snap::rhi::TextureViewCreateInfo& viewCreateInfo) {
    snap::rhi::TextureCreateInfo result = viewCreateInfo.texture->getCreateInfo();

    result.format = viewCreateInfo.viewInfo.format;
    result.mipLevels = viewCreateInfo.viewInfo.range.levelCount;
    result.size.depth = viewCreateInfo.viewInfo.range.layerCount;
    result.textureType = viewCreateInfo.viewInfo.textureType;
    result.components = viewCreateInfo.viewInfo.components;

    return result;
}

std::shared_ptr<snap::rhi::Device> createDeviceSafe(std::shared_ptr<snap::rhi::Device> device) {
    auto* devicePtr = device.get();
    return {devicePtr, [deviceRef = std::move(device)](auto* ptr) {
                auto* devicePtr =
                    snap::rhi::backend::common::smart_cast<snap::rhi::backend::common::Device>(deviceRef.get());
                devicePtr->waitForTrackedSubmissions();
                SNAP_RHI_VALIDATE(
                    devicePtr->getValidationLayer(),
                    deviceRef.use_count() == 1,
                    snap::rhi::ReportLevel::Warning,
                    snap::rhi::ValidationTag::DeviceOp,
                    "[snap::rhi::backend::common::createDeviceSafe] Device is being destroyed while there are "
                    "still %zu references to it.",
                    deviceRef.use_count() - 1);
                // Device will be destroyed when the last shared_ptr reference goes out of scope
            }};
}

#if SNAP_RHI_ENABLE_SLOW_VALIDATIONS()
void validateResourceLifetimes(snap::rhi::Device* device) {
    auto* deviceImpl = snap::rhi::backend::common::smart_cast<snap::rhi::backend::common::Device>(device);
    const auto& submissionTrackerPtr = deviceImpl->getSubmissionTrackerPtr();
    if (!submissionTrackerPtr) {
        return;
    }

    submissionTrackerPtr->validateResourceLifetimes();
}
#endif

snap::rhi::TextureViewInfo buildTextureViewInfo(const snap::rhi::TextureViewInfo& srcView,
                                                const snap::rhi::TextureViewInfo& targetView) {
    snap::rhi::TextureViewInfo result = targetView;
    result.range.baseArrayLayer += srcView.range.baseArrayLayer;
    result.range.baseMipLevel += srcView.range.baseMipLevel;
    return result;
}

snap::rhi::TextureViewCreateInfo buildTextureViewCreateInfo(const snap::rhi::TextureViewCreateInfo& srcView,
                                                            const snap::rhi::TextureViewCreateInfo& targetView) {
    snap::rhi::TextureViewCreateInfo result = targetView;
    result.viewInfo = buildTextureViewInfo(srcView.viewInfo, targetView.viewInfo);
    result.texture = srcView.texture;
    return result;
}
} // namespace snap::rhi::backend::common
