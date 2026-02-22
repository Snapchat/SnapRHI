#pragma once

#include "snap/rhi/backend/common/TextureInterop.h"
#include "snap/rhi/common/OS.h"
#include <snap/rhi/backend/opengl/OpenGL.h>
#include <snap/rhi/backend/opengl/TextureTarget.h>

namespace snap::rhi::backend::opengl {
class Profile;
} // namespace snap::rhi::backend::opengl

namespace snap::rhi::backend::opengl {
class TextureInterop : public virtual snap::rhi::backend::common::TextureInterop {
public:
    virtual GLuint getOpenGLTexture(const snap::rhi::backend::opengl::Profile& gl) = 0;
    virtual snap::rhi::backend::opengl::TextureTarget getOpenGLTextureTarget(
        const snap::rhi::backend::opengl::Profile& gl) = 0;
};
} // namespace snap::rhi::backend::opengl
