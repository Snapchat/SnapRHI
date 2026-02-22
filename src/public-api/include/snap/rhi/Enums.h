//
//  Enums.h
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 21.09.2020.
//  Copyright © 2021 Snapchat. All rights reserved.
//

#pragma once

#include "EnumOps.h"

#include <array>
#include <bit>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>

namespace snap::rhi {

/// @brief Encoding type of a command buffer/encoder.
enum class EncodingType : uint32_t { None = 0, Blit, Render, Compute };

/**
 * https://registry.khronos.org/vulkan/specs/latest/man/html/VK_WHOLE_SIZE.html
 */
constexpr uint32_t WholeSize = 0;

/**
 * @brief Specifies how a shader library should be created.
 */
enum class ShaderLibraryCreateFlag : uint32_t {
    /// Compile from source text (for example, GLSL/MSL/HLSL depending on backend).
    CompileFromSource = 0,
    /// Compile from precompiled binary/IR when supported by the backend.
    CompileFromBinary,

    Count
};

/**
 * @brief Result codes returned by SnapRHI operations.
 *
 * This is a compact, cross-backend result set (not a 1:1 mapping of backend-specific error codes).
 */
enum class Result : uint32_t {
    /// Operation completed successfully.
    Success = 0,
    /// Operation has not completed yet.
    NotReady,
    /// Operation completed but returned partial results.
    Incomplete,
    /// Unspecified failure.
    ErrorUnknown,

    Count
};

/**
 * @brief Bitmask describing intended buffer usage.
 *
 * Backends use these hints to select API-specific buffer usage flags/targets:
 * - OpenGL selects default bind targets (array/index/uniform/copy/pack/unpack buffers).
 * - Vulkan translates to `VkBufferUsageFlags`.
 */
enum class BufferUsage : uint32_t {
    /// No usage specified.
    None = 0,

    /// Vertex buffer binding.
    VertexBuffer = 1 << 0,
    /// Index buffer binding.
    IndexBuffer = 1 << 1,

    /// Uniform/constant buffer binding.
    UniformBuffer = 1 << 2,
    /// Storage/SSBO-like buffer binding.
    StorageBuffer = 1 << 3,

    /**
     * @brief Allows buffer <=> buffer copy operations (source).
     *
     * Vulkan maps this to `VK_BUFFER_USAGE_TRANSFER_SRC_BIT`.
     */
    CopySrc = 1 << 4,
    /**
     * @brief Allows buffer <=> buffer copy operations (destination).
     *
     * Vulkan maps this to `VK_BUFFER_USAGE_TRANSFER_DST_BIT`.
     */
    CopyDst = 1 << 5,

    /**
     * @brief Allows buffer <=> texture transfer operations (source).
     *
     * OpenGL uses this to choose pack/unpack buffer targets.
     */
    TransferSrc = 1 << 6,
    /**
     * @brief Allows buffer <=> texture transfer operations (destination).
     *
     * OpenGL uses this to choose pack/unpack buffer targets.
     */
    TransferDst = 1 << 7,

    // UNIFORM_TEXEL_BUFFER
    // STORAGE_TEXEL_BUFFER

    // INDIRECT_BUFFER
};
SNAP_RHI_DEFINE_ENUM_OPS(BufferUsage);

/**
 * @brief Dependency flags controlling how execution/memory dependencies are formed.
 *
 * Mirrors the Vulkan concept of dependency-by-region.
 */
enum class DependencyFlags {
    /// Dependencies are framebuffer-local (by region).
    ByRegion = 1 << 0,
};
SNAP_RHI_DEFINE_ENUM_OPS(DependencyFlags);

/**
 * @brief Bitmask specifying how pipeline creation should behave.
 */
enum class PipelineCreateFlags : uint32_t {
    /// Default pipeline creation behavior.
    None = 0,

    /// Allows creating the pipeline asynchronously (backend-dependent).
    AllowAsyncCreation = 1 << 0,
    /// Requests native reflection data if supported by the backend.
    AcquireNativeReflection = 1 << 1,
    /// If the pipeline cache misses, return `nullptr` instead of compiling.
    NullOnPipelineCacheMiss = 1 << 2,

    /// Convenience mask of all flags.
    All = AllowAsyncCreation | AcquireNativeReflection | NullOnPipelineCacheMiss
};
SNAP_RHI_DEFINE_ENUM_OPS(PipelineCreateFlags);

/**
 * @brief Bitmask specifying configuration options during CommandBuffer creation.
 */
enum class CommandBufferCreateFlags : uint32_t {
    /**
     * @brief Default behavior.
     *
     * SnapRHI retains strong references to resources used by commands recorded into the command buffer.
     */
    None = 0,

    /**
     * @brief Disables automatic resource reference counting for encoded commands.
     *
     * Backends use this flag to skip retaining resources for residency tracking (except when slow validations are
     * enabled).
     *
     * @warning The application must ensure resources referenced by the command buffer remain alive until GPU execution
     * completes.
     */
    UnretainedResources = 1 << 0,
};
SNAP_RHI_DEFINE_ENUM_OPS(CommandBufferCreateFlags);

/**
 * @brief Bitmask controlling shader module creation behavior.
 */
enum class ShaderModuleCreateFlags : uint32_t {
    /// Default behavior.
    None = 0,

    /// Allows creating the shader module asynchronously (backend-dependent).
    AllowAsyncCreation = 1 << 0,

    /// Convenience mask of all flags.
    All = AllowAsyncCreation
};
SNAP_RHI_DEFINE_ENUM_OPS(ShaderModuleCreateFlags);

/**
 * @brief Bitmask flags to configure `snap::rhi::Device` creation behavior.
 */
enum class DeviceCreateFlags : uint32_t {
    /// Default device creation behavior.
    None = 0,

