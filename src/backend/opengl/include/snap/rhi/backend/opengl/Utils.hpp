#pragma once

#include "snap/rhi/FramebufferCreateInfo.h"
#include "snap/rhi/Texture.hpp"
#include "snap/rhi/TextureViewCreateInfo.h"
#include "snap/rhi/backend/opengl/Commands.h"
#include "snap/rhi/backend/opengl/FramebufferDescription.h"
#include "snap/rhi/backend/opengl/OpenGL.h"

#include <snap/rhi/common/Scope.h>
#include <string_view>

#include <span>
#include <string>

namespace snap::rhi::backend::opengl {
class DeviceContext;
class Profile;
class Texture;
class Buffer;
class RenderPipeline;

bool isSystemResource(const std::string& name);

snap::rhi::SampleCount getFramebufferAttachmentSampleCount(const snap::rhi::TextureCreateInfo& createInfo,
                                                           const snap::rhi::AttachmentDescription& attDesc) noexcept;
snap::rhi::SampleCount getFramebufferAttachmentSampleCount(const snap::rhi::TextureCreateInfo& createInfo,
                                                           snap::rhi::SampleCount numAttachmentSamples) noexcept;

snap::rhi::backend::opengl::FramebufferAttachment buildFramebufferAttachment(
    DeviceContext* dc,
    const snap::rhi::backend::opengl::Texture* texture,
    const snap::rhi::TextureSubresourceRange& subresRange,
    snap::rhi::SampleCount numAttachmentSamples = snap::rhi::SampleCount::Undefined,
    const uint32_t viewMask = 0) noexcept;

snap::rhi::backend::opengl::FramebufferDescription buildFBODesc(DeviceContext* dc,
                                                                const snap::rhi::TextureSubresourceRange& subresRange,
                                                                const snap::rhi::Texture* fbAtt,
                                                                const snap::rhi::Texture* fbColorAtt);

void computeUnpackImageSize(const uint32_t width,
                            const uint32_t height,
                            const snap::rhi::PixelFormat format,
                            const uint32_t srcBytesPerRow,
                            const uint64_t srcBytesPerSlice,
                            uint32_t& bytesPerRow,
                            uint64_t& bytesPerSlice,
                            uint32_t& unpackRowLength,
                            uint32_t& unpackImageHeight);
void blitTextures(DeviceContext* dc,
                  snap::rhi::backend::opengl::RenderPipeline* renderPipeline,
                  const snap::rhi::Texture* srcTexture,
                  snap::rhi::Offset3D srcOrigin,
                  const snap::rhi::TextureSubresourceRange& srcDesc,
                  const snap::rhi::Texture* dstTexture,
                  snap::rhi::Offset3D dstOrigin,
                  const snap::rhi::TextureSubresourceRange& dstDesc,
                  snap::rhi::Extent3D srcExtent,
                  snap::rhi::Extent3D destExtent,
                  uint32_t mask,
                  // If blitting a depth texture (for msaa), we must provide a color texture for the fbo or the fbo will
                  // be considered incomplete for compatibility OpenGL
                  const snap::rhi::Texture* srcColorTexture = nullptr,
                  const snap::rhi::Texture* destColorTexture = nullptr,
                  uint32_t filter = GL_NEAREST);

void blitTextures(DeviceContext* dc,
                  snap::rhi::backend::opengl::RenderPipeline* renderPipeline,
                  const snap::rhi::Texture* srcTexture,
                  const snap::rhi::TextureSubresourceRange& srcDesc,
                  const snap::rhi::Texture* dstTexture,
                  const snap::rhi::TextureSubresourceRange& dstDesc,
                  uint32_t srcWidth,
                  uint32_t srcHeight,
                  uint32_t destWidth,
                  uint32_t destHeight,
                  uint32_t mask,
                  // If blitting a depth texture (for msaa), we must provide a color texture for the fbo or the fbo will
                  // be considered incomplete for compatibility OpenGL
                  const snap::rhi::Texture* srcColorTexture = nullptr,
                  const snap::rhi::Texture* destColorTexture = nullptr,
                  uint32_t filter = GL_NEAREST);

void blitTextures(DeviceContext* dc,
                  snap::rhi::backend::opengl::RenderPipeline* renderPipeline,
                  const snap::rhi::Texture* srcTexture,
                  const snap::rhi::AttachmentDescription& srcDesc,
                  const snap::rhi::Texture* dstTexture,
                  const snap::rhi::AttachmentDescription& dstDesc,
                  uint32_t srcWidth,
                  uint32_t srcHeight,
                  uint32_t destWidth,
                  uint32_t destHeight,
                  uint32_t mask,
                  // If blitting a depth texture (for msaa), we must provide a color texture for the fbo or the fbo will
                  // be considered incomplete for compatibility OpenGL
                  const snap::rhi::Texture* srcColorTexture = nullptr,
                  const snap::rhi::Texture* destColorTexture = nullptr,
                  uint32_t filter = GL_NEAREST);

void blitTextures(DeviceContext* dc,
                  snap::rhi::backend::opengl::RenderPipeline* renderPipeline,
                  const snap::rhi::TextureView& srcTexture,
                  const snap::rhi::TextureView& dstTexture,
                  uint32_t mask,
                  // If blitting a depth texture (for msaa), we must provide a color texture for the fbo or the fbo will
                  // be considered incomplete for compatibility OpenGL
                  const snap::rhi::Texture* srcColorTexture = nullptr,
                  const snap::rhi::Texture* destColorTexture = nullptr,
                  uint32_t filter = GL_NEAREST);

void restoreAttachmentsMask(DeviceContext* dc,
                            snap::rhi::backend::opengl::RenderPipeline* renderPipeline,
                            GLbitfield& clearMask);

uint64_t computeStagingDataSize(std::span<const snap::rhi::BufferTextureCopy> infos,
                                const snap::rhi::TextureCreateInfo& textureInfo,
                                const bool isCustomTexturePackingSupported);

void uploadBufferToTexture(snap::rhi::backend::opengl::Buffer* srcBuffer,
                           snap::rhi::backend::opengl::Texture* dstTexture,
                           std::span<const snap::rhi::BufferTextureCopy> infos,
                           snap::rhi::backend::opengl::Profile& gl,
                           const snap::rhi::backend::common::ValidationLayer& validationLayer,
                           snap::rhi::backend::opengl::DeviceContext* dc);

void barrier(snap::rhi::backend::opengl::Profile& gl, const snap::rhi::backend::opengl::PipelineBarrierCmd& cmd);

/// Try to find a GL constant name matching the provided int value or return a user provided fallback (empty by default)
/// \note Only available in debug builds, otherwise always uses the fallback string view.
/// \note This implementation is unaware of context (e.g., for what function we are picking a constant); so in collision
/// cases we should either choose a more frequently used constant or return a list of constants matching the int value.
std::string_view tryFindGLConstantName(int64_t value, std::string_view fallback = "") noexcept;

} // namespace snap::rhi::backend::opengl
