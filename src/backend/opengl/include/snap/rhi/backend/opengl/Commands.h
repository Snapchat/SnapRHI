//
//  Commands.h
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 1/17/20.
//  Copyright © 2020 Snapchat. All rights reserved.
//

#pragma once

#include "snap/rhi/ClearValue.h"
#include "snap/rhi/CopyInfo.h"
#include "snap/rhi/Enums.h"
#include "snap/rhi/Limits.h"
#include "snap/rhi/Structs.h"

#include <array>
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

namespace snap::rhi {
class RenderPipeline;
class Framebuffer;
class DescriptorSet;
class Texture;
class Buffer;
class Sampler;
class ComputePipeline;
} // namespace snap::rhi

namespace snap::rhi::backend::opengl {

static constexpr uint32_t MaxCopyInfos = 8u;
static constexpr uint32_t MaxBarrierInfos = 8u;

enum class Command : uint32_t {
    Unknown = 0,

    // common commands
    BindDescriptorSet,
    PipelineBarrier,

    // compute commands
    BeginComputePass,
    BindComputePipeline,
    Dispatch,
    // DispatchIndirect,
    EndComputePass,

    // render commands
    BeginRenderPass,
    BeginRenderPass1,
    SetRenderPipeline,
    SetViewport,
    SetVertexBuffers,
    SetVertexBuffer,
    SetIndexBuffer,
    SetDepthBias,
    SetStencilReference,
    SetBlendConstants,
    InvokeCustomCallback,
    Draw,
    DrawIndexed,
    EndRenderPass,

    // Transfer commands
    BeginBlitPass,
    CopyBufferToBuffer,
    CopyBufferToTexture,
    CopyTextureToBuffer,
    CopyTextureToTexture,
    GenerateMipmap,
    EndBlitPass,

