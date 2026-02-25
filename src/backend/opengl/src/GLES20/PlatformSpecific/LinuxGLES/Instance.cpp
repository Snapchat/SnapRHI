// GLES 2.0 platform-specific implementations for Linux-based GLES platforms.
// These platforms use EGL + GLES (no desktop GL) and need eglGetProcAddress
// to resolve EGL sync extension functions at runtime.

#include "../../Instance.hpp"
#include "snap/rhi/backend/opengl/Context.h"

#include "snap/rhi/common/Throw.h"
#include <cassert>

// Guard: GLES on Linux-based OS, but NOT Android (which has its own PlatformSpecific).
#if SNAP_RHI_GLES20 && SNAP_RHI_OS_LINUX_BASED() && !SNAP_RHI_OS_ANDROID()

#include <mutex>

static PFNEGLCREATESYNCKHRPROC _eglCreateSyncKHR = nullptr;
static PFNEGLWAITSYNCKHRPROC _eglWaitSyncKHR = nullptr;
static PFNEGLCLIENTWAITSYNCKHRPROC _eglClientWaitSyncKHR = nullptr;
static PFNEGLDESTROYSYNCKHRPROC _eglDestroySyncKHR = nullptr;

namespace snap::rhi::backend::opengl::es20 {
std::string Instance::getEGLExt() {
    std::string eglExtensionsList = (char*)eglQueryString(gl::getDefaultDisplay(), EGL_EXTENSIONS);
    return eglExtensionsList;
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
    glTexSubImage3DOES(static_cast<GLenum>(target),
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
    glTexImage3DOES(static_cast<GLenum>(target),
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
    glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, samples, static_cast<GLenum>(internalformat), width, height);
}

SNAP_RHI_GLsync Instance::fenceSync(GLenum condition, GLbitfield flags) {
    static std::once_flag of;
    std::call_once(of, [&] {
        _eglCreateSyncKHR = reinterpret_cast<PFNEGLCREATESYNCKHRPROC>(eglGetProcAddress("eglCreateSyncKHR"));
    });
    assert(condition == GL_SYNC_GPU_COMMANDS_COMPLETE && flags == 0);
    return reinterpret_cast<SNAP_RHI_GLsync>(_eglCreateSyncKHR(eglGetCurrentDisplay(), EGL_SYNC_FENCE_KHR, nullptr));
}

void Instance::waitSync(SNAP_RHI_GLsync sync, GLbitfield flags, GLuint64 timeout) {
    static std::once_flag of;
    std::call_once(
        of, [&] { _eglWaitSyncKHR = reinterpret_cast<PFNEGLWAITSYNCKHRPROC>(eglGetProcAddress("eglWaitSyncKHR")); });
    assert(flags == 0);
    [[maybe_unused]] const auto result = _eglWaitSyncKHR(eglGetCurrentDisplay(), reinterpret_cast<EGLSyncKHR>(sync), 0);
    assert(result == EGL_TRUE);
}

GLenum Instance::clientWaitSync(SNAP_RHI_GLsync sync, GLbitfield flags, GLuint64 timeout) {
    static std::once_flag of;
    std::call_once(of, [&] {
        _eglClientWaitSyncKHR =
            reinterpret_cast<PFNEGLCLIENTWAITSYNCKHRPROC>(eglGetProcAddress("eglClientWaitSyncKHR"));
    });
    assert(flags == GL_SYNC_FLUSH_COMMANDS_BIT || flags == 0);
    return _eglClientWaitSyncKHR(eglGetCurrentDisplay(), reinterpret_cast<EGLSyncKHR>(sync), flags, timeout);
}

void Instance::deleteSync(SNAP_RHI_GLsync sync) {
    static std::once_flag of;
    std::call_once(of, [&] {
        _eglDestroySyncKHR = reinterpret_cast<PFNEGLDESTROYSYNCKHRPROC>(eglGetProcAddress("eglDestroySyncKHR"));
    });
    _eglDestroySyncKHR(eglGetCurrentDisplay(), reinterpret_cast<EGLSyncKHR>(sync));
}

void Instance::queryCounter(GLuint id, GLenum target) {
    glQueryCounterEXT(id, target);
}

void Instance::getQueryObjectiv(GLuint id, GLenum pname, GLint* params) {
    glGetQueryObjectivEXT(id, pname, params);
}

void Instance::getQueryObjecti64v(GLuint id, GLenum pname, GLint64* params) {
    glGetQueryObjecti64vEXT(id, pname, params);
}

void Instance::getQueryObjectui64v(GLuint id, GLenum pname, GLuint64* params) {
    glGetQueryObjectui64vEXT(id, pname, params);
}

void Instance::getInteger64v(GLenum pname, GLint64* data) {
    glGetInteger64vEXT(pname, data);
}

} // namespace snap::rhi::backend::opengl::es20
#endif // SNAP_RHI_GLES20 && SNAP_RHI_OS_LINUX_BASED() && !SNAP_RHI_OS_ANDROID()
