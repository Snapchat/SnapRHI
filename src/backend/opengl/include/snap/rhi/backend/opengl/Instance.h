//
//  Instance.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 6/27/22.
//

#pragma once

#include "snap/rhi/backend/opengl/Features.h"
#include "snap/rhi/backend/opengl/Format.h"
#include "snap/rhi/backend/opengl/FramebufferAttachmentTarget.h"
#include "snap/rhi/backend/opengl/FramebufferStatus.h"
#include "snap/rhi/backend/opengl/FramebufferTarget.h"
#include "snap/rhi/backend/opengl/OpenGL.h"
#include "snap/rhi/backend/opengl/TextureTarget.h"

namespace snap::rhi::backend::opengl {
struct Instance {
    Features features;

    void (*texSubImage3D)(snap::rhi::backend::opengl::TextureTarget target,
                          GLint level,
                          GLint xoffset,
                          GLint yoffset,
                          GLint zoffset,
                          GLsizei width,
                          GLsizei height,
                          GLsizei depth,
                          snap::rhi::backend::opengl::FormatGroup format,
                          snap::rhi::backend::opengl::FormatDataType type,
                          const void* pixels) = nullptr;

    void (*texImage3D)(snap::rhi::backend::opengl::TextureTarget target,
                       GLint level,
                       snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                       GLsizei width,
                       GLsizei height,
                       GLsizei depth,
                       GLint border,
                       snap::rhi::backend::opengl::FormatGroup format,
                       snap::rhi::backend::opengl::FormatDataType type,
                       const void* data) = nullptr;

    void (*texStorage2D)(snap::rhi::backend::opengl::TextureTarget target,
                         GLsizei levels,
                         snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                         GLsizei width,
                         GLsizei height) = nullptr;
    void (*texStorage3D)(snap::rhi::backend::opengl::TextureTarget target,
                         GLsizei levels,
                         snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                         GLsizei width,
                         GLsizei height,
                         GLsizei depth) = nullptr;
    void (*texStorage2DMultisample)(snap::rhi::backend::opengl::TextureTarget target,
                                    GLsizei samples,
                                    snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                                    GLsizei width,
                                    GLsizei height,
                                    GLboolean fixedsamplelocations) = nullptr;
    void (*texStorage3DMultisample)(snap::rhi::backend::opengl::TextureTarget target,
                                    GLsizei samples,
                                    snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                                    GLsizei width,
                                    GLsizei height,
                                    GLsizei depth,
                                    GLboolean fixedsamplelocations) = nullptr;
    void (*copyImageSubData)(GLuint srcName,
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
                             GLsizei srcDepth) = nullptr;

    void (*renderbufferStorageMultisample)(snap::rhi::backend::opengl::TextureTarget target,
                                           GLsizei samples,
                                           snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                                           GLsizei width,
                                           GLsizei height) = nullptr;

    void (*drawBuffers)(GLsizei n, const snap::rhi::backend::opengl::FramebufferAttachmentTarget* bufs) = nullptr;
    void (*readBuffer)(snap::rhi::backend::opengl::FramebufferAttachmentTarget mode) = nullptr;

    void (*getActiveUniformBlockName)(
        GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei* length, GLchar* uniformBlockName) = nullptr;
    GLuint (*getUniformBlockIndex)(GLuint program, const GLchar* uniformBlockName) = nullptr;
    void (*getActiveUniformBlockiv)(GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint* params) = nullptr;
    void (*getActiveUniformsiv)(
        GLuint program, GLsizei uniformCount, const GLuint* uniformIndices, GLenum pname, GLint* params) = nullptr;
    void (*uniformBlockBinding)(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding) = nullptr;

    void (*uniform1uiv)(GLint location, GLsizei count, const GLuint* value) = nullptr;
    void (*uniform2uiv)(GLint location, GLsizei count, const GLuint* value) = nullptr;
    void (*uniform3uiv)(GLint location, GLsizei count, const GLuint* value) = nullptr;
    void (*uniform4uiv)(GLint location, GLsizei count, const GLuint* value) = nullptr;

