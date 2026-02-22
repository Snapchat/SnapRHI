#pragma once

#include "snap/rhi/Enums.h"
#include "snap/rhi/Limits.h"
#include "snap/rhi/Memory.h"
#include "snap/rhi/PixelFormat.h"
#include "snap/rhi/VertexAttributeFormat.h"
#include <array>
#include <bitset>
#include <compare>

namespace snap::rhi {
/**
 * @brief Per-format capability description.
 *
 * Describes which operations are supported for a particular `snap::rhi::PixelFormat`.
 * Backends populate these fields during device initialization.
 *
 * @note For most formats, these values are conservative: an operation may be available on some hardware but not
 * advertised if the backend cannot guarantee it across supported devices.
 */
struct PixelFormatProperties {
    /**
     * @brief Supported texture operations for this format.
     *
     * This is a bitmask of `snap::rhi::FormatFeatures` describing sampling, blitting, transfer, renderability, etc.
     *
     * @note Used for validation and for selecting fallback paths (e.g., mipmap generation filter choice).
     */
    snap::rhi::FormatFeatures textureFeatures = snap::rhi::FormatFeatures::None;

    /**
     * @brief Sample counts supported when the format is used as a framebuffer attachment.
     *
     * Applies to `TextureUsage::ColorAttachment` and `TextureUsage::DepthStencilAttachment`.
     */
    snap::rhi::SampleCount framebufferSampleCounts = snap::rhi::SampleCount::Count1;

    /**
     * @brief Sample counts supported when the format is used as a sampled 2D texture.
     *
     * Applies when `TextureUsage` contains `TextureUsage::Sampled` for `TextureType::Texture2D`.
     */
    snap::rhi::SampleCount sampledTexture2DColorSampleCounts = snap::rhi::SampleCount::Count1;

    /**
     * @brief Sample counts supported when the format is used as a sampled 2D-array texture.
     *
     * Applies when `TextureUsage` contains `TextureUsage::Sampled` for `TextureType::Texture2DArray` / array-like
     * types.
     */
    snap::rhi::SampleCount sampledTexture2DArrayColorSampleCounts = snap::rhi::SampleCount::Count1;

    /**
     *  Only snap::rhi::TextureType::Texture2D support multisampling.
     *
     *  Metal =>
     *    1 All devices
     *    2 All iOS and tvOS devices; some macOS devices
     *    4 All devices
     *    8 Some macOS devices
     * https://developer.apple.com/documentation/metal/mtldevice/1433355-supportstexturesamplecount?language=objc
     *
     * Vulkan =>
     *      Vulkan guarantees supporting 4xMSAA(VK_SAMPLE_COUNT_4_BIT) with non-integer formats
     * https://github.com/gpuweb/gpuweb/issues/108
     * https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkPhysicalDeviceLimits.html
     *
     * OpenGL ES 3.0+ =>
     *      fixed-point or depth/depth-stencil formats supported 4xMSAA
     *      floating formats should be checked with glGetInternalformativ
     * https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetInternalformativ.xhtml
     */

    constexpr friend auto operator<=>(const PixelFormatProperties&, const PixelFormatProperties&) noexcept = default;
};

/**
 * @brief Properties of a device queue family.
 *
 * Backends that expose multiple hardware queue families (e.g., Vulkan) use this to describe per-family capabilities.
 */
struct QueueFamilyProperties {
    /**
     * @brief Feature flags supported by queues in this family (graphics/compute/transfer).
     */
    CommandQueueFeatures queueFlags = CommandQueueFeatures::None;

    /**
     * @brief Number of queues available in this family.
     */
    uint32_t queueCount = 0;

    /**
     * @brief Whether timestamp queries are supported on this queue family.
     *
     * This affects `QueryPool`/timestamp availability and may be used for profiling features.
     */
    bool isTimestampQuerySupported = false;