    /**
     * @brief Enables asynchronous resource reclamation.
     *
     * When enabled, SnapRHI may spawn a background thread that monitors fence completion and reclaims resources
     * without blocking the main thread.
     */
    ReclaimAsync = 1 << 0,

    /**
     * @brief Enables backend debug callbacks when supported.
     *
     * Example:
     * - OpenGL uses this to enable KHR_debug/EGL_KHR_debug callbacks when available.
     */
    EnableDebugCallback = 1 << 1,
};
SNAP_RHI_DEFINE_ENUM_OPS(DeviceCreateFlags);

/**
 * @brief Bitmask specifying memory access types participating in a memory dependency.
 *
 * This is modeled after Vulkan `VkAccessFlagBits`.
 */
enum class AccessFlags {
    None = 0,

    IndexRead = 1 << 0,
    VertexAttributeRead = 1 << 1,
    UniformRead = 1 << 2,
    ShaderRead = 1 << 3,
    ShaderWrite = 1 << 4,
    ColorAttachmentRead = 1 << 5,
    ColorAttachmentWrite = 1 << 6,
    DepthStencilAttachmentRead = 1 << 7,
    DepthStencilAttachmentWrite = 1 << 8,
    TransferRead = 1 << 9,
    TransferWrite = 1 << 10,
    HostRead = 1 << 11,
    HostWrite = 1 << 12,
    MemoryRead = 1 << 13,
    MemoryWrite = 1 << 14,

    /// Convenience mask representing all access bits.
    All = IndexRead | VertexAttributeRead | UniformRead | ShaderRead | ShaderWrite | ColorAttachmentRead |
          ColorAttachmentWrite | DepthStencilAttachmentRead | DepthStencilAttachmentWrite | TransferRead |
          TransferWrite | HostRead | HostWrite | MemoryRead | MemoryWrite,
    Count = 15
};
SNAP_RHI_DEFINE_ENUM_OPS(AccessFlags);

constexpr std::string_view AccessFlagsToStr[] = {"IndexRead",
                                                 "VertexAttributeRead",
                                                 "UniformRead",
                                                 "ShaderRead",
                                                 "ShaderWrite",
                                                 "ColorAttachmentRead",
                                                 "ColorAttachmentWrite",
                                                 "DepthStencilAttachmentRead",
                                                 "DepthStencilAttachmentWrite",
                                                 "TransferRead",
                                                 "TransferWrite",
                                                 "HostRead",
                                                 "HostWrite",
                                                 "MemoryRead",
                                                 "MemoryWrite"};

/**
 * @brief Converts an `snap::rhi::AccessFlags` value to a human-readable string.
 *
 * This helper is primarily intended for debug/validation logging (for example in the OpenGL command encoder debug
 * output).
 *
 * @param accessFlags Access flags value.
 * @return A string representation:
 * - `"None"` for `AccessFlags::None`
 * - `"All"` for `AccessFlags::All`
 * - a comma-separated list for a *single-bit* value
 * - `"Undefined"` for unsupported patterns
 *
 * @warning This implementation only produces a list for values that match exactly one bit. Combined bitmasks other
 * than `All` will currently return `"Undefined"`.
 */
inline std::string getAccessFlagsToStr(const AccessFlags accessFlags) {
    if (accessFlags == AccessFlags::None) {
        return "None";
    }
    if (accessFlags == AccessFlags::All) {
        return "All";
    }

    std::string accessFlagsString;

    // Create string reperesenting list of flag names that are set in accessFlags
    for (uint32_t i = 0; i < static_cast<uint32_t>(AccessFlags::Count); ++i) {
        if (static_cast<uint32_t>(accessFlags) == (1 << i)) {
            accessFlagsString += std::string(AccessFlagsToStr[i]) + std::string(", ");
        }
    }

    if (!accessFlagsString.empty()) {
        return accessFlagsString.substr(0, accessFlagsString.length() - 2);
    }

    return "Undefined";
}

#undef SNAP_RHI_MEMORY_PROPERTY_BITS
#define SNAP_RHI_MEMORY_PROPERTY_BITS(X)                                                       \
    /**                                                                                        \
     * @brief The memory is allocated in the most efficient location for device (GPU) access.  \
     * @details                                                                                \
     * Typically resides in dedicated VRAM on discrete GPUs. CPU access to this memory         \
     * is often impossible unless combined with `HostVisible`.                                 \
     * @note This memory type is guaranteed to exist.                                          \
     */                                                                                        \
    X(DeviceLocal)                                                                             \
    /**                                                                                        \
     * @brief The memory can be mapped for host (CPU) access using `snap::rhi::Buffer::map()`. \
     * @details                                                                                \
     * If this bit is not set, the memory cannot be accessed directly by the host.             \
     */                                                                                        \
    X(HostVisible)                                                                             \
    /**                                                                                        \
     * @brief The host and device caches are automatically synchronized.                       \
     * @details                                                                                \
     * If set, `flushMappedMemoryRanges` and `invalidateMappedMemoryRanges` are not required   \
     * to make writes visible to the GPU or CPU.                                               \
     * @note The combination `HostVisible | HostCoherent` is guaranteed to exist.              \
     */                                                                                        \
    X(HostCoherent)                                                                            \
    /**                                                                                        \
     * @brief The memory is cached on the host (CPU).                                          \
     * @details                                                                                \
     * Host memory accesses to cached memory are significantly faster than to uncached memory, \
     * particularly for reads. However, cached memory is usually not `HostCoherent`, requiring \
     * explicit cache management (flush/invalidate) to synchronize with the device.            \
     */                                                                                        \
    X(HostCached)

#define SNAP_RHI_MEMORY_PROPERTY_BITS_INDEX_ENTRY(NAME) NAME,
enum class MemoryPropertyBitsIndex : uint32_t {
    SNAP_RHI_MEMORY_PROPERTY_BITS(SNAP_RHI_MEMORY_PROPERTY_BITS_INDEX_ENTRY)
};
#undef SNAP_RHI_MEMORY_PROPERTY_BITS_INDEX_ENTRY

#define SNAP_RHI_MEMORY_PROPERTY_BITS_ENUM_ENTRY(NAME) \
    NAME = 1u << static_cast<uint32_t>(MemoryPropertyBitsIndex::NAME),

/**
 * @brief Defines the memory characteristics and caching behavior for resource allocations.
 *
 * @details
 * These flags directly influence the physical memory heap selection and CPU-GPU cache coherency protocols.
 * Proper selection is critical for performance.
 *
 * **Guaranteed Memory Types:**
 * The underlying RHI guarantees that at least these two memory types exist on all compliant devices:
 * 1. `DeviceLocal`: The most efficient memory for GPU access.
 * 2. `HostVisible | HostCoherent`: The guaranteed fallback for CPU->GPU data transfer.
 * @note While guaranteed to be writable by the CPU, this type may be significantly slower
 * for the GPU to read compared to `DeviceLocal` memory.
 *
 * **Usage Guidelines:**
 * - Use `DeviceLocal` for high-bandwidth GPU resources (textures, static meshes, render targets).
 * - Use `HostVisible` for resources that require CPU read/write access.
 * - Use `HostCached` for frequent CPU reads (e.g., readback buffers) to avoid uncached memory access penalties.
 */
enum class MemoryProperties : uint32_t {
    None = 0,

