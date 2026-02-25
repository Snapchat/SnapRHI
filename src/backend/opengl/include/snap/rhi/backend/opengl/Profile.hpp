//
//  Profile.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 1/22/20.
//  Copyright © 2020 Snapchat. All rights reserved.
//

#pragma once

#include "snap/rhi/backend/common/Device.hpp"
#include "snap/rhi/backend/opengl/Features.h"
#include "snap/rhi/backend/opengl/Format.h"
#include "snap/rhi/backend/opengl/FramebufferAttachmentTarget.h"
#include "snap/rhi/backend/opengl/FramebufferStatus.h"
#include "snap/rhi/backend/opengl/FramebufferTarget.h"
#include "snap/rhi/backend/opengl/GPUInfo.h"
#include "snap/rhi/backend/opengl/Instance.h"
#include "snap/rhi/backend/opengl/OpenGL.h"
#include "snap/rhi/backend/opengl/TextureTarget.h"

#include "snap/rhi/Capabilities.h"
#include "snap/rhi/PixelFormat.h"
#include "snap/rhi/Structs.h"
#include "snap/rhi/backend/common/Logging.hpp"
#include "snap/rhi/backend/common/SrcLoc.hpp"
#include "snap/rhi/backend/common/ValidationLayer.hpp"
#include "snap/rhi/backend/opengl/DeviceContext.hpp"

#include "snap/rhi/common/NonCopyable.h"

#include <span>

#include <array>
#include <string>
#include <string_view>

// GL_KHR_debug object-type identifiers used by glObjectLabel / Profile::objectLabel.
// Provide fallback definitions for platforms whose system headers omit them
// (e.g. macOS desktop GL 4.1).  Values are from the Khronos GL_KHR_debug spec.
#ifndef GL_SHADER
#define GL_SHADER 0x82E1
#endif
#ifndef GL_PROGRAM
#define GL_PROGRAM 0x82E2
#endif

#if SNAP_RHI_PLATFORM_WEBASSEMBLY()
#include <emscripten/bind.h>
#include <webgl/webgl2.h>

namespace emscripten {
class val;
} // namespace emscripten
#endif

namespace snap::rhi::backend::opengl {
class DeviceContext;
class GLStateCache;

#if SNAP_RHI_PLATFORM_WEBASSEMBLY()
inline emscripten::val getTextureTargetWebGl(GLenum target) {
    const auto glCtx = emscripten::val::module_property("ctx");
    switch (target) {
        case GL_RENDERBUFFER:
            return glCtx["RENDERBUFFER"];
        case GL_TEXTURE_2D:
            return glCtx["TEXTURE_2D"];
        case GL_TEXTURE_2D_ARRAY:
            return glCtx["TEXTURE_2D_ARRAY"];
        case GL_TEXTURE_3D:
            return glCtx["TEXTURE_3D"];
        case GL_TEXTURE_CUBE_MAP:
            return glCtx["TEXTURE_CUBE_MAP"];
        case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
            return glCtx["TEXTURE_CUBE_MAP_POSITIVE_X"];
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
            return glCtx["TEXTURE_CUBE_MAP_NEGATIVE_X"];
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
            return glCtx["TEXTURE_CUBE_MAP_POSITIVE_Y"];
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
            return glCtx["TEXTURE_CUBE_MAP_NEGATIVE_Y"];
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
            return glCtx["TEXTURE_CUBE_MAP_POSITIVE_Z"];
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
            return glCtx["TEXTURE_CUBE_MAP_NEGATIVE_Z"];
        default:
            snap::rhi::common::throwException("Invalid WebGL Texture Target");
    }
}

inline emscripten::val getFormatGroupWebGl(GLenum format) {
    const auto glCtx = emscripten::val::module_property("ctx");
    switch (format) {
        case GL_RED:
            return glCtx["RED"];
        case GL_RED_INTEGER:
            return glCtx["RED_INTEGER"];
        case GL_RG:
            return glCtx["RG"];
        case GL_RG_INTEGER:
            return glCtx["RG_INTEGER"];
        case GL_RGB:
            return glCtx["RGB"];
        case GL_RGB_INTEGER:
            return glCtx["RGB_INTEGER"];
        case GL_RGBA:
            return glCtx["RGBA"];
        case GL_RGBA_INTEGER:
            return glCtx["RGBA_INTEGER"];
        case GL_DEPTH_COMPONENT:
            return glCtx["DEPTH_COMPONENT"];
        case GL_DEPTH_STENCIL:
            return glCtx["DEPTH_STENCIL"];
        case GL_LUMINANCE:
            return glCtx["LUMINANCE"];
        case GL_LUMINANCE_ALPHA:
            return glCtx["RG"];
        // GL_INTENSITY not available on WebGL
        default:
            snap::rhi::common::throwException("Invalid WebGL Format Group");
    }
}

inline emscripten::val getSizedInternalFormat(GLenum format) {
    const auto glCtx = emscripten::val::module_property("ctx");
    switch (format) {
        case GL_ALPHA:
            return glCtx["ALPHA"];
        case GL_RGB:
            return glCtx["RGB"];
        case GL_RGBA:
        case GL_RGBA8:
            return glCtx["RGBA"];
        case GL_LUMINANCE:
            return glCtx["LUMINANCE"];
        case GL_LUMINANCE_ALPHA:
            // LUMINANCE_ALPHA handling was discovered to be broken
            // RG8 internal format and RG format works, however.
            return glCtx["RG8"];
        case GL_R8:
            return glCtx["R8"];
        case GL_R16F:
            return glCtx["R16F"];
        case GL_R32F:
            return glCtx["R32F"];
        case GL_R8UI:
            return glCtx["R8UI"];
        case GL_RG8:
            return glCtx["RG8"];
        case GL_RG16F:
            return glCtx["RG16F"];
        case GL_RG32F:
            return glCtx["RG32F"];
        case GL_RG8UI:
            return glCtx["RG8UI"];
        case GL_RGB8:
            return glCtx["RGB8"];
        case GL_SRGB8:
            return glCtx["SRGB8"];
        case GL_RGB565:
            return glCtx["RGB565"];
        case GL_R11F_G11F_B10F:
            return glCtx["R11F_G11F_B10F"];
        case GL_RGB9_E5:
            return glCtx["RGB9_E5"];
        case GL_RGB16F:
            return glCtx["RGB16F"];
        case GL_RGB32F:
            return glCtx["RGB32F"];
        case GL_RGB8UI:
            return glCtx["RGB8UI"];
        case GL_SRGB8_ALPHA8:
            return glCtx["SRGB8_ALPHA8"];
        case GL_RGB5_A1:
            return glCtx["RGB5_A1"];
        case GL_RGBA4:
            return glCtx["RGBA4"];
        case GL_RGBA16F:
            return glCtx["RGBA16F"];
        case GL_RGBA32F:
            return glCtx["RGBA32F"];
        case GL_RGBA8UI:
            return glCtx["RGBA8UI"];
        default:
            snap::rhi::common::throwException("Invalid WebGL Sized Internal Format");
    }
}

inline emscripten::val getFormatDataTypeWebGl(GLenum format) {
    const auto glCtx = emscripten::val::module_property("ctx");
    switch (format) {
        case GL_UNSIGNED_BYTE:
            return glCtx["UNSIGNED_BYTE"];
        case GL_BYTE:
            return glCtx["BYTE"];
        case GL_HALF_FLOAT:
            return glCtx["HALF_FLOAT"];
        case GL_HALF_FLOAT_OES:
            return glCtx["HALF_FLOAT_OES"];
        case GL_FLOAT:
            return glCtx["FLOAT"];
        case GL_UNSIGNED_SHORT:
            return glCtx["UNSIGNED_SHORT"];
        case GL_SHORT:
            return glCtx["SHORT"];
        case GL_UNSIGNED_INT:
            return glCtx["UNSIGNED_INT"];
        case GL_INT:
            return glCtx["INT"];
        case GL_UNSIGNED_INT_10F_11F_11F_REV:
            return glCtx["UNSIGNED_INT_10F_11F_11F_REV"];
        case GL_UNSIGNED_INT_5_9_9_9_REV:
            return glCtx["UNSIGNED_INT_5_9_9_9_REV"];
        case GL_UNSIGNED_INT_2_10_10_10_REV:
            return glCtx["UNSIGNED_INT_2_10_10_10_REV"];
        case GL_UNSIGNED_INT_24_8:
            return glCtx["UNSIGNED_INT_24_8"];
        case GL_FLOAT_32_UNSIGNED_INT_24_8_REV:
            return glCtx["FLOAT_32_UNSIGNED_INT_24_8_REV"];
        default:
            snap::rhi::common::throwException("Invalid WebGL Format Data Type");
    }
}
#endif

#if !SNAP_RHI_ENABLE_API_DUMP
#define SNAP_RHI_GL_LOG(...)
#else
template<typename Enum>
std::string toString(Enum constant) {
    std::string_view c = tryFindGLConstantName(static_cast<int64_t>(constant));
    return c.empty() ? std::to_string(static_cast<int64_t>(constant)) : std::string(c);
}

template<typename Enum>
std::string toStringEnums(size_t n, const Enum* arr) {
    if (n == 0) {
        return "[]";
    }

    std::string strBuilder{};
    strBuilder += "[";
    for (size_t i = 0; i < n; ++i) {
        strBuilder += toString(arr[i]).data();
        if (i != (n - 1)) {
            strBuilder += ", ";
        }
    }
    strBuilder += "]";
    return strBuilder;
}

template<typename Enum>
std::string toStringVals(size_t n, const Enum* arr) {
    if (n == 0) {
        return "[]";
    }

    std::string strBuilder{};
    strBuilder += "[";
    for (size_t i = 0; i < n; ++i) {
        if constexpr (std::is_enum_v<Enum>) {
            strBuilder += std::to_string(static_cast<uint32_t>(arr[i]));
        } else if constexpr (std::is_same_v<Enum, const char*>) {
            strBuilder += "\"";
            strBuilder += std::string(arr[i]);
            strBuilder += "\"";
        } else {
            strBuilder += std::to_string(arr[i]);
        }
        if (i != (n - 1)) {
            strBuilder += ", ";
        }
    }
    strBuilder += "]";
    return strBuilder;
}

#define SNAP_RHI_GL_LOG(...) SNAP_RHI_LOGI("[SnapRHI/GL]: " __VA_ARGS__)
#endif

class Profile final : snap::rhi::common::NonCopyable {
public:
    Profile(snap::rhi::backend::opengl::Device* device);
    ~Profile() = default;

    const Features& getFeatures() const {
        return gl.features;
    }

    gl::APIVersion getNativeGLVersion() const noexcept {
        return nativeGLVersion;
    }

    snap::rhi::backend::common::Device* getDevice() const {
        return device;
    }

    const snap::rhi::backend::opengl::CompositeFormat& getTextureFormat(const snap::rhi::PixelFormat format) const {
        return gl.features.textureFormat[static_cast<uint32_t>(format)];
    }

    const snap::rhi::backend::opengl::RenderbufferFormatInfo& getRenderbufferFormat(
        const snap::rhi::PixelFormat format) const {
        return gl.features.renderbufferFormat[static_cast<uint32_t>(format)];
    }

    snap::rhi::Capabilities buildCapabilities() const;

    /**
     * Always should be called under OpenGL context
     **/
    void loadOpenGLInstance(const gl::APIVersion logicalApi,
                            const gl::APIVersion realApi,
                            std::string_view vendor,
                            std::string_view renderer);