    void (*bindImageTexture)(GLuint unit,
                             GLuint texture,
                             GLint level,
                             GLboolean layered,
                             GLint layer,
                             GLenum access,
                             GLenum format) = nullptr;

    void (*getProgramResourceiv)(GLuint program,
                                 GLenum programInterface,
                                 GLuint index,
                                 GLsizei propCount,
                                 const GLenum* props,
                                 GLsizei bufSize,
                                 GLsizei* length,
                                 GLint* params) = nullptr;

    void (*getProgramInterfaceiv)(GLuint program, GLenum programInterface, GLenum pname, GLint* params) = nullptr;

    void (*getProgramResourceName)(
        GLuint program, GLenum programInterface, GLuint index, GLsizei bufSize, GLsizei* length, char* name) = nullptr;

    GLuint (*getProgramResourceIndex)(GLuint program, GLenum programInterface, const char* name) = nullptr;

    SNAP_RHI_GLsync (*fenceSync)(GLenum condition, GLbitfield flags) = nullptr;
    void (*waitSync)(SNAP_RHI_GLsync sync, GLbitfield flags, GLuint64 timeout) = nullptr;
    GLenum (*clientWaitSync)(SNAP_RHI_GLsync sync, GLbitfield flags, GLuint64 timeout) = nullptr;
    void (*deleteSync)(SNAP_RHI_GLsync sync) = nullptr;

    void (*getInternalformativ)(snap::rhi::backend::opengl::TextureTarget target,
                                snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                                GLenum pname,
                                GLsizei bufSize,
                                GLint* params) = nullptr;

    void (*memoryBarrierByRegion)(GLbitfield barriers) = nullptr;

    void (*discardFramebuffer)(snap::rhi::backend::opengl::FramebufferTarget target,
                               GLsizei numAttachments,
                               const snap::rhi::backend::opengl::FramebufferAttachmentTarget* attachments) = nullptr;
    void (*genSamplers)(GLsizei n, GLuint* samplers) = nullptr;
    void (*deleteSamplers)(GLsizei n, const GLuint* samplers) = nullptr;
    void (*bindSampler)(GLuint unit, GLuint sampler) = nullptr;
    void (*samplerParameterf)(GLuint sampler, GLenum pname, GLfloat param) = nullptr;
    void (*samplerParameteri)(GLuint sampler, GLenum pname, GLint param) = nullptr;
    void (*samplerParameterfv)(GLuint sampler, GLenum pname, const GLfloat* params) = nullptr;
    GLboolean (*isSampler)(GLuint id) = nullptr;

    void (*copyBufferSubData)(
        GLenum readtarget, GLenum writetarget, GLintptr readoffset, GLintptr writeoffset, GLsizeiptr size) = nullptr;
    void (*blitFramebuffer)(GLint srcX0,
                            GLint srcY0,
                            GLint srcX1,
                            GLint srcY1,
                            GLint dstX0,
                            GLint dstY0,
                            GLint dstX1,
                            GLint dstY1,
                            GLbitfield mask,
                            GLenum filter) = nullptr;

    void (*genVertexArrays)(GLsizei n, GLuint* arrays) = nullptr;
    void (*bindVertexArray)(GLuint array) = nullptr;
    void (*deleteVertexArrays)(GLsizei n, const GLuint* arrays) = nullptr;

    void (*clearBufferiv)(GLenum buffer, GLint drawBuffer, const GLint* value) = nullptr;
    void (*clearBufferuiv)(GLenum buffer, GLint drawBuffer, const GLuint* value) = nullptr;
    void (*clearBufferfv)(GLenum buffer, GLint drawBuffer, const GLfloat* value) = nullptr;
    void (*clearBufferfi)(GLenum buffer, GLint drawBuffer, GLfloat depth, GLint stencil) = nullptr;

    void (*bindBufferRange)(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size) = nullptr;

    void (*flushMappedBufferRange)(GLenum target, GLintptr offset, GLsizeiptr length) = nullptr;
    void* (*mapBufferRange)(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access) = nullptr;
    GLboolean (*unmapBuffer)(GLenum target) = nullptr;