    SNAP_RHI_MEMORY_PROPERTY_BITS(SNAP_RHI_MEMORY_PROPERTY_BITS_ENUM_ENTRY)
};
SNAP_RHI_DEFINE_ENUM_OPS(MemoryProperties);
#undef SNAP_RHI_MEMORY_PROPERTY_BITS_ENUM_ENTRY
#undef SNAP_RHI_MEMORY_PROPERTY_BITS

/**
 * @brief Specifies the desired access permissions when mapping a resource.
 *
 * @details
 * These flags provide hints to the driver for optimization and validation.
 * Providing strict access flags (e.g., `Write` only) can allow the driver to
 * perform optimizations such as invalidating the previous buffer contents to avoid stalls.
 */
enum class MemoryAccess : uint32_t {
    None = 0,

    /**
     * @brief The mapped region will be read by the host.
     * @note Reading from `HostVisible` memory that is not also `HostCached` is extremely slow
     * on PCIe and mobile architectures.
     */
    Read = 1u << 0,

    /**
     * @brief The mapped region will be written to by the host.
     */
    Write = 1u << 1,

    ReadWrite = Read | Write,
};
SNAP_RHI_DEFINE_ENUM_OPS(MemoryAccess);

/**
 * @brief Sample count for multisampling.
 */
enum class SampleCount : uint32_t {
    Undefined = 0,
    Count1 = 1,
    Count2 = 2,
    Count4 = 4,
    Count8 = 8,
    Count16 = 16,
    Count32 = 32,
    Count64 = 64,
};

/**
 * @brief Logical shader stages.
 */
enum class ShaderStage : uint32_t {
    Vertex = 0,
    Fragment,

    Compute,

    Count,
};

/**
 * @brief Pipeline stage bitmask used for barrier-like operations.
 *
 * Modeled after Vulkan `VkPipelineStageFlagBits`.
 */
enum class PipelineStageBits : uint32_t {
    None = 0,

    TopOfPipeBit = 1u << 0,
    VertexInputBit = 1u << 1,
    VertexShaderBit = 1u << 2,
    FragmentShaderBit = 1u << 3,
    EarlyFragmentTestsBit = 1u << 4,
    LateFragmentTestsBit = 1u << 5,
    ColorAttachmentOutputBit = 1u << 6,
    ComputeShaderBit = 1u << 7,
    TransferBit = 1u << 8,
    BottomOfPipeBit = 1u << 9,
    HostBit = 1u << 10,
    AllGraphicsBit = 1u << 11,
    AllCommandsBit = 1u << 12,
};
SNAP_RHI_DEFINE_ENUM_OPS(PipelineStageBits);

/**
 * @brief Shader stage bitmask.
 *
 * Modeled after Vulkan `VkShaderStageFlagBits`.
 */
enum class ShaderStageBits : uint32_t {
    None = 0,

    VertexShaderBit = 1u << static_cast<uint32_t>(ShaderStage::Vertex),
    FragmentShaderBit = 1u << static_cast<uint32_t>(ShaderStage::Fragment),
    ComputeShaderBit = 1u << static_cast<uint32_t>(ShaderStage::Compute),

    AllGraphics = VertexShaderBit | FragmentShaderBit,
    All = AllGraphics | ComputeShaderBit,
    Count = 4
};
SNAP_RHI_DEFINE_ENUM_OPS(ShaderStageBits);

/**
 * @brief Blending factor used by blending state.
 */
enum class BlendFactor : uint32_t {
    Zero = 0,
    One,
    SrcColor,
    OneMinusSrcColor,
    DstColor,
    OneMinusDstColor,
    SrcAlpha,
    OneMinusSrcAlpha,
    DstAlpha,
    OneMinusDstAlpha,
    ConstantColor,
    OneMinusConstantColor,
    ConstantAlpha,
    OneMinusConstantAlpha,
    SrcAlphaSaturate,

    Count
};

/**
 * @brief Blending operation.
 */
enum class BlendOp : uint32_t {
    Add = 0,
    Subtract,
    ReverseSubtract,
    Min,
    Max,

    Count
};

/**
 * @brief Bitmask selecting which color channels are written.
 */
enum class ColorMask : uint32_t {
    None = 0,

    R = 1 << 0,
    G = 1 << 1,
    B = 1 << 2,
    A = 1 << 3,