    static void checkGLErrors(SNAP_RHI_SRC_LOCATION_TYPE srcLocation,
                              snap::rhi::backend::common::Device* device,
                              const snap::rhi::ValidationTag tags = snap::rhi::ValidationTag::GLProfileOp,
                              const snap::rhi::ReportLevel reportType = snap::rhi::ReportLevel::Error);

public:
    SNAP_RHI_ALWAYS_INLINE void debugMessageCallback(DebugProc callback, const void* userParam) const {
#if 0
    //!SNAP_RHI_PLATFORM_WEBASSEMBLY() && (defined(GL_KHR_debug) || defined(EGL_KHR_debug))
    glDebugMessageCallback(callback, userParam);
#endif

        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void debugMessageControl(
        GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint* ids, GLboolean enabled) const {
#if 0
    //!SNAP_RHI_PLATFORM_WEBASSEMBLY() && (defined(GL_KHR_debug) || defined(EGL_KHR_debug))
    glDebugMessageControl(source, type, severity, count, ids, enabled);
#endif
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    /**
     * GL_KHR_debug: label a GL object so it appears by name in GPU debuggers.
     * No-op on platforms where the GLAD function pointer macro is absent (macOS desktop GL 4.1).
     */
    SNAP_RHI_ALWAYS_INLINE void objectLabel([[maybe_unused]] GLenum identifier,
                                            [[maybe_unused]] GLuint name,
                                            [[maybe_unused]] std::string_view label) const {
#ifdef glObjectLabel
        if (!glObjectLabel || name == GL_NONE) {
            return;
        }
        SNAP_RHI_GL_LOG("glObjectLabel(identifier=0x%x, name=%u, label=\"%.*s\")",
                        identifier,
                        name,
                        static_cast<int>(label.size()),
                        label.data());
        glObjectLabel(identifier, name, static_cast<GLsizei>(label.size()), label.data());
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
#endif
    }

    /**
     * GL_KHR_debug: label a GL sync object (returned by glFenceSync).
     * No-op on platforms where the GLAD function pointer macro is absent.
     */
    SNAP_RHI_ALWAYS_INLINE void objectPtrLabel([[maybe_unused]] SNAP_RHI_GLsync sync,
                                               [[maybe_unused]] std::string_view label) const {
#ifdef glObjectPtrLabel
        if (!glObjectPtrLabel || sync == nullptr) {
            return;
        }
        SNAP_RHI_GL_LOG("glObjectPtrLabel(ptr=%p, label=\"%.*s\")", sync, static_cast<int>(label.size()), label.data());
        glObjectPtrLabel(sync, static_cast<GLsizei>(label.size()), label.data());
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
#endif
    }

    /**
     * GL_KHR_debug: open a debug group region visible in GPU debuggers.
     * No-op on platforms where the GLAD function pointer macro is absent.
     */
    SNAP_RHI_ALWAYS_INLINE void pushDebugGroup([[maybe_unused]] const char* label,
                                               [[maybe_unused]] GLsizei length) const {
#ifdef glPushDebugGroup
        if (!glPushDebugGroup) {
            return;
        }
        SNAP_RHI_GL_LOG("glPushDebugGroup(label=\"%.*s\")", static_cast<int>(length), label);
        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, length, label);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
#endif
    }

    /**
     * GL_KHR_debug: close the most-recently opened debug group region.
     * No-op on platforms where the GLAD function pointer macro is absent.
     */
    SNAP_RHI_ALWAYS_INLINE void popDebugGroup() const {
#ifdef glPopDebugGroup
        if (!glPopDebugGroup) {
            return;
        }
        SNAP_RHI_GL_LOG("glPopDebugGroup()");
        glPopDebugGroup();
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
#endif
    }

    SNAP_RHI_ALWAYS_INLINE void genQueries(GLsizei n, GLuint* ids) const {
        SNAP_RHI_GL_LOG("glGenQueries(n=%d, ids=%p)", n, ids);
        gl.genQueries(n, ids);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void deleteQueries(GLsizei n, const GLuint* ids) const {
        SNAP_RHI_GL_LOG("glDeleteQueries(n=%d, ids=%p)", n, ids);
        gl.deleteQueries(n, ids);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    [[nodiscard]] SNAP_RHI_ALWAYS_INLINE GLboolean isQuery(GLuint id) const {
        SNAP_RHI_GL_LOG("glIsQuery(id=%d)", id);
        auto result = gl.isQuery(id);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
        return result;
    }

    SNAP_RHI_ALWAYS_INLINE void beginQuery(GLenum target, GLuint id) const {
        SNAP_RHI_GL_LOG("glBeginQuery(target=%d, id=%d)", target, id);
        gl.beginQuery(target, id);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void endQuery(GLenum target) const {
        SNAP_RHI_GL_LOG("glEndQuery(target=%d)", target);
        gl.endQuery(target);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void getQueryiv(GLenum target, GLenum pname, GLint* params) const {
        SNAP_RHI_GL_LOG("glGetQueryiv(target=%d, pname=%d, params=%p)", target, pname, params);
        gl.getQueryiv(target, pname, params);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void getQueryObjectiv(GLuint id, GLenum pname, GLint* params) const {
        SNAP_RHI_GL_LOG("glGetQueryObjectiv(id=%d, pname=%d, params=%p)", id, pname, params);
        gl.getQueryObjectiv(id, pname, params);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void getQueryObjectuiv(GLuint id, GLenum pname, GLuint* params) const {
        SNAP_RHI_GL_LOG("glGetQueryObjectuiv(id=%d, pname=%d, params=%p)", id, pname, params);
        gl.getQueryObjectuiv(id, pname, params);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void queryCounter(GLuint id, GLenum target) const {
        SNAP_RHI_VALIDATE(validationLayer,
                          target == GL_TIMESTAMP,
                          snap::rhi::ReportLevel::Error,
                          snap::rhi::ValidationTag::QueryPoolOp,
                          "queryCounter only takes target == GL_TIMESTAMP");
        SNAP_RHI_GL_LOG("glQueryCounter(id=%d, target=%d)", id, static_cast<int>(target));
        gl.queryCounter(id, target);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void getQueryObjecti64v(GLuint id, GLenum pname, GLint64* params) const {
        SNAP_RHI_GL_LOG("glGetQueryObjecti64v(id=%d, pname=%d, params=%p)", id, pname, params);
        gl.getQueryObjecti64v(id, pname, params);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void getQueryObjectui64v(GLuint id, GLenum pname, GLuint64* params) const {
        SNAP_RHI_GL_LOG("glGetQueryObjectui64v(id=%d, pname=%d, params=%p)", id, pname, params);
        gl.getQueryObjectui64v(id, pname, params);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void getInteger64v(GLenum pname, GLint64* data) const {
        SNAP_RHI_GL_LOG("glGetInteger64v(pname=%d, data=%p)", pname, data);
        gl.getInteger64v(pname, data);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void validateFramebuffer(
        const snap::rhi::backend::opengl::FramebufferTarget target,
        const snap::rhi::backend::opengl::FramebufferId fbo,
        const snap::rhi::backend::opengl::FramebufferDescription& activeDescription) const {
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(
            snap::rhi::ReportLevel::Error, snap::rhi::ValidationTag::FramebufferOp, [&]() {
                FramebufferStatus result = checkFramebufferStatus(target);

                if (result != FramebufferStatus::Complete) {
                    std::string errorMessage = describe(target, fbo, activeDescription);

                    SNAP_RHI_LOGE("[validateFramebuffer] status: %d, description: %s\n",
                                  static_cast<int>(result),
                                  errorMessage.c_str());
                    snap::rhi::common::throwException<FramebufferIncompleteException>(
                        result,
                        snap::rhi::common::stringFormat("[validateFramebuffer] status: %d, description: %s\n",
                                                        static_cast<int>(result),
                                                        errorMessage.c_str()));
                }
            });
    }

    SNAP_RHI_ALWAYS_INLINE void genBuffers(GLsizei n, GLuint* buffers) const {
        assert(n > 0 && buffers);
        glGenBuffers(n, buffers);
        SNAP_RHI_GL_LOG("glGenBuffers(n=%d, buffers=%s)", n, toStringVals(n, buffers).c_str());

        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void bindBuffer(GLenum target, GLuint buffer, DeviceContext* dc) const {
        if (dc) {
            GLStateCache& cache = dc->getGLStateCache();
            if (!cache.shouldBindBuffer(target, buffer)) {
                return;
            }
        }

        SNAP_RHI_GL_LOG("glBindBuffer(target=%s, buffer=%u)", toString(target).c_str(), buffer);
        glBindBuffer(target, buffer);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void bufferData(GLenum target, GLsizeiptr size, const void* data, GLenum usage) const {
        SNAP_RHI_GL_LOG("glBufferData(target=%s, size=%ld, data=%p, usage=%s)",
                        toString(target).c_str(),
                        size,
                        data,
                        toString(usage).c_str());
        glBufferData(target, size, data, usage);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void bufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void* data) const {
        SNAP_RHI_GL_LOG(
            "glBufferSubData(target=%s, offset=%ld, size=%ld, data=%p)", toString(target).c_str(), offset, size, data);
        glBufferSubData(target, offset, size, data);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void deleteBuffers(GLsizei n, const GLuint* buffers) const {
        assert(n > 0 && buffers);
        SNAP_RHI_GL_LOG("glDeleteBuffers(n=%d, buffers=%s)", n, toStringVals(n, buffers).c_str());
        glDeleteBuffers(n, buffers);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void deleteRenderbuffers(GLsizei n,
                                                    snap::rhi::backend::opengl::TextureId* renderbuffers) const {
        assert(n > 0 && renderbuffers);
        SNAP_RHI_GL_LOG("glDeleteRenderbuffers(n=%d, renderbuffers=%s)", n, toStringVals(n, renderbuffers).c_str());
        glDeleteRenderbuffers(n, reinterpret_cast<GLuint*>(renderbuffers));
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void deleteTextures(GLsizei n, const snap::rhi::backend::opengl::TextureId* textures) const {
        assert(n > 0 && textures);
        SNAP_RHI_GL_LOG("glDeleteTextures(n=%d, textures=%s)", n, toStringVals(n, textures).c_str());
        glDeleteTextures(n, reinterpret_cast<const GLuint*>(textures));
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void texSubImage2D(snap::rhi::backend::opengl::TextureTarget target,
                                              GLint level,
                                              GLint xoffset,
                                              GLint yoffset,
                                              GLsizei width,
                                              GLsizei height,
                                              snap::rhi::backend::opengl::FormatGroup format,
                                              snap::rhi::backend::opengl::FormatDataType type,
                                              const void* pixels) const {
        SNAP_RHI_GL_LOG(
            "glTexSubImage2D(target=%s, level=%d, xoffset=%d, yoffset=%d, width=%d, height=%d, format=%s, type=%s, "
            "pixels=%p)",
            toString(target).c_str(),
            level,
            xoffset,
            yoffset,
            width,
            height,
            toString(format).c_str(),
            toString(type).c_str(),
            pixels);
        glTexSubImage2D(static_cast<GLenum>(target),
                        level,
                        xoffset,
                        yoffset,
                        width,
                        height,
                        static_cast<GLenum>(format),
                        static_cast<GLenum>(type),
                        pixels);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void bindTexture(snap::rhi::backend::opengl::TextureTarget target,
                                            snap::rhi::backend::opengl::TextureId texture,
                                            DeviceContext* dc) const {
        if (dc) {
            GLStateCache& cache = dc->getGLStateCache();
            if (!cache.shouldBindTexture(static_cast<GLenum>(target), static_cast<GLuint>(texture))) {
                return;
            }
        }

        SNAP_RHI_GL_LOG("glBindTexture(target=%s, texture=%u)", toString(target).c_str(), static_cast<GLuint>(texture));
        glBindTexture(static_cast<GLenum>(target), static_cast<GLuint>(texture));
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void texImage2D(snap::rhi::backend::opengl::TextureTarget target,
                                           GLint level,
                                           snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                                           GLsizei width,
                                           GLsizei height,
                                           GLint border,
                                           snap::rhi::backend::opengl::FormatGroup format,
                                           snap::rhi::backend::opengl::FormatDataType type,
                                           const void* data) const {
        SNAP_RHI_GL_LOG(
            "glTexImage2D(target=%s, level=%d, internalformat=%s, width=%d, height=%d, border=%d, format=%s, "
            "type=%s, data=%p)",
            toString(target).c_str(),
            level,
            toString(internalformat).c_str(),
            width,
            height,
            border,
            toString(format).c_str(),
            toString(type).c_str(),
            data);
        glTexImage2D(static_cast<GLenum>(target),
                     level,
                     static_cast<GLenum>(internalformat),
                     width,
                     height,
                     border,
                     static_cast<GLenum>(format),
                     static_cast<GLenum>(type),
                     data);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void compressedTexImage2D(snap::rhi::backend::opengl::TextureTarget target,
                                                     GLint level,
                                                     snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                                                     GLsizei width,
                                                     GLsizei height,
                                                     GLint border,
                                                     GLsizei imageSize,
                                                     const void* data) const {
        SNAP_RHI_GL_LOG(
            "glCompressedTexImage2D(target=%s, level=%d, internalformat=%s, width=%d, height=%d, border=%d, "
            "imageSize=%d, data=%p)",
            toString(target).c_str(),
            level,
            toString(internalformat).c_str(),
            width,
            height,
            border,
            imageSize,
            data);
        glCompressedTexImage2D(static_cast<GLenum>(target),
                               level,
                               static_cast<GLenum>(internalformat),
                               width,
                               height,
                               border,
                               imageSize,
                               data);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void compressedTexImage3D(snap::rhi::backend::opengl::TextureTarget target,
                                                     GLint level,
                                                     snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                                                     GLsizei width,
                                                     GLsizei height,
                                                     GLsizei depth,
                                                     GLint border,
                                                     GLsizei imageSize,
                                                     const void* data) const {
        SNAP_RHI_GL_LOG("glCompressedTexImage3D(target=%s, level=%d, internalformat=%s, width=%d, height=%d, depth=%d, "
                        "border=%d, imageSize=%d, data=%p)",
                        toString(target).c_str(),
                        level,
                        toString(internalformat).c_str(),
                        width,
                        height,
                        depth,
                        border,
                        imageSize,
                        data);
        glCompressedTexImage3D(static_cast<GLenum>(target),
                               level,
                               static_cast<GLenum>(internalformat),
                               width,
                               height,
                               depth,
                               border,
                               imageSize,
                               data);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void compressedTexSubImage2D(snap::rhi::backend::opengl::TextureTarget target,
                                                        GLint level,
                                                        GLint xoffset,
                                                        GLint yoffset,
                                                        GLsizei width,
                                                        GLsizei height,
                                                        snap::rhi::backend::opengl::SizedInternalFormat format,
                                                        GLsizei imageSize,
                                                        const void* data) const {
        SNAP_RHI_GL_LOG(
            "glCompressedTexSubImage2D(target=%s, level=%d, xoffset=%d, yoffset=%d, width=%d, height=%d, format=%s, "
            "imageSize=%d, data=%p)",
            toString(target).c_str(),
            level,
            xoffset,
            yoffset,
            width,
            height,
            toString(format).c_str(),
            imageSize,
            data);
        glCompressedTexSubImage2D(static_cast<GLenum>(target),
                                  level,
                                  xoffset,
                                  yoffset,
                                  width,
                                  height,
                                  static_cast<GLenum>(format),
                                  imageSize,
                                  data);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void compressedTexSubImage3D(snap::rhi::backend::opengl::TextureTarget target,
                                                        GLint level,
                                                        GLint xoffset,
                                                        GLint yoffset,
                                                        GLint zoffset,
                                                        GLsizei width,
                                                        GLsizei height,
                                                        GLsizei depth,
                                                        snap::rhi::backend::opengl::SizedInternalFormat format,
                                                        GLsizei imageSize,
                                                        const void* data) const {
        SNAP_RHI_GL_LOG(
            "glCompressedTexSubImage3D(target=%s, level=%d, xoffset=%d, yoffset=%d, zoffset=%d, width=%d, height=%d, "
            "depth=%d, format=%s, imageSize=%d, data=%p)",
            toString(target).c_str(),
            level,
            xoffset,
            yoffset,
            zoffset,
            width,
            height,
            depth,
            toString(format).c_str(),
            imageSize,
            data);
        glCompressedTexSubImage3D(static_cast<GLenum>(target),
                                  level,
                                  xoffset,
                                  yoffset,
                                  zoffset,
                                  width,
                                  height,
                                  depth,
                                  static_cast<GLenum>(format),
                                  imageSize,
                                  data);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void bindRenderbuffer(snap::rhi::backend::opengl::TextureTarget target,
                                                 snap::rhi::backend::opengl::TextureId renderbuffer) const {
        SNAP_RHI_GL_LOG("glBindRenderbuffer(target=%s, renderbuffer=%u)",
                        toString(target).c_str(),
                        static_cast<GLuint>(renderbuffer));
        glBindRenderbuffer(static_cast<GLenum>(target), static_cast<GLuint>(renderbuffer));
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void renderbufferStorage(snap::rhi::backend::opengl::TextureTarget target,
                                                    snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                                                    GLsizei width,
                                                    GLsizei height) const {
        SNAP_RHI_GL_LOG("glRenderbufferStorage(target=%s, internalformat=%s, width=%d, height=%d)",
                        toString(target).c_str(),
                        toString(internalformat).c_str(),
                        width,
                        height);
        glRenderbufferStorage(static_cast<GLenum>(target), static_cast<GLenum>(internalformat), width, height);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void genRenderbuffers(GLsizei n,
                                                 snap::rhi::backend::opengl::TextureId* renderbuffers) const {
        glGenRenderbuffers(n, reinterpret_cast<GLuint*>(renderbuffers));
        SNAP_RHI_GL_LOG("glGenRenderbuffers(n=%d, renderbuffers=%s)", n, toStringVals(n, renderbuffers).c_str());
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void genTextures(GLsizei n, snap::rhi::backend::opengl::TextureId* textures) const {
        assert(n > 0 && textures);
        glGenTextures(n, reinterpret_cast<GLuint*>(textures));
        SNAP_RHI_GL_LOG("glGenTextures(n=%d, textures=%s)", n, toStringVals(n, textures).c_str());
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void framebufferRenderbuffer(
        snap::rhi::backend::opengl::FramebufferTarget target,
        snap::rhi::backend::opengl::FramebufferAttachmentTarget attachment,
        snap::rhi::backend::opengl::TextureTarget renderbuffertarget,
        snap::rhi::backend::opengl::TextureId renderbuffer) const {
        SNAP_RHI_GL_LOG("glFramebufferRenderbuffer(target=%s, attachment=%s, renderbuffertarget=%s, renderbuffer=%u)",
                        toString(target).c_str(),
                        toString(attachment).c_str(),
                        toString(renderbuffertarget).c_str(),
                        static_cast<GLuint>(renderbuffer));
        glFramebufferRenderbuffer(static_cast<GLenum>(target),
                                  static_cast<GLenum>(attachment),
                                  static_cast<GLenum>(renderbuffertarget),
                                  static_cast<GLuint>(renderbuffer));
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    snap::rhi::backend::opengl::FramebufferStatus checkFramebufferStatus(
        snap::rhi::backend::opengl::FramebufferTarget target) const {
        GLenum result = glCheckFramebufferStatus(static_cast<GLenum>(target));
        SNAP_RHI_GL_LOG(
            "glCheckFramebufferStatus(target=%s) -> %s", toString(target).c_str(), toString(result).c_str());
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
        return snap::rhi::backend::opengl::FramebufferStatus(result);
    }

    SNAP_RHI_ALWAYS_INLINE void genFramebuffers(GLsizei n, snap::rhi::backend::opengl::FramebufferId* ids) const {
        glGenFramebuffers(n, reinterpret_cast<GLuint*>(ids));
        SNAP_RHI_GL_LOG("glGenFramebuffers(n=%d, ids=%s)", n, toStringVals(n, ids).c_str());
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void bindFramebuffer(snap::rhi::backend::opengl::FramebufferTarget target,
                                                snap::rhi::backend::opengl::FramebufferId id,
                                                DeviceContext* dc) const {
        SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[Profile][bindFramebuffer]");

        if (dc) {
            GLStateCache& cache = dc->getGLStateCache();
            if (!cache.shouldBindFramebuffer(static_cast<GLenum>(target), static_cast<GLuint>(id))) {
                return;
            }
        }

        SNAP_RHI_GL_LOG("glBindFramebuffer(target=%s, id=%u)", toString(target).c_str(), static_cast<GLuint>(id));
        glBindFramebuffer(static_cast<GLenum>(target), static_cast<GLuint>(id));
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void deleteFramebuffers(GLsizei n,
                                                   const snap::rhi::backend::opengl::FramebufferId* ids) const {
        SNAP_RHI_GL_LOG("glDeleteFramebuffers(n=%d, ids=%s)", n, toStringVals(n, ids).c_str());
        glDeleteFramebuffers(n, reinterpret_cast<const GLuint*>(ids));
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }
    SNAP_RHI_ALWAYS_INLINE void clearDepth(GLclampf depth) const {
        SNAP_RHI_GL_LOG("glClearDepth(depth=%f)", depth);
#if SNAP_RHI_GL_ES
        glClearDepthf(depth);
#else
        glClearDepth(depth);
#endif
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void clearStencil(GLint s) const {
        SNAP_RHI_GL_LOG("glClearStencil(s=%d)", s);
        glClearStencil(s);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void clear(GLbitfield mask) const {
        SNAP_RHI_GL_LOG("glClear(mask=0x%x)", mask);
        glClear(mask);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void viewport(GLint x, GLint y, GLsizei width, GLsizei height, DeviceContext* dc) const {
        if (dc) {
            GLStateCache& cache = dc->getGLStateCache();
            if (!cache.shouldViewport(x, y, width, height)) {
                return;
            }
        }

        SNAP_RHI_GL_LOG("glViewport(x=%d, y=%d, width=%d, height=%d)", x, y, width, height);
        glViewport(x, y, width, height);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void scissor(GLint x, GLint y, GLsizei width, GLsizei height, DeviceContext* dc) const {
        SNAP_RHI_GL_LOG("glScissor(x=%d, y=%d, width=%d, height=%d)", x, y, width, height);
        glScissor(x, y, width, height);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void finish() const {
        SNAP_RHI_GL_LOG("glFinish()");
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });

        /**
         * Some vendors may ignore glFinish,
         * so potentially we have to implement this fix.
         */
        //    if (OpenGL::getGLESVersion() >= static_cast<int>(OpenGL::OpenGLESVersion::V3_0) &&
        //        GPU::vendor() == GPU::Vendor::PowerVR) {
        //        GLsync sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        //        glFlush(); // .. and some adrenos ignore GL_SYNC_FLUSH_COMMANDS_BIT
        //        // also, some GPUs other that powerVR still require to issue glFinish if we want to write to android
        //        // graphicsBuffer.
        //        GLenum waitRes = glClientWaitSync(sync, GL_SYNC_FLUSH_COMMANDS_BIT, (GLint64)(1e8)); // wait for 100
        //        msec. if (waitRes == GL_TIMEOUT_EXPIRED) {
        //            LS_LOGW("glClientWaitSync(): timeout expired");
        //        }
        //        glDeleteSync(sync);
        //        return true;
        //    }

        glFinish();
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void flush() const {
        SNAP_RHI_GL_LOG("glFlush()");
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
        glFlush();
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void clearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) const {
        SNAP_RHI_GL_LOG("glClearColor(red=%f, green=%f, blue=%f, alpha=%f)", red, green, blue, alpha);
        glClearColor(red, green, blue, alpha);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    GLuint createShader(GLenum shaderType) const {
        GLuint result = glCreateShader(shaderType);
        SNAP_RHI_GL_LOG("glCreateShader(shaderType=%s) -> %u", toString(shaderType).c_str(), result);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
        return result;
    }

    SNAP_RHI_ALWAYS_INLINE void shaderSource(GLuint shader,
                                             GLsizei count,
                                             const GLchar* const* string,
                                             const GLint* length) const {
        SNAP_RHI_GL_LOG("glShaderSource(shader=%u, count=%d, string=%s, length=%s)",
                        shader,
                        count,
                        toStringVals(count, string).c_str(),
                        toStringVals(count, length).c_str());
        glShaderSource(shader, count, string, length);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void compileShader(GLuint shader) const {
        SNAP_RHI_GL_LOG("glCompileShader(shader=%u)", shader);
        glCompileShader(shader);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void getShaderiv(GLuint shader, GLenum pname, GLint* params) const {
        glGetShaderiv(shader, pname, params);
        SNAP_RHI_GL_LOG("glGetShaderiv(shader=%u, pname=%s, params=%s)",
                        shader,
                        toString(pname).c_str(),
                        toStringEnums(1, params).c_str());
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void getShaderInfoLog(GLuint shader,
                                                 GLsizei maxLength,
                                                 GLsizei* length,
                                                 GLchar* infoLog) const {
        glGetShaderInfoLog(shader, maxLength, length, infoLog);
        SNAP_RHI_GL_LOG("glGetShaderInfoLog(shader=%u, maxLength=%d, length=%d, infoLog=%*.s)",
                        shader,
                        maxLength,
                        int(*length),
                        int(*length),
                        infoLog);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void deleteShader(GLuint shader) const {
        SNAP_RHI_GL_LOG("glDeleteShader(shader=%u)", shader);
        glDeleteShader(shader);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    GLuint createProgram() const {
        GLuint result = glCreateProgram();
        SNAP_RHI_GL_LOG("glCreateProgram() -> %u", result);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
        return result;
    }

    SNAP_RHI_ALWAYS_INLINE void attachShader(GLuint program, GLuint shader) const {
        SNAP_RHI_GL_LOG("glAttachShader(program=%u, shader=%u)", program, shader);
        glAttachShader(program, shader);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void detachShader(GLuint program, GLuint shader) const {
        SNAP_RHI_GL_LOG("glDetachShader(program=%u, shader=%u)", program, shader);
        glDetachShader(program, shader);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void linkProgram(GLuint program) const {
        SNAP_RHI_GL_LOG("glLinkProgram(program=%u)", program);
        glLinkProgram(program);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void getProgramiv(GLuint program, GLenum pname, GLint* params) const {
        glGetProgramiv(program, pname, params);
        SNAP_RHI_GL_LOG("glGetProgramiv(program=%u, pname=%s, params=%s)",
                        program,
                        toString(pname).c_str(),
                        toStringEnums(1, params).c_str());
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void getProgramInfoLog(GLuint program,
                                                  GLsizei maxLength,
                                                  GLsizei* length,
                                                  GLchar* infoLog) const {
        glGetProgramInfoLog(program, maxLength, length, infoLog);
        SNAP_RHI_GL_LOG("glGetProgramInfoLog(program=%u, maxLength=%d, length=%d, infoLog=%*.s)",
                        program,
                        maxLength,
                        int(*length),
                        int(*length),
                        infoLog);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void deleteProgram(GLuint program) const {
        SNAP_RHI_GL_LOG("glDeleteProgram(program=%u)", program);
        glDeleteProgram(program);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void getActiveAttrib(
        GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLint* size, GLenum* type, GLchar* name) const {
        glGetActiveAttrib(program, index, bufSize, length, size, type, name);
        SNAP_RHI_GL_LOG("glGetActiveAttrib(program=%u, index=%u, bufSize=%d, length=%d, size=%d, type=%s, name=\"%s\")",
                        program,
                        index,
                        bufSize,
                        int(*length),
                        int(*size),
                        toString(*type).c_str(),
                        name);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void bindAttribLocation(GLuint program, GLuint index, const GLchar* name) const {
        SNAP_RHI_GL_LOG("glBindAttribLocation(program=%u, index=%u, name=\"%s\")", program, index, name);
        glBindAttribLocation(program, index, name);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    GLint getAttribLocation(GLuint program, const GLchar* name) const {
        GLint result = glGetAttribLocation(program, name);
        SNAP_RHI_GL_LOG("glGetAttribLocation(program=%u, name=\"%s\") -> %d", program, name, result);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
        return result;
    }

    SNAP_RHI_ALWAYS_INLINE void getActiveUniform(
        GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLint* size, GLenum* type, GLchar* name) const {
        glGetActiveUniform(program, index, bufSize, length, size, type, name);
        SNAP_RHI_GL_LOG(
            "glGetActiveUniform(program=%u, index=%u, bufSize=%d, length=%d, size=%d, type=%s, name=\"%s\")",
            program,
            index,
            bufSize,
            int(*length),
            int(*size),
            toString(*type).c_str(),
            name);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }
    GLint getUniformLocation(GLuint program, const GLchar* name) const {
        GLint result = glGetUniformLocation(program, name);
        SNAP_RHI_GL_LOG("glGetUniformLocation(program=%u, name=\"%s\") -> %d", program, name, result);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
        return result;
    }

    SNAP_RHI_ALWAYS_INLINE void getUniformfv(GLuint program, GLint location, GLfloat* params) const {
        glGetUniformfv(program, location, params);
        SNAP_RHI_GL_LOG(
            "glGetUniformfv(program=%u, location=%d, params=%s)", program, location, toStringVals(1, params).c_str());
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void getUniformiv(GLuint program, GLint location, GLint* params) const {
        glGetUniformiv(program, location, params);
        SNAP_RHI_GL_LOG(
            "glGetUniformiv(program=%u, location=%d, params=%s)", program, location, toStringVals(1, params).c_str());
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void useProgram(GLuint program, DeviceContext* dc) const {
        SNAP_RHI_GL_LOG("glUseProgram(program=%u)", program);
        if (dc) {
            GLStateCache& cache = dc->getGLStateCache();
            if (!cache.shouldUseProgram(program)) {
                return;
            }
        }

        glUseProgram(program);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    GLboolean isProgram(GLuint program) const {
        GLboolean result = glIsProgram(program);
        SNAP_RHI_GL_LOG("glIsProgram(program=%u) -> %s", program, result ? "true" : "false");
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
        return result;
    }

    GLboolean isBuffer(GLuint buffer) const {
        GLboolean result = glIsBuffer(buffer);
        SNAP_RHI_GL_LOG("glIsBuffer(buffer=%u) -> %s", buffer, result ? "true" : "false");
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
        return result;
    }

    GLboolean isTexture(GLuint texture) const {
        GLboolean result = glIsTexture(texture);
        SNAP_RHI_GL_LOG("glIsTexture(texture=%u) -> %s", texture, result ? "true" : "false");
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
        return result;
    }

    SNAP_RHI_ALWAYS_INLINE void cullFace(GLenum mode, DeviceContext* dc) const {
        if (dc) {
            GLStateCache& cache = dc->getGLStateCache();
            if (!cache.shouldCullFace(mode)) {
                return;
            }
        }

        SNAP_RHI_GL_LOG("glCullFace(mode=%s)", toString(mode).c_str());
        glCullFace(mode);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void frontFace(GLenum mode) const {
        SNAP_RHI_GL_LOG("glFrontFace(mode=%s)", toString(mode).c_str());
        glFrontFace(mode);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void enable(GLenum cap, DeviceContext* dc) const {
        if (!gl.features.isRasterizationDisableSupported && cap == GL_RASTERIZER_DISCARD) {
            return;
        }

        if (dc) {
            GLStateCache& cache = dc->getGLStateCache();
            if (!cache.shouldEnable(cap)) {
                return;
            }
        }

        SNAP_RHI_GL_LOG("glEnable(cap=%s)", toString(cap).c_str());
        glEnable(cap);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void disable(GLenum cap, DeviceContext* dc) const {
        if (!gl.features.isRasterizationDisableSupported && cap == GL_RASTERIZER_DISCARD) {
            return;
        }

        if (dc) {
            GLStateCache& cache = dc->getGLStateCache();
            if (!cache.shouldDisable(cap)) {
                return;
            }
        }

        SNAP_RHI_GL_LOG("glDisable(cap=%s)", toString(cap).c_str());
        glDisable(cap);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void polygonMode(GLenum face, GLenum mode) const {
        SNAP_RHI_GL_LOG("glPolygonMode(face=%s, mode=%s)", toString(face).c_str(), toString(mode).c_str());
#if SNAP_RHI_GL_ES
        if (mode != GL_FILL) {
            snap::rhi::common::throwException("[Profile] OpenGL ES supported only GL_FILL polygonMode.");
        }
#else
        glPolygonMode(face, mode);
#endif
    }

    SNAP_RHI_ALWAYS_INLINE void sampleCoverage(GLfloat value, GLboolean invert) const {
        SNAP_RHI_GL_LOG("glSampleCoverage(value=%f, invert=%d)", value, invert);
        glSampleCoverage(value, invert);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void blendColor(
        GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha, DeviceContext* dc) const {
        if (dc) {
            GLStateCache& cache = dc->getGLStateCache();
            if (!cache.shouldBlendColor(red, green, blue, alpha)) {
                return;
            }
        }

        SNAP_RHI_GL_LOG("glBlendColor(red=%f, green=%f, blue=%f, alpha=%f)", red, green, blue, alpha);
        glBlendColor(red, green, blue, alpha);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void colorMask(
        GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha, DeviceContext* dc) const {
        if (dc) {
            GLStateCache& cache = dc->getGLStateCache();
            if (!cache.shouldColorMask(red, green, blue, alpha)) {
                return;
            }
        }

        SNAP_RHI_GL_LOG("glColorMask(red=%d, green=%d, blue=%d, alpha=%d)", red, green, blue, alpha);
        glColorMask(red, green, blue, alpha);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void blendFuncSeparate(
        GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha, DeviceContext* dc) const {
        if (dc) {
            GLStateCache& cache = dc->getGLStateCache();
            if (!cache.shouldBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha)) {
                return;
            }
        }

        SNAP_RHI_GL_LOG("glBlendFuncSeparate(srcRGB=%s, dstRGB=%s, srcAlpha=%s, dstAlpha=%s)",
                        toString(srcRGB).c_str(),
                        toString(dstRGB).c_str(),
                        toString(srcAlpha).c_str(),
                        toString(dstAlpha).c_str());
        glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void stencilOpSeparate(
        GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass, DeviceContext* dc) const {
        if (dc) {
            GLStateCache& cache = dc->getGLStateCache();
            if (!cache.shouldStencilOpSeparate(face, sfail, dpfail, dppass)) {
                return;
            }
        }

        SNAP_RHI_GL_LOG("glStencilOpSeparate(face=%s, sfail=%s, dpfail=%s, dppass=%s)",
                        toString(face).c_str(),
                        toString(sfail).c_str(),
                        toString(dpfail).c_str(),
                        toString(dppass).c_str());
        glStencilOpSeparate(face, sfail, dpfail, dppass);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void stencilMaskSeparate(GLenum face, GLuint mask, DeviceContext* dc) const {
        if (dc) {
            GLStateCache& cache = dc->getGLStateCache();
            if (!cache.shouldStencilMaskSeparate(face, mask)) {
                return;
            }
        }

        SNAP_RHI_GL_LOG("glStencilMaskSeparate(face=%s, mask=%u)", toString(face).c_str(), mask);
        glStencilMaskSeparate(face, mask);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void stencilFuncSeparate(
        GLenum face, GLenum func, GLint ref, GLuint mask, DeviceContext* dc) const {
        if (dc) {
            GLStateCache& cache = dc->getGLStateCache();
            if (!cache.shouldStencilFuncSeparate(face, func, ref, mask)) {
                return;
            }
        }

        SNAP_RHI_GL_LOG("glStencilFuncSeparate(face=%s, func=%s, ref=%d, mask=%u)",
                        toString(face).c_str(),
                        toString(func).c_str(),
                        ref,
                        mask);
        glStencilFuncSeparate(face, func, ref, mask);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void stencilFunc(GLenum func, GLint ref, GLuint mask, DeviceContext* dc) const {
        if (dc) {
            GLStateCache& cache = dc->getGLStateCache();
            if (!cache.shouldStencilFunc(func, ref, mask)) {
                return;
            }
        }

        SNAP_RHI_GL_LOG("glStencilFunc(func=%s, ref=%d, mask=%u)", toString(func).c_str(), ref, mask);
        glStencilFunc(func, ref, mask);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void depthRangef(GLfloat n, GLfloat f) const {
        SNAP_RHI_GL_LOG("glDepthRangef(n=%f, f=%f)", n, f);
#if SNAP_RHI_GL_ES
        glDepthRangef(n, f);
#else
        glDepthRange(n, f);
#endif
        checkErrors(SNAP_RHI_SOURCE_LOCATION());
    }

    SNAP_RHI_ALWAYS_INLINE void depthFunc(GLenum func, DeviceContext* dc) const {
        if (dc) {
            GLStateCache& cache = dc->getGLStateCache();
            if (!cache.shouldDepthFunc(func)) {
                return;
            }
        }

        SNAP_RHI_GL_LOG("glDepthFunc(func=%s)", toString(func).c_str());
        glDepthFunc(func);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void depthMask(GLboolean flag) const {
        SNAP_RHI_GL_LOG("glDepthMask(flag=%d)", flag);
        glDepthMask(flag);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void generateMipmap(snap::rhi::backend::opengl::TextureTarget target) const {
        SNAP_RHI_GL_LOG("glGenerateMipmap(target=%s)", toString(target).c_str());
        checkErrors(SNAP_RHI_SOURCE_LOCATION(), snap::rhi::ValidationTag::TextureOp);
        glGenerateMipmap(static_cast<GLenum>(target));
        checkErrors(SNAP_RHI_SOURCE_LOCATION(), snap::rhi::ValidationTag::TextureOp);
    }

    SNAP_RHI_ALWAYS_INLINE void getIntegerv(GLenum pname, GLint* params) const {
        glGetIntegerv(pname, params);
        SNAP_RHI_GL_LOG(
            "glGetIntegerv(pname=%s, params=%s)", toString(pname).c_str(), toStringEnums(1, params).c_str());
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void activeTexture(GLenum texture, DeviceContext* dc) const {
        if (dc) {
            GLStateCache& cache = dc->getGLStateCache();
            if (!cache.shouldActiveTexture(texture)) {
                return;
            }
        }

        SNAP_RHI_GL_LOG("glActiveTexture(texture=%s)", toString(texture).c_str());
        glActiveTexture(texture);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void uniform1i(GLint location, GLint v0) const {
        SNAP_RHI_GL_LOG("glUniform1i(location=%d, v0=%d)", location, v0);
        glUniform1i(location, v0);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void uniform1iv(GLint location, GLsizei count, const GLint* value) const {
        SNAP_RHI_GL_LOG(
            "glUniform1iv(location=%d, count=%d, value=%s)", location, count, toStringVals(count, value).c_str());
        glUniform1iv(location, count, value);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void uniform2iv(GLint location, GLsizei count, const GLint* value) const {
        SNAP_RHI_GL_LOG(
            "glUniform2iv(location=%d, count=%d, value=%s)", location, count, toStringVals(count * 2, value).c_str());
        glUniform2iv(location, count, value);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void uniform3iv(GLint location, GLsizei count, const GLint* value) const {
        SNAP_RHI_GL_LOG(
            "glUniform3iv(location=%d, count=%d, value=%s)", location, count, toStringVals(count * 3, value).c_str());
        glUniform3iv(location, count, value);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void uniform4iv(GLint location, GLsizei count, const GLint* value) const {
        SNAP_RHI_GL_LOG(
            "glUniform4iv(location=%d, count=%d, value=%s)", location, count, toStringVals(count * 4, value).c_str());
        glUniform4iv(location, count, value);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void uniform1uiv(GLint location, GLsizei count, const GLuint* value) const {
        SNAP_RHI_GL_LOG(
            "glUniform1uiv(location=%d, count=%d, value=%s)", location, count, toStringVals(count, value).c_str());
        gl.uniform1uiv(location, count, value);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void uniform2uiv(GLint location, GLsizei count, const GLuint* value) const {
        SNAP_RHI_GL_LOG(
            "glUniform2uiv(location=%d, count=%d, value=%s)", location, count, toStringVals(count * 2, value).c_str());
        gl.uniform2uiv(location, count, value);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void uniform3uiv(GLint location, GLsizei count, const GLuint* value) const {
        SNAP_RHI_GL_LOG(
            "glUniform3uiv(location=%d, count=%d, value=%s)", location, count, toStringVals(count * 3, value).c_str());
        gl.uniform3uiv(location, count, value);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void uniform4uiv(GLint location, GLsizei count, const GLuint* value) const {
        SNAP_RHI_GL_LOG(
            "glUniform4uiv(location=%d, count=%d, value=%s)", location, count, toStringVals(count * 4, value).c_str());
        gl.uniform4uiv(location, count, value);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void uniform1fv(GLint location, GLsizei count, const GLfloat* value) const {
        SNAP_RHI_GL_LOG(
            "glUniform1fv(location=%d, count=%d, value=%s)", location, count, toStringVals(count, value).c_str());
        glUniform1fv(location, count, value);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void uniform2fv(GLint location, GLsizei count, const GLfloat* value) const {
        SNAP_RHI_GL_LOG(
            "glUniform2fv(location=%d, count=%d, value=%s)", location, count, toStringVals(count * 2, value).c_str());
        glUniform2fv(location, count, value);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void uniform3fv(GLint location, GLsizei count, const GLfloat* value) const {
        SNAP_RHI_GL_LOG(
            "glUniform3fv(location=%d, count=%d, value=%s)", location, count, toStringVals(count * 3, value).c_str());
        glUniform3fv(location, count, value);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void uniform4fv(GLint location, GLsizei count, const GLfloat* value) const {
        SNAP_RHI_GL_LOG(
            "glUniform4fv(location=%d, count=%d, value=%s)", location, count, toStringVals(count * 4, value).c_str());
        glUniform4fv(location, count, value);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void uniform1f(GLint location, GLfloat v0) const {
        SNAP_RHI_GL_LOG("glUniform1f(location=%d, v0=%f)", location, v0);
        glUniform1f(location, v0);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void uniform2f(GLint location, GLfloat v0, GLfloat v1) const {
        SNAP_RHI_GL_LOG("glUniform2f(location=%d, v0=%f, v1=%f)", location, v0, v1);
        glUniform2f(location, v0, v1);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void uniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2) const {
        SNAP_RHI_GL_LOG("glUniform3f(location=%d, v0=%f, v1=%f, v2=%f)", location, v0, v1, v2);
        glUniform3f(location, v0, v1, v2);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void uniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) const {
        SNAP_RHI_GL_LOG("glUniform4f(location=%d, v0=%f, v1=%f, v2=%f, v3=%f)", location, v0, v1, v2, v3);
        glUniform4f(location, v0, v1, v2, v3);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void uniformMatrix2fv(GLint location,
                                                 GLsizei count,
                                                 GLboolean transpose,
                                                 const GLfloat* value) const {
        SNAP_RHI_GL_LOG("glUniformMatrix2fv(location=%d, count=%d, transpose=%d, value=%s)",
                        location,
                        count,
                        transpose,
                        toStringVals(count * 4, value).c_str());
        glUniformMatrix2fv(location, count, transpose, value);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void uniformMatrix3fv(GLint location,
                                                 GLsizei count,
                                                 GLboolean transpose,
                                                 const GLfloat* value) const {
        SNAP_RHI_GL_LOG("glUniformMatrix3fv(location=%d, count=%d, transpose=%d, value=%s)",
                        location,
                        count,
                        transpose,
                        toStringVals(count * 9, value).c_str());
        glUniformMatrix3fv(location, count, transpose, value);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void uniformMatrix4fv(GLint location,
                                                 GLsizei count,
                                                 GLboolean transpose,
                                                 const GLfloat* value) const {
        SNAP_RHI_GL_LOG("glUniformMatrix4fv(location=%d, count=%d, transpose=%d, value=%s)",
                        location,
                        count,
                        transpose,
                        toStringVals(count * 16, value).c_str());
        glUniformMatrix4fv(location, count, transpose, value);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void copyTexSubImage2D(snap::rhi::backend::opengl::TextureTarget target,
                                                  GLint level,
                                                  GLint xoffset,
                                                  GLint yoffset,
                                                  GLint x,
                                                  GLint y,
                                                  GLsizei width,
                                                  GLsizei height) const {
        SNAP_RHI_GL_LOG(
            "glCopyTexSubImage2D(target=%s, level=%d, xoffset=%d, yoffset=%d, x=%d, y=%d, width=%d, height=%d)",
            toString(target).c_str(),
            level,
            xoffset,
            yoffset,
            x,
            y,
            width,
            height);
        glCopyTexSubImage2D(static_cast<GLenum>(target), level, xoffset, yoffset, x, y, width, height);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void copyTexSubImage3D(snap::rhi::backend::opengl::TextureTarget target,
                                                  GLint level,
                                                  GLint xoffset,
                                                  GLint yoffset,
                                                  GLint zoffset,
                                                  GLint x,
                                                  GLint y,
                                                  GLsizei width,
                                                  GLsizei height) const {
        SNAP_RHI_GL_LOG(
            "glCopyTexSubImage3D(target=%s, level=%d, xoffset=%d, yoffset=%d, zoffset=%d, x=%d, y=%d, width=%d, "
            "height=%d)",
            toString(target).c_str(),
            level,
            xoffset,
            yoffset,
            zoffset,
            x,
            y,
            width,
            height);
        gl.copyTexSubImage3D(static_cast<GLenum>(target), level, xoffset, yoffset, zoffset, x, y, width, height);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void readPixels(GLint x,
                                           GLint y,
                                           GLsizei width,
                                           GLsizei height,
                                           snap::rhi::backend::opengl::FormatGroup format,
                                           snap::rhi::backend::opengl::FormatDataType type,
                                           void* data) const {
        SNAP_RHI_GL_LOG("glReadPixels(x=%d, y=%d, width=%d, height=%d, format=%s, type=%s, data=%p)",
                        x,
                        y,
                        width,
                        height,
                        toString(format).c_str(),
                        toString(type).c_str(),
                        data);
        glReadPixels(x, y, width, height, static_cast<GLenum>(format), static_cast<GLenum>(type), data);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void vertexAttrib4fv(GLuint index, const GLfloat* v) const {
        SNAP_RHI_GL_LOG("glVertexAttrib4fv(index=%u, v=%s)", index, toStringVals(4, v).c_str());
        glVertexAttrib4fv(index, v);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void polygonOffset(GLfloat factor, GLfloat units, DeviceContext* dc) const {
        if (dc) {
            GLStateCache& cache = dc->getGLStateCache();
            if (!cache.shouldPolygonOffset(factor, units)) {
                return;
            }
        }

        SNAP_RHI_GL_LOG("glPolygonOffset(factor=%f, units=%f)", factor, units);
        glPolygonOffset(factor, units);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void pixelStorei(GLenum pname, GLint param) const {
        SNAP_RHI_GL_LOG("glPixelStorei(pname=%s, param=%d)", toString(pname).c_str(), param);
        SNAP_RHI_VALIDATE(
            validationLayer,
            (pname != GL_PACK_ROW_LENGTH && pname != GL_UNPACK_ROW_LENGTH && pname != GL_UNPACK_IMAGE_HEIGHT) ||
                param == 0 || gl.features.isCustomTexturePackingSupported,
            snap::rhi::ReportLevel::Error,
            snap::rhi::ValidationTag::TextureOp,
            "[pixelStorei] pname{%d} is unsupported pixelStore option",
            static_cast<uint32_t>(pname));

        gl.pixelStorei(pname, param);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void texParameteri(snap::rhi::backend::opengl::TextureTarget target,
                                              GLenum pname,
                                              GLint param) const {
        SNAP_RHI_GL_LOG("glTexParameteri(target=%s, pname=%s, param=%s)",
                        toString(target).c_str(),
                        toString(pname).c_str(),
                        toString(param).c_str());
        SNAP_RHI_VALIDATE(validationLayer,
                          pname != GL_CLAMP_TO_BORDER || gl.features.isClampToBorderSupported,
                          snap::rhi::ReportLevel::Error,
                          snap::rhi::ValidationTag::TextureOp,
                          "[texParameteri] GL_CLAMP_TO_BORDER is unsupported");

        glTexParameteri(static_cast<GLenum>(target), pname, param);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void framebufferTexture2D(snap::rhi::backend::opengl::FramebufferTarget target,
                                                     snap::rhi::backend::opengl::FramebufferAttachmentTarget attachment,
                                                     snap::rhi::backend::opengl::TextureTarget textarget,
                                                     snap::rhi::backend::opengl::TextureId texture,
                                                     GLint level) const {
        SNAP_RHI_GL_LOG("glFramebufferTexture2D(target=%s, attachment=%s, textarget=%s, texture=%u, level=%d)",
                        toString(target).c_str(),
                        toString(attachment).c_str(),
                        toString(textarget).c_str(),
                        static_cast<GLuint>(texture),
                        level);
        SNAP_RHI_VALIDATE(validationLayer,
                          level == 0 || gl.features.isFBORenderToNonZeroMipSupported,
                          snap::rhi::ReportLevel::Error,
                          snap::rhi::ValidationTag::FramebufferOp,
                          "[framebufferTexture2D] can't use non zero level");

        if (attachment == snap::rhi::backend::opengl::FramebufferAttachmentTarget::DepthStencilAttachment &&
            !gl.features.isDepthStencilAttachmentSupported) {
            /**
             * If DepthStencilAttachment is not supported(OpenGL ES 2.0),
             * a separate attachment must be used.
             *
             * https://docs.gl/es2/glFramebufferTexture2D
             **/
            glFramebufferTexture2D(static_cast<GLenum>(target),
                                   GL_DEPTH_ATTACHMENT,
                                   static_cast<GLenum>(textarget),
                                   static_cast<GLuint>(texture),
                                   level);
            glFramebufferTexture2D(static_cast<GLenum>(target),
                                   GL_STENCIL_ATTACHMENT,
                                   static_cast<GLenum>(textarget),
                                   static_cast<GLuint>(texture),
                                   level);
        } else {
            glFramebufferTexture2D(static_cast<GLenum>(target),
                                   static_cast<GLenum>(attachment),
                                   static_cast<GLenum>(textarget),
                                   static_cast<GLuint>(texture),
                                   level);
        }
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void texSubImage3D(snap::rhi::backend::opengl::TextureTarget target,
                                              GLint level,
                                              GLint xoffset,
                                              GLint yoffset,
                                              GLint zoffset,
                                              GLsizei width,
                                              GLsizei height,
                                              GLsizei depth,
                                              snap::rhi::backend::opengl::FormatGroup format,
                                              snap::rhi::backend::opengl::FormatDataType type,
                                              const void* pixels) const {
        SNAP_RHI_GL_LOG(
            "glTexSubImage3D(target=%s, level=%d, xoffset=%d, yoffset=%d, zoffset=%d, width=%d, height=%d, depth=%d, "
            "format=%s, type=%s, pixels=%p)",
            toString(target).c_str(),
            level,
            xoffset,
            yoffset,
            zoffset,
            width,
            height,
            depth,
            toString(format).c_str(),
            toString(type).c_str(),
            pixels);
        SNAP_RHI_VALIDATE(validationLayer,
                          gl.features.isTexture3DSupported,
                          snap::rhi::ReportLevel::Error,
                          snap::rhi::ValidationTag::TextureOp,
                          "[texSubImage3D] 3D texture doesn't supported");

        gl.texSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void texImage3D(snap::rhi::backend::opengl::TextureTarget target,
                                           GLint level,
                                           snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                                           GLsizei width,
                                           GLsizei height,
                                           GLsizei depth,
                                           GLint border,
                                           snap::rhi::backend::opengl::FormatGroup format,
                                           snap::rhi::backend::opengl::FormatDataType type,
                                           const void* data) const {
        SNAP_RHI_GL_LOG(
            "glTexImage3D(target=%s, level=%d, internalformat=%s, width=%d, height=%d, depth=%d, border=%d, "
            "format=%s, type=%s, data=%p)",
            toString(target).c_str(),
            level,
            toString(internalformat).c_str(),
            width,
            height,
            depth,
            border,
            toString(format).c_str(),
            toString(type).c_str(),
            data);
        gl.texImage3D(target, level, internalformat, width, height, depth, border, format, type, data);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

#if SNAP_RHI_PLATFORM_WEBASSEMBLY()
    SNAP_RHI_ALWAYS_INLINE void texSubImage2D(snap::rhi::backend::opengl::TextureTarget target,
                                              GLint level,
                                              GLint xoffset,
                                              GLint yoffset,
                                              GLsizei width,
                                              GLsizei height,
                                              snap::rhi::backend::opengl::FormatGroup format,
                                              snap::rhi::backend::opengl::FormatDataType type,
                                              const emscripten::val& pixels) const {
        SNAP_RHI_GL_LOG(
            "texSubImage2D(target=%s, level=%d, xoffset=%d, yoffset=%d, width=%d, height=%d, format=%s, type=%s, "
            "pixels=<emscripten::val>)",
            toString(target).c_str(),
            level,
            xoffset,
            yoffset,
            width,
            height,
            toString(format).c_str(),
            toString(type).c_str());
        auto glCtx = emscripten::val::module_property("ctx");
        glCtx.call<void>("texSubImage2D",
                         getTextureTargetWebGl(static_cast<GLenum>(target)),
                         level,
                         xoffset,
                         yoffset,
                         width,
                         height,
                         getFormatGroupWebGl(static_cast<GLenum>(format)),
                         getFormatDataTypeWebGl(static_cast<GLenum>(type)),
                         pixels);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void texImage2D(snap::rhi::backend::opengl::TextureTarget target,
                                           GLint level,
                                           snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                                           GLsizei width,
                                           GLsizei height,
                                           GLint border,
                                           snap::rhi::backend::opengl::FormatGroup format,
                                           snap::rhi::backend::opengl::FormatDataType type,
                                           const emscripten::val& pixels) const {
        SNAP_RHI_GL_LOG(
            "texImage2D(target=%s, level=%d, internalformat=%s, width=%d, height=%d, border=%d, format=%s, type=%s",
            toString(target).c_str(),
            level,
            toString(internalformat).c_str(),
            width,
            height,
            border,
            toString(format).c_str(),
            toString(type).c_str());
        auto glCtx = emscripten::val::module_property("ctx");
        glCtx.call<void>("texImage2D",
                         getTextureTargetWebGl(static_cast<GLenum>(target)),
                         level,
                         getSizedInternalFormat(static_cast<GLenum>(internalformat)),
                         width,
                         height,
                         border,
                         getFormatGroupWebGl(static_cast<GLenum>(format)),
                         getFormatDataTypeWebGl(static_cast<GLenum>(type)),
                         pixels);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void texSubImage3D(snap::rhi::backend::opengl::TextureTarget target,
                                              GLint level,
                                              GLint xoffset,
                                              GLint yoffset,
                                              GLint zoffset,
                                              GLsizei width,
                                              GLsizei height,
                                              GLsizei depth,
                                              snap::rhi::backend::opengl::FormatGroup format,
                                              snap::rhi::backend::opengl::FormatDataType type,
                                              const emscripten::val& pixels) const {
        SNAP_RHI_GL_LOG(
            "texSubImage3D(target=%s, level=%d, xoffset=%d, yoffset=%d, zoffset=%d, width=%d, height=%d, depth=%d, "
            "format=%s, type=%s, pixels=<emscripten::val>)",
            toString(target).c_str(),
            level,
            xoffset,
            yoffset,
            zoffset,
            width,
            height,
            depth,
            toString(format).c_str(),
            toString(type).c_str());
        SNAP_RHI_VALIDATE(validationLayer,
                          gl.features.isTexture3DSupported,
                          snap::rhi::ReportLevel::Error,
                          snap::rhi::ValidationTag::TextureOp,
                          "[texSubImage3D] 3D texture doesn't supported");
        auto glCtx = emscripten::val::module_property("ctx");
        glCtx.call<void>("texSubImage3D",
                         getTextureTargetWebGl(static_cast<GLenum>(target)),
                         level,
                         xoffset,
                         yoffset,
                         zoffset,
                         width,
                         height,
                         depth,
                         getFormatGroupWebGl(static_cast<GLenum>(format)),
                         getFormatDataTypeWebGl(static_cast<GLenum>(type)),
                         pixels);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void texImage3D(snap::rhi::backend::opengl::TextureTarget target,
                                           GLint level,
                                           snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                                           GLsizei width,
                                           GLsizei height,
                                           GLsizei depth,
                                           GLint border,
                                           snap::rhi::backend::opengl::FormatGroup format,
                                           snap::rhi::backend::opengl::FormatDataType type,
                                           const emscripten::val& pixels) const {
        SNAP_RHI_GL_LOG(
            "texImage3D(target=%s, level=%d, internalformat=%s, width=%d, height=%d, depth=%d, border=%d, format=%s, "
            "type=%s)",
            toString(target).c_str(),
            level,
            toString(internalformat).c_str(),
            width,
            height,
            depth,
            border,
            toString(format).c_str(),
            toString(type).c_str());
        auto glCtx = emscripten::val::module_property("ctx");
        glCtx.call<void>("texImage3D",
                         getTextureTargetWebGl(static_cast<GLenum>(target)),
                         level,
                         getSizedInternalFormat(static_cast<GLenum>(internalformat)),
                         width,
                         height,
                         depth,
                         border,
                         getFormatGroupWebGl(static_cast<GLenum>(format)),
                         getFormatDataTypeWebGl(static_cast<GLenum>(type)),
                         pixels);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }
#endif

    SNAP_RHI_ALWAYS_INLINE void texStorage2D(snap::rhi::backend::opengl::TextureTarget target,
                                             GLsizei levels,
                                             snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                                             GLsizei width,
                                             GLsizei height) const {
        SNAP_RHI_GL_LOG("glTexStorage2D(target=%s, levels=%d, internalformat=%s, width=%d, height=%d)",
                        toString(target).c_str(),
                        levels,
                        toString(internalformat).c_str(),
                        width,
                        height);
        gl.texStorage2D(target, levels, internalformat, width, height);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void texStorage3D(snap::rhi::backend::opengl::TextureTarget target,
                                             GLsizei levels,
                                             snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                                             GLsizei width,
                                             GLsizei height,
                                             GLsizei depth) const {
        SNAP_RHI_GL_LOG("glTexStorage3D(target=%s, levels=%d, internalformat=%s, width=%d, height=%d, depth=%d)",
                        toString(target).c_str(),
                        levels,
                        toString(internalformat).c_str(),
                        width,
                        height,
                        depth);
        gl.texStorage3D(target, levels, internalformat, width, height, depth);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void texStorage2DMultisample(snap::rhi::backend::opengl::TextureTarget target,
                                                        GLsizei samples,
                                                        snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                                                        GLsizei width,
                                                        GLsizei height,
                                                        GLboolean fixedsamplelocations) const {
        SNAP_RHI_GL_LOG("glTexStorage2DMultisample(target=%s, samples=%d, internalformat=%s, width=%d, height=%d, "
                        "fixedsamplelocations=%d)",
                        toString(target).c_str(),
                        samples,
                        toString(internalformat).c_str(),
                        width,
                        height,
                        fixedsamplelocations);
        gl.texStorage2DMultisample(target, samples, internalformat, width, height, fixedsamplelocations);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void texStorage3DMultisample(snap::rhi::backend::opengl::TextureTarget target,
                                                        GLsizei samples,
                                                        snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                                                        GLsizei width,
                                                        GLsizei height,
                                                        GLsizei depth,
                                                        GLboolean fixedsamplelocations) const {
        SNAP_RHI_GL_LOG(
            "glTexStorage3DMultisample(target=%s, samples=%d, internalformat=%s, width=%d, height=%d, depth=%d, "
            "fixedsamplelocations=%d)",
            toString(target).c_str(),
            samples,
            toString(internalformat).c_str(),
            width,
            height,
            depth,
            fixedsamplelocations);
        gl.texStorage3DMultisample(target, samples, internalformat, width, height, depth, fixedsamplelocations);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void copyImageSubData(GLuint srcName,
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
                                                 GLsizei srcDepth) const {
        SNAP_RHI_GL_LOG(
            "glCopyImageSubData(srcName=%u, srcTarget=%s, srcLevel=%d, srcX=%d, srcY=%d, srcZ=%d, dstName=%u, "
            "dstTarget=%s, dstLevel=%d, dstX=%d, dstY=%d, dstZ=%d, srcWidth=%d, srcHeight=%d, srcDepth=%d)",
            srcName,
            toString(srcTarget).c_str(),
            srcLevel,
            srcX,
            srcY,
            srcZ,
            dstName,
            toString(dstTarget).c_str(),
            dstLevel,
            dstX,
            dstY,
            dstZ,
            srcWidth,
            srcHeight,
            srcDepth);
        gl.copyImageSubData(srcName,
                            srcTarget,
                            srcLevel,
                            srcX,
                            srcY,
                            srcZ,
                            dstName,
                            dstTarget,
                            dstLevel,
                            dstX,
                            dstY,
                            dstZ,
                            srcWidth,
                            srcHeight,
                            srcDepth);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void renderbufferStorageMultisample(
        snap::rhi::backend::opengl::TextureTarget target,
        GLsizei samples,
        snap::rhi::backend::opengl::SizedInternalFormat internalformat,
        GLsizei width,
        GLsizei height) const {
        SNAP_RHI_GL_LOG(
            "glRenderbufferStorageMultisample(target=%s, samples=%d, internalformat=%s, width=%d, height=%d)",
            toString(target).c_str(),
            samples,
            toString(internalformat).c_str(),
            width,
            height);
        gl.renderbufferStorageMultisample(target, samples, internalformat, width, height);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void drawBuffers(GLsizei n,
                                            const snap::rhi::backend::opengl::FramebufferAttachmentTarget* bufs) const {
        SNAP_RHI_GL_LOG("glDrawBuffers(n=%d, bufs=%s)", n, toStringEnums(n, bufs).c_str());
        gl.drawBuffers(n, bufs);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void readBuffer(snap::rhi::backend::opengl::FramebufferAttachmentTarget mode) const {
        SNAP_RHI_GL_LOG("glReadBuffer(mode=%s)", toString(mode).c_str());
        gl.readBuffer(mode);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void getActiveUniformBlockName(
        GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei* length, GLchar* uniformBlockName) const {
        gl.getActiveUniformBlockName(program, uniformBlockIndex, bufSize, length, uniformBlockName);
        SNAP_RHI_GL_LOG(
            "glGetActiveUniformBlockName(program=%u, uniformBlockIndex=%u, bufSize=%d, length=%s, uniformBlockName=%s)",
            program,
            uniformBlockIndex,
            bufSize,
            toStringVals(1, length).c_str(),
            uniformBlockName);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    GLuint getUniformBlockIndex(GLuint program, const GLchar* uniformBlockName) const {
        GLuint result = gl.getUniformBlockIndex(program, uniformBlockName);
        SNAP_RHI_GL_LOG(
            "glGetUniformBlockIndex(program=%u, uniformBlockName=%s) -> %u", program, uniformBlockName, result);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
        return result;
    }

    SNAP_RHI_ALWAYS_INLINE void getActiveUniformBlockiv(GLuint program,
                                                        GLuint uniformBlockIndex,
                                                        GLenum pname,
                                                        GLint* params) const {
        gl.getActiveUniformBlockiv(program, uniformBlockIndex, pname, params);
        SNAP_RHI_GL_LOG("glGetActiveUniformBlockiv(program=%u, uniformBlockIndex=%u, pname=%s, params=%s)",
                        program,
                        uniformBlockIndex,
                        toString(pname).c_str(),
                        toStringVals(1, params).c_str());
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void getActiveUniformsiv(
        GLuint program, GLsizei uniformCount, const GLuint* uniformIndices, GLenum pname, GLint* params) const {
        gl.getActiveUniformsiv(program, uniformCount, uniformIndices, pname, params);
        SNAP_RHI_GL_LOG("glGetActiveUniformsiv(program=%u, uniformCount=%d, uniformIndices=%s, pname=%s, params=%s)",
                        program,
                        uniformCount,
                        toStringVals(uniformCount, uniformIndices).c_str(),
                        toString(pname).c_str(),
                        toStringVals(uniformCount, params).c_str());
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void uniformBlockBinding(GLuint program,
                                                    GLuint uniformBlockIndex,
                                                    GLuint uniformBlockBinding) const {
        SNAP_RHI_GL_LOG("glUniformBlockBinding(program=%u, uniformBlockIndex=%u, uniformBlockBinding=%u)",
                        program,
                        uniformBlockIndex,
                        uniformBlockBinding);
        gl.uniformBlockBinding(program, uniformBlockIndex, uniformBlockBinding);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void bindImageTexture(
        GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format) const {
        SNAP_RHI_GL_LOG("glBindImageTexture(unit=%u, texture=%u, level=%d, layered=%d, layer=%d, access=%s, format=%s)",
                        unit,
                        texture,
                        level,
                        layered,
                        layer,
                        toString(access).c_str(),
                        toString(format).c_str());
        gl.bindImageTexture(unit, texture, level, layered, layer, access, format);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void getProgramResourceiv(GLuint program,
                                                     GLenum programInterface,
                                                     GLuint index,
                                                     GLsizei propCount,
                                                     const GLenum* props,
                                                     GLsizei bufSize,
                                                     GLsizei* length,
                                                     GLint* params) const {
        gl.getProgramResourceiv(program, programInterface, index, propCount, props, bufSize, length, params);
        SNAP_RHI_GL_LOG(
            "glGetProgramResourceiv(program=%u, programInterface=%s, index=%u, propCount=%d, props=%s, bufSize=%d, "
            "length=%p, params=%p)",
            program,
            toString(programInterface).c_str(),
            index,
            propCount,
            toStringEnums(propCount, props).c_str(),
            bufSize,
            length,
            params);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void getProgramInterfaceiv(GLuint program,
                                                      GLenum programInterface,
                                                      GLenum pname,
                                                      GLint* params) const {
        gl.getProgramInterfaceiv(program, programInterface, pname, params);
        SNAP_RHI_GL_LOG("glGetProgramInterfaceiv(program=%u, programInterface=%s, pname=%s, params=%s)",
                        program,
                        toString(programInterface).c_str(),
                        toString(pname).c_str(),
                        toStringVals(1, params).c_str());
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void getProgramResourceName(
        GLuint program, GLenum programInterface, GLuint index, GLsizei bufSize, GLsizei* length, char* name) const {
        gl.getProgramResourceName(program, programInterface, index, bufSize, length, name);
        SNAP_RHI_GL_LOG(
            "glGetProgramResourceName(program=%u, programInterface=%s, index=%u, bufSize=%d, length=%s, name=%p)",
            program,
            toString(programInterface).c_str(),
            index,
            bufSize,
            toStringVals(1, length).c_str(),
            name);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    GLuint getProgramResourceIndex(GLuint program, GLenum programInterface, const char* name) const {
        GLuint result = gl.getProgramResourceIndex(program, programInterface, name);
        SNAP_RHI_GL_LOG("glGetProgramResourceIndex(program=%u, programInterface=%s, name=\"%s\") -> %u",
                        program,
                        toString(programInterface).c_str(),
                        name,
                        result);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
        return result;
    }

    SNAP_RHI_GLsync fenceSync(GLenum condition, GLbitfield flags) const {
        GLsync result = nullptr;
        if (gl.features.isFenceSupported) {
            result = static_cast<GLsync>(gl.fenceSync(condition, flags));
            SNAP_RHI_GL_LOG("glFenceSync(condition=%s, flags=0x%x) -> %p", toString(condition).c_str(), flags, result);
        } else {
            SNAP_RHI_GL_LOG("glFenceSync(...) -> glFinish()");
            finish();
        }
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
        return result;
    }

    SNAP_RHI_ALWAYS_INLINE void waitSync(SNAP_RHI_GLsync sync, GLbitfield flags, GLuint64 timeout) const {
        if (gl.features.isSemaphoreSupported) {
            SNAP_RHI_GL_LOG("glWaitSync(sync=%p, flags=0x%x, timeout=%" PRIu64 ")", sync, flags, timeout);
            gl.waitSync(sync, flags, timeout);
        } else {
            SNAP_RHI_GL_LOG("glWaitSync(...) -> glFinish()");
            finish();
        }
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    GLenum clientWaitSync(SNAP_RHI_GLsync sync, GLbitfield flags, GLuint64 timeout) const {
        GLenum status = 0;
        if (gl.features.isFenceSupported) {
            status = gl.clientWaitSync(sync, flags, timeout);
            SNAP_RHI_GL_LOG("glClientWaitSync(sync=%p, flags=0x%x, timeout=%" PRIu64 ")", sync, flags, timeout);
            SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                               snap::rhi::ValidationTag::GLProfileOp,
                                               [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
        } else {
            status = GL_CONDITION_SATISFIED;
            SNAP_RHI_GL_LOG("glClientWaitSync(...) -> glFinish()");
            finish();
        }
        return status;
    }

    SNAP_RHI_ALWAYS_INLINE void deleteSync(SNAP_RHI_GLsync sync) const {
        if (gl.features.isFenceSupported) {
            SNAP_RHI_GL_LOG("glDeleteSync(sync=%p)", sync);
            gl.deleteSync(sync);
        }
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void getInternalformativ(snap::rhi::backend::opengl::TextureTarget target,
                                                    snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                                                    GLenum pname,
                                                    GLsizei bufSize,
                                                    GLint* params) const {
        if (!gl.features.isMultisampleSupported) {
            if (pname == GL_NUM_SAMPLE_COUNTS && bufSize) {
                *params = 1;
            }

            if (pname == GL_SAMPLES && bufSize) {
                *params = 0;
            }
        } else {
            gl.getInternalformativ(target, internalformat, pname, bufSize, params);
            SNAP_RHI_GL_LOG(
                "glGetInternalformativ(target=%s, internalformat=%s, pname=%s, bufSize=%d, params=%s)",
                toString(target).c_str(),
                toString(internalformat).c_str(),
                toString(pname).c_str(),
                bufSize,
                // https://registry.khronos.org/OpenGL-Refpages/es3/html/glGetInternalformativ.xhtml
                // bufSize - Specifies the maximum number of integers that may be written to params by the function
                toStringVals(bufSize, params).c_str());
            SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                               snap::rhi::ValidationTag::GLProfileOp,
                                               [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
        }
    }

    SNAP_RHI_ALWAYS_INLINE void memoryBarrierByRegion(GLbitfield barriers) const {
        SNAP_RHI_GL_LOG("glMemoryBarrierByRegion(barriers=0x%x)", barriers);
        gl.memoryBarrierByRegion(barriers);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void discardFramebuffer(
        snap::rhi::backend::opengl::FramebufferTarget target,
        GLsizei numAttachments,
        const snap::rhi::backend::opengl::FramebufferAttachmentTarget* attachments) const {
        if (!gl.features.isDiscardFramebufferSupported) {
            return;
        }

        SNAP_RHI_GL_LOG("glInvalidateFramebuffer(target=%s, numAttachments=%d, attachments=%s)",
                        toString(target).c_str(),
                        numAttachments,
                        toStringEnums(numAttachments, attachments).c_str());
        gl.discardFramebuffer(target, numAttachments, attachments);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void genSamplers(GLsizei n, GLuint* samplers) const {
        gl.genSamplers(n, samplers);
        SNAP_RHI_GL_LOG("glGenSamplers(n=%d, samplers=%s)", n, toStringVals(n, samplers).c_str());
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void deleteSamplers(GLsizei n, const GLuint* samplers) const {
        SNAP_RHI_GL_LOG("glDeleteSamplers(n=%d, samplers=%s)", n, toStringVals(n, samplers).c_str());
        gl.deleteSamplers(n, samplers);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void bindSampler(GLuint unit, GLuint sampler, DeviceContext* dc) const {
        if (dc) {
            GLStateCache& cache = dc->getGLStateCache();
            if (!cache.shouldBindSampler(unit, sampler)) {
                return;
            }
        }

        SNAP_RHI_GL_LOG("glBindSampler(unit=%u, sampler=%u)", unit, sampler);
        gl.bindSampler(unit, sampler);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void samplerParameterf(GLuint sampler, GLenum pname, GLfloat param) const {
        SNAP_RHI_GL_LOG("glSamplerParameterf(sampler=%u, pname=%s, param=%f)", sampler, toString(pname).c_str(), param);
        gl.samplerParameterf(sampler, pname, param);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void samplerParameteri(GLuint sampler, GLenum pname, GLint param) const {
        SNAP_RHI_GL_LOG("glSamplerParameteri(sampler=%u, pname=%s, param=%d)", sampler, toString(pname).c_str(), param);
        gl.samplerParameteri(sampler, pname, param);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void samplerParameterfv(GLuint sampler, GLenum pname, const GLfloat* params) const {
        SNAP_RHI_GL_LOG("glSamplerParameterfv(sampler=%u, pname=%s, params=%s)",
                        sampler,
                        toString(pname).c_str(),
                        toStringVals(1, params).c_str());
        gl.samplerParameterfv(sampler, pname, params);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    GLboolean isSampler(GLuint id) const {
        SNAP_RHI_GL_LOG("glIsSampler(id=%u)", id);
        GLboolean result = gl.isSampler(id);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
        return result;
    }

    SNAP_RHI_ALWAYS_INLINE void copyBufferSubData(
        GLenum readtarget, GLenum writetarget, GLintptr readoffset, GLintptr writeoffset, GLsizeiptr size) const {
        SNAP_RHI_GL_LOG("glCopyBufferSubData(readtarget=%s, writetarget=%s, readoffset=%ld, writeoffset=%ld, size=%ld)",
                        toString(readtarget).c_str(),
                        toString(writetarget).c_str(),
                        readoffset,
                        writeoffset,
                        size);
        gl.copyBufferSubData(readtarget, writetarget, readoffset, writeoffset, size);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void blitFramebuffer(GLint srcX0,
                                                GLint srcY0,
                                                GLint srcX1,
                                                GLint srcY1,
                                                GLint dstX0,
                                                GLint dstY0,
                                                GLint dstX1,
                                                GLint dstY1,
                                                GLbitfield mask,
                                                GLenum filter) const {
        SNAP_RHI_GL_LOG(
            "glBlitFramebuffer(srcX0=%d, srcY0=%d, srcX1=%d, srcY1=%d, dstX0=%d, dstY0=%d, dstX1=%d, dstY1=%d, "
            "mask=0x%x, filter=%s)",
            srcX0,
            srcY0,
            srcX1,
            srcY1,
            dstX0,
            dstY0,
            dstX1,
            dstY1,
            mask,
            toString(filter).c_str());
        gl.blitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void genVertexArrays(GLsizei n, GLuint* arrays) const {
        gl.genVertexArrays(n, arrays);
        SNAP_RHI_GL_LOG("glGenVertexArrays(n=%d, arrays=%s)", n, toStringVals(n, arrays).c_str());
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void bindVertexArray(GLuint array) const {
        SNAP_RHI_GL_LOG("glBindVertexArray(array=%u)", array);
        gl.bindVertexArray(array);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void deleteVertexArrays(GLsizei n, const GLuint* arrays) const {
        SNAP_RHI_GL_LOG("glDeleteVertexArrays(n=%d, arrays=%s)", n, toStringVals(n, arrays).c_str());
        gl.deleteVertexArrays(n, arrays);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void clearBufferiv(GLenum buffer, GLint drawBuffer, const GLint* value) const {
        SNAP_RHI_GL_LOG("glClearBufferiv(buffer=%s, drawBuffer=%d, value=%s)",
                        toString(buffer).c_str(),
                        drawBuffer,
                        toStringVals(1, value).c_str());
        gl.clearBufferiv(buffer, drawBuffer, value);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void clearBufferuiv(GLenum buffer, GLint drawBuffer, const GLuint* value) const {
        SNAP_RHI_GL_LOG("glClearBufferuiv(buffer=%s, drawBuffer=%d, value=%s)",
                        toString(buffer).c_str(),
                        drawBuffer,
                        toStringVals(1, value).c_str());
        gl.clearBufferuiv(buffer, drawBuffer, value);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void clearBufferfv(GLenum buffer, GLint drawBuffer, const GLfloat* value) const {
        SNAP_RHI_GL_LOG("glClearBufferfv(buffer=%s, drawBuffer=%d, value=%s)",
                        toString(buffer).c_str(),
                        drawBuffer,
                        toStringVals(1, value).c_str());
        gl.clearBufferfv(buffer, drawBuffer, value);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void clearBufferfi(GLenum buffer, GLint drawBuffer, GLfloat depth, GLint stencil) const {
        SNAP_RHI_GL_LOG("glClearBufferfi(buffer=%s, drawBuffer=%d, depth=%f, stencil=%d)",
                        toString(buffer).c_str(),
                        drawBuffer,
                        depth,
                        stencil);
        gl.clearBufferfi(buffer, drawBuffer, depth, stencil);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void bindBufferRange(
        GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size, DeviceContext* dc) const {
        if (dc) {
            GLStateCache& cache = dc->getGLStateCache();
            if (!cache.shouldBindBufferRange(target)) {
                return;
            }
        }

        SNAP_RHI_GL_LOG("glBindBufferRange(target=%s, index=%u, buffer=%u, offset=%ld, size=%ld)",
                        toString(target).c_str(),
                        index,
                        buffer,
                        offset,
                        size);
        gl.bindBufferRange(target, index, buffer, offset, size);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void flushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length) const {
        SNAP_RHI_GL_LOG(
            "glFlushMappedBufferRange(target=%s, offset=%ld, length=%ld)", toString(target).c_str(), offset, length);
        gl.flushMappedBufferRange(target, offset, length);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    [[nodiscard]] SNAP_RHI_ALWAYS_INLINE void* mapBufferRange(GLenum target,
                                                              GLintptr offset,
                                                              GLsizeiptr length,
                                                              GLbitfield access) const {
        SNAP_RHI_GL_LOG("glMapBufferRange(target=%s, offset=%ld, length=%ld, access=0x%x)",
                        toString(target).c_str(),
                        offset,
                        length,
                        access);
        void* result = gl.mapBufferRange(target, offset, length, access);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
        return result;
    }

    GLboolean unmapBuffer(GLenum target) const {
        SNAP_RHI_GL_LOG("glUnmapBuffer(target=%s)", toString(target).c_str());
        GLboolean result = gl.unmapBuffer(target);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
        return result;
    }

    void texParameterfv(snap::rhi::backend::opengl::TextureTarget target, GLenum pname, const GLfloat* params) const {
        SNAP_RHI_GL_LOG("glTexParameterfv(target=%s, pname=%s, params=%s)",
                        toString(target).c_str(),
                        toString(pname).c_str(),
                        toStringVals(1, params).c_str());
        SNAP_RHI_VALIDATE(validationLayer,
                          gl.features.isClampToBorderSupported || pname != GL_CLAMP_TO_BORDER,
                          snap::rhi::ReportLevel::Error,
                          snap::rhi::ValidationTag::TextureOp,
                          "[texParameterfv] GL_CLAMP_TO_BORDER is unsupported");

        glTexParameterfv(static_cast<GLenum>(target), pname, params);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    void texParameterf(snap::rhi::backend::opengl::TextureTarget target, GLenum pname, GLfloat param) const {
        SNAP_RHI_GL_LOG(
            "glTexParameterf(target=%s, pname=%s, param=%f)", toString(target).c_str(), toString(pname).c_str(), param);
        SNAP_RHI_VALIDATE(validationLayer,
                          gl.features.isTexMinMaxLodSupported ||
                              !(pname == GL_TEXTURE_MIN_LOD || pname == GL_TEXTURE_MAX_LOD),
                          snap::rhi::ReportLevel::Error,
                          snap::rhi::ValidationTag::TextureOp,
                          "[texParameterf] GL_TEXTURE_MIN_LOD/GL_TEXTURE_MAX_LOD is unsupported");

        glTexParameterf(static_cast<GLenum>(target), pname, param);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    void blendEquationSeparate(GLenum modeRGB, GLenum modeAlpha, DeviceContext* dc) const {
        SNAP_RHI_VALIDATE(validationLayer,
                          gl.features.isMinMaxBlendModeSupported ||
                              !(modeRGB == GL_MIN || modeRGB == GL_MAX || modeAlpha == GL_MIN || modeAlpha == GL_MAX),
                          snap::rhi::ReportLevel::Error,
                          snap::rhi::ValidationTag::RenderPipelineOp,
                          "[blendEquationSeparate] GL_MIN and GL_MAX is unsupported");

        if (dc) {
            GLStateCache& cache = dc->getGLStateCache();
            if (!cache.shouldBlendEquationSeparate(modeRGB, modeAlpha)) {
                return;
            }
        }

        SNAP_RHI_GL_LOG("glBlendEquationSeparate(modeRGB=%s, modeAlpha=%s)",
                        toString(modeRGB).c_str(),
                        toString(modeAlpha).c_str());
        glBlendEquationSeparate(modeRGB, modeAlpha);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    void vertexAttribI4iv(GLuint index, const GLint* v) const {
        SNAP_RHI_GL_LOG("glVertexAttribI4iv(index=%u, v=%s)", index, toStringVals(4, v).c_str());
        gl.vertexAttribI4iv(index, v);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    void vertexAttribI4uiv(GLuint index, const GLuint* v) const {
        SNAP_RHI_GL_LOG("glVertexAttribI4uiv(index=%u, v=%s)", index, toStringVals(4, v).c_str());
        gl.vertexAttribI4uiv(index, v);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    void framebufferTextureLayer(snap::rhi::backend::opengl::FramebufferTarget target,
                                 snap::rhi::backend::opengl::FramebufferAttachmentTarget attachment,
                                 snap::rhi::backend::opengl::TextureId texture,
                                 int32_t level,
                                 int32_t layer) const {
        SNAP_RHI_GL_LOG("glFramebufferTextureLayer(target=%s, attachment=%s, texture=%u, level=%d, layer=%d)",
                        toString(target).c_str(),
                        toString(attachment).c_str(),
                        static_cast<GLuint>(texture),
                        level,
                        layer);
        gl.framebufferTextureLayer(target, attachment, texture, level, layer);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    void framebufferTextureMultiviewOVR(snap::rhi::backend::opengl::FramebufferTarget target,
                                        snap::rhi::backend::opengl::FramebufferAttachmentTarget attachment,
                                        snap::rhi::backend::opengl::TextureId texture,
                                        int32_t level,
                                        int32_t baseViewIndex,
                                        int32_t numViews) const {
        SNAP_RHI_GL_LOG(
            "glFramebufferTextureMultiviewOVR(target=%s, attachment=%s, texture=%u, level=%d, baseViewIndex=%d, "
            "numViews=%d)",
            toString(target).c_str(),
            toString(attachment).c_str(),
            static_cast<GLuint>(texture),
            level,
            baseViewIndex,
            numViews);
        gl.framebufferTextureMultiviewOVR(target, attachment, texture, level, baseViewIndex, numViews);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    void framebufferTextureMultisampleMultiviewOVR(snap::rhi::backend::opengl::FramebufferTarget target,
                                                   snap::rhi::backend::opengl::FramebufferAttachmentTarget attachment,
                                                   snap::rhi::backend::opengl::TextureId texture,
                                                   int32_t level,
                                                   int32_t samples,
                                                   int32_t baseViewIndex,
                                                   int32_t numViews) const {
        SNAP_RHI_GL_LOG(
            "glFramebufferTextureMultisampleMultiviewOVR(target=%s, attachment=%s, texture=%u, level=%d, samples=%d, "
            "baseViewIndex=%d, numViews=%d)",
            toString(target).c_str(),
            toString(attachment).c_str(),
            static_cast<GLuint>(texture),
            level,
            samples,
            baseViewIndex,
            numViews);
        gl.framebufferTextureMultisampleMultiviewOVR(
            target, attachment, texture, level, samples, baseViewIndex, numViews);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    void colorMaski(
        GLuint buf, GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha, DeviceContext* dc) const {
        if (dc) {
            GLStateCache& cache = dc->getGLStateCache();
            if (!cache.shouldColorMaski(buf, red, green, blue, alpha)) {
                return;
            }
        }
        SNAP_RHI_GL_LOG("glColorMaski(buf=%u, red=%d, green=%d, blue=%d, alpha=%d)", buf, red, green, blue, alpha);
        gl.colorMaski(buf, red, green, blue, alpha);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    void enablei(GLenum cap, GLuint index, DeviceContext* dc) const {
        if (!gl.features.isRasterizationDisableSupported && cap == GL_RASTERIZER_DISCARD) {
            return;
        }

        if (dc) {
            GLStateCache& cache = dc->getGLStateCache();
            if (!cache.shouldEnablei(cap, index)) {
                return;
            }
        }

        SNAP_RHI_GL_LOG("glEnablei(cap=%s, index=%u)", toString(cap).c_str(), index);
        gl.enablei(cap, index);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    void disablei(GLenum cap, GLuint index, DeviceContext* dc) const {
        if (!gl.features.isRasterizationDisableSupported && cap == GL_RASTERIZER_DISCARD) {
            return;
        }

        if (dc) {
            GLStateCache& cache = dc->getGLStateCache();
            if (!cache.shouldDisablei(cap, index)) {
                return;
            }
        }

        SNAP_RHI_GL_LOG("glDisablei(cap=%s, index=%u)", toString(cap).c_str(), index);
        gl.disablei(cap, index);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    void blendEquationSeparatei(GLuint buf, GLenum modeRGB, GLenum modeAlpha, DeviceContext* dc) const {
        if (dc) {
            GLStateCache& cache = dc->getGLStateCache();
            if (!cache.shouldBlendEquationSeparatei(buf, modeRGB, modeAlpha)) {
                return;
            }
        }

        SNAP_RHI_GL_LOG("glBlendEquationSeparatei(buf=%u, modeRGB=%s, modeAlpha=%s)",
                        buf,
                        toString(modeRGB).c_str(),
                        toString(modeAlpha).c_str());
        gl.blendEquationSeparatei(buf, modeRGB, modeAlpha);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    void blendFuncSeparatei(
        GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha, DeviceContext* dc) const {
        if (dc) {
            GLStateCache& cache = dc->getGLStateCache();
            if (!cache.shouldBlendFuncSeparatei(buf, srcRGB, dstRGB, srcAlpha, dstAlpha)) {
                return;
            }
        }

        SNAP_RHI_GL_LOG("glBlendFuncSeparatei(buf=%u, srcRGB=%s, dstRGB=%s, srcAlpha=%s, dstAlpha=%s)",
                        buf,
                        toString(srcRGB).c_str(),
                        toString(dstRGB).c_str(),
                        toString(srcAlpha).c_str(),
                        toString(dstAlpha).c_str());
        gl.blendFuncSeparatei(buf, srcRGB, dstRGB, srcAlpha, dstAlpha);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    void dispatchCompute(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z) const {
        SNAP_RHI_GL_LOG("glDispatchCompute(num_groups_x=%u, num_groups_y=%u, num_groups_z=%u)",
                        num_groups_x,
                        num_groups_y,
                        num_groups_z);
        gl.dispatchCompute(num_groups_x, num_groups_y, num_groups_z);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    void hint(GLenum target, GLenum mode) const {
        SNAP_RHI_GL_LOG("glHint(target=%s, mode=%s)", toString(target).c_str(), toString(mode).c_str());
        gl.hint(target, mode);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    void getTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint* params) const {
        gl.getTexLevelParameteriv(target, level, pname, params);
        SNAP_RHI_GL_LOG("glGetTexLevelParameteriv(target=%s, level=%d, pname=%s, params=%s)",
                        toString(target).c_str(),
                        level,
                        toString(pname).c_str(),
                        toStringVals(1, params).c_str());
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    void getTexParameteriv(GLenum target, GLenum pname, GLint* params) const {
        gl.getTexParameteriv(target, pname, params);
        SNAP_RHI_GL_LOG("glGetTexParameteriv(target=%s, pname=%s, params=%s)",
                        toString(target).c_str(),
                        toString(pname).c_str(),
                        toStringVals(1, params).c_str());
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    void validateProgram(GLuint program) const {
        SNAP_RHI_GL_LOG("glValidateProgram(program=%u)", program);
        glValidateProgram(program);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    void programParameteri(GLuint program, GLenum pname, GLint value) const {
        SNAP_RHI_GL_LOG("glProgramParameteri(program=%u, pname=%s, value=%d)", program, toString(pname).c_str(), value);
        if (!gl.features.isProgramBinarySupported) {
            snap::rhi::common::throwException("[programParameteri] ProgramBinary doesn't supported");
        }

        gl.programParameteri(program, pname, value);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    void programBinary(GLuint program, GLenum binaryFormat, const void* binary, GLsizei length) const {
        SNAP_RHI_GL_LOG("glProgramBinary(program=%u, binaryFormat=%s, binary=%p, length=%d)",
                        program,
                        toString(binaryFormat).c_str(),
                        binary,
                        length);
        if (!gl.features.isProgramBinarySupported) {
            snap::rhi::common::throwException("[programBinary] ProgramBinary doesn't supported");
        }

        gl.programBinary(program, binaryFormat, binary, length);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void getProgramBinary(
        GLuint program, GLsizei bufsize, GLsizei* length, GLenum* binaryFormat, void* binary) const {
        gl.getProgramBinary(program, bufsize, length, binaryFormat, binary);
        SNAP_RHI_GL_LOG("glGetProgramBinary(program=%u, bufsize=%d, length=%d, binaryFormat=%s, binary=%p)",
                        program,
                        bufsize,
                        length ? int(*length) : 0,
                        toString(*binaryFormat).c_str(),
                        binary);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void vertexAttrib(GLuint index,
                                             GLint size,
                                             GLenum type,
                                             GLboolean normalized,
                                             GLsizei stride,
                                             const void* pointer,
                                             GLuint buffer,
                                             DeviceContext* dc) const {
        if (dc) {
            GLStateCache& cache = dc->getGLStateCache();
            if (!cache.shouldVertexAttrib(index, size, type, normalized, stride, pointer, buffer)) {
                return;
            }
        }

        {
            bindBuffer(GL_ARRAY_BUFFER, buffer, dc);
            SNAP_RHI_GL_LOG(
                "glVertexAttribPointer(index=%u, size=%d, type=%s, normalized=%d, stride=%d, pointer=%p, buffer=%u)",
                index,
                size,
                toString(type).c_str(),
                normalized,
                stride,
                pointer,
                buffer);
            glVertexAttribPointer(index, size, type, normalized, stride, pointer);
        }
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void enableVertexAttribArray(GLuint index, DeviceContext* dc) const {
        if (dc) {
            GLStateCache& cache = dc->getGLStateCache();
            if (!cache.shouldEnableVertexAttribArray(index)) {
                return;
            }
        }

        SNAP_RHI_GL_LOG("glEnableVertexAttribArray(index=%u)", index);
        glEnableVertexAttribArray(index);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void disableVertexAttribArray(GLuint index, DeviceContext* dc) const {
        if (dc) {
            GLStateCache& cache = dc->getGLStateCache();
            if (!cache.shouldDisableVertexAttribArray(index)) {
                return;
            }
        }

        SNAP_RHI_GL_LOG("glDisableVertexAttribArray(index=%u)", index);
        glDisableVertexAttribArray(index);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void vertexAttribDivisor(GLuint index, GLuint divisor, DeviceContext* dc) const {
        if (dc) {
            GLStateCache& cache = dc->getGLStateCache();
            if (!cache.shouldVertexAttribDivisor(index, divisor)) {
                return;
            }
        }

        SNAP_RHI_GL_LOG("glVertexAttribDivisor(index=%u, divisor=%u)", index, divisor);
        gl.vertexAttribDivisor(index, divisor);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void drawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei primcount) const {
        SNAP_RHI_GL_LOG("glDrawArraysInstanced(mode=%s, first=%d, count=%d, primcount=%d)",
                        toString(mode).c_str(),
                        first,
                        count,
                        primcount);

        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
        gl.drawArraysInstanced(mode, first, count, primcount);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void drawElementsInstanced(
        GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instancecount) const {
        SNAP_RHI_GL_LOG("glDrawElementsInstanced(mode=%s, count=%d, type=%s, indices=%p, instancecount=%d)",
                        toString(mode).c_str(),
                        count,
                        toString(type).c_str(),
                        indices,
                        instancecount);

        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
        gl.drawElementsInstanced(mode, count, type, indices, instancecount);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void drawArrays(GLenum mode, GLint first, GLsizei count) const {
        SNAP_RHI_GL_LOG("glDrawArrays(mode=%s, first=%d, count=%d)", toString(mode).c_str(), first, count);

        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
        glDrawArrays(mode, first, count);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

    SNAP_RHI_ALWAYS_INLINE void drawElements(GLenum mode, GLsizei count, GLenum type, const void* indices) const {
        SNAP_RHI_GL_LOG("glDrawElements(mode=%s, count=%d, type=%s, indices=%p)",
                        toString(mode).c_str(),
                        count,
                        toString(type).c_str(),
                        indices);

        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
        glDrawElements(mode, count, type, indices);
        SNAP_RHI_VALIDATE_WITH_CUSTOM_FUNC(snap::rhi::ReportLevel::Error,
                                           snap::rhi::ValidationTag::GLProfileOp,
                                           [this]() { checkErrors(SNAP_RHI_SOURCE_LOCATION()); });
    }

private:
    void checkErrors(SNAP_RHI_SRC_LOCATION_TYPE srcLocation,
                     const snap::rhi::ValidationTag tags = snap::rhi::ValidationTag::GLProfileOp,
                     const snap::rhi::ReportLevel reportType = snap::rhi::ReportLevel::Error) const;
    std::string describe(snap::rhi::backend::opengl::FramebufferTarget target,
                         const snap::rhi::backend::opengl::FramebufferId fbo,
                         const snap::rhi::backend::opengl::FramebufferDescription& activeDescription) const;

    snap::rhi::backend::common::Device* device = nullptr;
    const snap::rhi::backend::common::ValidationLayer& validationLayer;

    snap::rhi::backend::opengl::GPUVendor vendor = snap::rhi::backend::opengl::GPUVendor::Unknown;
    snap::rhi::backend::opengl::GPUModel model = snap::rhi::backend::opengl::GPUModel::Unknown;

    gl::APIVersion nativeGLVersion = gl::APIVersion::None;
    Instance gl;
};
} // namespace snap::rhi::backend::opengl
