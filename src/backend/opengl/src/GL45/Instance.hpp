#pragma once

#include "../GL41/Instance.hpp"
#include "snap/rhi/Structs.h"

#if SNAP_RHI_GL45
namespace snap::rhi::backend::opengl45 {
struct Instance : opengl41::Instance {
public:
    static snap::rhi::backend::opengl::Features buildFeatures();

    static void discardFramebuffer(snap::rhi::backend::opengl::FramebufferTarget target,
                                   GLsizei numAttachments,
                                   const snap::rhi::backend::opengl::FramebufferAttachmentTarget* attachments);

    static void getInternalformativ(snap::rhi::backend::opengl::TextureTarget target,
                                    snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                                    GLenum pname,
                                    GLsizei bufSize,
                                    GLint* params);

    static void texStorage2DMultisample(snap::rhi::backend::opengl::TextureTarget target,
                                        GLsizei samples,
                                        snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                                        GLsizei width,
                                        GLsizei height,
                                        GLboolean fixedsamplelocations);
    static void texStorage3DMultisample(snap::rhi::backend::opengl::TextureTarget target,
                                        GLsizei samples,
                                        snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                                        GLsizei width,
                                        GLsizei height,
                                        GLsizei depth,
                                        GLboolean fixedsamplelocations);
    static void getProgramInterfaceiv(GLuint program, GLenum programInterface, GLenum pname, GLint* params);

    static void getProgramResourceName(
        GLuint program, GLenum programInterface, GLuint index, GLsizei bufSize, GLsizei* length, char* name);

    static GLuint getProgramResourceIndex(GLuint program, GLenum programInterface, const char* name);

    static void bindImageTexture(
        GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format);

    static void getProgramResourceiv(GLuint program,
                                     GLenum programInterface,
                                     GLuint index,
                                     GLsizei propCount,
                                     const GLenum* props,
                                     GLsizei bufSize,
                                     GLsizei* length,
                                     GLint* params);
    static void memoryBarrierByRegion(GLbitfield barriers);

    static void dispatchCompute(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z);

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
} // namespace snap::rhi::backend::opengl45
#endif // SNAP_RHI_GL45
