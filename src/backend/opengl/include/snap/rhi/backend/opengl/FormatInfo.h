//
//  FormatInfo.h
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 6/27/22.
//

#pragma once

#include "snap/rhi/backend/opengl/Format.h"
#include "snap/rhi/backend/opengl/FramebufferAttachmentTarget.h"
#include "snap/rhi/backend/opengl/FramebufferStatus.h"
#include "snap/rhi/backend/opengl/FramebufferTarget.h"
#include "snap/rhi/backend/opengl/OpenGL.h"
#include "snap/rhi/backend/opengl/TextureTarget.h"

#include "snap/rhi/PixelFormat.h"

namespace snap::rhi::backend::opengl {
struct RenderbufferFormatInfo {
    snap::rhi::backend::opengl::SizedInternalFormat internalFormat =
        snap::rhi::backend::opengl::SizedInternalFormat::UNKNOWN;
};

enum class FormatFilteringType : uint8_t {
    None = 0,

    NearestOnly,
    NearestAndLinear,
    NearestAndLinearWithMSAA,
};

enum class FormatFeatures : uint8_t {
    None = 0,

    Uploadable = 1 << 0, // pixel unpack buffer / raw data array => texture
    Readable = 1 << 1,   // texture => pixel pack buffer / raw data pointer
    Copyable = 1 << 2,   // glCopyTexImage2D / glBlitFramebuffer
    Renderable = 1 << 3, // color or depth/stencil rendering
    Storage = 1 << 4,    // shader storage image

    AllNonStorage = Uploadable | Readable | Copyable | Renderable,

    All = AllNonStorage | Storage
};
SNAP_RHI_DEFINE_ENUM_OPS(FormatFeatures);

struct FormatOpInfo {
    FormatFeatures features = FormatFeatures::None;
    FormatFilteringType formatFilteringType = FormatFilteringType::None;

    bool isRenderable() const {
        return (features & FormatFeatures::Renderable) == FormatFeatures::Renderable;
    }

    bool isUploadable() const {
        return (features & FormatFeatures::Uploadable) == FormatFeatures::Uploadable;
    }

    bool isReadable() const {
        return (features & FormatFeatures::Readable) == FormatFeatures::Readable;
    }

    bool isStorage() const {
        return (features & FormatFeatures::Storage) == FormatFeatures::Storage;
    }

    bool isCopyable() const {
        return (features & FormatFeatures::Copyable) == FormatFeatures::Copyable;
    }
};

static_assert(sizeof(snap::rhi::backend::opengl::SizedInternalFormat) == sizeof(GLenum));
static_assert(sizeof(snap::rhi::backend::opengl::FormatGroup) == sizeof(GLenum));
static_assert(sizeof(snap::rhi::backend::opengl::FormatDataType) == sizeof(GLenum));
static_assert(sizeof(snap::rhi::backend::opengl::FramebufferAttachmentTarget) == sizeof(GLenum));
static_assert(sizeof(snap::rhi::backend::opengl::FramebufferTarget) == sizeof(GLenum));
static_assert(sizeof(snap::rhi::backend::opengl::TextureTarget) == sizeof(GLenum));
static_assert(sizeof(snap::rhi::backend::opengl::TextureId) == sizeof(GLuint));
static_assert(sizeof(snap::rhi::backend::opengl::FramebufferId) == sizeof(GLuint));

} // namespace snap::rhi::backend::opengl
