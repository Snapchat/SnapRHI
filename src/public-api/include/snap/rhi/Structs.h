#pragma once

#include "snap/rhi/ClearValue.h"
#include "snap/rhi/DeviceChild.hpp"
#include "snap/rhi/Enums.h"
#include "snap/rhi/Limits.h"
#include "snap/rhi/PixelFormat.h"

#include <compare>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace snap::rhi {
class Buffer;
class Texture;
class RenderPass;
class Framebuffer;

/**
 * @brief 2D integer offset.
 */
struct Offset2D {
    int32_t x = 0;
    int32_t y = 0;

    constexpr friend auto operator<=>(const Offset2D&, const Offset2D&) noexcept = default;
};

/**
 * @brief 2D integer extent (width/height).
 */
struct Extent2D {
    uint32_t width = 0;
    uint32_t height = 0;

    constexpr friend auto operator<=>(const Extent2D&, const Extent2D&) noexcept = default;
};

/**
 * @brief 2D rectangle defined by an offset and extent.
 */
struct Rect2D {
    Offset2D offset{};
    Extent2D extent{};

    constexpr friend auto operator<=>(const Rect2D&, const Rect2D&) noexcept = default;
};

/**
 * @brief Viewport definition used for rasterization.
 *
 * The viewport is specified in pixel coordinates with a depth range.
 *
 * Backend notes:
 * - Vulkan/Metal consume this as a native viewport.
 * - Some backends may also set a matching scissor rectangle when the viewport is set.
 */
struct Viewport {
    /**
     * @brief X coordinate of the viewport origin.
     */
    uint32_t x = 0;

    /**
     * @brief Y coordinate of the viewport origin.
     */
    uint32_t y = 0;

    /**
     * @brief Width of the viewport in pixels.
     */
    uint32_t width = 0;

    /**
     * @brief Height of the viewport in pixels.
     */
    uint32_t height = 0;

    /**
     * @brief Depth value of the near clipping plane.
     */
    float znear = 0.0f;

    /**
     * @brief Depth value of the far clipping plane.
     */
    float zfar = 1.0f;

    // The names "near" and "far" cannot be used because these names are used in the "Windows.h"
    // https://stackoverflow.com/questions/8948493/visual-studio-doesnt-allow-me-to-use-certain-variable-names
};

/**
 * @brief 3D integer offset.
 */
struct Offset3D {
    int32_t x = 0;
    int32_t y = 0;
    int32_t z = 0;

    constexpr friend auto operator<=>(const Offset3D&, const Offset3D&) noexcept = default;
};

/**
 * @brief 3D extent (width/height/depth).
 */
struct Extent3D {
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t depth = 0;

    constexpr friend auto operator<=>(const Extent3D&, const Extent3D&) noexcept = default;
};

/// @brief Represents the memory footprint of a single allocated resource.
struct ResourceMemoryEntry {
    /// @brief The size of the resource in bytes.
    uint64_t sizeInBytes = 0;

    /// @brief A weak reference to the resource (prevents keeping the resource alive just for reporting).
    std::weak_ptr<snap::rhi::DeviceChild> resource;
};

/// @brief Aggregates memory usage for a specific category of resources (e.g., all Textures).
struct ResourceTypeGroup {
    /// @brief The specific type of resources in this group.
    ResourceType type = ResourceType::Undefined;

    /// @brief The total memory used by all resources of this type in bytes.
    uint64_t totalSizeInBytes = 0;

    /// @brief List of individual resource allocations contributing to this total.
    /// @note This was changed from 'ResourceTypeMemoryUsageInfo' to prevent invalid recursion.
    std::vector<ResourceMemoryEntry> entries;
};

/// @brief Represents the memory usage of a specific memory domain (e.g., CPU or GPU).
struct MemoryDomainUsage {
    /// @brief The total aggregated memory usage for this domain in bytes.
    uint64_t totalSizeInBytes = 0;

    /// @brief The total count of resources allocated in this domain.
    uint64_t totalResourceCount = 0;

    /// @brief Breakdown of memory usage by resource type.
    std::vector<ResourceTypeGroup> groups;
};

/// @brief A complete snapshot of the device's memory state at a specific point in time.
struct DeviceMemorySnapshot {
    /// @brief Memory resident on the host (system RAM).
    MemoryDomainUsage cpu;

    /// @brief Memory resident on the device (VRAM).
    MemoryDomainUsage gpu;
};

/**
 * @brief Buffer memory barrier description.
 *
 * Describes a dependency between prior and subsequent accesses to a buffer.
 */
struct BufferMemoryBarrierInfo {
    /// Access mask for operations that must happen-before the barrier.
    AccessFlags srcAccessMask = AccessFlags::None;
    /// Access mask for operations that must happen-after the barrier.
    AccessFlags dstAccessMask = AccessFlags::None;

    /// Target buffer.
    snap::rhi::Buffer* buffer = nullptr;

    /// Byte offset into @ref buffer.
    uint64_t offset = 0;
    /// Byte size of the affected range.
    uint64_t size = 0;

    constexpr friend auto operator<=>(const BufferMemoryBarrierInfo&,
                                      const BufferMemoryBarrierInfo&) noexcept = default;
};

/**
 * @brief Subresource range for a texture.
 */
struct TextureSubresourceRange {
    uint32_t baseMipLevel = 0;
    uint32_t levelCount = 1;

