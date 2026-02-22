#pragma once

#include "snap/rhi/common/HashCombine.h"
#include <array>
#include <compare>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "snap/rhi/Enums.h"
#include "snap/rhi/Limits.h"
#include "snap/rhi/PixelFormat.h"
#include "snap/rhi/VertexAttributeFormat.h"
#include "snap/rhi/metal/RenderPipelineInfo.h"
#include "snap/rhi/opengl/RenderPipelineInfo.h"
#include "snap/rhi/reflection/RenderPipelineInfo.h"

namespace snap::rhi {
class ShaderModule;
class PipelineCache;
class RenderPipeline;
class RenderPass;
class PipelineLayout;

/**
 * @brief Stencil operations and masks for a single face.
 *
 * This mirrors common graphics API stencil state: which operation to apply on various test outcomes and which bits are
 * readable/writable.
 */
struct StencilOpState {
    /// Operation applied when the stencil test fails.
    StencilOp failOp = StencilOp::Keep;
    /// Operation applied when both stencil and depth tests pass.
    StencilOp passOp = StencilOp::Keep;
    /// Operation applied when stencil passes but depth fails.
    StencilOp depthFailOp = StencilOp::Keep;

    /// Comparison function used for the stencil test.
    CompareFunction stencilFunc = CompareFunction::Always;

    /// Bitmask selecting the stencil bits that are compared.
    StencilMask readMask = StencilMask::None;
    /// Bitmask selecting the stencil bits that may be written.
    StencilMask writeMask = StencilMask::None;

    /**
     * @brief Three-way comparison.
     */
    constexpr friend auto operator<=>(const StencilOpState&, const StencilOpState&) noexcept = default;
};

/**
 * @brief Depth/stencil state for a render pipeline.
 */
struct DepthStencilStateCreateInfo {
    /**
     *  If we do not need depth testing, but do need depth writing, we should use all:
     *      - depthFunc = CompareFunction::Always;
     *      - depthTest = true;
     *      - depthWrite = true;
     *
     *  When depth testing is disabled, writes to the depth buffer are also disabled.
     */
    bool depthTest = false;
    /// Enables writing to the depth buffer.
    bool depthWrite = false;
    /// Depth comparison function.
    CompareFunction depthFunc = CompareFunction::Always;

    /// Enables stencil testing.
    bool stencilEnable = false;

    /// Stencil state for front-facing primitives.
    StencilOpState stencilFront;
    /// Stencil state for back-facing primitives.
    StencilOpState stencilBack;

    /**
     * @brief Three-way comparison.
     */
    constexpr friend auto operator<=>(const DepthStencilStateCreateInfo&,
                                      const DepthStencilStateCreateInfo&) noexcept = default;
};

/**
 * @brief Per-attachment color blend state.
 */
struct RenderPipelineColorBlendAttachmentState {
    /// Enables blending for this attachment.
    bool blendEnable = false;

    BlendFactor srcColorBlendFactor = BlendFactor::One;
    BlendFactor dstColorBlendFactor = BlendFactor::Zero;
    BlendOp colorBlendOp = BlendOp::Add;
    BlendFactor srcAlphaBlendFactor = BlendFactor::One;
    BlendFactor dstAlphaBlendFactor = BlendFactor::Zero;
    BlendOp alphaBlendOp = BlendOp::Add;

    /// Color component write mask.
    ColorMask colorWriteMask = ColorMask::None;

    /**
     * @brief Three-way comparison.
     */
    constexpr friend auto operator<=>(const RenderPipelineColorBlendAttachmentState&,
                                      const RenderPipelineColorBlendAttachmentState&) noexcept = default;
};

/**
 * @brief Global color blend state.
 */
struct ColorBlendStateCreateInfo {
    /// Blend state for each color attachment.
    std::array<snap::rhi::RenderPipelineColorBlendAttachmentState, MaxColorAttachments> colorAttachmentsBlendState;
    /// Number of active entries in @ref colorAttachmentsBlendState.
    uint32_t colorAttachmentsCount = 0;
};

/**
 * @brief Rasterization state.
 */
struct RasterizationStateCreateInfo {
    /// Enables rasterization. When disabled, primitives are discarded.
    bool rasterizationEnabled = false;

