#pragma once

#include "../GLES31/Instance.hpp"
#include "snap/rhi/Structs.h"

#if SNAP_RHI_GLES32
namespace snap::rhi::backend::opengl::es32 {
struct Instance : es31::Instance {
public:
    static snap::rhi::backend::opengl::Features buildFeatures(gl::APIVersion realApiVersion);

    static void texStorage3DMultisample(snap::rhi::backend::opengl::TextureTarget target,
                                        GLsizei samples,
                                        snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                                        GLsizei width,
                                        GLsizei height,
                                        GLsizei depth,
                                        GLboolean fixedsamplelocations);

    static void colorMaski(GLuint buf, GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
    static void enablei(GLenum cap, GLuint index);
    static void disablei(GLenum cap, GLuint index);
    static void blendEquationSeparatei(GLuint buf, GLenum modeRGB, GLenum modeAlpha);
    static void blendFuncSeparatei(GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
    static void copyImageSubData(GLuint srcName,
                                 GLenum srcTarget,
                                 GLint srcLevel,
                                 GLint srcX,
                                 GLint srcY,
                                 GLint srcZ,
                                 GLuint dstName,
                                 GLenum dstTarget,
                                 GLint dstLevel,
                                 GLint dstX,
                                 GLint dstY,
                                 GLint dstZ,
                                 GLsizei srcWidth,
                                 GLsizei srcHeight,
                                 GLsizei srcDepth);
};
} // namespace snap::rhi::backend::opengl::es32
#endif // SNAP_RHI_GLES32