    constexpr friend auto operator<=>(const QueueFamilyProperties&, const QueueFamilyProperties&) noexcept = default;
};

/**
 * @brief Device capabilities and limits.
 *
 * This is a snapshot of backend/device features and numeric limits, filled at device creation time.
 * Obtain it via `snap::rhi::Device::getCapabilities()`.
 *
 * The intent is:
 * - Feature flags (`is*Supported`) answer "can I use this feature on the current backend/device?"
 * - Numeric limits (`max*`, `min*`) provide safe upper/lower bounds for validation and resource creation.
 * - Per-format/per-stage arrays describe *capability families* (formats, shader stages, queue families).
 *
 * ## Backend population
 * - **OpenGL**: derived from queried GL limits/extensions (see `backend/opengl/Profile::buildCapabilities()`).
 * - **Vulkan**: derived from `VkPhysicalDeviceProperties`/features/extensions (see
 * `backend/vulkan/Device::initCapabilities()`).
 * - **Metal**: derived from Metal feature set tables and OS availability checks.
 * - **NoOp**: minimal defaults.
 */
struct Capabilities {
    /**
     * @brief Constructs a capabilities snapshot with safe defaults.
     *
     * Backends overwrite these fields during device initialization.
     */
    Capabilities() = default;

    /** @brief Whether rasterizer discard / rasterization disable is supported. */
    bool isRasterizerDisableSupported = false;
    /** @brief Whether non-power-of-two textures support wrap addressing modes. */
    bool isNPOTWrapModeSupported = false;
    /** @brief Whether clamp-to-border sampler addressing is supported. */
    bool isClampToBorderSupported = false;
    /** @brief Whether clamp-to-zero addressing mode is supported (backend-specific). */
    bool isClampToZeroSupported = false;
    /** @brief Whether texture arrays are supported. */
    bool isTextureArraySupported = false;
    /** @brief Whether 3D textures are supported. */
    bool isTexture3DSupported = false;
    /** @brief Whether min/max blend equations are supported. */
    bool isMinMaxBlendModeSupported = false;
    /** @brief Whether rendering directly to non-zero mip levels is supported. */
    bool isRenderToMipmapSupported = false;
    /** @brief Whether `glGenerateMipmap`/equivalent supports NPOT textures. */
    bool isNPOTGenerateMipmapCmdSupported = false;
    /** @brief Whether depth cube map textures are supported. */
    bool isDepthCubeMapSupported = false;
    /** @brief Whether polygon offset clamp is supported (if applicable). */
    bool isPolygonOffsetClampSupported = false;
    /** @brief Whether alpha-to-coverage is supported. */
    bool isAlphaToCoverageSupported = false;
    /** @brief Whether alpha-to-one is supported (mostly backend-specific). */
    bool isAlphaToOneEnableSupported = false;
    /** @brief Whether MRT blend settings can differ per render target. */
    bool isMRTBlendSettingsDifferent = false;
    /** @brief Whether sampler min/max LOD clamping is supported. */
    bool isSamplerMinMaxLodSupported = false;
    /** @brief Whether unnormalized texture coordinates are supported in samplers. */
    bool isSamplerUnnormalizedCoordsSupported = false;
    /** @brief Whether compare samplers (depth compare) are supported. */
    bool isSamplerCompareFuncSupported = false;
    /** @brief Whether creating texture views (aliasing subresources) is supported. */
    bool isTextureViewSupported = false;
    /** @brief Whether format swizzle in texture views/images is supported. */
    bool isTextureFormatSwizzleSupported = false;
    /** @brief Whether non-fill polygon modes (line/point) are supported. */
    bool isNonFillPolygonModeSupported = false;
    /** @brief Whether shader storage buffers (SSBO / storage buffers) are supported. */
    bool isShaderStorageBufferSupported = false;
    /** @brief Whether 32-bit index buffers are supported. */
    bool isUInt32IndexSupported = false;

    /**
     * @brief Whether dynamic rendering is supported.
     *
     * On Vulkan, this corresponds to VK_KHR_dynamic_rendering or Vulkan 1.3.
     */
    bool isDynamicRenderingSupported = false;

    /**
     * @brief Vertex attribute data types that can be used with `VertexInputRate::Constant`.
     */
    FormatDataTypeBits constantInputRateSupportingBits = FormatDataTypeBits::Float;

    /** @brief Whether `VertexInputRate::PerInstance` is supported (instancing). */
    bool isVertexInputRatePerInstanceSupported = false;