    PolygonMode polygonMode = PolygonMode::Fill;
    CullMode cullMode = CullMode::Back;
    Winding windingMode = Winding::CCW;

    /// Enables depth bias when supported by the active backend/device.
    bool depthBiasEnable = false;

    /**
     * Used for clip geometry against user-defined half space.
     * Must be less or equal snap::rhi::Capabilities::maxClipDistances.
     *
     * https://docs.vulkan.org/samples/latest/samples/extensions/dynamic_primitive_clipping/README.html
     */
    uint32_t clipDistancePlaneCount = 0;

    /**
     * @brief Three-way comparison.
     */
    constexpr friend auto operator<=>(const RasterizationStateCreateInfo&,
                                      const RasterizationStateCreateInfo&) noexcept = default;
};

/**
 * @brief Multisampling state.
 */
struct MultisampleStateCreateInfo {
    /**
     * @brief Sample count.
     *
     * `snap::rhi::SampleCount::Count1` means MSAA is disabled.
     */
    snap::rhi::SampleCount samples = snap::rhi::SampleCount::Count1;

    bool alphaToCoverageEnable = false;
    bool alphaToOneEnable = false;

    /**
     * @brief Three-way comparison.
     */
    constexpr friend auto operator<=>(const MultisampleStateCreateInfo&,
                                      const MultisampleStateCreateInfo&) noexcept = default;
};

/**
 * @brief Description of a single vertex attribute.
 */
struct VertexInputAttributeDescription {
    /**
     * @location is the attribute location.
     * Must be in [0; snap::rhi::Capabilities::maxVertexInputAttributes - 1].
     */
    uint32_t location = Undefined;

    /**
     * @binding is the binding number which this attribute takes its data from.
     * Must be in [0; snap::rhi::Capabilities::maxVertexInputBindings - 1].
     */
    uint32_t binding = 0;

    /**
     * @offset is a byte offset of this attribute relative to the start of an element in the vertex input binding.
     *
     * Vulkan/OpenGL:
     *      no restrictions.
     * Metal:
     *      the @offset value must be a multiple of 4 bytes.
     *      https://developer.apple.com/documentation/metal/mtlvertexattributedescriptor/1515785-offset?language=objc
     *
     * SnapRHI:
     *      the @offset value must be a multiple of 4 bytes.
     */
    uint32_t offset = 0;

    VertexAttributeFormat format =
        VertexAttributeFormat::Undefined; // format is the size and type of the vertex attribute data.

    /**
     * @brief Three-way comparison.
     */
    constexpr friend auto operator<=>(const VertexInputAttributeDescription&,
                                      const VertexInputAttributeDescription&) noexcept = default;
};

/**
 * @brief Description of a single vertex buffer binding.
 */
struct VertexInputBindingDescription {
    /**
     * @binding is the binding number which this attribute takes its data from.
     * Must be in [0; snap::rhi::Capabilities::maxVertexInputBindings - 1].
     */
    uint32_t binding = 0;

    /**
     * @divisor is the number of successive instances that will use the same value of the vertex attribute
     * when instanced rendering is enabled. For example, if the divisor is N,
     * the same vertex attribute will be applied to N successive instances before moving on to the next vertex
     * attribute.
     *
     * The maximum value of divisor is implementation-dependent and can be queried using
     * Capabilities::maxVertexAttribDivisor.
     *
     * A value of 0 is unsupported value.
     *
     * Vulkan:
     *      https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VK_EXT_vertex_attribute_divisor.html
     *
     * OpenGL:
     *      https://www.khronos.org/registry/OpenGL-Refpages/es3.0/html/glVertexAttribDivisor.xhtml
     *
     * Metal:
     *      https://developer.apple.com/documentation/metal/mtlvertexbufferlayoutdescriptor/1516148-steprate?language=objc
     */
    uint32_t divisor = 1;

