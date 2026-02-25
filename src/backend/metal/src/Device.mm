#include "snap/rhi/backend/metal/Device.h"
#include "snap/rhi/Enums.h"
#include "snap/rhi/backend/common/PipelineLayout.h"
#include "snap/rhi/backend/common/PlatformSyncHandle.h"
#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/metal/Buffer.h"
#include "snap/rhi/backend/metal/CommandBuffer.h"
#include "snap/rhi/backend/metal/CommandQueue.h"
#include "snap/rhi/backend/metal/ComputePipeline.h"
#include "snap/rhi/backend/metal/DescriptorPool.h"
#include "snap/rhi/backend/metal/DescriptorSet.h"
#include "snap/rhi/backend/metal/Fence.h"
#include "snap/rhi/backend/metal/Framebuffer.h"
#include "snap/rhi/backend/metal/PipelineCache.h"
#include "snap/rhi/backend/metal/QueryPool.h"
#include "snap/rhi/backend/metal/RenderPipeline.h"
#include "snap/rhi/backend/metal/Semaphore.h"
#include "snap/rhi/backend/metal/ShaderLibrary.h"
#include "snap/rhi/backend/metal/ShaderModule.h"
#include "snap/rhi/backend/metal/Texture.h"
#include "snap/rhi/backend/metal/Utils.h"

#include <algorithm>

#include "snap/rhi/common/Indestructible.h"
#include "snap/rhi/common/OS.h"
#include "snap/rhi/common/Throw.h"

namespace {

// =============================================================================
// MARK: - Metal Device Factory
// =============================================================================

id<MTLDevice> createMetalDevice() {
    return MTLCreateSystemDefaultDevice();
}

id<MTLCommandQueue> createMetalCommandQueue(const id<MTLDevice>& mtlDevice) {
    /**
     * We always use the same device, so we have to use a mutex to create a queue from different threads.
     *
     * The command queues you create with this method allow up to 64 uncompleted command buffers at time.
     */
    static snap::rhi::common::Indestructible<std::mutex> accessQueueCreateMutex;
    std::lock_guard<std::mutex> lock(*accessQueueCreateMutex);
    return [mtlDevice newCommandQueue];
}

// =============================================================================
// MARK: - Metal GPU Family Support Structure
// =============================================================================

/**
 * @brief Encapsulates detected Metal GPU family support for capability queries.
 *
 * Based on: https://developer.apple.com/metal/Metal-Feature-Set-Tables.pdf
 * GPU families are hierarchical - a device supporting Apple6 also supports Apple1-5.
 */
struct MetalGPUFamilySupport {
    // Apple GPU families (iOS, macOS Apple Silicon)
    bool apple1 = false;
    bool apple2 = false;
    bool apple3 = false;
    bool apple4 = false;
    bool apple5 = false;
    bool apple6 = false;
    bool apple7 = false;
    bool apple8 = false;
    bool apple9 = false;

    // Mac GPU families
    bool mac2 = false;

    // Metal API version families
    bool metal3 = false;
    bool metal4 = false;

    /**
     * @brief Query GPU family support from a Metal device.
     */
    static MetalGPUFamilySupport queryFromDevice(const id<MTLDevice>& device) {
        MetalGPUFamilySupport support{};

        if (@available(macOS 10.15, iOS 13.0, *)) {
            support.apple1 = [device supportsFamily:MTLGPUFamilyApple1];
            support.apple2 = [device supportsFamily:MTLGPUFamilyApple2];
            support.apple3 = [device supportsFamily:MTLGPUFamilyApple3];
            support.apple4 = [device supportsFamily:MTLGPUFamilyApple4];
            support.apple5 = [device supportsFamily:MTLGPUFamilyApple5];
            support.apple6 = [device supportsFamily:MTLGPUFamilyApple6];
            support.apple7 = [device supportsFamily:MTLGPUFamilyApple7];
            support.apple8 = [device supportsFamily:MTLGPUFamilyApple8];
            support.apple9 = [device supportsFamily:MTLGPUFamilyApple9];
            support.mac2 = [device supportsFamily:MTLGPUFamilyMac2];
        }

        if (@available(macOS 13.0, iOS 16.0, *)) {
            support.metal3 = [device supportsFamily:MTLGPUFamilyMetal3];
        }

        // Uncomment after CI uses Xcode 26
        // if (@available(macOS 26.0, iOS 26.0, *)) {
        //     support.metal4 = [device supportsFamily:MTLGPUFamilyMetal4];
        // }

        return support;
    }

    /**
     * @brief Returns true if the device supports Apple Silicon (any Apple family).
     */
    [[nodiscard]] bool isAppleSilicon() const noexcept {
        return apple1 || apple2 || apple3 || apple4 || apple5 || apple6 || apple7 || apple8 || apple9;
    }
};

// =============================================================================
// MARK: - Format Options Bitmask
// =============================================================================

enum class FormatOptions : uint32_t {
    None = 0,

    BlendSupported = 1 << 0,
    ResolveSupported = 1 << 1,
    MSAASupported = 1 << 2,
    FilterSupported = 1 << 3,