    /**
     * @brief Whether framebuffer fetch / programmable blending is supported.
     *
     * Backend notes:
     * - OpenGL: EXT/ARM shader framebuffer fetch extensions.
     * - Metal: fragment input `[[color(m)]]`.
     * - Vulkan: not supported in this currently. https://github.com/KhronosGroup/Vulkan-Docs/issues/1649
     */
    bool isFramebufferFetchSupported = false;

    /**
     * @brief Whether primitive restart is enabled and which restart index values apply.
     *
     * If true, the restart index depends on the bound index type:
     * - UINT32: 0xFFFFFFFF
     * - UINT16: 0xFFFF
     * - UINT8:  0xFF
     */
    bool isPrimitiveRestartIndexEnabled = false;

    /** @brief Maximum number of array layers that can be rendered to in a framebuffer. */
    uint32_t maxFramebufferLayers = 1;

    /** @brief Supported layer rendering approach (instancing vs other techniques). */
    LayerRenderingTypeBits layerRenderingType = LayerRenderingTypeBits::WithInstancing;

    /** @brief Maximum number of array layers for array/cubemap textures. */
    uint32_t maxTextureArrayLayers = 0;

    /** @brief Maximum supported size for 2D textures (max of width/height). */
    uint32_t maxTextureDimension2D = 0;

    /** @brief Maximum supported size for 3D textures (max of width/height/depth). */
    uint32_t maxTextureDimension3D = 0;

    /** @brief Maximum supported size for cube textures (max of width/height). */
    uint32_t maxTextureDimensionCube = 0;

    /** @brief Maximum number of multiview views supported. */
    uint32_t maxMultiviewViewCount = 0;

    /**
     * @brief Whether MSAA resolve is handled implicitly by the backend for multiview render targets.
     *
     *  If true :
     *      => SubpassDescription::colorAttachments shouldn't be used, MSAA texture will be created and resolved
     * automatically by GPU driver. only SubpassDescription::resolveAttachments should contain non MSAA textures, that
     * will be automatically resolved.
     *
     * If false :
     *      => SubpassDescription::resolveAttachments should used as resolveAttachments and
     *      SubpassDescription::colorAttachments should used as colorAttachments.
     *
     *  https://www.khronos.org/registry/OpenGL/extensions/OVR/OVR_multiview_multisampled_render_to_texture.txt
     */
    bool isMultiviewMSAAImplicitResolveEnabled = false;

    /** @brief Maximum number of clip distances supported in a single shader stage. */
    uint32_t maxClipDistances = 0;
    /** @brief Maximum number of cull distances supported in a single shader stage. */
    uint32_t maxCullDistances = 0;

    /**
     * @brief Maximum combined number of clip + cull distances per stage.
     */
    uint32_t maxCombinedClipAndCullDistances = 0;

    /** @brief Maximum number of uniform buffers accessible per shader stage. */
    uint32_t maxPerStageUniformBuffers = snap::rhi::SupportedLimit::MaxPerStageUniformBuffers;

    /** @brief Maximum number of uniform buffers accessible by a pipeline across all stages. */
    uint32_t maxUniformBuffers = snap::rhi::SupportedLimit::MaxUniformBuffers;

    /** @brief Total uniform-buffer bytes available per stage across all bound UBOs. */
    std::array<uint64_t, static_cast<uint32_t>(snap::rhi::ShaderStage::Count)> maxPerStageTotalUniformBufferSize{
        snap::rhi::SupportedLimit::MaxVertexShaderTotalUniformBufferSize,
        snap::rhi::SupportedLimit::MaxFragmentShaderTotalUniformBufferSize,
        snap::rhi::SupportedLimit::MaxComputeShaderTotalUniformBufferSize,
    };

    /** @brief Maximum size in bytes of a single uniform buffer per stage. */
    std::array<uint64_t, static_cast<uint32_t>(snap::rhi::ShaderStage::Count)> maxPerStageUniformBufferSize{
        snap::rhi::SupportedLimit::MaxVertexShaderUniformBufferSize,
        snap::rhi::SupportedLimit::MaxFragmentShaderUniformBufferSize,
        snap::rhi::SupportedLimit::MaxComputeShaderUniformBufferSize,
    };