    // debug commands
    BeginDebugGroup,
    EndDebugGroup,
};

/**
 * These structures are intended to be allocated via placement new into a
 * pre-allocated memory block and deallocated by freeing that block directly.
 *
 * @warning **CRITICAL RESTRICTION:**
 * These structures must remain **Trivially Destructible**.
 * The command allocator for these types DOES NOT call the destructor
 * when reclaiming memory. It simply calls `free()` or `delete[]` on the
 * underlying bytes.
 *
 * @details
 * If you add any member that requires cleanup (e.g., `std::string`, `std::vector`,
 * or a smart pointer), you will cause a resource leak.
 *
 * - **Safe members:** `int`, `float`, `char[]`, pointers (if strict ownership is not implied), other trivial structs.
 * - **Unsafe members:** `std::string`, `std::vector`, `std::unique_ptr`, custom classes with destructors.
 *
 * @see std::is_trivially_destructible
 */
#define SNAP_RHI_BACKEND_OPENGL_COMMANDS_TRIVIALLY_DESTRUCTIBLE_CHECK(Type)                                            \
    static_assert(std::is_trivially_destructible_v<Type>,                                                              \
                  #Type " must be trivially destructible! Do not add members that require cleanup.");

struct BeginRenderPassCmd {
    snap::rhi::Framebuffer* framebuffer = nullptr;
    snap::rhi::RenderPass* renderPass = nullptr;
    std::array<snap::rhi::ClearValue, snap::rhi::MaxAttachments> clearValues{};
    uint32_t clearValueCount = 0;
};
SNAP_RHI_BACKEND_OPENGL_COMMANDS_TRIVIALLY_DESTRUCTIBLE_CHECK(BeginRenderPassCmd);

struct BeginRenderPass1Cmd {
    uint32_t layers = 1;
    uint32_t viewMask = 0;
    std::array<RenderingAttachmentInfo, snap::rhi::MaxColorAttachments> colorAttachments{};
    uint32_t colorAttachmentCount = 0;
    RenderingAttachmentInfo depthAttachment;
    RenderingAttachmentInfo stencilAttachment;
};
SNAP_RHI_BACKEND_OPENGL_COMMANDS_TRIVIALLY_DESTRUCTIBLE_CHECK(BeginRenderPass1Cmd);

using SetViewportCmd = snap::rhi::Viewport;
SNAP_RHI_BACKEND_OPENGL_COMMANDS_TRIVIALLY_DESTRUCTIBLE_CHECK(SetViewportCmd);

struct SetRenderPipelineCmd {
    snap::rhi::RenderPipeline* pipeline = nullptr;
};
SNAP_RHI_BACKEND_OPENGL_COMMANDS_TRIVIALLY_DESTRUCTIBLE_CHECK(SetRenderPipelineCmd);

struct SetDepthBiasCmd {
    float depthBiasConstantFactor = 0.0f;
    float depthBiasSlopeFactor = 0.0f;
    float depthBiasClamp = 0.0f;
};
SNAP_RHI_BACKEND_OPENGL_COMMANDS_TRIVIALLY_DESTRUCTIBLE_CHECK(SetDepthBiasCmd);

struct BindDescriptorSetCmd {
    uint32_t binding = 0;
    snap::rhi::DescriptorSet* descriptorSet = nullptr;
    std::array<uint32_t, snap::rhi::MaxDynamicOffsetsPerDescriptorSet> dynamicOffsets{};
    uint32_t dynamicOffsetCount = 0;
};
SNAP_RHI_BACKEND_OPENGL_COMMANDS_TRIVIALLY_DESTRUCTIBLE_CHECK(BindDescriptorSetCmd);

struct BeginComputePassCmd {};
SNAP_RHI_BACKEND_OPENGL_COMMANDS_TRIVIALLY_DESTRUCTIBLE_CHECK(BeginComputePassCmd);

struct BindComputePipelineCmd {
    snap::rhi::ComputePipeline* pipeline = nullptr;
};
SNAP_RHI_BACKEND_OPENGL_COMMANDS_TRIVIALLY_DESTRUCTIBLE_CHECK(BindComputePipelineCmd);

struct DispatchCmd {
    uint32_t groupSizeX = 0;
    uint32_t groupSizeY = 0;
    uint32_t groupSizeZ = 0;
};
SNAP_RHI_BACKEND_OPENGL_COMMANDS_TRIVIALLY_DESTRUCTIBLE_CHECK(DispatchCmd);

struct EndComputePassCmd {};
SNAP_RHI_BACKEND_OPENGL_COMMANDS_TRIVIALLY_DESTRUCTIBLE_CHECK(EndComputePassCmd);

struct PipelineBarrierCmd {
    snap::rhi::PipelineStageBits srcStageMask = snap::rhi::PipelineStageBits::None;
    snap::rhi::PipelineStageBits dstStageMask = snap::rhi::PipelineStageBits::None;
    snap::rhi::DependencyFlags dependencyFlags = snap::rhi::DependencyFlags::ByRegion;

    std::array<snap::rhi::MemoryBarrierInfo, MaxBarrierInfos> memoryBarriers{};
    uint32_t memoryBarriersCount = 0;

    std::array<snap::rhi::BufferMemoryBarrierInfo, MaxBarrierInfos> bufferMemoryBarriers{};
    uint32_t bufferMemoryBarriersCount = 0;

    std::array<snap::rhi::TextureMemoryBarrierInfo, MaxBarrierInfos> textureMemoryBarriers{};
    uint32_t textureMemoryBarriersCount = 0;
};
SNAP_RHI_BACKEND_OPENGL_COMMANDS_TRIVIALLY_DESTRUCTIBLE_CHECK(PipelineBarrierCmd);

struct SetVertexBuffersCmd {
    uint32_t firstBinding = 0;
    uint32_t vertexBuffersCount = 0;
    std::array<snap::rhi::Buffer*, snap::rhi::MaxVertexBuffers> vertexBuffers{};
    std::array<uint32_t, snap::rhi::MaxVertexBuffers> offsets{};
};
SNAP_RHI_BACKEND_OPENGL_COMMANDS_TRIVIALLY_DESTRUCTIBLE_CHECK(SetVertexBuffersCmd);

struct SetVertexBufferCmd {
    uint32_t binding = 0;
    snap::rhi::Buffer* vertexBuffer = nullptr;
    uint32_t offset = 0;
};
SNAP_RHI_BACKEND_OPENGL_COMMANDS_TRIVIALLY_DESTRUCTIBLE_CHECK(SetVertexBufferCmd);

struct SetIndexBufferCmd {
    snap::rhi::Buffer* indexBuffer = nullptr;
    uint32_t offset = 0;
    IndexType indexType = IndexType::None;
};
SNAP_RHI_BACKEND_OPENGL_COMMANDS_TRIVIALLY_DESTRUCTIBLE_CHECK(SetIndexBufferCmd);

struct DrawCmd {
    uint32_t firstVertex = 0;
    uint32_t vertexCount = 0;
    uint32_t instanceCount = 1;
};
SNAP_RHI_BACKEND_OPENGL_COMMANDS_TRIVIALLY_DESTRUCTIBLE_CHECK(DrawCmd);

struct DrawIndexedCmd {
    uint32_t firstIndex = 0;
    uint32_t indexCount = 0;
    uint32_t instanceCount = 1;
};
SNAP_RHI_BACKEND_OPENGL_COMMANDS_TRIVIALLY_DESTRUCTIBLE_CHECK(DrawIndexedCmd);

struct SetStencilReferenceCmd {
    StencilFace face = StencilFace::FrontAndBack;
    uint32_t reference = 0;
};
SNAP_RHI_BACKEND_OPENGL_COMMANDS_TRIVIALLY_DESTRUCTIBLE_CHECK(SetStencilReferenceCmd);

struct SetBlendConstantsCmd {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 0.0f;
};
SNAP_RHI_BACKEND_OPENGL_COMMANDS_TRIVIALLY_DESTRUCTIBLE_CHECK(SetBlendConstantsCmd);

struct InvokeCustomCallbackCmd {
    std::function<void()>* callback = nullptr;
};
SNAP_RHI_BACKEND_OPENGL_COMMANDS_TRIVIALLY_DESTRUCTIBLE_CHECK(InvokeCustomCallbackCmd);

struct EndRenderPassCmd {};
SNAP_RHI_BACKEND_OPENGL_COMMANDS_TRIVIALLY_DESTRUCTIBLE_CHECK(EndRenderPassCmd);

struct BeginBlitPassCmd {};
SNAP_RHI_BACKEND_OPENGL_COMMANDS_TRIVIALLY_DESTRUCTIBLE_CHECK(BeginBlitPassCmd);

struct CopyBufferToBufferCmd {
    snap::rhi::Buffer* srcBuffer = nullptr;
    snap::rhi::Buffer* dstBuffer = nullptr;

    std::array<BufferCopy, MaxCopyInfos> infos{};
    uint32_t infoCount = 0;
};
SNAP_RHI_BACKEND_OPENGL_COMMANDS_TRIVIALLY_DESTRUCTIBLE_CHECK(CopyBufferToBufferCmd);

struct CopyBufferToTextureCmd {
    snap::rhi::Buffer* srcBuffer = nullptr;
    snap::rhi::Texture* dstTexture = nullptr;

    std::array<BufferTextureCopy, MaxCopyInfos> infos{};
    uint32_t infoCount = 0;
};
SNAP_RHI_BACKEND_OPENGL_COMMANDS_TRIVIALLY_DESTRUCTIBLE_CHECK(CopyBufferToTextureCmd);

struct CopyTextureToBufferCmd {
    snap::rhi::Texture* srcTexture = nullptr;
    snap::rhi::Buffer* dstBuffer = nullptr;

    std::array<BufferTextureCopy, MaxCopyInfos> infos;
    uint32_t infoCount = 0;
};
SNAP_RHI_BACKEND_OPENGL_COMMANDS_TRIVIALLY_DESTRUCTIBLE_CHECK(CopyTextureToBufferCmd);

struct CopyTextureToTextureCmd {
    snap::rhi::Texture* srcTexture = nullptr;
    snap::rhi::Texture* dstTexture = nullptr;

    std::array<TextureCopy, MaxCopyInfos> infos;
    uint32_t infoCount = 0;
};
SNAP_RHI_BACKEND_OPENGL_COMMANDS_TRIVIALLY_DESTRUCTIBLE_CHECK(CopyTextureToTextureCmd);

struct GenerateMipmapCmd {
    snap::rhi::Texture* texture = nullptr;
};
SNAP_RHI_BACKEND_OPENGL_COMMANDS_TRIVIALLY_DESTRUCTIBLE_CHECK(GenerateMipmapCmd);

struct EndDebugGroupCmd {};
SNAP_RHI_BACKEND_OPENGL_COMMANDS_TRIVIALLY_DESTRUCTIBLE_CHECK(EndDebugGroupCmd);

struct BeginDebugGroupCmd {
    std::array<char, snap::rhi::MaxDebugMarkerNameSize> labelBuffer{};
    uint32_t labelSize = 0;
};
SNAP_RHI_BACKEND_OPENGL_COMMANDS_TRIVIALLY_DESTRUCTIBLE_CHECK(BeginDebugGroupCmd);

struct EndBlitPassCmd {};
SNAP_RHI_BACKEND_OPENGL_COMMANDS_TRIVIALLY_DESTRUCTIBLE_CHECK(EndBlitPassCmd);
} // namespace snap::rhi::backend::opengl