    uint32_t baseArrayLayer = 0;
    uint32_t layerCount = 1;

    constexpr friend auto operator<=>(const TextureSubresourceRange&,
                                      const TextureSubresourceRange&) noexcept = default;
};

/**
 * @brief Global memory barrier description.
 */
struct MemoryBarrierInfo {
    AccessFlags srcAccessMask = AccessFlags::None;
    AccessFlags dstAccessMask = AccessFlags::None;

    constexpr friend auto operator<=>(const MemoryBarrierInfo&, const MemoryBarrierInfo&) noexcept = default;
};

/**
 * @brief Texture memory barrier description.
 *
 * Describes a dependency between prior and subsequent accesses to a texture and a subresource range.
 */
struct TextureMemoryBarrierInfo {
    AccessFlags srcAccessMask = AccessFlags::None;
    AccessFlags dstAccessMask = AccessFlags::None;

    snap::rhi::Texture* texture = nullptr;

    TextureSubresourceRange subresourceRange{};

    constexpr friend auto operator<=>(const TextureMemoryBarrierInfo&,
                                      const TextureMemoryBarrierInfo&) noexcept = default;
};

/**
 * @brief Description of implicit MSAA resolve behavior for an attachment.
 */
struct AutoresolvedAttachmentDescription {
    /**
     * @brief Sample count used by the implicit resolve path.
     */
    snap::rhi::SampleCount autoresolvedSamples = snap::rhi::SampleCount::Undefined;

    constexpr friend auto operator<=>(const AutoresolvedAttachmentDescription&,
                                      const AutoresolvedAttachmentDescription&) noexcept = default;
};

/**
 * @brief View into a texture used as an attachment.
 *
 * This is an API-agnostic representation of a single mip level and array layer (slice).
 */
struct TextureView {
    /// Texture being referenced.
    snap::rhi::Texture* texture = nullptr;

    /// Mipmap level used for the attachment.
    uint32_t mipLevel = 0;

    /// Array layer (slice) used for the attachment.
    uint32_t layer = 0;

    constexpr friend auto operator<=>(const TextureView&, const TextureView&) noexcept = default;
};

/**
 * @brief Attachment description for dynamic rendering.
 */
struct RenderingAttachmentInfo {
    /// Primary attachment view.
    TextureView attachment;

    /// Resolve target (if MSAA resolve is requested).
    TextureView resolveAttachment;

    /// Load operation for the attachment.
    snap::rhi::AttachmentLoadOp loadOp = snap::rhi::AttachmentLoadOp::DontCare;
    /// Store operation for the attachment.
    snap::rhi::AttachmentStoreOp storeOp = snap::rhi::AttachmentStoreOp::DontCare;

    /// Clear value used when @ref loadOp is Clear.
    ClearValue clearValue;

    /// Optional implicit resolve configuration.
    std::optional<AutoresolvedAttachmentDescription> autoresolvedAttachment;
};

/**
 * @brief Dynamic rendering parameters.
 *
 * This structure is consumed by `RenderCommandEncoder::beginEncoding(const RenderingInfo&)` when using dynamic
 * rendering.
 */
struct RenderingInfo {
    /**
     * @brief Number of array layers rendered.
     *
     * When `viewMask != 0`, multiview rendering is enabled and the interpretation of @layers may differ per backend.
     */
    uint32_t layers = 1;

    /**
     * @brief Bitmask selecting multiview views.
     *
     * @note The viewMask must have contiguous bits set. The first bit set is the first view rendered and
     * the number of bits set is the number of views rendered.
     */
    uint32_t viewMask = 0;

    /// Color attachments for this rendering scope.
    std::vector<RenderingAttachmentInfo> colorAttachments;
    /// Depth attachment (if used).
    RenderingAttachmentInfo depthAttachment;
    /// Stencil attachment (if used).
    RenderingAttachmentInfo stencilAttachment;
};

/**
 * @brief Parameters to start a render pass backed by a `snap::rhi::RenderPass` / `snap::rhi::Framebuffer`.
 */
struct RenderPassBeginInfo {
    /// Render pass that describes attachments/subpasses.
    RenderPass* renderPass = nullptr;
    /// Framebuffer providing the concrete attachment images.
    Framebuffer* framebuffer = nullptr;

    /**
     * @brief Clear values for attachments.
     *
     * The number and order of values must match the attachments of @ref renderPass.
     */
    std::vector<ClearValue> clearValues;
};

/**
 * @brief Debug message payload used by debug callbacks.
 */
struct DebugCallbackInfo {
    std::string message;
};

/**
 * @brief Query parameters for texture format properties.
 */
struct TextureFormatInfo {
    snap::rhi::PixelFormat format = snap::rhi::PixelFormat::Undefined;
    snap::rhi::TextureType type = snap::rhi::TextureType::Texture2D;
    snap::rhi::TextureUsage usage = snap::rhi::TextureUsage::None;
};

/**
 * @brief Format capability information returned by `Device::getTextureFormatProperties()`.
 */
struct TextureFormatProperties {
    snap::rhi::Extent3D maxExtent{0, 0, 0};
    uint32_t maxMipLevels = 0;
    uint32_t maxArrayLayers = 0;
    snap::rhi::SampleCount sampleCounts = snap::rhi::SampleCount::Undefined;
    uint64_t maxResourceSize = 0;
};
} // namespace snap::rhi