    /**
     * @stride is the distance, in bytes, between the attribute data of two vertices in the buffer.
     * @stride should be at leat 1.
     *
     * Vulkan/OpenGL:
     *      no restrictions.
     * Metal:
     *      the @stride value must be a multiple of 4 bytes.
     *      https://developer.apple.com/documentation/metal/mtlvertexbufferlayoutdescriptor/1515441-stride?language=objc
     *
     * SnapRHI:
     *      the @stride value must be a multiple of 4 bytes.
     */
    uint32_t stride = Undefined;

    /**
     * If @inputRate is VertexInputRate::Constant, only float attributes always supported.
     * Other attribute format supporting should by verified with  @Capabilities::constantInputRateSupportingBits.
     */
    VertexInputRate inputRate = VertexInputRate::PerVertex;

    /**
     * @brief Three-way comparison.
     */
    constexpr friend auto operator<=>(const VertexInputBindingDescription&,
                                      const VertexInputBindingDescription&) noexcept = default;
};

/**
 * @brief Input assembly state.
 */
struct InputAssemblyStateCreateInfo {
    Topology primitiveTopology = Topology::Triangles;
    // bool primitiveRestartEnable = true;
    /**
     * Vulkan
     * This enable only applies to indexed draws, and the special index value is either 0xFFFFFFFF when the indexType
     * parameter is equal to VK_INDEX_TYPE_UINT32, 0xFF when indexType is equal to VK_INDEX_TYPE_UINT8_EXT, or 0xFFFF
     * when indexType is equal to VK_INDEX_TYPE_UINT16. Primitive restart is not allowed for “list” topologies.
     *
     * OpenGL
     * GL_PRIMITIVE_RESTART_FIXED_INDEX
     *
     * Metal
     * Metal does not allow to disable primitive restart, and the primitive restart value is chosen depending on the
     * index buffer format.
     *
     * SnapRHI Solution:
     * Primitive restart alway enabled if can(check DeviceCaps), and the primitive restart value is chosen depending on
     * the index buffer format.
     */

    /**
     * @brief Three-way comparison.
     */
    constexpr friend auto operator<=>(const InputAssemblyStateCreateInfo&,
                                      const InputAssemblyStateCreateInfo&) noexcept = default;
};

/**
 * @brief Vertex input state (attributes + buffer bindings).
 */
struct VertexInputStateCreateInfo {
    std::array<VertexInputAttributeDescription, MaxVertexAttributes> attributeDescription;
    std::array<VertexInputBindingDescription, MaxVertexBuffers> bindingDescription;

    /// Number of active entries in @ref attributeDescription.
    uint32_t attributesCount = 0;
    /// Number of active entries in @ref bindingDescription.
    uint32_t bindingsCount = 0;
};

/**
 * @brief Attachment formats used for dynamic rendering.
 *
 * This is used when a backend supports dynamic rendering and `RenderPipelineCreateInfo::renderPass` is null.
 */
struct AttachmentFormatsCreateInfo {
    /// Color attachment formats in draw order.
    std::vector<snap::rhi::PixelFormat> colorAttachmentFormats;
    /// Depth attachment format, or `PixelFormat::Undefined` if unused.
    snap::rhi::PixelFormat depthAttachmentFormat = snap::rhi::PixelFormat::Undefined;
    /// Stencil attachment format, or `PixelFormat::Undefined` if unused.
    snap::rhi::PixelFormat stencilAttachmentFormat = snap::rhi::PixelFormat::Undefined;

    /**
     * @brief Equality comparison.
     */
    constexpr friend bool operator==(const AttachmentFormatsCreateInfo& l,
                                     const AttachmentFormatsCreateInfo& r) noexcept = default;
};

/**
 * @brief Render pipeline creation parameters.
 *
 * This structure describes shader stages and fixed-function state for a graphics pipeline.
 *
 * Render target compatibility:
 * - When using a render pass based backend, set @ref renderPass and @ref subpass.
 * - When using dynamic rendering (when supported), @ref renderPass may be null and attachment formats must be provided
 *   via @ref attachmentFormatsCreateInfo.
 */
struct RenderPipelineCreateInfo {
    PipelineCreateFlags pipelineCreateFlags = PipelineCreateFlags::None;
    std::vector<ShaderModule*> stages{};
    VertexInputStateCreateInfo vertexInputState{};
    InputAssemblyStateCreateInfo inputAssemblyState{};
    RasterizationStateCreateInfo rasterizationState{};
    MultisampleStateCreateInfo multisampleState{};
    DepthStencilStateCreateInfo depthStencilState{};
    ColorBlendStateCreateInfo colorBlendState{};