    All = R | G | B | A,
};
SNAP_RHI_DEFINE_ENUM_OPS(ColorMask);

/**
 * @brief Component swizzle mode used when creating texture views.
 */
enum class ComponentSwizzle : uint32_t {
    Zero = 0,
    One,

    R,
    G,
    B,
    A,
};

/**
 * @brief Texture view type.
 */
enum class TextureType : uint32_t {
    Texture2D = 0,
    Texture3D,

    Texture2DArray,
    /// Cubemap face order is: +X, -X, +Y, -Y, +Z, -Z.
    TextureCubemap,

    Count
};

/**
 * @brief Bitmask controlling layered rendering semantics.
 */
enum class LayerRenderingTypeBits : uint32_t {
    /// Use instancing to render layers.
    WithInstancing = 1 << 0,
    /// Use shader invocations / built-ins to select layer.
    WithShaderInvocations = 1 << 1,
};
SNAP_RHI_DEFINE_ENUM_OPS(LayerRenderingTypeBits);

/**
 * @brief Bitmask describing intended texture usage.
 */
enum class TextureUsage : uint32_t {
    None = 0u,

    Sampled = 1u << 0,
    Storage = 1u << 1,

    ColorAttachment = 1u << 2,
    DepthStencilAttachment = 1u << 3,

    TransferSrc = 1u << 4,
    TransferDst = 1u << 5,
};
SNAP_RHI_DEFINE_ENUM_OPS(TextureUsage);

/**
 * @brief Bitmask describing numeric data type(s) supported by a pixel format.
 */
enum class FormatDataTypeBits : uint32_t {
    None = 0,

    Float = 1 << 0,
    HalfFloat = 1 << 1,
    Int = 1 << 2,
    NormalizedInt = 1 << 3,

    All = Float | HalfFloat | Int | NormalizedInt
};
SNAP_RHI_DEFINE_ENUM_OPS(FormatDataTypeBits);

/**
 * @brief Rasterization fill mode.
 */
enum class PolygonMode : uint32_t {
    Fill = 0,
    Line,

    Count
};

/**
 * @brief Face culling mode.
 */
enum class CullMode : uint32_t {
    Back = 0,
    Front,
    None,

    Count
};

/**
 * @brief Vertex winding order used for front-face determination.
 */
enum class Winding : uint32_t {
    CCW = 0,
    CW,

    Count
};

/**
 * @brief Depth/stencil compare function.
 */
enum class CompareFunction : uint32_t {
    Never = 0,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always,

    Count
};

/**
 * @brief Stencil buffer operation.
 */
enum class StencilOp : uint32_t {
    Keep = 0,
    Zero,
    Replace,
    IncAndClamp,
    DecAndClamp,
    Invert,
    IncAndWrap,
    DecAndWrap,

    Count
};

/**
 * @brief Bitmask of stencil bits.
 */
enum class StencilMask : uint32_t {
    None = 0,
    AllBitsMask = ~0u,
};

/**
 * @brief Index buffer element type.
 */
enum class IndexType : uint32_t {
    None = 0,
    UInt16,
    UInt32,

    Count
};

/**
 * @brief Primitive topology.
 */
enum class Topology : uint32_t {
    Triangles = 0,
    TriangleStrip,
    Points,
    Lines,
    LineStrip,

    Count
};

/**
 * @brief Vertex attribute input rate.
 */
enum class VertexInputRate : uint32_t {
    /// Advance per vertex.
    PerVertex = 0,
    /// Advance per instance.
    PerInstance,
    /// Constant for all vertices.
    /// To verify that VertexInputRate::Constant supported please use Capabilities::isVertexInputRateConstantSupported.
    Constant,

    Count
};

/**
 * @brief Controls the scope of a flush operation.
 */
enum class FlushType : uint32_t {
    /// Flush all pending operations.
    AllOperations = 0,
};

/**
 * @brief Minification/magnification filter.
 */
enum class SamplerMinMagFilter : uint32_t {
    Nearest = 0,
    Linear,

    Count
};

/**
 * @brief Mipmap selection/filtering mode.
 */
enum class SamplerMipFilter : uint32_t {
    NotMipmapped,
    Nearest,
    Linear,

    Count
};

/**
 * @brief Texture coordinate addressing mode.
 */
enum class WrapMode : uint32_t {
    ClampToEdge = 0,
    Repeat,
    MirrorRepeat,
    ClampToBorderColor,

    Count
};

/**
 * @brief Border color used with `WrapMode::ClampToBorderColor`.
 */
enum class SamplerBorderColor : uint32_t {
    /// {0,0,0,0}
    TransparentBlack = 0,
    /// {0,0,0,1}
    OpaqueBlack,
    /// {1,1,1,1}
    OpaqueWhite,

    /// Transparent/opaque zero depending on format alpha.
    /// (0,0,0,0) for images with an alpha channel
    /// (0,0,0,1) for images without an alpha channel
    ClampToZero,

    Count
};

/**
 * @brief Fence completion state.
 */
enum class FenceStatus : uint32_t {
    NotReady = 0,
    /// Fence is completed and can be reset.
    Completed,
    /// Fence is in error state and cannot be reset.
    Error,

    Count
};

/**
 * @brief Command buffer lifetime stage.
 */
enum class CommandBufferStatus : uint8_t { Initial = 0, Recording, Submitted, Count };

/**
 * @brief Command buffer wait behavior.
 */
enum class CommandBufferWaitType : uint32_t {
    DoNotWait = 0,
    WaitUntilScheduled,
    WaitUntilCompleted,
};

/**
 * @brief Pipeline cache creation flags.
 */
enum class PipelineCacheCreateFlags : uint32_t {
    None = 0,

