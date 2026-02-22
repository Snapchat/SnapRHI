#pragma once

#include "../GL21/Instance.hpp"
#include "snap/rhi/Structs.h"

#if SNAP_RHI_GL41
namespace snap::rhi::backend::opengl41 {
struct Instance : opengl21::Instance {
public:
    static snap::rhi::backend::opengl::Features buildFeatures();

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

    static void getInternalformativ(snap::rhi::backend::opengl::TextureTarget target,
                                    snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                                    GLenum pname,
                                    GLsizei bufSize,
                                    GLint* params);
    static void genSamplers(GLsizei n, GLuint* samplers);
    static void deleteSamplers(GLsizei n, const GLuint* samplers);
    static void bindSampler(GLuint unit, GLuint sampler);
    static void samplerParameterf(GLuint sampler, GLenum pname, GLfloat param);
    static void samplerParameteri(GLuint sampler, GLenum pname, GLint param);
    static void samplerParameterfv(GLuint sampler, GLenum pname, const GLfloat* params);
    static GLboolean isSampler(GLuint id);

    static void copyBufferSubData(
        GLenum readtarget, GLenum writetarget, GLintptr readoffset, GLintptr writeoffset, GLsizeiptr size);

    static void genVertexArrays(GLsizei n, GLuint* arrays);
    static void bindVertexArray(GLuint array);
    static void deleteVertexArrays(GLsizei n, const GLuint* arrays);

    static void clearBufferiv(GLenum buffer, GLint drawBuffer, const GLint* value);
    static void clearBufferuiv(GLenum buffer, GLint drawBuffer, const GLuint* value);
    static void clearBufferfv(GLenum buffer, GLint drawBuffer, const GLfloat* value);
    static void clearBufferfi(GLenum buffer, GLint drawBuffer, GLfloat depth, GLint stencil);

    static void bindBufferRange(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size);

    static void vertexAttribI4iv(GLuint index, const GLint* v);
    static void vertexAttribI4uiv(GLuint index, const GLuint* v);

    static void* mapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access);

    static void framebufferTextureLayer(snap::rhi::backend::opengl::FramebufferTarget target,
                                        snap::rhi::backend::opengl::FramebufferAttachmentTarget attachment,
                                        snap::rhi::backend::opengl::TextureId texture,
                                        int32_t level,
                                        int32_t layer);

    static void colorMaski(GLuint buf, GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
    static void enablei(GLenum cap, GLuint index);
    static void disablei(GLenum cap, GLuint index);
    static void blendEquationSeparatei(GLuint buf, GLenum modeRGB, GLenum modeAlpha);
    static void blendFuncSeparatei(GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);

    static void hint(GLenum target, GLenum mode);

    static void programParameteri(GLuint program, GLenum pname, GLint value);
    static void programBinary(GLuint program, GLenum binaryFormat, const void* binary, GLsizei length);
    static void getProgramBinary(GLuint program, GLsizei bufsize, GLsizei* length, GLenum* binaryFormat, void* binary);

    static void queryCounter(GLuint id, GLenum target);
    static void getQueryObjecti64v(GLuint id, GLenum pname, GLint64* params);
    static void getQueryObjectui64v(GLuint id, GLenum pname, GLuint64* params);
};
} // namespace snap::rhi::backend::opengl41
#endif // SNAP_RHI_GL41