    void (*drawArraysInstanced)(GLenum mode, GLint first, GLsizei count, GLsizei primcount) = nullptr;
    void (*drawElementsInstanced)(GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instancecount) =
        nullptr;
    void (*vertexAttribDivisor)(GLuint index, GLuint divisor) = nullptr;

    void (*vertexAttribI4iv)(GLuint index, const GLint* v) = nullptr;
    void (*vertexAttribI4uiv)(GLuint index, const GLuint* v) = nullptr;

    void (*framebufferTextureLayer)(snap::rhi::backend::opengl::FramebufferTarget target,
                                    snap::rhi::backend::opengl::FramebufferAttachmentTarget attachment,
                                    snap::rhi::backend::opengl::TextureId texture,
                                    int32_t level,
                                    int32_t layer) = nullptr;

    void (*framebufferTextureMultiviewOVR)(snap::rhi::backend::opengl::FramebufferTarget target,
                                           snap::rhi::backend::opengl::FramebufferAttachmentTarget attachment,
                                           snap::rhi::backend::opengl::TextureId texture,
                                           int32_t level,
                                           int32_t baseViewIndex,
                                           int32_t numViews) = nullptr;

    void (*framebufferTextureMultisampleMultiviewOVR)(
        snap::rhi::backend::opengl::FramebufferTarget target,
        snap::rhi::backend::opengl::FramebufferAttachmentTarget attachment,
        snap::rhi::backend::opengl::TextureId texture,
        int32_t level,
        int32_t samples,
        int32_t baseViewIndex,
        int32_t numViews) = nullptr;

    void (*colorMaski)(GLuint buf, GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) = nullptr;
    void (*enablei)(GLenum cap, GLuint index) = nullptr;
    void (*disablei)(GLenum cap, GLuint index) = nullptr;
    void (*blendEquationSeparatei)(GLuint buf, GLenum modeRGB, GLenum modeAlpha) = nullptr;
    void (*blendFuncSeparatei)(GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) = nullptr;

    void (*dispatchCompute)(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z) = nullptr;
    void (*hint)(GLenum target, GLenum mode) = nullptr;

    void (*getTexLevelParameteriv)(GLenum target, GLint level, GLenum pname, GLint* params) = nullptr;
    void (*getTexParameteriv)(GLenum target, GLenum pname, GLint* params) = nullptr;

    void (*pixelStorei)(GLenum pname, GLint param) = nullptr;

    void (*copyTexSubImage3D)(GLenum target,
                              GLint level,
                              GLint xoffset,
                              GLint yoffset,
                              GLint zoffset,
                              GLint x,
                              GLint y,
                              GLsizei width,
                              GLsizei height) = nullptr;

    void (*programParameteri)(GLuint program, GLenum pname, GLint value) = nullptr;
    void (*programBinary)(GLuint program, GLenum binaryFormat, const void* binary, GLsizei length) = nullptr;
    void (*getProgramBinary)(GLuint program, GLsizei bufsize, GLsizei* length, GLenum* binaryFormat, void* binary) =
        nullptr;

    void (*genQueries)(GLsizei n, GLuint* ids) = nullptr;
    void (*deleteQueries)(GLsizei n, const GLuint* ids) = nullptr;
    GLboolean (*isQuery)(GLuint id) = nullptr;
    void (*beginQuery)(GLenum target, GLuint id) = nullptr;
    void (*endQuery)(GLenum target) = nullptr;
    void (*getQueryiv)(GLenum target, GLenum pname, GLint* params) = nullptr;
    void (*getQueryObjectiv)(GLuint id, GLenum pname, GLint* params) = nullptr;
    void (*getQueryObjectuiv)(GLuint id, GLenum pname, GLuint* params) = nullptr;

    void (*queryCounter)(GLuint id, GLenum target) = nullptr;
    void (*getQueryObjecti64v)(GLuint id, GLenum pname, GLint64* params) = nullptr;
    void (*getQueryObjectui64v)(GLuint id, GLenum pname, GLuint64* params) = nullptr;
    void (*getInteger64v)(GLenum pname, GLint64* data) = nullptr;
};

void addCompressedFormat(Features& features);
bool checkOpenGLErrors();
} // namespace snap::rhi::backend::opengl