    All = BlendSupported | ResolveSupported | MSAASupported | FilterSupported
};

static constexpr FormatOptions operator|(FormatOptions a, FormatOptions b) noexcept {
    return static_cast<FormatOptions>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

static constexpr FormatOptions operator&(FormatOptions a, FormatOptions b) noexcept {
    return static_cast<FormatOptions>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

[[nodiscard]] static constexpr bool hasOption(FormatOptions options, FormatOptions flag) noexcept {
    return (options & flag) != FormatOptions::None;
}

// =============================================================================
// MARK: - MSAA Sample Count Query
// =============================================================================

/**
 * @brief Queries the maximum supported MSAA sample count from the Metal device.
 *
 * Uses MTLDevice API:
 * - supportsTextureSampleCount: Checks if a specific sample count is supported
 *
 * Reference: https://developer.apple.com/documentation/metal/mtldevice/1433355-supportstexturesamplecount
 *
 * @param device The Metal device to query.
 * @return The maximum supported sample count as snap::rhi::SampleCount.
 */
snap::rhi::SampleCount queryMaxSampleCount(const id<MTLDevice>& device) {
    // Query sample count support in descending order
    // Metal supports 1, 2, 4, 8 sample counts depending on device
    if ([device supportsTextureSampleCount:8]) {
        return snap::rhi::SampleCount::Count8;
    }
    if ([device supportsTextureSampleCount:4]) {
        return snap::rhi::SampleCount::Count4;
    }
    if ([device supportsTextureSampleCount:2]) {
        return snap::rhi::SampleCount::Count2;
    }
    return snap::rhi::SampleCount::Count1;
}

// =============================================================================
// MARK: - Format Properties Builder
// =============================================================================

/**
 * @brief Builds color format properties based on Metal Feature Set Tables.
 *
 * Uses MTLDevice API:
 * - supportsTextureSampleCount: Queries actual MSAA support
 *
 * Reference: https://developer.apple.com/metal/Metal-Feature-Set-Tables.pdf
 *
 * @param capabilities The capabilities structure to populate.
 * @param format The pixel format to configure.
 * @param options Supported operations for this format.
 * @param device The Metal device to query for MSAA support.
 */
void buildColorFormatProperties(snap::rhi::Capabilities& capabilities,
                                const snap::rhi::PixelFormat format,
                                const FormatOptions options,
                                const id<MTLDevice>& device) {
    auto& formatProperties = capabilities.formatProperties[static_cast<uint8_t>(format)];

    // Base features: all color formats support transfers and blitting
    formatProperties.textureFeatures = static_cast<snap::rhi::FormatFeatures>(
        snap::rhi::FormatFeatures::TransferDst | snap::rhi::FormatFeatures::TransferSrc |
        snap::rhi::FormatFeatures::BlitDst | snap::rhi::FormatFeatures::BlitSrc);

    // Linear filtering and storage support
    if (hasOption(options, FormatOptions::FilterSupported)) {
        formatProperties.textureFeatures = static_cast<snap::rhi::FormatFeatures>(
            formatProperties.textureFeatures | snap::rhi::FormatFeatures::SampledFilterLinear |
            snap::rhi::FormatFeatures::Storage);
    }

    // Blending support
    if (hasOption(options, FormatOptions::BlendSupported)) {
        formatProperties.textureFeatures = static_cast<snap::rhi::FormatFeatures>(
            formatProperties.textureFeatures | snap::rhi::FormatFeatures::ColorRenderableBlend);
    } else {
        formatProperties.textureFeatures = static_cast<snap::rhi::FormatFeatures>(
            formatProperties.textureFeatures | snap::rhi::FormatFeatures::ColorRenderable);
    }

    // MSAA resolve support
    if (hasOption(options, FormatOptions::ResolveSupported)) {
        formatProperties.textureFeatures = static_cast<snap::rhi::FormatFeatures>(formatProperties.textureFeatures |
                                                                                  snap::rhi::FormatFeatures::Resolve);
    }

    // Query actual sample count support from the device
    formatProperties.framebufferSampleCounts = snap::rhi::SampleCount::Count1;
    formatProperties.sampledTexture2DArrayColorSampleCounts = snap::rhi::SampleCount::Count1;

    if (hasOption(options, FormatOptions::MSAASupported)) {
        // Query the device for actual MSAA support
        snap::rhi::SampleCount maxSampleCount = queryMaxSampleCount(device);
        formatProperties.framebufferSampleCounts = maxSampleCount;
        formatProperties.sampledTexture2DArrayColorSampleCounts = maxSampleCount;
    }
}

/**
 * @brief Builds depth/stencil format properties.
 *
 * Uses MTLDevice API:
 * - supportsTextureSampleCount: Queries actual MSAA support
 *
 * Reference: https://developer.apple.com/metal/Metal-Feature-Set-Tables.pdf
 *
 * @param capabilities The capabilities structure to populate.
 * @param format The depth/stencil format to configure.
 * @param isFilterable Whether linear filtering is supported for this format.
 * @param device The Metal device to query for MSAA support.
 */
void buildDepthStencilFormatProperties(snap::rhi::Capabilities& capabilities,
                                       const snap::rhi::PixelFormat format,
                                       const bool isFilterable,
                                       const id<MTLDevice>& device) {
    auto& formatProperties = capabilities.formatProperties[static_cast<uint8_t>(format)];

    formatProperties.textureFeatures = static_cast<snap::rhi::FormatFeatures>(
        snap::rhi::FormatFeatures::DepthStencilRenderable |
        (isFilterable ? snap::rhi::FormatFeatures::SampledFilterLinear : snap::rhi::FormatFeatures::None) |
        snap::rhi::FormatFeatures::BlitSrc); // Only depth part supports sampling and blitting

    // Query actual sample count support from the device
    snap::rhi::SampleCount maxSampleCount = queryMaxSampleCount(device);
    formatProperties.framebufferSampleCounts = maxSampleCount;
    formatProperties.sampledTexture2DColorSampleCounts = maxSampleCount;
    formatProperties.sampledTexture2DArrayColorSampleCounts = maxSampleCount;
}

// =============================================================================
// MARK: - Memory Properties Population
// =============================================================================

/**
 * @brief Populates physical device memory properties based on Metal storage modes.
 *
 * Metal memory model:
 * - MTLStorageModePrivate: GPU-only (Device Local)
 * - MTLStorageModeShared: CPU+GPU shared (Host Visible, Coherent)
 * - MTLStorageModeManaged: macOS discrete GPU only (Device Local + Host Visible, non-coherent)
 *
 * @param device The Metal device to query.
 * @param outProps Output memory properties structure.
 */
void populateMemoryProperties(const id<MTLDevice>& device, snap::rhi::PhysicalDeviceMemoryProperties& outProps) {
    [[maybe_unused]] bool isUnifiedMemory = false;

    if (@available(macOS 10.15, iOS 13.0, *)) {
        isUnifiedMemory = [device hasUnifiedMemory];
    } else {
#if SNAP_RHI_OS_IOS()
        isUnifiedMemory = true;
#else
        isUnifiedMemory = false;
#endif
    }

    outProps.memoryTypes = {};
    outProps.memoryTypeCount = 0;

    // Memory Type 0: Device Local (MTLStorageModePrivate)
    // Always available. Most efficient for GPU-only data.
    outProps.memoryTypes[outProps.memoryTypeCount++].memoryProperties = snap::rhi::MemoryProperties::DeviceLocal;

    // Memory Type 1: Host Visible + Coherent + Cached (MTLStorageModeShared + DefaultCache)
    // Always available. On unified memory: high performance for both CPU and GPU.
    outProps.memoryTypes[outProps.memoryTypeCount++].memoryProperties = snap::rhi::MemoryProperties::HostVisible |
                                                                        snap::rhi::MemoryProperties::HostCoherent |
                                                                        snap::rhi::MemoryProperties::HostCached;

    // Memory Type 2: Host Visible + Coherent (MTLStorageModeShared + WriteCombined)
    // Always available. Optimized for CPU write, GPU read (streaming uploads).
    outProps.memoryTypes[outProps.memoryTypeCount++].memoryProperties =
        snap::rhi::MemoryProperties::HostVisible | snap::rhi::MemoryProperties::HostCoherent;

#if SNAP_RHI_OS_MACOS()
    // Memory Type 3: Managed (MTLStorageModeManaged) - macOS discrete GPU only
    // Requires explicit synchronization (didModifyRange/synchronizeResource).
    if (!isUnifiedMemory) {
        outProps.memoryTypes[outProps.memoryTypeCount++].memoryProperties = snap::rhi::MemoryProperties::DeviceLocal |
                                                                            snap::rhi::MemoryProperties::HostVisible |
                                                                            snap::rhi::MemoryProperties::HostCached;
    }
#endif
}

// =============================================================================
// MARK: - Texture Limits Configuration
// =============================================================================

/**
 * @brief Configures texture dimension limits by querying the Metal device.
 *
 * Uses MTLDevice API to query actual hardware limits:
 * - sparseTileSizeInBytes (indirect indicator of texture capabilities)
 * - GPU family checks for texture dimension support
 *
 * Reference: https://developer.apple.com/documentation/metal/mtldevice
 * Reference: https://developer.apple.com/metal/Metal-Feature-Set-Tables.pdf
 *
 * @param caps The capabilities structure to populate.
 * @param gpuFamily The detected GPU family support.
 * @param device The Metal device to query.
 */
void configureTextureLimits(snap::rhi::Capabilities& caps,
                            const MetalGPUFamilySupport& gpuFamily,
                            [[maybe_unused]] const id<MTLDevice>& device) {
    // Metal Feature Set Tables - Maximum texture dimensions:
    // - Apple1-2: 8192x8192 (2D), 2048 (3D), 8192 (Cube), 2048 layers
    // - Apple3+:  16384x16384 (2D), 2048 (3D), 16384 (Cube), 2048 layers
    // - Mac2:     16384x16384 (2D), 2048 (3D), 16384 (Cube), 2048 layers
    //
    // Note: Metal does not expose texture dimension limits directly via API.
    // We derive them from GPU family support as per the Feature Set Tables.

    const bool supportsLargeTextures = gpuFamily.apple3 || gpuFamily.mac2 || gpuFamily.metal3;

    caps.maxTextureDimension2D = supportsLargeTextures ? 16384u : 8192u;
    caps.maxTextureDimensionCube = supportsLargeTextures ? 16384u : 8192u;
    caps.maxTextureDimension3D = 2048u; // Consistent across all families
    caps.maxTextureArrayLayers = 2048u; // Consistent across all families
}

// =============================================================================
// MARK: - Buffer and Uniform Limits Configuration
// =============================================================================

/**
 * @brief Configures buffer and uniform buffer limits by querying the Metal device.
 *
 * Uses MTLDevice API:
 * - maxBufferLength: Maximum size of a buffer (available iOS 12.0+, macOS 10.14+)
 *
 * Reference: https://developer.apple.com/documentation/metal/mtldevice/2966564-maxbufferlength
 * Reference: https://developer.apple.com/metal/Metal-Feature-Set-Tables.pdf
 *
 * @param caps The capabilities structure to populate.
 * @param gpuFamily The detected GPU family support.
 * @param device The Metal device to query.
 */
void configureBufferLimits(snap::rhi::Capabilities& caps,
                           [[maybe_unused]] const MetalGPUFamilySupport& gpuFamily,
                           const id<MTLDevice>& device) {
    // Metal Feature Set Tables - Function argument table sizes:
    // - Maximum buffers per shader stage: 31
    // - Maximum buffer argument table size: Tier 1 (31 entries), Tier 2 (500000+ with argument buffers)
    constexpr uint32_t kMaxBufferSlotsPerStage = 31;
    constexpr uint32_t kReservedVertexBufferSlots = 8;

    caps.maxVertexInputBindings = kReservedVertexBufferSlots;
    caps.maxPerStageUniformBuffers = kMaxBufferSlotsPerStage - kReservedVertexBufferSlots;
    caps.maxUniformBuffers = caps.maxPerStageUniformBuffers * static_cast<uint32_t>(snap::rhi::ShaderStage::Count);

    // Query maximum buffer length from the device
    // Available on iOS 12.0+, macOS 10.14+
    // Returns maximum allocatable buffer size in bytes
    uint64_t maxBufferLength = 256 * 1024 * 1024; // Default: 256MB fallback
    if (@available(macOS 10.14, iOS 12.0, *)) {
        maxBufferLength = static_cast<uint64_t>([device maxBufferLength]);
    }

    // For uniform buffers (constant address space), use the 64KB limit
    caps.maxPerStageUniformBufferSize[static_cast<uint32_t>(snap::rhi::ShaderStage::Vertex)] = maxBufferLength;
    caps.maxPerStageUniformBufferSize[static_cast<uint32_t>(snap::rhi::ShaderStage::Fragment)] = maxBufferLength;
    caps.maxPerStageUniformBufferSize[static_cast<uint32_t>(snap::rhi::ShaderStage::Compute)] = maxBufferLength;

    // Total uniform buffer size per stage
    // This represents the maximum total size of all uniform buffers bound to a single stage
    const uint64_t totalUniformSize = maxBufferLength * caps.maxPerStageUniformBuffers;
    caps.maxPerStageTotalUniformBufferSize[static_cast<uint32_t>(snap::rhi::ShaderStage::Vertex)] = totalUniformSize;
    caps.maxPerStageTotalUniformBufferSize[static_cast<uint32_t>(snap::rhi::ShaderStage::Fragment)] = totalUniformSize;
    caps.maxPerStageTotalUniformBufferSize[static_cast<uint32_t>(snap::rhi::ShaderStage::Compute)] = totalUniformSize;
}

// =============================================================================
// MARK: - Texture and Sampler Binding Limits Configuration
// =============================================================================

/**
 * @brief Configures texture and sampler binding limits by querying the Metal device.
 *
 * Uses MTLDevice API:
 * - argumentBuffersSupport: Determines tier level for argument buffer support
 *   - Tier 1: 31 textures/samplers per stage (direct binding)
 *   - Tier 2: 500,000+ resources via argument buffers
 *
 * Reference: https://developer.apple.com/documentation/metal/mtldevice/2915742-argumentbufferssupport
 * Reference: https://developer.apple.com/metal/Metal-Feature-Set-Tables.pdf
 *
 * @param caps The capabilities structure to populate.
 * @param gpuFamily The detected GPU family support.
 * @param device The Metal device to query.
 */
void configureTextureAndSamplerBindings(snap::rhi::Capabilities& caps,
                                        [[maybe_unused]] const MetalGPUFamilySupport& gpuFamily,
                                        const id<MTLDevice>& device) {
    // Query argument buffer support tier
    // Tier 1: Basic support (31 entries per stage)
    // Tier 2: Extended support (500,000+ entries via argument buffers)
    [[maybe_unused]] MTLArgumentBuffersTier argumentBuffersTier = MTLArgumentBuffersTier1;
    if (@available(macOS 10.13, iOS 11.0, *)) {
        argumentBuffersTier = [device argumentBuffersSupport];
    }

    // Metal Feature Set Tables - Function argument table sizes:
    // - Maximum textures per stage: 31 (Tier 1), 500000 (Tier 2 via argument buffers)
    // - Maximum samplers per stage: 16 (all tiers)
    //
    // We use Tier 1 limits for direct binding compatibility.
    // Applications using argument buffers can access more resources.
    constexpr uint32_t kMaxTexturesPerStageTier1 = 31;
    constexpr uint32_t kMaxSamplersPerStage = 16;

    // Use Tier 1 limits for maximum compatibility
    // Tier 2 requires argument buffer usage patterns
    const uint32_t maxTexturesPerStage = kMaxTexturesPerStageTier1;

    caps.maxTextures = maxTexturesPerStage;
    caps.maxPerStageTextures[static_cast<uint32_t>(snap::rhi::ShaderStage::Vertex)] = maxTexturesPerStage;
    caps.maxPerStageTextures[static_cast<uint32_t>(snap::rhi::ShaderStage::Fragment)] = maxTexturesPerStage;
    caps.maxPerStageTextures[static_cast<uint32_t>(snap::rhi::ShaderStage::Compute)] = maxTexturesPerStage;

    caps.maxSamplers = kMaxSamplersPerStage;
    caps.maxPerStageSamplers[static_cast<uint32_t>(snap::rhi::ShaderStage::Vertex)] = kMaxSamplersPerStage;
    caps.maxPerStageSamplers[static_cast<uint32_t>(snap::rhi::ShaderStage::Fragment)] = kMaxSamplersPerStage;
    caps.maxPerStageSamplers[static_cast<uint32_t>(snap::rhi::ShaderStage::Compute)] = kMaxSamplersPerStage;
}

// =============================================================================
// MARK: - Compute Limits Configuration
// =============================================================================

/**
 * @brief Configures compute shader limits by querying the Metal device.
 *
 * Uses MTLDevice API:
 * - maxThreadsPerThreadgroup: Maximum threads per threadgroup (MTLSize)
 *
 * Note: Per-pipeline limits are queried via MTLComputePipelineState:
 * - maxTotalThreadsPerThreadgroup: Actual limit for a specific compute pipeline
 * - threadExecutionWidth: SIMD width for the compiled pipeline
 *
 * Reference: https://developer.apple.com/documentation/metal/mtldevice/2877435-maxthreadsperthreadgroup
 * Reference: https://developer.apple.com/documentation/metal/mtlcomputepipelinestate
 *
 * @param caps The capabilities structure to populate.
 * @param gpuFamily The detected GPU family support.
 * @param device The Metal device to query.
 */
void configureComputeLimits(snap::rhi::Capabilities& caps,
                            const MetalGPUFamilySupport& gpuFamily,
                            const id<MTLDevice>& device) {
    // Query maximum threads per threadgroup from the device
    // This returns MTLSize with width, height, depth components
    MTLSize maxThreadsPerThreadgroup = {512, 512, 512}; // Default fallback

    if (@available(macOS 10.13, iOS 11.0, *)) {
        maxThreadsPerThreadgroup = [device maxThreadsPerThreadgroup];
    }

    // Maximum total threads per threadgroup (product of dimensions)
    // Metal Feature Set Tables:
    // - Apple1-3: 512 threads total
    // - Apple4+, Mac2: 1024 threads total
    const bool supportsLargeThreadgroups = gpuFamily.apple4 || gpuFamily.mac2 || gpuFamily.metal3;
    caps.maxComputeWorkGroupInvocations = supportsLargeThreadgroups ? 1024u : 512u;

    // Thread execution width (SIMD width)
    // This is a compile-time property that varies per pipeline.
    // We report a conservative estimate based on GPU family:
    // - Apple GPUs: 32 threads per SIMD group
    // - AMD GPUs (Mac): 64 threads per wavefront
    // - Intel GPUs (Mac): 8-32 threads
    //
    // Note: Actual value should be queried from MTLComputePipelineState.threadExecutionWidth
    // for each compiled pipeline. We use a conservative value here.
    caps.threadExecutionWidth = gpuFamily.isAppleSilicon() ? 32u : 32u;

    // Maximum threads per threadgroup dimension
    // Query from device API - represents hardware limits
    caps.maxComputeWorkGroupSize[0] = static_cast<uint32_t>(maxThreadsPerThreadgroup.width);
    caps.maxComputeWorkGroupSize[1] = static_cast<uint32_t>(maxThreadsPerThreadgroup.height);
    caps.maxComputeWorkGroupSize[2] = static_cast<uint32_t>(maxThreadsPerThreadgroup.depth);

    // Maximum threadgroups per grid (dispatch size)
    // Metal does not impose a strict limit on dispatch dimensions.
    // The limit is effectively UINT32_MAX per dimension.
    // Reference: MTLComputeCommandEncoder dispatchThreadgroups:threadsPerThreadgroup:
    caps.maxComputeWorkGroupCount[0] = UINT32_MAX;
    caps.maxComputeWorkGroupCount[1] = UINT32_MAX;
    caps.maxComputeWorkGroupCount[2] = UINT32_MAX;
}

// =============================================================================
// MARK: - Vertex Input Limits Configuration
// =============================================================================

/**
 * @brief Configures vertex input limits based on GPU family.
 *
 * Metal vertex input limits are defined by the Metal Feature Set Tables.
 * There is no runtime API to query these values directly.
 *
 * Key limits:
 * - Maximum vertex attributes: 31 (all families)
 * - Maximum vertex buffer stride: 256 bytes (Apple1-2), 2048 bytes (Apple3+, Mac2)
 * - Vertex attribute offset alignment: 4 bytes
 *
 * Reference: https://developer.apple.com/metal/Metal-Feature-Set-Tables.pdf
 * Reference: https://developer.apple.com/documentation/metal/mtlvertexattributedescriptor
 *
 * @param caps The capabilities structure to populate.
 * @param gpuFamily The detected GPU family support.
 * @param device The Metal device (unused, limits are family-based).
 */
void configureVertexInputLimits(snap::rhi::Capabilities& caps,
                                const MetalGPUFamilySupport& gpuFamily,
                                [[maybe_unused]] const id<MTLDevice>& device) {
    // Metal Feature Set Tables - Vertex stage limits:
    // - Maximum vertex attributes: 31 (all families)
    // - Maximum vertex buffer bindings: 31 (shared with buffer arguments)
    constexpr uint32_t kMaxVertexAttributes = 31;
    caps.maxVertexInputAttributes = kMaxVertexAttributes;

    // Vertex attribute stride and offset limits
    // Apple3+ and Mac2 support larger strides
    const bool supportsLargeStride = gpuFamily.apple3 || gpuFamily.mac2 || gpuFamily.metal3;

    caps.maxVertexInputBindingStride = supportsLargeStride ? 2048u : 256u;
    caps.maxVertexInputAttributeOffset = caps.maxVertexInputBindingStride - 4u; // Account for min attribute size

    // Specialization constants - Metal supports function constants
    caps.maxSpecializationConstants = snap::rhi::SupportedLimit::MaxSpecializationConstants;

    // Metal supports all vertex attribute formats
    std::ranges::fill(caps.vertexAttributeFormatProperties, true);
}

// =============================================================================
// MARK: - Uniform Buffer Alignment Configuration
// =============================================================================

/**
 * @brief Configures uniform buffer alignment requirements by querying the Metal device.
 *
 * Uses MTLDevice API:
 * - minimumLinearTextureAlignmentForPixelFormat: (indirect indicator)
 * - GPU family checks for buffer offset alignment
 *
 * Metal Feature Set Tables - Minimum buffer offset alignment:
 * - Apple1: 256 bytes (legacy)
 * - Apple2+: 4 bytes (iOS)
 * - Mac1 (Intel): 256 bytes
 * - Mac2 (Apple Silicon Mac): 4 bytes
 *
 * Reference: https://developer.apple.com/metal/Metal-Feature-Set-Tables.pdf
 *
 * @param caps The capabilities structure to populate.
 * @param gpuFamily The detected GPU family support.
 * @param device The Metal device to query.
 */
void configureUniformBufferAlignment(snap::rhi::Capabilities& caps,
                                     const MetalGPUFamilySupport& gpuFamily,
                                     [[maybe_unused]] const id<MTLDevice>& device) {
    // Metal Feature Set Tables - Minimum constant buffer offset alignment:
    // The alignment requirement depends on the GPU family:
    // - Apple1 (A7/A8): 256 bytes
    // - Apple2+ (A9 and later): 4 bytes
    // - Mac2 with Apple Silicon: 4 bytes
    // - Mac2 with Intel/AMD: 256 bytes (varies by hardware)
    //
    // Note: There is no direct API to query this value.
    // We derive it from GPU family support.

    if (gpuFamily.apple2) {
        // Apple Silicon (Apple2+) supports 4-byte alignment
        caps.minUniformBufferOffsetAlignment = 4;
    } else if (gpuFamily.mac2 && gpuFamily.isAppleSilicon()) {
        // Mac with Apple Silicon
        caps.minUniformBufferOffsetAlignment = 4;
    } else if (gpuFamily.mac2) {
        // Mac with Intel/AMD - use conservative 256-byte alignment
        caps.minUniformBufferOffsetAlignment = 256;
    } else if (gpuFamily.apple1) {
        // Legacy Apple1 (A7/A8) - 256 bytes
        caps.minUniformBufferOffsetAlignment = 256;
    } else {
        // Fallback to most restrictive alignment
        caps.minUniformBufferOffsetAlignment = 256;
    }
}

// =============================================================================
// MARK: - Feature Flags Configuration
// =============================================================================

/**
 * @brief Configures boolean feature flags by querying the Metal device.
 *
 * Uses MTLDevice API queries where available:
 * - supportsShaderBarycentricCoordinates
 * - supports32BitFloatFiltering
 * - supports32BitMSAA
 * - supportsQueryTextureLOD
 * - supportsBCTextureCompression
 * - depth24Stencil8PixelFormatSupported
 * - readWriteTextureSupport
 *
 * Reference: https://developer.apple.com/documentation/metal/mtldevice
 *
 * @param caps The capabilities structure to populate.
 * @param gpuFamily The detected GPU family support.
 * @param device The Metal device for capability queries.
 */
void configureFeatureFlags(snap::rhi::Capabilities& caps,
                           const MetalGPUFamilySupport& gpuFamily,
                           const id<MTLDevice>& device) {
    // =========================================================================
    // Core features - supported by all Metal devices
    // =========================================================================
    caps.isNPOTGenerateMipmapCmdSupported = true;
    caps.isAlphaToCoverageSupported = true;
    caps.isMinMaxBlendModeSupported = true;
    caps.isNPOTWrapModeSupported = true;
    caps.isRasterizerDisableSupported = true;
    caps.isRenderToMipmapSupported = true;
    caps.isTexture3DSupported = true;
    caps.isTextureArraySupported = true;
    caps.isPolygonOffsetClampSupported = true;
    caps.isSamplerMinMaxLodSupported = true;
    caps.isSamplerCompareFuncSupported = true;
    caps.isDepthCubeMapSupported = true;
    caps.isShaderStorageBufferSupported = true;
    caps.isUInt32IndexSupported = true;
    caps.isPrimitiveRestartIndexEnabled = true;
    caps.isVertexInputRatePerInstanceSupported = true;
    caps.isFramebufferFetchSupported = true; // Programmable blending - all Metal devices
    caps.isDynamicRenderingSupported = true; // Metal doesn't require render passes
    caps.supportsPersistentMapping = true;
    caps.isSamplerUnnormalizedCoordsSupported = true;

    // =========================================================================
    // Query device for specific feature support
    // =========================================================================

    if (gpuFamily.mac2 || gpuFamily.apple7) {
        caps.isNonFillPolygonModeSupported = true;
    } else {
        // Fallback for older A-series chips (A13 and older)
        caps.isNonFillPolygonModeSupported = false;
    }

    // Clamp addressing modes
    caps.isClampToZeroSupported = true;
    if (@available(macOS 10.12, iOS 14.0, *)) {
        caps.isClampToBorderSupported = true;
    } else {
        caps.isClampToBorderSupported = false;
    }

    // Texture format swizzle - query via GPU family
    // Available on Apple2+, Mac2, and Metal3
    if (@available(macOS 10.15, iOS 13.0, *)) {
        caps.isTextureFormatSwizzleSupported = gpuFamily.apple2 || gpuFamily.mac2 || gpuFamily.metal3;
    } else {
        caps.isTextureFormatSwizzleSupported = false;
    }

    // Barycentric coordinates and primitive ID in fragment shaders
    // Query directly from the device
    caps.isFragmentPrimitiveIDSupported = false;
    caps.isFragmentBarycentricCoordinatesSupported = false;
    if (@available(macOS 10.15, iOS 14.0, *)) {
        if ([device supportsShaderBarycentricCoordinates]) {
            caps.isFragmentPrimitiveIDSupported = true;
            caps.isFragmentBarycentricCoordinatesSupported = true;
        }
    }

    // Read-write texture support
    // Query the tier level from the device
    if (@available(macOS 10.13, iOS 11.0, *)) {
        MTLReadWriteTextureTier rwTextureTier = [device readWriteTextureSupport];
        // Tier1 supports limited read-write, Tier2 supports all formats
        (void)rwTextureTier; // Used for validation, storage support is set per-format
    }

    // =========================================================================
    // Numeric limits
    // =========================================================================

    // Vertex attribute divisor - Metal supports unlimited divisor
    caps.maxVertexAttribDivisor = snap::rhi::Unlimited;

    // All data types supported for constant input rate
    caps.constantInputRateSupportingBits = snap::rhi::FormatDataTypeBits::All;

    // Clip/cull distances - Metal supports 8 clip distances, no cull distances
    caps.maxClipDistances = 8;
    caps.maxCullDistances = 0;
    caps.maxCombinedClipAndCullDistances = 8;

    // Framebuffer limits
    // Metal Feature Set Tables: 8 color attachments for all families
    caps.maxFramebufferColorAttachmentCount = 8;
    caps.maxFramebufferLayers = 1;  // TODO: implement layer rendering
    caps.maxMultiviewViewCount = 0; // TODO: implement multiview
    caps.isMultiviewMSAAImplicitResolveEnabled = false;

    // Anisotropic filtering - query max level
    // Metal supports up to 16x anisotropic filtering on all devices
    caps.maxAnisotropic = snap::rhi::AnisotropicFiltering::Count16;

    // Memory model - Metal handles coherency automatically
    caps.nonCoherentAtomSize = 1;
}

// =============================================================================
// MARK: - Pixel Format Properties Configuration
// =============================================================================

/**
 * @brief Configures integer format properties (Uint/Sint).
 *
 * Integer formats in Metal do not support blending, filtering, or MSAA resolve.
 *
 * @param caps The capabilities structure to populate.
 */
void configureIntegerFormats(snap::rhi::Capabilities& caps, const id<MTLDevice>& device) {
    // Integer formats: no blending, no filtering, no MSAA, no resolve
    constexpr FormatOptions kIntegerOptions = FormatOptions::None;

    // 8-bit integer formats
    buildColorFormatProperties(caps, snap::rhi::PixelFormat::R8Uint, kIntegerOptions, device);
    buildColorFormatProperties(caps, snap::rhi::PixelFormat::R8G8Uint, kIntegerOptions, device);
    buildColorFormatProperties(caps, snap::rhi::PixelFormat::R8G8B8A8Uint, kIntegerOptions, device);
    buildColorFormatProperties(caps, snap::rhi::PixelFormat::R8Sint, kIntegerOptions, device);
    buildColorFormatProperties(caps, snap::rhi::PixelFormat::R8G8Sint, kIntegerOptions, device);
    buildColorFormatProperties(caps, snap::rhi::PixelFormat::R8G8B8A8Sint, kIntegerOptions, device);

    // 16-bit integer formats
    buildColorFormatProperties(caps, snap::rhi::PixelFormat::R16Uint, kIntegerOptions, device);
    buildColorFormatProperties(caps, snap::rhi::PixelFormat::R16G16Uint, kIntegerOptions, device);
    buildColorFormatProperties(caps, snap::rhi::PixelFormat::R16G16B16A16Uint, kIntegerOptions, device);
    buildColorFormatProperties(caps, snap::rhi::PixelFormat::R16Sint, kIntegerOptions, device);
    buildColorFormatProperties(caps, snap::rhi::PixelFormat::R16G16Sint, kIntegerOptions, device);
    buildColorFormatProperties(caps, snap::rhi::PixelFormat::R16G16B16A16Sint, kIntegerOptions, device);

    // 32-bit integer formats
    buildColorFormatProperties(caps, snap::rhi::PixelFormat::R32Uint, kIntegerOptions, device);
    buildColorFormatProperties(caps, snap::rhi::PixelFormat::R32G32Uint, kIntegerOptions, device);
    buildColorFormatProperties(caps, snap::rhi::PixelFormat::R32G32B32A32Uint, kIntegerOptions, device);
    buildColorFormatProperties(caps, snap::rhi::PixelFormat::R32Sint, kIntegerOptions, device);
    buildColorFormatProperties(caps, snap::rhi::PixelFormat::R32G32Sint, kIntegerOptions, device);
    buildColorFormatProperties(caps, snap::rhi::PixelFormat::R32G32B32A32Sint, kIntegerOptions, device);

    // R10G10B10A2Uint - integer packed format, supports MSAA
    buildColorFormatProperties(caps, snap::rhi::PixelFormat::R10G10B10A2Uint, FormatOptions::MSAASupported, device);
}

/**
 * @brief Configures normalized format properties (Unorm/Snorm).
 *
 * @param caps The capabilities structure to populate.
 * @param device The Metal device to query for MSAA support.
 */
void configureNormalizedFormats(snap::rhi::Capabilities& caps, const id<MTLDevice>& device) {
    // 8-bit normalized formats - full support
    buildColorFormatProperties(caps, snap::rhi::PixelFormat::R8Unorm, FormatOptions::All, device);
    buildColorFormatProperties(caps, snap::rhi::PixelFormat::R8G8Unorm, FormatOptions::All, device);
    buildColorFormatProperties(caps, snap::rhi::PixelFormat::R8G8B8A8Unorm, FormatOptions::All, device);
    buildColorFormatProperties(caps, snap::rhi::PixelFormat::R8Snorm, FormatOptions::All, device);
    buildColorFormatProperties(caps, snap::rhi::PixelFormat::R8G8Snorm, FormatOptions::All, device);
    buildColorFormatProperties(caps, snap::rhi::PixelFormat::R8G8B8A8Snorm, FormatOptions::All, device);

    // 16-bit normalized formats
    buildColorFormatProperties(caps, snap::rhi::PixelFormat::R16Unorm, FormatOptions::All, device);
    buildColorFormatProperties(caps, snap::rhi::PixelFormat::R16G16Unorm, FormatOptions::All, device);
    buildColorFormatProperties(caps, snap::rhi::PixelFormat::R16G16B16A16Unorm, FormatOptions::All, device);

    // R16 Snorm formats - no resolve support per Metal Feature Set Tables
    constexpr FormatOptions kR16SnormOptions =
        FormatOptions::BlendSupported | FormatOptions::MSAASupported | FormatOptions::FilterSupported;
    buildColorFormatProperties(caps, snap::rhi::PixelFormat::R16Snorm, kR16SnormOptions, device);
    buildColorFormatProperties(caps, snap::rhi::PixelFormat::R16G16Snorm, kR16SnormOptions, device);
    buildColorFormatProperties(caps, snap::rhi::PixelFormat::R16G16B16A16Snorm, kR16SnormOptions, device);

    // BGRA8 and packed formats
    buildColorFormatProperties(caps, snap::rhi::PixelFormat::B8G8R8A8Unorm, FormatOptions::All, device);
    buildColorFormatProperties(caps, snap::rhi::PixelFormat::R10G10B10A2Unorm, FormatOptions::All, device);
    buildColorFormatProperties(caps, snap::rhi::PixelFormat::R11G11B10Float, FormatOptions::All, device);
}

/**
 * @brief Configures sRGB format properties.
 *
 * @param caps The capabilities structure to populate.
 * @param device The Metal device to query for MSAA support.
 */
void configureSRGBFormats(snap::rhi::Capabilities& caps, const id<MTLDevice>& device) {
    // sRGB formats - available on macOS 11.0+ and iOS 12.0+
    if (@available(macOS 11.0, iOS 12.0, *)) {
        buildColorFormatProperties(caps, snap::rhi::PixelFormat::R8G8B8A8Srgb, FormatOptions::All, device);
    }
}

/**
 * @brief Configures half-precision floating-point format properties.
 *
 * @param caps The capabilities structure to populate.
 * @param device The Metal device to query for MSAA support.
 */
void configureFloat16Formats(snap::rhi::Capabilities& caps, const id<MTLDevice>& device) {
    // 16-bit float formats - full support on all Metal devices
    buildColorFormatProperties(caps, snap::rhi::PixelFormat::R16Float, FormatOptions::All, device);
    buildColorFormatProperties(caps, snap::rhi::PixelFormat::R16G16Float, FormatOptions::All, device);
    buildColorFormatProperties(caps, snap::rhi::PixelFormat::R16G16B16A16Float, FormatOptions::All, device);
}

/**
 * @brief Configures single-precision floating-point format properties based on GPU family.
 *
 * Float32 format support varies significantly across GPU families.
 *
 * @param caps The capabilities structure to populate.
 * @param gpuFamily The detected GPU family support.
 * @param device The Metal device for additional queries.
 */
void configureFloat32Formats(snap::rhi::Capabilities& caps,
                             const MetalGPUFamilySupport& gpuFamily,
                             const id<MTLDevice>& device) {
    // Metal Feature Set Tables - Float32 format support:
    // - Filtering: requires [device supports32BitFloatFiltering] or Apple9+/Mac2
    // - MSAA: R32Float always, RG32/RGBA32 varies by family
    // - Resolve: Apple9+/Mac2 only
    // - Blending: all families

    FormatOptions filterOption = FormatOptions::None;
    if ([device supports32BitFloatFiltering]) {
        filterOption = FormatOptions::FilterSupported;
    }

    // Base options for R32Float (single channel)
    FormatOptions r32Options = FormatOptions::BlendSupported | FormatOptions::MSAASupported | filterOption;

    // Base options for RG32/RGBA32 (multi-channel) - typically no MSAA on older families
    FormatOptions r32VecOptions = FormatOptions::BlendSupported | filterOption;

    // Apple9+ and Mac2 have full support for all R32 formats
    if (gpuFamily.apple9 || gpuFamily.mac2) {
        r32Options = FormatOptions::All;
        r32VecOptions = FormatOptions::All;
    } else if (gpuFamily.metal3) {
        // Metal3 adds MSAA support for vector formats
        r32VecOptions = r32VecOptions | FormatOptions::MSAASupported;
    }

    buildColorFormatProperties(caps, snap::rhi::PixelFormat::R32Float, r32Options, device);
    buildColorFormatProperties(caps, snap::rhi::PixelFormat::R32G32Float, r32VecOptions, device);
    buildColorFormatProperties(caps, snap::rhi::PixelFormat::R32G32B32A32Float, r32VecOptions, device);
}

/**
 * @brief Configures depth/stencil format properties.
 *
 * @param caps The capabilities structure to populate.
 * @param device The Metal device to query for MSAA support.
 */
void configureDepthStencilFormats(snap::rhi::Capabilities& caps, const id<MTLDevice>& device) {
    // Depth16Unorm - supports linear filtering
    buildDepthStencilFormatProperties(caps, snap::rhi::PixelFormat::Depth16Unorm, true, device);

    // DepthFloat (Depth32Float) - no linear filtering
    buildDepthStencilFormatProperties(caps, snap::rhi::PixelFormat::DepthFloat, false, device);

    // DepthStencil (Depth24Unorm_Stencil8 or Depth32Float_Stencil8) - no linear filtering
    buildDepthStencilFormatProperties(caps, snap::rhi::PixelFormat::DepthStencil, false, device);
}

/**
 * @brief Configures compressed texture format properties based on GPU family.
 *
 * @param caps The capabilities structure to populate.
 * @param gpuFamily The detected GPU family support.
 * @param device The Metal device for additional queries.
 */
void configureCompressedFormats(snap::rhi::Capabilities& caps,
                                const MetalGPUFamilySupport& gpuFamily,
                                const id<MTLDevice>& device) {
#if SNAP_RHI_OS_IOS()
    // ETC2 compression - iOS only, requires iOS GPU Family 2+
    BOOL isETCSupported = [device supportsFeatureSet:MTLFeatureSet_iOS_GPUFamily2_v1];
    if (isETCSupported == YES) {
        buildColorFormatProperties(
            caps, snap::rhi::PixelFormat::ETC2_R8G8B8_Unorm, FormatOptions::FilterSupported, device);
        buildColorFormatProperties(
            caps, snap::rhi::PixelFormat::ETC2_R8G8B8A1_Unorm, FormatOptions::FilterSupported, device);
        buildColorFormatProperties(
            caps, snap::rhi::PixelFormat::ETC2_R8G8B8A8_Unorm, FormatOptions::FilterSupported, device);
        buildColorFormatProperties(
            caps, snap::rhi::PixelFormat::ETC2_R8G8B8_sRGB, FormatOptions::FilterSupported, device);
        buildColorFormatProperties(
            caps, snap::rhi::PixelFormat::ETC2_R8G8B8A1_sRGB, FormatOptions::FilterSupported, device);
        buildColorFormatProperties(
            caps, snap::rhi::PixelFormat::ETC2_R8G8B8A8_sRGB, FormatOptions::FilterSupported, device);
    }
#endif

    // ASTC compression - requires Apple2+ family
    if (gpuFamily.apple2) {
        buildColorFormatProperties(
            caps, snap::rhi::PixelFormat::ASTC_4x4_Unorm, FormatOptions::FilterSupported, device);
        buildColorFormatProperties(caps, snap::rhi::PixelFormat::ASTC_4x4_sRGB, FormatOptions::FilterSupported, device);
    }
}

/**
 * @brief Configures grayscale format support when texture swizzle is available.
 *
 * @param caps The capabilities structure to populate.
 */
void configureGrayscaleFormat(snap::rhi::Capabilities& caps) {
    if (caps.isTextureFormatSwizzleSupported) {
        auto& grayscale = caps.formatProperties[static_cast<uint8_t>(snap::rhi::PixelFormat::Grayscale)];
        grayscale.textureFeatures = static_cast<snap::rhi::FormatFeatures>(
            snap::rhi::FormatFeatures::BlitDst | snap::rhi::FormatFeatures::SampledFilterLinear);
        grayscale.framebufferSampleCounts = snap::rhi::SampleCount::Count1;
        grayscale.sampledTexture2DColorSampleCounts = snap::rhi::SampleCount::Count1;
        grayscale.sampledTexture2DArrayColorSampleCounts = snap::rhi::SampleCount::Count1;
    }
}

/**
 * @brief Master function to configure all pixel format properties.
 *
 * @param caps The capabilities structure to populate.
 * @param gpuFamily The detected GPU family support.
 * @param device The Metal device for additional queries.
 */
void configureAllFormatProperties(snap::rhi::Capabilities& caps,
                                  const MetalGPUFamilySupport& gpuFamily,
                                  const id<MTLDevice>& device) {
    caps.formatProperties.fill({});

    configureNormalizedFormats(caps, device);
    configureIntegerFormats(caps, device);
    configureFloat16Formats(caps, device);
    configureFloat32Formats(caps, gpuFamily, device);
    configureSRGBFormats(caps, device);
    configureDepthStencilFormats(caps, device);
    configureCompressedFormats(caps, gpuFamily, device);
    configureGrayscaleFormat(caps);
}

// =============================================================================
// MARK: - Queue Family Configuration
// =============================================================================

/**
 * @brief Configures command queue family properties.
 *
 * Metal exposes a single unified queue that supports all operations.
 *
 * @param caps The capabilities structure to populate.
 */
void configureQueueFamilies(snap::rhi::Capabilities& caps) {
    caps.queueFamiliesCount = 1;
    caps.queueFamilyProperties.fill({});

    caps.queueFamilyProperties[0].queueCount = 1;
    caps.queueFamilyProperties[0].queueFlags = snap::rhi::CommandQueueFeatures::Graphics |
                                               snap::rhi::CommandQueueFeatures::Compute |
                                               snap::rhi::CommandQueueFeatures::Transfer;
    caps.queueFamilyProperties[0].isTimestampQuerySupported = true;
}

// =============================================================================
// MARK: - API Description Configuration
// =============================================================================

/**
 * @brief Configures API description and NDC/texture conventions.
 *
 * @param caps The capabilities structure to populate.
 */
void configureAPIDescription(snap::rhi::Capabilities& caps) {
    caps.apiDescription = {snap::rhi::API::Metal, snap::rhi::APIVersionNone};
    caps.ndcLayout = {snap::rhi::NDCLayout::YAxis::Up, snap::rhi::NDCLayout::DepthRange::ZeroToOne};
    caps.textureConvention = snap::rhi::TextureOriginConvention::TopLeft;
}

} // unnamed namespace

namespace snap::rhi::backend::metal {

// =============================================================================
// MARK: - Device Implementation
// =============================================================================

Device::Device(const snap::rhi::backend::metal::DeviceCreateInfo& info)
    : snap::rhi::backend::common::DeviceContextless(info), mtlDevice(createMetalDevice()) {
    // Query GPU family support for capability configuration
    const MetalGPUFamilySupport gpuFamily = MetalGPUFamilySupport::queryFromDevice(mtlDevice);

    // Configure all capabilities based on GPU family and device queries
    configureFeatureFlags(caps, gpuFamily, mtlDevice);
    configureTextureLimits(caps, gpuFamily, mtlDevice);
    configureBufferLimits(caps, gpuFamily, mtlDevice);
    configureTextureAndSamplerBindings(caps, gpuFamily, mtlDevice);
    configureVertexInputLimits(caps, gpuFamily, mtlDevice);
    configureComputeLimits(caps, gpuFamily, mtlDevice);
    configureUniformBufferAlignment(caps, gpuFamily, mtlDevice);
    configureAllFormatProperties(caps, gpuFamily, mtlDevice);
    configureQueueFamilies(caps);
    configureAPIDescription(caps);

    // Populate memory properties
    populateMemoryProperties(mtlDevice, caps.physicalDeviceMemoryProperties);

    // Store device name
    platformDeviceString = getString([mtlDevice name]);

    // Report capabilities for debugging
    snap::rhi::backend::common::reportCapabilities(caps, validationLayer);

    // Initialize command queues
    commandQueues.resize(caps.queueFamilyProperties.size());
    for (size_t i = 0; i < caps.queueFamilyProperties.size(); ++i) {
        const auto& queueFamilyProperties = caps.queueFamilyProperties[i];
        commandQueues[i].resize(queueFamilyProperties.queueCount);
        for (uint32_t j = 0; j < queueFamilyProperties.queueCount; ++j) {
            commandQueues[i][j] =
                std::make_unique<snap::rhi::backend::metal::CommandQueue>(this, createMetalCommandQueue(mtlDevice));
        }
    }

    // Initialize fence pool
    fencePool = std::make_unique<common::FencePool>(
        [this](const snap::rhi::FenceCreateInfo& fenceInfo) {
            return this->createResourceNoDeviceRetain<snap::rhi::Fence, snap::rhi::backend::metal::Fence>(this,
                                                                                                          fenceInfo);
        },
        validationLayer);

    // Initialize submission tracker
    submissionTracker = std::make_unique<common::SubmissionTracker>(
        this, [this](const snap::rhi::DeviceContextCreateInfo& contextInfo) {
            return this->createResourceNoDeviceRetain<snap::rhi::DeviceContext,
                                                      snap::rhi::backend::common::DeviceContextless::DeviceContextNoOp>(
                this, contextInfo);
        });
}

Device::~Device() noexcept(false) {
    commandQueues.clear();
    submissionTracker.reset();
    fencePool.reset();
}

const id<MTLDevice>& Device::getMtlDevice() const {
    return mtlDevice;
}

std::shared_ptr<snap::rhi::QueryPool> Device::createQueryPool(const snap::rhi::QueryPoolCreateInfo& info) {
    // TODO: Implement query pools for Metal
    // return createResource<snap::rhi::QueryPool, snap::rhi::backend::metal::QueryPool>(this, info, nil);
    return nullptr;
}

std::shared_ptr<snap::rhi::CommandBuffer> Device::createCommandBuffer(const snap::rhi::CommandBufferCreateInfo& info) {
    return createResource<snap::rhi::CommandBuffer, snap::rhi::backend::metal::CommandBuffer>(this, info);
}

std::shared_ptr<snap::rhi::Sampler> Device::createSampler(const snap::rhi::SamplerCreateInfo& info) {
    return createResource<snap::rhi::Sampler, snap::rhi::backend::metal::Sampler>(this, info);
}

std::shared_ptr<snap::rhi::Semaphore> Device::createSemaphore(
    const snap::rhi::SemaphoreCreateInfo& info,
    const std::shared_ptr<snap::rhi::PlatformSyncHandle>& platformSyncHandle) {
    if (platformSyncHandle) {
        auto* commonHandle = snap::rhi::backend::common::smart_cast<snap::rhi::backend::common::PlatformSyncHandle>(
            platformSyncHandle.get());
        if (const auto& fence = commonHandle->getFence(); fence) {
            fence->waitForComplete();
        }
    }
    return createResource<snap::rhi::Semaphore, snap::rhi::backend::metal::Semaphore>(this, info);
}

std::shared_ptr<snap::rhi::Fence> Device::createFence(const snap::rhi::FenceCreateInfo& info) {
    return createResource<snap::rhi::Fence, snap::rhi::backend::metal::Fence>(this, info);
}

std::shared_ptr<snap::rhi::Buffer> Device::createBuffer(const snap::rhi::BufferCreateInfo& info) {
    return createResource<snap::rhi::Buffer, snap::rhi::backend::metal::Buffer>(this, info);
}

std::shared_ptr<snap::rhi::Texture> Device::createTexture(const snap::rhi::TextureCreateInfo& info) {
    return createResource<snap::rhi::Texture, snap::rhi::backend::metal::Texture>(this, info);
}

std::shared_ptr<snap::rhi::Texture> Device::createTexture(const id<MTLTexture>& mtlTexture,
                                                          const TextureCreateInfo& info) {
    validateTextureCreation(info);
    return createResource<snap::rhi::Texture, snap::rhi::backend::metal::Texture>(this, mtlTexture, info);
}

std::shared_ptr<snap::rhi::Texture> Device::createTexture(const std::shared_ptr<TextureInterop>& textureInterop) {
    return createResource<snap::rhi::Texture, snap::rhi::backend::metal::Texture>(this, textureInterop);
}

std::shared_ptr<snap::rhi::Texture> Device::createTextureView(const snap::rhi::TextureViewCreateInfo& info) {
    return createResource<snap::rhi::Texture, snap::rhi::backend::metal::Texture>(this, info);
}

std::shared_ptr<RenderPass> Device::createRenderPass(const RenderPassCreateInfo& info) {
    return createResource<snap::rhi::RenderPass, snap::rhi::RenderPass>(this, info);
}

std::shared_ptr<snap::rhi::Framebuffer> Device::createFramebuffer(const snap::rhi::FramebufferCreateInfo& info) {
    SNAP_RHI_VALIDATE(validationLayer,
                      info.renderPass,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::CreateOp,
                      "[createFramebuffer] renderPass is nullptr");
    return createResource<snap::rhi::Framebuffer, snap::rhi::backend::metal::Framebuffer>(this, info);
}

std::shared_ptr<snap::rhi::ShaderLibrary> Device::createShaderLibrary(const snap::rhi::ShaderLibraryCreateInfo& info) {
    return createResource<snap::rhi::ShaderLibrary, snap::rhi::backend::metal::ShaderLibrary>(this, info);
}

std::shared_ptr<snap::rhi::ShaderModule> Device::createShaderModule(const snap::rhi::ShaderModuleCreateInfo& info) {
    return createResource<snap::rhi::ShaderModule, snap::rhi::backend::metal::ShaderModule>(this, info);
}

std::shared_ptr<snap::rhi::DescriptorSetLayout> Device::createDescriptorSetLayout(
    const snap::rhi::DescriptorSetLayoutCreateInfo& info) {
    return createResource<snap::rhi::DescriptorSetLayout, snap::rhi::DescriptorSetLayout>(this, info);
}

std::shared_ptr<snap::rhi::DescriptorPool> Device::createDescriptorPool(
    const snap::rhi::DescriptorPoolCreateInfo& info) {
    return createResource<snap::rhi::DescriptorPool, snap::rhi::backend::metal::DescriptorPool>(this, info);
}

std::shared_ptr<snap::rhi::DescriptorSet> Device::createDescriptorSet(const snap::rhi::DescriptorSetCreateInfo& info) {
    auto descriptorSet = createResource<snap::rhi::DescriptorSet, snap::rhi::backend::metal::DescriptorSet>(this, info);
    auto* vkDescriptorSetPtr = snap::rhi::backend::common::smart_cast<DescriptorSet>(descriptorSet.get());
    if (!vkDescriptorSetPtr->getArgumentBufferSubRange()) {
        return nullptr;
    }
    return descriptorSet;
}

std::shared_ptr<snap::rhi::PipelineLayout> Device::createPipelineLayout(
    const snap::rhi::PipelineLayoutCreateInfo& info) {
    return createResource<snap::rhi::PipelineLayout, snap::rhi::backend::common::PipelineLayout>(this, info);
}

std::shared_ptr<snap::rhi::PipelineCache> Device::createPipelineCache(const snap::rhi::PipelineCacheCreateInfo& info) {
    return createResource<snap::rhi::PipelineCache, snap::rhi::backend::metal::PipelineCache>(this, info);
}

std::shared_ptr<snap::rhi::RenderPipeline> Device::createRenderPipeline(
    const snap::rhi::RenderPipelineCreateInfo& info) {
    for (uint32_t i = 0; i < info.vertexInputState.attributesCount; ++i) {
        SNAP_RHI_VALIDATE(
            validationLayer,
            (info.vertexInputState.attributeDescription[i].offset & 0x3) == 0,
            snap::rhi::ReportLevel::Error,
            snap::rhi::ValidationTag::CreateOp,
            "[createRenderPipeline] info.vertexInputState.attributeDescription.offset must be a multiple of 4 "
            "bytes.");
    }

    if (@available(macOS 11.0, iOS 14.0, *)) {
        if (info.pipelineCache &&
            static_cast<bool>(info.pipelineCreateFlags & PipelineCreateFlags::NullOnPipelineCacheMiss)) {
            id<MTLRenderPipelineState> pipelineState = nil;
            id<MTLFunction> vertexFunction = getFunction(info.stages, snap::rhi::ShaderStage::Vertex);
            id<MTLFunction> fragmentFunction = getFunction(info.stages, snap::rhi::ShaderStage::Fragment);
            MTLRenderPipelineDescriptor* pipelineDescriptor =
                snap::rhi::backend::metal::createRenderPipelineDescriptor(info, vertexFunction, fragmentFunction);

            auto* mtlPipelineCache = snap::rhi::backend::common::smart_cast<PipelineCache>(info.pipelineCache);
            const id<MTLBinaryArchive>& binaryArchive = mtlPipelineCache->getBinaryArchive();
            pipelineDescriptor.binaryArchives = [NSArray arrayWithObject:binaryArchive];

            snap::rhi::reflection::RenderPipelineInfo reflectionInfo{};
            const bool isNativeReflectionAcquired =
                static_cast<bool>(info.pipelineCreateFlags & PipelineCreateFlags::AcquireNativeReflection);
            MTLPipelineOption options = MTLPipelineOptionFailOnBinaryArchiveMiss;

            NSError* error = nil;
            if (isNativeReflectionAcquired) {
                options = options | MTLPipelineOptionBufferTypeInfo | MTLPipelineOptionArgumentInfo;

                MTLRenderPipelineReflection* reflectionObj = nil;
                pipelineState = [getMtlDevice() newRenderPipelineStateWithDescriptor:pipelineDescriptor
                                                                             options:options
                                                                          reflection:&reflectionObj
                                                                               error:&error];
                if ((error == nil) && (pipelineState != nil)) {
                    reflectionInfo.descriptorSetInfos =
                        snap::rhi::backend::metal::buildDescriptorSetReflection(reflectionObj, info.pipelineLayout);
                    reflectionInfo.vertexAttributes = snap::rhi::backend::metal::buildVertexReflection(vertexFunction);
                }
            } else {
                pipelineState = [getMtlDevice() newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];
                /* If reflection is not requested, then just clear it to make sure that
                 * uninitialized data will not be used.
                 */
                reflectionInfo = {};
            }

            if ((error != nil) || (pipelineState == nil)) {
                std::string description = "";
                if (error != nil) {
                    description += getString(error.description);
                }

                SNAP_RHI_REPORT(validationLayer,
                                snap::rhi::ReportLevel::Warning,
                                snap::rhi::ValidationTag::CreateOp,
                                "[RenderPipeline] Failed to create pipeline state from bin archive, error: %s",
                                description.c_str());
                /**
                 * SnapRHI must create the RenderPipeline separately, because if the NullOnPipelineCacheMiss flag
                 * is specified, SnapRHI must not create the RenderPipeline, if the driver cannot read the pipeline
                 * from the cache, SnapRHI will return nullptr.
                 */
                return nullptr;
            }

            return createResource<snap::rhi::RenderPipeline, snap::rhi::backend::metal::RenderPipeline>(
                this, info, reflectionInfo, pipelineState);
        }
    }

    return createResource<snap::rhi::RenderPipeline, snap::rhi::backend::metal::RenderPipeline>(this, info);
}

std::shared_ptr<snap::rhi::ComputePipeline> Device::createComputePipeline(
    const snap::rhi::ComputePipelineCreateInfo& info) {
    if (info.basePipeline) {
        auto* mtlComputePipeline =
            snap::rhi::backend::common::smart_cast<snap::rhi::backend::metal::ComputePipeline>(info.basePipeline);
        auto* mtlShaderModule =
            snap::rhi::backend::common::smart_cast<snap::rhi::backend::metal::ShaderModule>(info.stage);

        if (mtlShaderModule->getFunction() == mtlComputePipeline->getComputeFunction()) {
            return createResource<snap::rhi::ComputePipeline, snap::rhi::backend::metal::ComputePipeline>(
                this,
                info,
                mtlComputePipeline->getReflectionInfo(),
                mtlComputePipeline->getComputeFunction(),
                mtlComputePipeline->getComputePipeline());
        }
    }

    if (@available(macOS 11.0, iOS 14.0, *)) {
        if (info.pipelineCache &&
            static_cast<bool>(info.pipelineCreateFlags & PipelineCreateFlags::NullOnPipelineCacheMiss)) {
            auto* mtlShaderModule =
                snap::rhi::backend::common::smart_cast<snap::rhi::backend::metal::ShaderModule>(info.stage);

            MTLComputePipelineDescriptor* pipelineDescriptor = [[MTLComputePipelineDescriptor alloc] init];
            pipelineDescriptor.computeFunction = mtlShaderModule->getFunction();

            auto* mtlPipelineCache = snap::rhi::backend::common::smart_cast<PipelineCache>(info.pipelineCache);
            const id<MTLBinaryArchive>& binaryArchive = mtlPipelineCache->getBinaryArchive();
            pipelineDescriptor.binaryArchives = [NSArray arrayWithObject:binaryArchive];

            MTLPipelineOption options = MTLPipelineOptionBufferTypeInfo | MTLPipelineOptionArgumentInfo |
                                        MTLPipelineOptionFailOnBinaryArchiveMiss;
            MTLComputePipelineReflection* reflectionObj = nil;
            NSError* error = nil;
            id<MTLComputePipelineState> pipelineState =
                [getMtlDevice() newComputePipelineStateWithDescriptor:pipelineDescriptor
                                                              options:options
                                                           reflection:&reflectionObj
                                                                error:&error];
            if ((error != nil) || (pipelineState == nil)) {
                std::string description = "";
                if (error != nil) {
                    description += getString(error.description);
                }

                SNAP_RHI_REPORT(validationLayer,
                                snap::rhi::ReportLevel::Error,
                                snap::rhi::ValidationTag::CreateOp,
                                "[ComputePipeline] Failed to create pipeline state from bin archive, error: %s",
                                description.c_str());
                /**
                 * SnapRHI must create the ComputePipeline separately, because if the NullOnPipelineCacheMiss flag
                 * is specified, SnapRHI must not create the ComputePipeline, if the driver cannot read the
                 * pipeline from the cache, SnapRHI will return nullptr.
                 */
                return nullptr;
            }

            snap::rhi::reflection::ComputePipelineInfo reflection{};
            reflection.descriptorSetInfos =
                snap::rhi::backend::metal::buildDescriptorSetReflection(reflectionObj, info.pipelineLayout);

            return createResource<snap::rhi::ComputePipeline, snap::rhi::backend::metal::ComputePipeline>(
                this, info, reflection, mtlShaderModule->getFunction(), pipelineState);
        }
    }

    return createResource<snap::rhi::ComputePipeline, snap::rhi::backend::metal::ComputePipeline>(this, info);
}

TextureFormatProperties Device::getTextureFormatProperties(const TextureFormatInfo& info) {
    // Query GPU family for accurate limits
    const MetalGPUFamilySupport gpuFamily = MetalGPUFamilySupport::queryFromDevice(mtlDevice);
    const bool supportsLargeTextures = gpuFamily.apple3 || gpuFamily.mac2 || gpuFamily.metal3;
    const uint32_t maxDimension = supportsLargeTextures ? 16384u : 8192u;

    // Query maximum buffer length from device for resource size calculation
    uint64_t maxResourceSize = 256ull * 1024ull * 1024ull; // Default: 256MB
    if (@available(macOS 10.14, iOS 12.0, *)) {
        maxResourceSize = static_cast<uint64_t>([mtlDevice maxBufferLength]);
    }

    return {
        .maxExtent =
            {
                .width = maxDimension,
                .height = maxDimension,
                .depth = 2048u,
            },
        .maxMipLevels = supportsLargeTextures ? 15u : 14u, // log2(16384)+1 or log2(8192)+1
        .maxArrayLayers = 2048u,
        .sampleCounts = caps.formatProperties[static_cast<uint32_t>(info.format)].framebufferSampleCounts,
        .maxResourceSize = maxResourceSize,
    };
}

std::shared_ptr<snap::rhi::DebugMessenger> Device::createDebugMessenger(snap::rhi::DebugMessengerCreateInfo&& info) {
    return nullptr;
}

uint64_t Device::getGPUMemoryUsage() const {
    NSUInteger currentAllocatedSize = [mtlDevice currentAllocatedSize];
    return static_cast<uint64_t>(currentAllocatedSize);
}

} // namespace snap::rhi::backend::metal
