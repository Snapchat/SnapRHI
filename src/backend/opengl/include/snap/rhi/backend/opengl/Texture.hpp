//
//  Texture.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 21.10.2021.
//

#pragma once

#include "snap/rhi/Texture.hpp"
#include "snap/rhi/backend/common/ValidationLayer.hpp"
#include "snap/rhi/backend/opengl/Format.h"
#include "snap/rhi/backend/opengl/GLObject.h"
#include "snap/rhi/backend/opengl/OpenGL.h"
#include "snap/rhi/backend/opengl/TextureTarget.h"

#if SNAP_RHI_PLATFORM_WEBASSEMBLY()
namespace emscripten {
class val;
} // namespace emscripten
#endif

namespace snap::rhi::backend::opengl {
class Profile;
class Device;
class DeviceContext;

using TextureUUID = uint64_t;
constexpr TextureUUID UndefinedTextureUUID = 0;

class Texture : public snap::rhi::Texture {
public:
    explicit Texture(snap::rhi::backend::opengl::Device* device,
                     const TextureCreateInfo& info,
                     TextureUUID textureUUID);
    explicit Texture(snap::rhi::backend::opengl::Device* device,
                     const std::shared_ptr<snap::rhi::TextureInterop>& textureInterop,
                     TextureUUID textureUUID);
    explicit Texture(snap::rhi::backend::opengl::Device* device,
                     const TextureCreateInfo& info,
                     const snap::rhi::backend::opengl::TextureId textureID,
                     const snap::rhi::backend::opengl::TextureTarget target,
                     const bool isTextureOwner,
                     TextureUUID textureUUID);
    ~Texture() override;

    void setDebugLabel(std::string_view label) override;

    TextureId getTextureID(DeviceContext* dc) const;
    TextureTarget getTarget() const noexcept;
    snap::rhi::backend::opengl::SizedInternalFormat getInternalFormat() const noexcept;

    void upload(const Offset3D& offset,
                const Extent3D& size,
                const uint32_t mipmapLevel,
                const uint8_t* pixels,
                const uint32_t unpackRowLength,
                const uint32_t unpackImageHeight,
                DeviceContext* dc);

#if SNAP_RHI_PLATFORM_WEBASSEMBLY()
    void upload(const Offset3D& offset,
                const Extent3D& size,
                const uint32_t mipmapLevel,
                const emscripten::val& pixels,
                const uint32_t unpackRowLength,
                const uint32_t unpackImageHeight,
                DeviceContext* dc);
#endif

    void updateTextureUUID() const;

    TextureUUID getTextureUUID() const;

private:
    void tryAllocate(DeviceContext* dc);
    void destroyGLTexture();

    const Profile& gl;
    const snap::rhi::backend::common::ValidationLayer& validationLayer;

    mutable snap::rhi::backend::opengl::SizedInternalFormat internalFormat =
        snap::rhi::backend::opengl::SizedInternalFormat::UNKNOWN;
    mutable TextureId texture = TextureId::Null;
    mutable TextureTarget target = TextureTarget::Detached;
    const bool isTextureOwner = true;

    mutable TextureUUID textureUUID = UndefinedTextureUUID;
    GLObject glObject{};
};
} // namespace snap::rhi::backend::opengl
