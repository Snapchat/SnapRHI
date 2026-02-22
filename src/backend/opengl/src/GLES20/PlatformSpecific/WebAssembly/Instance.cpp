#include "../../Instance.hpp"
#include "snap/rhi/backend/opengl/Context.h"
#include "snap/rhi/common/OS.h"

#include "snap/rhi/common/Throw.h"
#include <cassert>

#if SNAP_RHI_GLES20 && SNAP_RHI_PLATFORM_WEBASSEMBLY()
namespace snap::rhi::backend::opengl::es20 {
std::string Instance::getEGLExt() {
    // https://github.com/emscripten-core/emscripten/blob/16bbfdd9353c5cb0e4262d41ff73cbf50cb5ff50/src/library_egl.js#L524
    // WebGL/Emscripten does not support any EGL extensions
    return "";
}

void* Instance::mapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access) {
    return nullptr;
}

GLboolean Instance::unmapBuffer(GLenum target) {
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
    snap::rhi::common::throwException("WebGL1 doesn't support texSubImage3D");
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
    snap::rhi::common::throwException("WebGL1 doesn't support texImage3D");
}

void Instance::renderbufferStorageMultisample(snap::rhi::backend::opengl::TextureTarget target,
                                              GLsizei samples,
                                              snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                                              GLsizei width,
                                              GLsizei height) {
    snap::rhi::common::throwException(
        "renderbufferStorageMultisampleEXT not available on WebGL1. It is only available on WebGL2");
}

SNAP_RHI_GLsync Instance::fenceSync(GLenum condition, GLbitfield flags) {
    snap::rhi::common::throwException("WebGL1 does not support fenceSync");
}

void Instance::waitSync(SNAP_RHI_GLsync sync, GLbitfield flags, GLuint64 timeout) {
    snap::rhi::common::throwException("WebGL1 does not support waitSync");
}

GLenum Instance::clientWaitSync(SNAP_RHI_GLsync sync, GLbitfield flags, GLuint64 timeout) {
    snap::rhi::common::throwException("WebGL1 does not support clientWaitSync");
}

void Instance::deleteSync(SNAP_RHI_GLsync sync) {
    snap::rhi::common::throwException("WebGL1 does not support deleteSync");
}

void Instance::queryCounter(GLuint id, GLenum target) {
    snap::rhi::common::throwException("WebGL1 does not support glQueryCounterEXT");
}

void Instance::getQueryObjectiv(GLuint id, GLenum pname, GLint* params) {
    snap::rhi::common::throwException("WebGL1 does not support glGetQueryObjectivEXT");
}

void Instance::getQueryObjecti64v(GLuint id, GLenum pname, GLint64* params) {
    snap::rhi::common::throwException("WebGL1 does not support glGetQueryObjecti64vEXT");
}

void Instance::getQueryObjectui64v(GLuint id, GLenum pname, GLuint64* params) {
    snap::rhi::common::throwException("WebGL1 does not support glGetQueryObjectui64vEXT");
}

void Instance::getInteger64v(GLenum pname, GLint64* data) {
    snap::rhi::common::throwException("WebGL1 does not support glGetInteger64vEXT");
}
} // namespace snap::rhi::backend::opengl::es20
#endif // SNAP_RHI_PLATFORM_WEBASSEMBLY()