    /*
     * The render pass object must be alive
     * as long as any renderpipeline created from it is still in use.
     *
     * According to the Vulkan specification, two render passes are considered compatible if:
     *
     * - They have the same number of attachments.
     * - The formats of corresponding attachments are identical or compatible.
     * - The number of subpasses is the same.
     * - The formats and sample counts of attachments used in subpasses match.
     * - Subpass dependencies must be the same.
     * - Attachment load/store operations do not have to match. (This allows minor differences in behavior without
     * requiring pipeline recreation.)
     */
    RenderPass* renderPass = nullptr;
    uint32_t subpass = 0;

    /*
     * If DynamicRendering is supported, renderPass may be nullptr, and the user may instead fill in an
     * AttachmentFormatsCreateInfo structure.
     */
    AttachmentFormatsCreateInfo attachmentFormatsCreateInfo{};

    RenderPipeline* basePipeline = nullptr;
    PipelineCache* pipelineCache = nullptr;
    PipelineLayout* pipelineLayout = nullptr;

    /**
     * @brief Optional OpenGL-specific pipeline metadata.
     *
     * OpenGL configurations require extra information to bind resources correctly.
     * When provided, this data is consumed only by the OpenGL backend.
     */
    std::optional<snap::rhi::opengl::RenderPipelineInfo> glRenderPipelineInfo = std::nullopt;

    /**
     * @brief Optional Metal-specific pipeline metadata.
     *
     * Metal configurations require extra information to bind resources correctly.
     * When provided, this data is consumed only by the Metal backend.
     */
    std::optional<snap::rhi::metal::RenderPipelineInfo> mtlRenderPipelineInfo = std::nullopt;
};
} // namespace snap::rhi