    /** @brief Maximum number of sampled textures accessible per stage. */
    std::array<uint32_t, static_cast<uint32_t>(snap::rhi::ShaderStage::Count)> maxPerStageTextures{
        snap::rhi::SupportedLimit::MaxVertexShaderTextures,
        snap::rhi::SupportedLimit::MaxFragmentShaderTextures,
        snap::rhi::SupportedLimit::MaxComputeShaderTextures,
    };

    /** @brief Maximum number of sampled textures accessible by a pipeline across all stages. */
    uint32_t maxTextures = snap::rhi::SupportedLimit::MaxTextures;

    /** @brief Maximum number of samplers accessible per stage. */
    std::array<uint32_t, static_cast<uint32_t>(snap::rhi::ShaderStage::Count)> maxPerStageSamplers{
        snap::rhi::SupportedLimit::MaxVertexShaderSamplers,
        snap::rhi::SupportedLimit::MaxFragmentShaderSamplers,
        snap::rhi::SupportedLimit::MaxComputeShaderSamplers,
    };

    /** @brief Maximum number of samplers accessible by a pipeline across all stages. */
    uint32_t maxSamplers = snap::rhi::SupportedLimit::MaxSamplers;

    /** @brief Maximum number of specialization constants supported in a pipeline. */
    uint32_t maxSpecializationConstants = snap::rhi::SupportedLimit::MaxSpecializationConstants;

    /** @brief Maximum number of vertex input attributes. */
    uint32_t maxVertexInputAttributes = snap::rhi::SupportedLimit::MaxVertexInputAttributes;

    /** @brief Maximum number of vertex input bindings (vertex buffers). */
    uint32_t maxVertexInputBindings = snap::rhi::SupportedLimit::MaxVertexInputBindings;

    /** @brief Maximum vertex attribute offset in bytes. */
    uint32_t maxVertexInputAttributeOffset = snap::rhi::SupportedLimit::MaxVertexInputAttributeOffset;

    /** @brief Maximum stride (bytes) for a vertex input binding. */
    uint32_t maxVertexInputBindingStride = snap::rhi::SupportedLimit::MaxVertexInputBindingStride;

    /** @brief Maximum supported vertex attribute divisor for instancing. */
    uint32_t maxVertexAttribDivisor = 1;

    /** @brief Maximum number of descriptor sets that can be bound simultaneously. */
    uint32_t maxBoundDescriptorSets = snap::rhi::SupportedLimit::MaxBoundDescriptorSets;

    /** @brief Maximum number of compute invocations in a single local workgroup. */
    uint32_t maxComputeWorkGroupInvocations = 0;

    /**
     * @brief Native execution width for compute (warp/wavefront-like size).
     *
     * For best performance, choose threadgroup sizes that are multiples of this value where applicable.
     */
    uint32_t threadExecutionWidth = 0;

    /** @brief Maximum number of compute workgroups that can be dispatched per dimension. */
    std::array<uint32_t, 3> maxComputeWorkGroupCount{0, 0, 0};

    /** @brief Maximum size of a compute workgroup per dimension. */
    std::array<uint32_t, 3> maxComputeWorkGroupSize{0, 0, 0};

    /** @brief Whether fragment-shader primitive ID is supported. */
    bool isFragmentPrimitiveIDSupported = false;

    /** @brief Whether fragment-shader barycentric coordinates are supported. */
    bool isFragmentBarycentricCoordinatesSupported = false;

    /**
     * @brief Minimum alignment for dynamic uniform-buffer offsets.
     *
     * Descriptor set updates using dynamic offsets must satisfy:
     * `offset % minUniformBufferOffsetAlignment == 0`.
     */
    uint64_t minUniformBufferOffsetAlignment = 1;

    /** @brief Maximum number of color attachments in a framebuffer. */
    uint32_t maxFramebufferColorAttachmentCount = 1;

    /** @brief Maximum anisotropic filtering level supported by the device. */
    snap::rhi::AnisotropicFiltering maxAnisotropic = snap::rhi::AnisotropicFiltering::None;