    /// The cache object is externally synchronized.
    ExternallySynchronized,
};
SNAP_RHI_DEFINE_ENUM_OPS(PipelineCacheCreateFlags);

/**
 * @brief Attachment load operation.
 */
enum class AttachmentLoadOp : uint32_t {
    DontCare = 0,
    Load,
    Clear,

    Count
};

/**
 * @brief Attachment store operation.
 */
enum class AttachmentStoreOp : uint32_t {
    DontCare = 0,
    Store,

    Count
};

/**
 * @brief Requested anisotropic filtering level.
 */
enum class AnisotropicFiltering : uint32_t {
    None = 0,

    Count1,
    Count2,
    Count3,
    Count4,
    Count5,
    Count6,
    Count7,
    Count8,
    Count9,
    Count10,
    Count11,
    Count12,
    Count13,
    Count14,
    Count15,
    Count16,
};

/**
 * @brief Descriptor binding type.
 */
enum class DescriptorType : uint32_t {
    Undefined = 0,

    Sampler,

    SampledTexture,
    StorageTexture,

    UniformBuffer,
    StorageBuffer,

    UniformBufferDynamic,
    StorageBufferDynamic,

    Count
};

/**
 * @brief Pipeline bind point.
 */
enum class PipelineBindPoint : uint32_t {
    /// Graphics/Render pipeline bind point.
    Graphics,
    /// Compute pipeline bind point.
    Compute,
};

/**
 * @brief Backend API identifier.
 */
enum class API : uint32_t {
    Undefined = 0,

    OpenGL,
    Metal,
    Vulkan,
    WebGPU,

    /// No-op backend.
    NoOp,

    /// Virtualized APIs.
    VirtualOpenGL,
    VirtualMetal,
    VirtualVulkan,

    Count,
};

/**
 * @brief Indicates how a backend defines the origin of texture coordinates.
 */
enum class TextureOriginConvention : uint32_t {
    /// Origin is at top-left (Metal/Vulkan).
    TopLeft,
    /// Origin is at bottom-left (OpenGL).
    BottomLeft,

    Count
};

/**
 * @brief Describes the normalized device coordinate (NDC) layout conventions.
 *
 * Different graphics APIs use different conventions for ndc-space coordinates:
 * - Y-axis direction varies: OpenGL/DirectX/Metal use Y-up, Vulkan uses Y-down
 * - Depth range varies: OpenGL uses [-1, 1], others use [0, 1]
 *
 * This struct captures both aspects to enable proper coordinate transformations
 * in cross-API rendering code.
 */
struct NDCLayout {
    /**
     * @brief Clip-space Y-axis direction.
     */
    enum class YAxis : uint8_t {
        /// Y grows upward (standard Cartesian) — OpenGL, DirectX, Metal.
        Up,
        /// Y grows downward — Vulkan.
        Down
    };

    /**
     * @brief NDC-space depth (Z) range after projection.
     */
    enum class DepthRange : uint8_t {
        /// Depth in [-1, 1] — OpenGL (legacy/compatibility).
        MinusOneToOne,
        /// Depth in [0, 1] — Vulkan, DirectX, Metal.
        ZeroToOne
    };

    YAxis yAxis = YAxis::Up;
    DepthRange depthRange = DepthRange::MinusOneToOne;

    constexpr friend auto operator<=>(const NDCLayout&, const NDCLayout&) noexcept = default;
};

/**
 * @brief Runtime resource type identifier (used for validation/reporting).
 */
#undef SNAP_RHI_RESOURCE_TYPES
#define SNAP_RHI_RESOURCE_TYPES(X)                                                                                  \
    X(Undefined), X(Texture), X(Buffer), X(CommandBuffer), X(CommandQueue), X(Fence), X(Semaphore), X(Framebuffer), \
        X(RenderPass), X(ComputePipeline), X(RenderPipeline), X(Sampler), X(ShaderLibrary), X(ShaderModule),        \
        X(DescriptorSet), X(DescriptorSetLayout), X(DescriptorPool), X(PipelineCache), X(DeviceContext),            \
        X(PipelineLayout), X(DebugMessenger), X(ComputeCommandEncoder), X(RenderCommandEncoder), X(QueryPool)

#define SNAP_RHI_RESOURCE_TYPE_ENUM(name) name
enum class ResourceType : uint32_t {
    SNAP_RHI_RESOURCE_TYPES(SNAP_RHI_RESOURCE_TYPE_ENUM),

    Count
};
#undef SNAP_RHI_RESOURCE_TYPE_ENUM

#define SNAP_RHI_RESOURCE_TYPE_STRING(name) std::string_view(#name)
constexpr std::array resourceTypeToString{
    SNAP_RHI_RESOURCE_TYPES(SNAP_RHI_RESOURCE_TYPE_STRING),
};
#undef SNAP_RHI_RESOURCE_TYPE_STRING
#undef SNAP_RHI_RESOURCE_TYPES

/**
 * @brief Returns a short human-readable name for a resource type.
 *
 * @param type Resource type.
 * @return String view corresponding to @p type, or `"Undefined"` if out of range.
 */
constexpr std::string_view getResourceTypeStr(const snap::rhi::ResourceType type) {
    if (type >= ResourceType::Count) {
        return "Undefined";
    }

    return resourceTypeToString[static_cast<uint32_t>(type)];
}

/**
 * @brief Validation report severity.
 */
enum class ReportLevel : uint32_t {
    All = 0,

    Debug,   // Informational message only on Debug build
    Info,    // Informational message like the creation of a resource
    Warning, // Message about behavior that is not necessarily an error, but very likely a bug in your application
    PerformanceWarning, // Same as "Warning", but also indicates a potential cause of performance degradation
    Error,              // Message about behavior that is invalid and may cause crashes
    CriticalError,      // Message about behavior that is definitely invalid and will cause crashes(like pAssert)

