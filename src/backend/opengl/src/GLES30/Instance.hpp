#pragma once

#include "../GLES20/Instance.hpp"
#include "snap/rhi/Structs.h"

#if SNAP_RHI_GLES30
namespace snap::rhi::backend::opengl::es30 {
struct Instance : es20::Instance {
public:
    static snap::rhi::backend::opengl::Features buildFeatures(gl::APIVersion realApiVersion);

    static void texSubImage3D(snap::rhi::backend::opengl::TextureTarget target,
                              GLint level,
                              GLint xoffset,
                              GLint yoffset,
                              GLint zoffset,
                              GLsizei width,
                              GLsizei height,
                              GLsizei depth,
                              snap::rhi::backend::opengl::FormatGroup format,
                              snap::rhi::backend::opengl::FormatDataType type,
                              const void* pixels);

    static void texImage3D(snap::rhi::backend::opengl::TextureTarget target,
                           GLint level,
                           snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                           GLsizei width,
                           GLsizei height,
                           GLsizei depth,
                           GLint border,
                           snap::rhi::backend::opengl::FormatGroup format,
                           snap::rhi::backend::opengl::FormatDataType type,
                           const void* data);

    static void texStorage2D(snap::rhi::backend::opengl::TextureTarget target,
                             GLsizei levels,
                             snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                             GLsizei width,
                             GLsizei height);
    static void texStorage3D(snap::rhi::backend::opengl::TextureTarget target,
                             GLsizei levels,
                             snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                             GLsizei width,
                             GLsizei height,
                             GLsizei depth);

    static void renderbufferStorageMultisample(snap::rhi::backend::opengl::TextureTarget target,
                                               GLsizei samples,
                                               snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                                               GLsizei width,
                                               GLsizei height);

    static void drawBuffers(GLsizei n, const snap::rhi::backend::opengl::FramebufferAttachmentTarget* bufs);
    static void readBuffer(snap::rhi::backend::opengl::FramebufferAttachmentTarget mode);

    static void getActiveUniformBlockName(
        GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei* length, GLchar* uniformBlockName);
    static GLuint getUniformBlockIndex(GLuint program, const GLchar* uniformBlockName);
    static void getActiveUniformBlockiv(GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint* params);
    static void getActiveUniformsiv(
        GLuint program, GLsizei uniformCount, const GLuint* uniformIndices, GLenum pname, GLint* params);
    static void uniformBlockBinding(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding);

    static void uniform1uiv(GLint location, GLsizei count, const GLuint* value);
    static void uniform2uiv(GLint location, GLsizei count, const GLuint* value);
    static void uniform3uiv(GLint location, GLsizei count, const GLuint* value);
    static void uniform4uiv(GLint location, GLsizei count, const GLuint* value);

    static SNAP_RHI_GLsync fenceSync(GLenum condition, GLbitfield flags);
    static void waitSync(SNAP_RHI_GLsync sync, GLbitfield flags, GLuint64 timeout);
    static GLenum clientWaitSync(SNAP_RHI_GLsync sync, GLbitfield flags, GLuint64 timeout);
    static void deleteSync(SNAP_RHI_GLsync sync);

    static void getInternalformativ(snap::rhi::backend::opengl::TextureTarget target,
                                    snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                                    GLenum pname,
                                    GLsizei bufSize,
                                    GLint* params);

    static void discardFramebuffer(snap::rhi::backend::opengl::FramebufferTarget target,
                                   GLsizei numAttachments,
                                   const snap::rhi::backend::opengl::FramebufferAttachmentTarget* attachments);

    static void genSamplers(GLsizei n, GLuint* samplers);
    static void deleteSamplers(GLsizei n, const GLuint* samplers);
    static void bindSampler(GLuint unit, GLuint sampler);
    static void samplerParameterf(GLuint sampler, GLenum pname, GLfloat param);
    static void samplerParameteri(GLuint sampler, GLenum pname, GLint param);
    static void samplerParameterfv(GLuint sampler, GLenum pname, const GLfloat* params);
    static GLboolean isSampler(GLuint id);