    /** @brief Queue family descriptions (up to `SupportedLimit::MaxQueueFamilies`). */
    std::array<QueueFamilyProperties, snap::rhi::SupportedLimit::MaxQueueFamilies> queueFamilyProperties{};

    /** @brief Number of populated entries in `queueFamilyProperties`. */
    uint32_t queueFamiliesCount = 0;

    /**
     * @brief Per-pixel-format capabilities.
     *
     * Indexed by `static_cast<uint32_t>(snap::rhi::PixelFormat)`.
     */
    std::array<snap::rhi::PixelFormatProperties, static_cast<uint32_t>(snap::rhi::PixelFormat::Count)>
        formatProperties{};

    /**
     * @brief Supported vertex attribute formats.
     *
     * Indexed by `static_cast<uint32_t>(snap::rhi::VertexAttributeFormat)`.
     */
    std::array<bool, static_cast<uint32_t>(snap::rhi::VertexAttributeFormat::Count)> vertexAttributeFormatProperties{};

    /**
     * @brief NDC (Normalized Device Coordinates) layout conventions used by the backend.
     *
     * After perspective division, coordinates are in NDC space.
     * This struct captures the API-specific conventions for Y-axis
     * direction and depth range in NDC.
     */
    snap::rhi::NDCLayout ndcLayout{snap::rhi::NDCLayout::YAxis::Up, snap::rhi::NDCLayout::DepthRange::MinusOneToOne};

    /** @brief Texture origin convention used by the backend (top-left vs bottom-left). */
    snap::rhi::TextureOriginConvention textureConvention = snap::rhi::TextureOriginConvention::BottomLeft;

    /** @brief Backend API identifier and version. */
    snap::rhi::APIDescription apiDescription{snap::rhi::API::Undefined, snap::rhi::APIVersionNone};

    /**
     * @brief The table of memory types supported by the physical device.
     *
     * @details
     * This structure provides a detailed inventory of the memory heaps available on the device.
     * - **Vulkan:** Directly maps to `VkPhysicalDeviceMemoryProperties`.
     * - **Metal:** Maps to the available `MTLStorageMode` options on the current device (Shared vs. Managed).
     * - **OpenGL:** Since OpenGL abstracts memory management, this table is populated with conservative
     * approximations (e.g., always reporting a Coherent type) to maintain API consistency.
     */
    snap::rhi::PhysicalDeviceMemoryProperties physicalDeviceMemoryProperties{};

    /**
     * @brief Indicates whether buffers can remain mapped while the GPU is accessing them.
     *
     * @details
     * This capability determines the lifetime validity of the pointer returned by `snap::rhi::Buffer::map()`.
     *
     * - **true (Persistent Mapping):** You may hold the mapped pointer indefinitely. You do not need to call
     * `unmap()` before GPU execution. This enables "Zero-Copy" patterns and reduces driver overhead.
     * - **false (Must Unmap):** You **must** call `unmap()` before submitting any command buffer that
     * references the mapped resource. Failure to do so results in undefined behavior (often a crash or GPU hang).
     *
     * @note Even with persistent mapping, you must still ensure correct synchronization (fences/barriers)
     * to prevent race conditions between the CPU and GPU.
     */
    bool supportsPersistentMapping = false;

    /**
     * @brief The required alignment size for non-coherent memory operations.
     *
     * @details
     * This value specifies the granularity at which non-coherent memory must be flushed or invalidated.
     * When calling `flushMappedMemoryRanges` or `invalidateMappedMemoryRanges`:
     * 1. The `offset` must be a multiple of this value.
     * 2. The `size` must be a multiple of this value (unless the range extends to the end of the buffer).
     *
     * - **Vulkan:** Corresponds to `VkPhysicalDeviceLimits::nonCoherentAtomSize`.
     * - **Other Backends:** If the API handles coherency automatically or doesn't enforce alignment,
     * this will be set to 1.
     */
    uint64_t nonCoherentAtomSize = 1;

    constexpr friend auto operator<=>(const Capabilities&, const Capabilities&) noexcept = default;
};
} // namespace snap::rhi