    Count
};

/// String table for `ReportLevel` (used by `getReportLevelStr`).
constexpr std::array<std::string_view, static_cast<uint32_t>(snap::rhi::ReportLevel::Count)> reportLevelToString{
    {"All", "Debug", "Info", "Warning", "PerformanceWarning", "Error", "CriticalError"}};

/**
 * @brief Returns a short human-readable name for a report level.
 *
 * Used by backend validation/reporting code when formatting log messages.
 *
 * @param level Report severity level.
 * @return String view corresponding to @p level, or `"Undefined"` if out of range.
 */
constexpr std::string_view getReportLevelStr(const ReportLevel level) {
    if (level >= ReportLevel::Count) {
        return "Undefined";
    }

    return reportLevelToString[static_cast<uint32_t>(level)];
}

#undef SNAP_RHI_VALIDATION_TAGS
#define SNAP_RHI_VALIDATION_TAGS(X)                                                                        \
    /*!! @brief Resource creation and initialization operations. */                                        \
    X(CreateOp)                                                                                            \
    /*!! @brief Resource destruction and teardown operations. */                                           \
    X(DestroyOp)                                                                                           \
    /*!! @brief Device/context creation, configuration, and API-level capability/feature setup. */         \
    X(DeviceContextOp)                                                                                     \
    /*!! @brief Command queue operations such as submit/present/signal/wait (API-agnostic). */             \
    X(CommandQueueOp)                                                                                      \
    /*!! @brief Command buffer lifecycle and recording/submit-related validations (API-agnostic). */       \
    X(CommandBufferOp)                                                                                     \
    /*!! @brief Render command encoder operations (recording render commands). */                          \
    X(RenderCommandEncoderOp)                                                                              \
    /*!! @brief Compute command encoder operations (recording compute commands). */                        \
    X(ComputeCommandEncoderOp)                                                                             \
    /*!! @brief Blit/copy command encoder operations (copy/resolve/transfer commands). */                  \
    X(BlitCommandEncoderOp)                                                                                \
    /*!! @brief Render pass operations (begin/end pass, attachments, load/store semantics). */             \
    X(RenderPassOp)                                                                                        \
    /*!! @brief Framebuffer object and attachment configuration/compatibility validations. */              \
    X(FramebufferOp)                                                                                       \
    /*!! @brief Graphics/render pipeline operations (creation/binding/compatibility). */                   \
    X(RenderPipelineOp)                                                                                    \
    /*!! @brief Compute pipeline operations (creation/binding/compatibility). */                           \
    X(ComputePipelineOp)                                                                                   \
    /*!! @brief Shader module operations (compilation, reflection, backend module creation). */            \
    X(ShaderModuleOp)                                                                                      \
    /*!! @brief Shader library operations (collections of shader functions/modules). */                    \
    X(ShaderLibraryOp)                                                                                     \
    /*!! @brief Sampler operations and state validation (filtering, addressing, comparison, etc.). */      \
    X(SamplerOp)                                                                                           \
    /*!! @brief Texture operations and state validation (creation, views, layout/state tracking). */       \
    X(TextureOp)                                                                                           \
    /*!! @brief Buffer operations and state validation (creation, mapping, upload/copy, usage flags). */   \
    X(BufferOp)                                                                                            \
    /*!! @brief Descriptor set layout operations (bindings, types, visibility, compatibility rules). */    \
    X(DescriptorSetLayoutOp)                                                                               \
    /*!! @brief Pipeline layout operations (descriptor set layouts, push constants, compatibility). */     \
    X(PipelineLayoutOp)                                                                                    \
    /*!! @brief Descriptor pool allocation/free/reset operations and capacity validation. */               \
    X(DescriptorPoolOp)                                                                                    \
    /*!! @brief Descriptor set allocation/update/bind operations and validation. */                        \
    X(DescriptorSetOp)                                                                                     \
    /*!! @brief Fence operations (creation, reset, wait, signal, export/import). */                        \
    X(FenceOp)                                                                                             \
    /*!! @brief Semaphore operations (signal/wait, timeline/binary semantics, export/import). */           \
    X(SemaphoreOp)                                                                                         \
    /*!! @brief Device-level operations not covered by DeviceContextOp. */                                 \
    X(DeviceOp)                                                                                            \
    /*!! @brief Pipeline cache operations (load/store/merge/cache-miss behavior). */                       \
    X(PipelineCacheOp)                                                                                     \
    /*!! @brief Query pool operations (timestamp/occlusion/statistics queries). */                         \
    X(QueryPoolOp)                                                                                         \
    /*!! @brief OpenGL queue: errors triggered by external system/driver state (mapped from GL errors). */ \
    X(GLCommandQueueExternalError)                                                                         \
    /*!! @brief OpenGL queue: internal SnapRHI invariant violations leading to GL errors. */               \
    X(GLCommandQueueInternalError)                                                                         \
    /*!! @brief OpenGL state cache validations (cached GL state vs driver state). */                       \
    X(GLStateCacheOp)                                                                                      \
    /*!! @brief OpenGL program validation (link/validate checks and diagnostics). */                       \
    X(GLProgramValidationOp)                                                                               \
    /*!! @brief OpenGL native calls validation through glGetError. */                                      \
    X(GLProfileOp)

// 1) Shadow indices (CreateOp_i, DestroyOp_i, ...) used to derive bit shifts.
#define SNAP_RHI_VALIDATION_TAG_INDEX_ENTRY(NAME) NAME##_i,

enum class ValidationTagIndex : uint64_t {
    SNAP_RHI_VALIDATION_TAGS(SNAP_RHI_VALIDATION_TAG_INDEX_ENTRY) ValidationTag_Count
};

#undef SNAP_RHI_VALIDATION_TAG_INDEX_ENTRY

// 2) Real bitmask enum values (1ull << NAME##_i)
#define SNAP_RHI_VALIDATION_TAG_ENUM_ENTRY(NAME) NAME = (1ull << static_cast<uint64_t>(ValidationTagIndex::NAME##_i)),

/// String table for `ValidationTag` (used by `getValidationTagStr`).
constexpr auto ValidationTagStr = []() {
    std::array<std::string_view, static_cast<uint64_t>(ValidationTagIndex::ValidationTag_Count)> arr{};

#define SNAP_RHI_VALIDATION_TAG_FILL_ENTRY(NAME) arr[static_cast<uint64_t>(ValidationTagIndex::NAME##_i)] = #NAME;
    SNAP_RHI_VALIDATION_TAGS(SNAP_RHI_VALIDATION_TAG_FILL_ENTRY)
#undef SNAP_RHI_VALIDATION_TAG_FILL_ENTRY

    return arr;
}();

/**
 * @brief Validation category bitmask.
 */
enum class ValidationTag : uint64_t {
    None = 0,

