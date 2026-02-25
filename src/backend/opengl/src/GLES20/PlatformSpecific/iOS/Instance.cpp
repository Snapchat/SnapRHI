#include "../../Instance.hpp"
#include "snap/rhi/backend/opengl/Context.h"
#include "snap/rhi/common/OS.h"

#include "snap/rhi/common/Throw.h"
#include <cassert>

#if SNAP_RHI_GLES20 && (SNAP_RHI_OS_IOS() || SNAP_RHI_OS_IOS_SIMULATOR())
namespace snap::rhi::backend::opengl::es20 {
std::string Instance::getEGLExt() {
    return "";
}

void* Instance::mapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access) {
    // glMapBufferOES
    return nullptr;
}

GLboolean Instance::unmapBuffer(GLenum target) {
    // glUnmapBufferOES
    return GL_FALSE;
}

void Instance::texSubImage3D(snap::rhi::backend::opengl::TextureTarget target,
                             GLint level,
                             GLint xoffset,
                             GLint yoffset,
                             GLint zoffset,
                             GLsizei width,
                             GLsizei height,
                             GLsizei depth,
                             snap::rhi::backend::opengl::FormatGroup format,
                             snap::rhi::backend::opengl::FormatDataType type,
                             const void* pixels) {
    glTexSubImage3D(static_cast<GLenum>(target),
                    level,
                    xoffset,
                    yoffset,
                    zoffset,
                    width,
                    height,
                    depth,
                    static_cast<GLenum>(format),
                    static_cast<GLenum>(type),
                    pixels);
}

void Instance::texImage3D(snap::rhi::backend::opengl::TextureTarget target,
                          GLint level,
                          snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                          GLsizei width,
                          GLsizei height,
                          GLsizei depth,
                          GLint border,
                          snap::rhi::backend::opengl::FormatGroup format,
                          snap::rhi::backend::opengl::FormatDataType type,
                          const void* data) {
    glTexImage3D(static_cast<GLenum>(target),
                 level,
                 static_cast<GLenum>(internalformat),
                 width,
                 height,
                 depth,
                 border,
                 static_cast<GLenum>(format),
                 static_cast<GLenum>(type),
                 data);
}

void Instance::renderbufferStorageMultisample(snap::rhi::backend::opengl::TextureTarget target,
                                              GLsizei samples,
                                              snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                                              GLsizei width,
                                              GLsizei height) {
    glRenderbufferStorageMultisampleAPPLE(GL_RENDERBUFFER, samples, static_cast<GLenum>(internalformat), width, height);
}

SNAP_RHI_GLsync Instance::fenceSync(GLenum condition, GLbitfield flags) {
    return reinterpret_cast<SNAP_RHI_GLsync>(glFenceSyncAPPLE(condition, flags));
}

void Instance::waitSync(SNAP_RHI_GLsync sync, GLbitfield flags, GLuint64 timeout) {
    glWaitSyncAPPLE(reinterpret_cast<GLsync>(sync), flags, timeout);
}

GLenum Instance::clientWaitSync(SNAP_RHI_GLsync sync, GLbitfield flags, GLuint64 timeout) {
    return glClientWaitSyncAPPLE(reinterpret_cast<GLsync>(sync), flags, timeout);
}

void Instance::deleteSync(SNAP_RHI_GLsync sync) {
    glDeleteSyncAPPLE(reinterpret_cast<GLsync>(sync));
}

void Instance::queryCounter(GLuint id, GLenum target) {
    snap::rhi::common::throwException("[GLES20] queryCounter not supported");
}

void Instance::getQueryObjectiv(GLuint id, GLenum pname, GLint* params) {
    snap::rhi::common::throwException("[GLES20] getQueryObjectiv not supported");
}

void Instance::getQueryObjecti64v(GLuint id, GLenum pname, GLint64* params) {
    snap::rhi::common::throwException("[GLES20] getQueryObjecti64v not supported");
}

void Instance::getQueryObjectui64v(GLuint id, GLenum pname, GLuint64* params) {
    snap::rhi::common::throwException("[GLES20] getQueryObjectui64v not supported");
}

void Instance::getInteger64v(GLenum pname, GLint64* data) {
    snap::rhi::common::throwException("[GLES20] getInteger64v not supported");
}

} // namespace snap::rhi::backend::opengl::es20
#endif //(SNAP_RHI_OS_IOS() || SNAP_RHI_OS_IOS_SIMULATOR())