namespace std {
template<>
struct hash<snap::rhi::StencilOpState> {
    size_t operator()(const snap::rhi::StencilOpState& info) const noexcept {
        return snap::rhi::common::hash_combine(
            info.failOp, info.passOp, info.depthFailOp, info.stencilFunc, info.readMask, info.writeMask);
    }
};

template<>
struct hash<snap::rhi::DepthStencilStateCreateInfo> {
    size_t operator()(const snap::rhi::DepthStencilStateCreateInfo& info) const noexcept {
        return snap::rhi::common::hash_combine(info.depthTest,
                                               info.depthWrite,
                                               info.depthFunc,
                                               info.stencilEnable,
                                               std::hash<snap::rhi::StencilOpState>()(info.stencilFront),
                                               std::hash<snap::rhi::StencilOpState>()(info.stencilBack));
    }
};

template<>
struct hash<snap::rhi::RenderPipelineColorBlendAttachmentState> {
    size_t operator()(const snap::rhi::RenderPipelineColorBlendAttachmentState& info) const noexcept {
        return snap::rhi::common::hash_combine(info.blendEnable,
                                               info.srcColorBlendFactor,
                                               info.dstColorBlendFactor,
                                               info.colorBlendOp,
                                               info.srcAlphaBlendFactor,
                                               info.dstAlphaBlendFactor,
                                               info.alphaBlendOp,
                                               info.colorWriteMask);
    }
};

template<>
struct hash<snap::rhi::ColorBlendStateCreateInfo> {
    size_t operator()(const snap::rhi::ColorBlendStateCreateInfo& info) const noexcept {
        size_t result = info.colorAttachmentsCount;
        for (uint32_t i = 0; i < info.colorAttachmentsCount; ++i) {
            result = snap::rhi::common::hash_combine(
                result,
                std::hash<snap::rhi::RenderPipelineColorBlendAttachmentState>()(info.colorAttachmentsBlendState[i]));
        }
        return result;
    }
};

template<>
struct hash<snap::rhi::RasterizationStateCreateInfo> {
    size_t operator()(const snap::rhi::RasterizationStateCreateInfo& info) const noexcept {
        return snap::rhi::common::hash_combine(info.rasterizationEnabled,
                                               info.polygonMode,
                                               info.cullMode,
                                               info.windingMode,
                                               info.depthBiasEnable,
                                               info.clipDistancePlaneCount);
    }
};

template<>
struct hash<snap::rhi::MultisampleStateCreateInfo> {
    size_t operator()(const snap::rhi::MultisampleStateCreateInfo& info) const noexcept {
        return snap::rhi::common::hash_combine(info.samples, info.alphaToCoverageEnable, info.alphaToOneEnable);
    }
};

template<>
struct hash<snap::rhi::VertexInputAttributeDescription> {
    size_t operator()(const snap::rhi::VertexInputAttributeDescription& info) const noexcept {
        return snap::rhi::common::hash_combine(info.location, info.binding, info.offset, info.format);
    }
};

template<>
struct hash<snap::rhi::VertexInputBindingDescription> {
    size_t operator()(const snap::rhi::VertexInputBindingDescription& info) const noexcept {
        return snap::rhi::common::hash_combine(info.binding, info.divisor, info.stride, info.inputRate);
    }
};

template<>
struct hash<snap::rhi::InputAssemblyStateCreateInfo> {
    size_t operator()(const snap::rhi::InputAssemblyStateCreateInfo& info) const noexcept {
        return snap::rhi::common::hash_combine(0, info.primitiveTopology);
    }
};

template<>
struct hash<snap::rhi::VertexInputStateCreateInfo> {
    size_t operator()(const snap::rhi::VertexInputStateCreateInfo& info) const noexcept {
        size_t result = snap::rhi::common::hash_combine(info.bindingsCount, info.attributesCount);
        for (uint32_t i = 0; i < info.attributesCount; ++i) {
            result = snap::rhi::common::hash_combine(
                result, std::hash<snap::rhi::VertexInputAttributeDescription>()(info.attributeDescription[i]));
        }

        for (uint32_t i = 0; i < info.bindingsCount; ++i) {
            result = snap::rhi::common::hash_combine(
                result, std::hash<snap::rhi::VertexInputBindingDescription>()(info.bindingDescription[i]));
        }
        return result;
    }
};

template<>
struct hash<snap::rhi::AttachmentFormatsCreateInfo> {
    size_t operator()(const snap::rhi::AttachmentFormatsCreateInfo& info) const noexcept {
        size_t result = snap::rhi::common::hash_combine(info.depthAttachmentFormat, info.stencilAttachmentFormat);
        for (uint32_t i = 0; i < info.colorAttachmentFormats.size(); ++i) {
            result = snap::rhi::common::hash_combine(result, info.colorAttachmentFormats[i]);
        }
        return result;
    }
};

template<>
struct hash<snap::rhi::RenderPipelineCreateInfo> {
    size_t operator()(const snap::rhi::RenderPipelineCreateInfo& info) const noexcept {
        size_t result = 0;
        for (auto stage : info.stages) {
            result = snap::rhi::common::hash_combine(result, stage);
        }

        return snap::rhi::common::hash_combine(
            result,
            static_cast<uint32_t>(info.pipelineCreateFlags),
            std::hash<snap::rhi::VertexInputStateCreateInfo>()(info.vertexInputState),
            std::hash<snap::rhi::InputAssemblyStateCreateInfo>()(info.inputAssemblyState),
            std::hash<snap::rhi::RasterizationStateCreateInfo>()(info.rasterizationState),
            std::hash<snap::rhi::MultisampleStateCreateInfo>()(info.multisampleState),
            std::hash<snap::rhi::DepthStencilStateCreateInfo>()(info.depthStencilState),
            std::hash<snap::rhi::ColorBlendStateCreateInfo>()(info.colorBlendState),
            info.renderPass,
            info.subpass,
            info.basePipeline,
            info.pipelineCache);
    }
};
} // namespace std