    SNAP_RHI_VALIDATION_TAGS(SNAP_RHI_VALIDATION_TAG_ENUM_ENTRY)

        All = ~0ull,
    Count = static_cast<uint64_t>(ValidationTagIndex::ValidationTag_Count),
};
SNAP_RHI_DEFINE_ENUM_OPS(ValidationTag);

// Compile-time selection of validation tags.
// SNAP_RHI_ENABLE_VALIDATION_TAG_<TAG> flags are generated by the public-api target.
// This section derives a single snap::rhi::ValidationTag bitmask from the tag list in snap/rhi/Enums.h,
// so we don't have to duplicate tag names here.

#ifndef SNAP_RHI_ENABLED_VALIDATION_TAGS
#define SNAP_RHI_VALIDATION_TAG_OR_ENABLED(NAME)                                                     \
    static_cast<uint64_t>((SNAP_RHI_ENABLE_VALIDATION_TAG_##NAME) ? snap::rhi::ValidationTag::NAME : \
                                                                    snap::rhi::ValidationTag::None) |

#define SNAP_RHI_ENABLED_VALIDATION_TAGS                                                                      \
    static_cast<uint64_t>(static_cast<uint64_t>(snap::rhi::ValidationTag::None) |                             \
                          SNAP_RHI_VALIDATION_TAGS(SNAP_RHI_VALIDATION_TAG_OR_ENABLED) static_cast<uint64_t>( \
                              snap::rhi::ValidationTag::None))

static constexpr uint64_t EnabledValidationTags = (static_cast<uint64_t>(SNAP_RHI_ENABLED_VALIDATION_TAGS));

#undef SNAP_RHI_VALIDATION_TAG_OR_ENABLED
#endif

#undef SNAP_RHI_VALIDATION_TAG_ENUM_ENTRY

inline std::string convertToString(snap::rhi::ValidationTag tag) {
    if (tag == 0)
        return "None";

    std::string result;
    bool first = true;
    auto val = static_cast<uint64_t>(tag);

    while (val != 0) {
        auto bit_index = std::countr_zero(val);

        if (!first)
            result += " | ";

        // Safety check: ensure the bit index is within our generated count
        if (bit_index < ValidationTagStr.size()) {
            result += ValidationTagStr[bit_index];
        } else {
            result += "Unknown(" + std::to_string(bit_index) + ")";
        }

        first = false;
        val &= ~(1ull << bit_index);
    }
    return result;
}

#undef SNAP_RHI_VALIDATION_TAGS

/**
 * @brief Sentinel value used to represent an unused attachment index.
 */
constexpr uint32_t AttachmentUnused = ~(0u);

using APIVersionType = uint32_t;

constexpr APIVersionType APIVersionNone = 0;
constexpr APIVersionType APIVersionLatest = std::numeric_limits<APIVersionType>::max();

struct APIDescription {
    API api;
    APIVersionType version;

    /// @brief Check if API is strictly native Metal.
    /// @note If true, then Metal-specific functions can be used
    [[nodiscard]] constexpr bool isNativeMetal() const {
        return api == API::Metal;
    }

    /// @brief Check if API is strictly native OpenGL.
    /// @note If true, then OpenGL-specific functions can be used, e.g.: snap::rhi::Utils::APISpecific::GLES::*.
    [[nodiscard]] constexpr bool isNativeOpenGL() const {
        return api == API::OpenGL;
    }

    /// @brief Check if API is strictly native Vulkan.
    /// @note If true, then Vulkan-specific functions can be used, e.g.: snap::rhi::Utils::APISpecific::Vulkan::*.
    [[nodiscard]] constexpr bool isNativeVulkan() const {
        return api == API::Vulkan;
    }

    /// @brief Check if API is virtual (not native).
    /// @note If true, then API-specific functions cannot be used.
    [[nodiscard]] constexpr bool isVirtual() const {
        switch (api) {
            case API::VirtualOpenGL:
            case API::VirtualMetal:
            case API::VirtualVulkan:
                return true;
            default:
                return false;
        }
    }

    /// @brief Check if API is native (not virtual).
    /// @note If true, then API-specific functions for the corresponding backend can be used,
    /// for example, snap::rhi::Utils::APISpecific::GLES::* for native OpenGL backends.
    [[nodiscard]] constexpr bool isNative() const {
        return !isVirtual();
    }

    /// @brief Check if API is any Metal backend.
    /// @note This is useful for making API-based decisions, e.g.: which shading language to use.
    /// Importantly, using API-specific functions will fail for the virtual backend.
    /// Ensure to check if isNative() first.
    [[nodiscard]] constexpr bool isAnyMetal() const {
        return api == API::Metal || api == API::VirtualMetal;
    }

    /// @brief Check if API is any OpenGL backend.
    /// @note This is useful for making API-based decisions, e.g.: which shading language to use.
    /// Importantly, using API-specific functions will fail for the virtual backend.
    /// Ensure to check if isNative() first.
    [[nodiscard]] constexpr bool isAnyOpenGL() const {
        return api == API::OpenGL || api == API::VirtualOpenGL;
    }

    /// @brief Check if API is any Vulkan backend.
    /// @note This is useful for making API-based decisions, e.g.: which shading language to use.
    /// Importantly, using API-specific functions will fail for the virtual backend.
    /// Ensure to check if isNative() first.
    [[nodiscard]] constexpr bool isAnyVulkan() const {
        return api == API::Vulkan || api == API::VirtualVulkan;
    }

    constexpr friend auto operator<=>(APIDescription, APIDescription) noexcept = default;
};

// https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkFormatFeatureFlagBits.html
enum class FormatFeatures : uint32_t {
    None = 0u,

    Sampled = (1u << 0),

    /**
     * Specifies that an texture can be used with a sampler that has either of magFilter or minFilter set to
     * Filter::Linear, or mipmapMode set to MipMapMode::Linear.
     *
     * If the format being queried is a depth/stencil format, this bit only specifies that the depth aspect (not the
     * stencil aspect) of an image of this format supports linear filtering.
     *
     * Vulkan:
     *      VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT
     *
     * OpenGL:
     *      Some texture format extensions may require only nearest filter
     *
     * Metal:
     *      No restrictions
     */
    SampledFilterLinear = (1u << 1) | Sampled,

    /**
     * Specifies that an texture can be used only as a framebuffer color attachment
     */
    ColorRenderable = (1u << 2),

    /**
     * Specifies that an texture can be used as a framebuffer color attachment that supports blending
     *
     * Vulkan:
     *      VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT
     *
     * OpenGL:
     *      No restrictions (all renderable texture can use blend)
     *
     * Metal:
     *      https://developer.apple.com/metal/Metal-Feature-Set-Tables.pdf
     */
    ColorRenderableBlend = (1u << 3) | ColorRenderable,

    /**
     * Specifies that an texture can be used as a framebuffer depth/stencil attachment.
     */
    DepthStencilRenderable = (1u << 4),

    /**
     * Specifies that an texture can be used as a source image for only image copy commands(texture <-> texture).
     */
    BlitSrc = (1u << 5),

    /**
     * Specifies that an texture can be used as a destination image for only image copy commands(texture <-> texture).
     */
    BlitDst = (1u << 6),

    /**
     * The texture can be used as the destination for resolved MSAA data.
     */
    Resolve = (1u << 7),

    /**
     * Specifies that an texture can be used as a source image for transfer commands (texture -> buffer).
     */
    TransferSrc = (1u << 8),

    /**
     * Specifies that an texture can be used as a destination image for transfer commands (buffer -> texture).
     * and clear commands.
     */
    TransferDst = (1u << 9),

    /**
     * Specifies that an texture can be used as a storage texture.
     * */
    Storage = (1u << 10)
};
SNAP_RHI_DEFINE_ENUM_OPS(FormatFeatures);

enum class CommandQueueFeatures : uint32_t {
    None = 0,

    /// Queue supports graphics rendering commands.
    Graphics = 1u << 0,
    /// Queue supports compute commands.
    Compute = 1u << 1,
    /// Queue supports transfer/copy commands.
    Transfer = 1u << 2
};
SNAP_RHI_DEFINE_ENUM_OPS(CommandQueueFeatures);

enum class DepthStencilFormatTraits : uint32_t {
    None = 0,
    HasDepthAspect = 1 << 0,
    HasStencilAspect = 1 << 1,
    HasDepthStencilAspects = HasDepthAspect | HasStencilAspect
};
SNAP_RHI_DEFINE_ENUM_OPS(DepthStencilFormatTraits);

enum class StencilFace : uint32_t {
    Front = 0,
    Back,
    FrontAndBack,
};

/**
 * @brief Where to place a timestamp relative to a measured workload.
 */
enum class TimestampLocation {
    /**
     * @brief Marks the *beginning* of a block of work to be measured.
     * Guarantees the timestamp is recorded *before* subsequent work begins.
     */
    Start = 0,

    /**
     * @brief Marks the *end* of a block of work to be measured.
     * Guarantees the timestamp is recorded *after* all previous work has finished.
     */
    End
};

constexpr std::string_view ShaderStageBitsToStr[] = {
    "None",
    "Vertex",
    "Fragment",
    "AllGraphics",
};

/**
 * @brief Converts pipeline stage bits to a human-readable string.
 *
 * Currently used for OpenGL validation/debug logging.
 *
 * @param pipelineStage Stage bitmask.
 * @return String representation. Currently returns the numeric value.
 */
inline std::string getPipelineStageBitsStr(const PipelineStageBits pipelineStage) {
    return std::to_string(static_cast<uint32_t>(pipelineStage));
}

/**
 * @brief Converts shader stage bits to a human-readable string.
 *
 * @param shaderStage Shader stage bitmask.
 * @return A readable string for common values. For unrecognized values returns `"Undefined"`.
 */
inline std::string getShaderStageBitsStr(const ShaderStageBits shaderStage) {
    if (shaderStage == ShaderStageBits::None) {
        return "None";
    }
    if (shaderStage == ShaderStageBits::All) {
        return "All";
    }

    std::string shaderStageBitsString;

    // Create a string representation list of shader stage names that are set in shaderStageBits
    for (uint32_t i = 0; i < static_cast<uint32_t>(ShaderStageBits::Count); ++i) {
        if ((static_cast<uint32_t>(shaderStage) & (1u << i)) != 0) {
            if (!shaderStageBitsString.empty()) {
                shaderStageBitsString += ", ";
            }

            shaderStageBitsString += std::string(ShaderStageBitsToStr[i]);
        }
    }

    if (!shaderStageBitsString.empty()) {
        return shaderStageBitsString;
    }

    return "Undefined";
}
} // namespace snap::rhi