    static void copyBufferSubData(
        GLenum readtarget, GLenum writetarget, GLintptr readoffset, GLintptr writeoffset, GLsizeiptr size);
    static void blitFramebuffer(GLint srcX0,
                                GLint srcY0,
                                GLint srcX1,
                                GLint srcY1,
                                GLint dstX0,
                                GLint dstY0,
                                GLint dstX1,
                                GLint dstY1,
                                GLbitfield mask,
                                GLenum filter);

    static void genVertexArrays(GLsizei n, GLuint* arrays);
    static void bindVertexArray(GLuint array);
    static void deleteVertexArrays(GLsizei n, const GLuint* arrays);

    static void clearBufferiv(GLenum buffer, GLint drawBuffer, const GLint* value);
    static void clearBufferuiv(GLenum buffer, GLint drawBuffer, const GLuint* value);
    static void clearBufferfv(GLenum buffer, GLint drawBuffer, const GLfloat* value);
    static void clearBufferfi(GLenum buffer, GLint drawBuffer, GLfloat depth, GLint stencil);

    static void bindBufferRange(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size);

    static void flushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length);
    static void* mapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access);
    static GLboolean unmapBuffer(GLenum target);

    static void drawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei primcount);
    static void drawElementsInstanced(
        GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instancecount);
    static void vertexAttribDivisor(GLuint index, GLuint divisor);

    static void vertexAttribI4iv(GLuint index, const GLint* v);
    static void vertexAttribI4uiv(GLuint index, const GLuint* v);

    static void framebufferTextureLayer(snap::rhi::backend::opengl::FramebufferTarget target,
                                        snap::rhi::backend::opengl::FramebufferAttachmentTarget attachment,
                                        snap::rhi::backend::opengl::TextureId texture,
                                        int32_t level,
                                        int32_t layer);

    static void framebufferTextureMultiviewOVR(snap::rhi::backend::opengl::FramebufferTarget target,
                                               snap::rhi::backend::opengl::FramebufferAttachmentTarget attachment,
                                               snap::rhi::backend::opengl::TextureId texture,
                                               int32_t level,
                                               int32_t baseViewIndex,
                                               int32_t numViews);

    static void framebufferTextureMultisampleMultiviewOVR(
        snap::rhi::backend::opengl::FramebufferTarget target,
        snap::rhi::backend::opengl::FramebufferAttachmentTarget attachment,
        snap::rhi::backend::opengl::TextureId texture,
        int32_t level,
        int32_t samples,
        int32_t baseViewIndex,
        int32_t numViews);
    static void getTexParameteriv(GLenum target, GLenum pname, GLint* params);

    static void pixelStorei(GLenum pname, GLint param);

    static void programParameteri(GLuint program, GLenum pname, GLint value);
    static void programBinary(GLuint program, GLenum binaryFormat, const void* binary, GLsizei length);
    static void getProgramBinary(GLuint program, GLsizei bufsize, GLsizei* length, GLenum* binaryFormat, void* binary);

    static void copyTexSubImage3D(GLenum target,
                                  GLint level,
                                  GLint xoffset,
                                  GLint yoffset,
                                  GLint zoffset,
                                  GLint x,
                                  GLint y,
                                  GLsizei width,
                                  GLsizei height);

    static void genQueries(GLsizei n, GLuint* ids);
    static void deleteQueries(GLsizei n, const GLuint* ids);
    static GLboolean isQuery(GLuint id);
    static void beginQuery(GLenum target, GLuint id);
    static void endQuery(GLenum target);
    static void getQueryiv(GLenum target, GLenum pname, GLint* params);
    static void getQueryObjectuiv(GLuint id, GLenum pname, GLuint* params);
};
} // namespace snap::rhi::backend::opengl::es30
#endif // SNAP_RHI_GL41
