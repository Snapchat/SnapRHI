#pragma once

#include <snap/rhi/common/OS.h>
#include <snap/rhi/common/Platform.h>

#include <cstdint>
#include <string>
#include <string_view>

#include <OpenGL/API.h>
#include <OpenGL/runtime.h>

#if GLAD_PLATFORM_OPENGL_ES()
#define SNAP_RHI_GL_ES 1 // OpenGL ES
#elif GLAD_PLATFORM_OPENGL()
#define SNAP_RHI_GL_ES 0 // OpenGL ES
#endif

#if SNAP_RHI_GL_ES
#define SNAP_RHI_GLES20 (1)
#else
#define SNAP_RHI_GLES20 (0)
#endif

#if SNAP_RHI_GL_ES
#define SNAP_RHI_GLES30 (1)
#else
#define SNAP_RHI_GLES30 (0)
#endif

#if SNAP_RHI_GL_ES &&                                                                                                  \
    !(SNAP_RHI_OS_MACOS() || SNAP_RHI_OS_IOS() || SNAP_RHI_OS_IOS_SIMULATOR() || SNAP_RHI_PLATFORM_WEBASSEMBLY())
#define SNAP_RHI_GLES31 (1)
#else
#define SNAP_RHI_GLES31 (0)
#endif

#if SNAP_RHI_GL_ES &&                                                                                                  \
    !(SNAP_RHI_OS_MACOS() || SNAP_RHI_OS_IOS() || SNAP_RHI_OS_IOS_SIMULATOR() || SNAP_RHI_PLATFORM_WEBASSEMBLY())
#define SNAP_RHI_GLES32 (1)
#else
#define SNAP_RHI_GLES32 (0)
#endif

#if !SNAP_RHI_GL_ES
#define SNAP_RHI_GL21 (1)
#else
#define SNAP_RHI_GL21 (0)
#endif

#if !SNAP_RHI_GL_ES
#define SNAP_RHI_GL41 (1)
#else
#define SNAP_RHI_GL41 (0)
#endif

#if !SNAP_RHI_GL_ES &&                                                                                                 \
    !(SNAP_RHI_OS_MACOS() || SNAP_RHI_OS_IOS() || SNAP_RHI_OS_IOS_SIMULATOR() || SNAP_RHI_PLATFORM_WEBASSEMBLY())
#define SNAP_RHI_GL45 (1)
#else
#define SNAP_RHI_GL45 (0)
#endif

#include "snap/rhi/backend/opengl/Constants.h"

namespace snap::rhi::backend::opengl {
constexpr GLint InvalidLocation = -1;
constexpr GLint InvalidBinding = -1;

gl::APIVersion computeVersion(const char* versionString);

bool isExtensionSupported(std::string_view extensionsList, std::string_view extName);

constexpr std::string_view getErrorStr(GLenum code) {
    switch (code) {
        case GL_NO_ERROR:
            return "GL_NO_ERROR";
        case GL_INVALID_ENUM:
            return "GL_INVALID_ENUM";
        case GL_INVALID_VALUE:
            return "GL_INVALID_VALUE";
        case GL_INVALID_OPERATION:
            return "GL_INVALID_OPERATION";
        case GL_STACK_OVERFLOW:
            return "GL_STACK_OVERFLOW";
        case GL_STACK_UNDERFLOW:
            return "GL_STACK_UNDERFLOW";
        case GL_OUT_OF_MEMORY:
            return "GL_OUT_OF_MEMORY";
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            return "GL_INVALID_FRAMEBUFFER_OPERATION";

        default:
            return "GL_UNDEFINED_ERROR";
    }
}

std::string getGLErrorsString();
} // namespace snap::rhi::backend::opengl
