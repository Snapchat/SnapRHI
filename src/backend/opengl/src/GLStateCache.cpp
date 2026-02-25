#include "snap/rhi/backend/opengl/GLStateCache.hpp"
#include "snap/rhi/backend/opengl/Device.hpp"
#include "snap/rhi/backend/opengl/Profile.hpp"
#include "snap/rhi/common/OS.h"

namespace {
#if SNAP_RHI_ENABLE_SLOW_VALIDATIONS()
constexpr float CacheEpsilon = float(1e-3);

GLboolean isBlendingEnabed(const snap::rhi::Capabilities& capabilities, GLuint index) {
#if !(SNAP_RHI_OS_IOS() || SNAP_RHI_OS_IOS_SIMULATOR() || SNAP_RHI_PLATFORM_WEBASSEMBLY())
    if (capabilities.isMRTBlendSettingsDifferent) {
        return glIsEnabledi(GL_BLEND, index);
    }
#endif

    return glIsEnabled(GL_BLEND);
}

void getColorWriteMask(const snap::rhi::Capabilities& capabilities, GLuint index, GLboolean* data) {
    if (capabilities.isMRTBlendSettingsDifferent) {
#if !(SNAP_RHI_OS_IOS() || SNAP_RHI_OS_IOS_SIMULATOR() || SNAP_RHI_PLATFORM_WEBASSEMBLY())
        glGetBooleani_v(GL_COLOR_WRITEMASK, index, data);
#else
        glGetBooleanv(GL_COLOR_WRITEMASK, data);
#endif
    } else {
        glGetBooleanv(GL_COLOR_WRITEMASK, data);
    }
}

void getBlendEquation(const snap::rhi::Capabilities& capabilities, GLuint index, GLenum* alpha, GLenum* rgb) {
    if (capabilities.isMRTBlendSettingsDifferent) {
        glGetIntegeri_v(GL_BLEND_EQUATION_ALPHA, index, reinterpret_cast<GLint*>(alpha));
        glGetIntegeri_v(GL_BLEND_EQUATION_RGB, index, reinterpret_cast<GLint*>(rgb));
    } else {
        glGetIntegerv(GL_BLEND_EQUATION_ALPHA, reinterpret_cast<GLint*>(alpha));
        glGetIntegerv(GL_BLEND_EQUATION_RGB, reinterpret_cast<GLint*>(rgb));
    }
}

void getBlendFuncSeparate(const snap::rhi::Capabilities& capabilities,
                          GLuint index,
                          GLenum* srcRGB,
                          GLenum* dstRGB,
                          GLenum* srcAlpha,
                          GLenum* dstAlpha) {
    if (capabilities.isMRTBlendSettingsDifferent) {
        glGetIntegeri_v(GL_BLEND_SRC_RGB, index, reinterpret_cast<GLint*>(srcRGB));
        glGetIntegeri_v(GL_BLEND_DST_RGB, index, reinterpret_cast<GLint*>(dstRGB));
        glGetIntegeri_v(GL_BLEND_SRC_ALPHA, index, reinterpret_cast<GLint*>(srcAlpha));
        glGetIntegeri_v(GL_BLEND_DST_ALPHA, index, reinterpret_cast<GLint*>(dstAlpha));
    } else {
        glGetIntegerv(GL_BLEND_SRC_RGB, reinterpret_cast<GLint*>(srcRGB));
        glGetIntegerv(GL_BLEND_DST_RGB, reinterpret_cast<GLint*>(dstRGB));
        glGetIntegerv(GL_BLEND_SRC_ALPHA, reinterpret_cast<GLint*>(srcAlpha));
        glGetIntegerv(GL_BLEND_DST_ALPHA, reinterpret_cast<GLint*>(dstAlpha));
    }
}

#define initGetType(container, type, value)                                                                            \
    container[static_cast<uint32_t>(snap::rhi::backend::opengl::GLStateCache::type)] = value

using FramebufferGetType =
    std::array<GLenum, static_cast<uint32_t>(snap::rhi::backend::opengl::GLStateCache::FramebufferType::Count)>;
constexpr FramebufferGetType buildFramebufferGetType() {
    FramebufferGetType result{};

    for (auto& v : result) {
        v = GL_NONE;
    }
    initGetType(result, FramebufferType::Read, GL_READ_FRAMEBUFFER_BINDING);
    initGetType(result, FramebufferType::Draw, GL_DRAW_FRAMEBUFFER_BINDING);

    return result;
}

using BufferGetType =
    std::array<GLenum, static_cast<uint32_t>(snap::rhi::backend::opengl::GLStateCache::BufferType::Count)>;
constexpr BufferGetType buildBufferGetType() {
    BufferGetType result{};

    for (auto& v : result) {
        v = GL_NONE;
    }
    initGetType(result, BufferType::Array, GL_ARRAY_BUFFER_BINDING);
    initGetType(result, BufferType::CopyRead, GL_COPY_READ_BUFFER_BINDING);
    initGetType(result, BufferType::CopyWrite, GL_COPY_WRITE_BUFFER_BINDING);
    initGetType(result, BufferType::DrawIndirect, GL_DRAW_INDIRECT_BUFFER_BINDING);
    initGetType(result, BufferType::DispathIndirect, GL_DISPATCH_INDIRECT_BUFFER_BINDING);
    initGetType(result, BufferType::ElementArray, GL_ELEMENT_ARRAY_BUFFER_BINDING);
    initGetType(result, BufferType::PixelPack, GL_PIXEL_PACK_BUFFER_BINDING);
    initGetType(result, BufferType::PixelUnpack, GL_PIXEL_UNPACK_BUFFER_BINDING);
    initGetType(result, BufferType::Texture, GL_TEXTURE_BUFFER_BINDING);

    return result;
}

using TextureGetType =
    std::array<GLenum, static_cast<uint32_t>(snap::rhi::backend::opengl::GLStateCache::TextureType::Count)>;
constexpr TextureGetType buildTextureGetType() {
    TextureGetType result{};

    for (auto& v : result) {
        v = GL_NONE;
    }
    initGetType(result, TextureType::Texture2D, GL_TEXTURE_BINDING_2D);
    initGetType(result, TextureType::Texture2DMultisample, GL_TEXTURE_BINDING_2D_MULTISAMPLE);
    initGetType(result, TextureType::Texture2DMultisampleArray, GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY);
    initGetType(result, TextureType::Texture3D, GL_TEXTURE_BINDING_3D);
    initGetType(result, TextureType::Texture2DArray, GL_TEXTURE_BINDING_2D_ARRAY);
    initGetType(result, TextureType::TextureCubeMap, GL_TEXTURE_BINDING_CUBE_MAP);
    initGetType(result, TextureType::TextureCubeMapArray, GL_TEXTURE_BINDING_CUBE_MAP_ARRAY);
    initGetType(result, TextureType::TextureBuffer, GL_TEXTURE_BINDING_BUFFER);

    return result;
}

using StencilMaskGetType =
    std::array<GLenum, static_cast<uint32_t>(snap::rhi::backend::opengl::GLStateCache::StencilType::Count)>;
constexpr StencilMaskGetType buildStencilMaskGetType() {
    StencilMaskGetType result{};

    for (auto& v : result) {
        v = GL_NONE;
    }
    initGetType(result, StencilType::Front, GL_STENCIL_WRITEMASK);
    initGetType(result, StencilType::Back, GL_STENCIL_BACK_WRITEMASK);

    return result;
}

struct GLStencilFunc {
    GLenum func = GL_NONE;
    GLenum ref = GL_NONE;
    GLenum valueMask = GL_NONE;

    bool isEmpty() const {
        return func == GL_NONE && ref == GL_NONE && valueMask == GL_NONE;
    }
};

using StencilFuncGetType =
    std::array<GLStencilFunc, static_cast<uint32_t>(snap::rhi::backend::opengl::GLStateCache::StencilType::Count)>;
constexpr StencilFuncGetType buildStencilFuncGetType() {
    StencilFuncGetType result{};

    for (auto& v : result) {
        v = {};
    }

    result[static_cast<uint32_t>(snap::rhi::backend::opengl::GLStateCache::StencilType::Front)] = {
        GL_STENCIL_FUNC, GL_STENCIL_REF, GL_STENCIL_VALUE_MASK};
    result[static_cast<uint32_t>(snap::rhi::backend::opengl::GLStateCache::StencilType::Back)] = {
        GL_STENCIL_BACK_FUNC, GL_STENCIL_BACK_REF, GL_STENCIL_BACK_VALUE_MASK};

    return result;
}

struct GLStencilOp {
    GLenum sfail = GL_NONE;
    GLenum dpfail = GL_NONE;
    GLenum dppass = GL_NONE;

    bool isEmpty() const {
        return sfail == GL_NONE && dpfail == GL_NONE && dppass == GL_NONE;
    }
};
using StencilOpGetType =
    std::array<GLStencilOp, static_cast<uint32_t>(snap::rhi::backend::opengl::GLStateCache::StencilType::Count)>;
constexpr StencilOpGetType buildStencilOpGetType() {
    StencilOpGetType result{};

    for (auto& v : result) {
        v = {};
    }

    result[static_cast<uint32_t>(snap::rhi::backend::opengl::GLStateCache::StencilType::Front)] = {
        GL_STENCIL_FAIL, GL_STENCIL_PASS_DEPTH_FAIL, GL_STENCIL_PASS_DEPTH_PASS};
    result[static_cast<uint32_t>(snap::rhi::backend::opengl::GLStateCache::StencilType::Back)] = {
        GL_STENCIL_BACK_FAIL, GL_STENCIL_BACK_PASS_DEPTH_FAIL, GL_STENCIL_BACK_PASS_DEPTH_PASS};

    return result;
}

using EnableGetType =
    std::array<GLenum, static_cast<uint32_t>(snap::rhi::backend::opengl::GLStateCache::EnableType::Count)>;
constexpr EnableGetType buildEnableGetType() {
    EnableGetType result{};

    for (auto& v : result) {
        v = GL_NONE;
    }

    initGetType(result, EnableType::CullFace, GL_CULL_FACE);
    initGetType(result, EnableType::DepthTest, GL_DEPTH_TEST);
    initGetType(result, EnableType::PolygonOffsetFill, GL_POLYGON_OFFSET_FILL);
    initGetType(result, EnableType::PrimitiveRestartFixedIndex, GL_PRIMITIVE_RESTART_FIXED_INDEX);
    initGetType(result, EnableType::RasterizerDiscard, GL_RASTERIZER_DISCARD);
    initGetType(result, EnableType::SampleAlphaToCoverage, GL_SAMPLE_ALPHA_TO_COVERAGE);
    initGetType(result, EnableType::SampleCoverage, GL_SAMPLE_COVERAGE);
    initGetType(result, EnableType::ScissorTest, GL_SCISSOR_TEST);
    initGetType(result, EnableType::StencilTest, GL_STENCIL_TEST);

    return result;
}

constexpr FramebufferGetType toFramebufferGetType = buildFramebufferGetType();
constexpr BufferGetType toBufferGetType = buildBufferGetType();
constexpr TextureGetType toTextureGetType = buildTextureGetType();
constexpr StencilMaskGetType toStencilMaskGetType = buildStencilMaskGetType();
constexpr StencilFuncGetType toStencilFuncGetType = buildStencilFuncGetType();
constexpr StencilOpGetType toStencilOpGetType = buildStencilOpGetType();
constexpr EnableGetType toEnableGetType = buildEnableGetType();
#endif
} // unnamed namespace

namespace snap::rhi::backend::opengl {
GLStateCache::GLStateCache(const Profile& gl)
    : device(common::smart_cast<Device>(gl.getDevice())),
      validationLayer(device->getValidationLayer()),
      isStateValidationsEnabled((snap::rhi::ValidationTag::GLStateCacheOp & validationLayer.getValidationTags()) !=
                                snap::rhi::ValidationTag::None),
      features(gl.getFeatures()) {
    bindTexture.resize(features.maxCombinedTextureImageUnits);
    bindSampler.resize(features.maxCombinedTextureImageUnits);
    enableVertexAttribArray.resize(features.maxVertexAttribs);
    vertexAttribPointer.resize(features.maxVertexAttribs);
    vertexAttribDivisor.resize(features.maxVertexAttribs);
    blending.resize(features.maxDrawBuffers);
    reset();
}

void GLStateCache::reset() {
    useProgram = UndefinedProgramBinding;
    cullFace = UndefinedCullFace;
    depthFunc = UndefinedDepthFunc;
    blendColor = {};
    viewport = {};
    activeTexture = UndefinedActiveTexture;
    polygonOffset = {};

    bindFramebuffer.fill(UndefinedFramebufferBinding);
    bindBuffer.fill(UndefinedBufferBinding);
    stencilMaskSeparate.fill({});
    stencilFuncSeparate.fill({});
    stencilOpSeparate.fill({});
    enable.fill({});

    for (auto& samplerInfo : bindSampler) {
        samplerInfo = UndefinedSamplerBinding;
    }

    for (auto& bindTexInfo : bindTexture) {
        bindTexInfo.fill(UndefinedTextureBinding);
    }

    for (auto& info : enableVertexAttribArray) {
        info = {};
    }
    for (auto& info : vertexAttribPointer) {
        info = {};
    }
    for (auto& info : vertexAttribDivisor) {
        info = UndefinedVertexAttribDiviser;
    }

    for (auto& attachment : blending) {
        attachment.enabled = {};
        attachment.colorMask = {};
        attachment.blendEquationSeparate = {};
        attachment.blendFuncSeparate = {};
    }
}

void GLStateCache::resetOpenGLClipDistance() {
    constexpr uint32_t SupportedMaxClipDistances = 8;
    assert(features.maxClipDistances <= SupportedMaxClipDistances);
    for (uint32_t i = 0; i < SupportedMaxClipDistances; ++i) {
        const uint32_t idx = (static_cast<uint32_t>(EnableType::ClipDistance0) + i);

        auto& info = enable[idx];
        if (info.isInitialized && (info.state == GL_TRUE)) {
            glDisable(GL_CLIP_DISTANCE0 + i);
            info.state = GL_FALSE;
        }
    }
}

#if SNAP_RHI_ENABLE_SLOW_VALIDATIONS()
void GLStateCache::validateProgramState() {
    if (useProgram == UndefinedProgramBinding) {
        return;
    }

    if (isStateValidationsEnabled) {
        GLuint activeProgram = GL_NONE;
        glGetIntegerv(GL_CURRENT_PROGRAM, reinterpret_cast<GLint*>(&activeProgram));

        GLuint cacheProgram = useProgram;
        if (activeProgram != cacheProgram) {
            SNAP_RHI_REPORT(validationLayer,
                            snap::rhi::ReportLevel::Error,
                            snap::rhi::ValidationTag::GLStateCacheOp,
                            "[validateProgramState] wrong cache state, cache value: %d, gl state value: %d",
                            cacheProgram,
                            activeProgram);
        }
    };
}

void GLStateCache::validateViewport() {
    if (isStateValidationsEnabled) {
        GLint viewportData[4] = {GL_NONE, GL_NONE, GL_NONE, GL_NONE};
        glGetIntegerv(GL_VIEWPORT, reinterpret_cast<GLint*>(viewportData));

        if (viewport.state.x != viewportData[0] || viewport.state.y != viewportData[1] ||
            viewport.state.width != viewportData[2] || viewport.state.height != viewportData[3]) {
            SNAP_RHI_REPORT(validationLayer,
                            snap::rhi::ReportLevel::Error,
                            snap::rhi::ValidationTag::GLStateCacheOp,
                            "[validateViewport] wrong cache state, {x} cache value: %d, gl state value: "
                            "%d, {y} cache value: %d, gl state value: %d, {width} cache value: %d, gl state "
                            "value: %d, {height} cache value: %d, gl state value: %d",
                            viewport.state.x,
                            viewportData[0],
                            viewport.state.y,
                            viewportData[1],
                            viewport.state.width,
                            viewportData[2],
                            viewport.state.height,
                            viewportData[3]);
        }
    };
}

void GLStateCache::validateCullFaceState() {
    if (isStateValidationsEnabled) {
        GLuint activeMode = GL_NONE;
        glGetIntegerv(GL_CULL_FACE_MODE, reinterpret_cast<GLint*>(&activeMode));

        GLuint cacheMode = cullFace;
        if (activeMode != cacheMode) {
            SNAP_RHI_REPORT(validationLayer,
                            snap::rhi::ReportLevel::Error,
                            snap::rhi::ValidationTag::GLStateCacheOp,
                            "[validateCullFaceState] wrong cache state, cache value: %d, gl state value: %d",
                            cacheMode,
                            activeMode);
        }
    }
}

void GLStateCache::validateDepthFuncState() {
    if (isStateValidationsEnabled) {
        GLuint activeFunc = GL_NONE;
        glGetIntegerv(GL_DEPTH_FUNC, reinterpret_cast<GLint*>(&activeFunc));

        GLuint cacheFunc = depthFunc;
        if (activeFunc != cacheFunc) {
            SNAP_RHI_REPORT(validationLayer,
                            snap::rhi::ReportLevel::Error,
                            snap::rhi::ValidationTag::GLStateCacheOp,
                            "[validateDepthFuncState] wrong cache state, cache value: %d, gl state value: %d",
                            cacheFunc,
                            activeFunc);
        }
    };
}

void GLStateCache::validateBlendColorState() {
    if (!blendColor.isInitialized) {
        return;
    }

    if (isStateValidationsEnabled) {
        GLfloat activeColor[4]{0.0f, 0.0f, 0.0f, 0.0f};
        glGetFloatv(GL_BLEND_COLOR, activeColor);

        const BlendColor& cacheColor = blendColor.state;
        if (!snap::rhi::backend::common::epsilonEqual(cacheColor.red, activeColor[0], CacheEpsilon) ||
            !snap::rhi::backend::common::epsilonEqual(cacheColor.green, activeColor[1], CacheEpsilon) ||
            !snap::rhi::backend::common::epsilonEqual(cacheColor.blue, activeColor[2], CacheEpsilon) ||
            !snap::rhi::backend::common::epsilonEqual(cacheColor.alpha, activeColor[3], CacheEpsilon)) {
            SNAP_RHI_REPORT(validationLayer,
                            snap::rhi::ReportLevel::Error,
                            snap::rhi::ValidationTag::GLStateCacheOp,
                            "[validateBlendColorState] wrong cache state, cache value: "
                            "{%f, %f, %f, %f}, gl state value: {%f, %f, %f, %f}",
                            cacheColor.red,
                            cacheColor.green,
                            cacheColor.blue,
                            cacheColor.alpha,
                            activeColor[0],
                            activeColor[1],
                            activeColor[2],
                            activeColor[3]);
        }
    };
}

void GLStateCache::validateBindFramebuffer() {
    if (isStateValidationsEnabled) {
        for (size_t i = 0; i < bindFramebuffer.size(); ++i) {
            const auto& framebuffer = bindFramebuffer[i];
            if (framebuffer == UndefinedFramebufferBinding || toFramebufferGetType[i] == GL_NONE) {
                continue;
            }

            GLuint cacheValue = framebuffer;

            GLuint glValue = GL_NONE;
            glGetIntegerv(toFramebufferGetType[i], reinterpret_cast<GLint*>(&glValue));

            if (cacheValue != glValue) {
                SNAP_RHI_REPORT(
                    validationLayer,
                    snap::rhi::ReportLevel::Error,
                    snap::rhi::ValidationTag::GLStateCacheOp,
                    "[validateBindFramebuffer] wrong cache buffer{%d} state cache value: {%d}, gl state value: {%d}",
                    i,
                    cacheValue,
                    glValue);
            }
        }
    };
}

void GLStateCache::validateBindBuffer() {
    if (isStateValidationsEnabled) {
        for (size_t i = 0; i < bindBuffer.size(); ++i) {
            const auto& buffer = bindBuffer[i];
            if (buffer == UndefinedBufferBinding || toBufferGetType[i] == GL_NONE) {
                continue;
            }

            GLuint cacheBuffer = buffer;

            GLuint bufferID = GL_NONE;
            glGetIntegerv(toBufferGetType[i], reinterpret_cast<GLint*>(&bufferID));

            if (cacheBuffer != bufferID) {
                SNAP_RHI_REPORT(
                    validationLayer,
                    snap::rhi::ReportLevel::Error,
                    snap::rhi::ValidationTag::GLStateCacheOp,
                    "[validateBindBuffer] wrong cache buffer state{%d}, cache value: {%d}, gl state value: {%d}",
                    i,
                    cacheBuffer,
                    bufferID);
            }
        }
    };
}

void GLStateCache::validateActiveTexture() {
    if (activeTexture == UndefinedActiveTexture) {
        return;
    }

    if (isStateValidationsEnabled) {
        GLuint nativeActiveTexture = GL_NONE;
        glGetIntegerv(GL_ACTIVE_TEXTURE, reinterpret_cast<GLint*>(&nativeActiveTexture));

        GLuint cacheActiveTexture = static_cast<GLuint>(activeTexture);
        if (cacheActiveTexture != nativeActiveTexture) {
            SNAP_RHI_REPORT(validationLayer,
                            snap::rhi::ReportLevel::Error,
                            snap::rhi::ValidationTag::GLStateCacheOp,
                            "[validateActiveTexture] wrong cache state, cache value: {%d}, gl state value: {%d}",
                            cacheActiveTexture,
                            nativeActiveTexture);
        }
    };
}

void GLStateCache::validateBindSampler() {}

void GLStateCache::validateBindTexture() {
    if (activeTexture == UndefinedActiveTexture) {
        return;
    }

    if (isStateValidationsEnabled) {
        const GLenum activeTextureIndex = activeTexture - GL_TEXTURE0;

        assert(activeTextureIndex < bindTexture.size());
        const auto& bindActiveTexture = bindTexture[activeTextureIndex];

        for (size_t i = 0; i < bindActiveTexture.size(); ++i) {
            const auto& texture = bindActiveTexture[i];
            if ((texture == UndefinedTextureBinding) || toTextureGetType[i] == GL_NONE) {
                continue;
            }

            GLuint cacheTexture = static_cast<GLuint>(texture);

            GLuint texID = GL_NONE;
            glGetIntegerv(toTextureGetType[i], reinterpret_cast<GLint*>(&texID));

            if (cacheTexture != texID) {
                SNAP_RHI_REPORT(
                    validationLayer,
                    snap::rhi::ReportLevel::Error,
                    snap::rhi::ValidationTag::GLStateCacheOp,
                    "[validateBindTexture] wrong cache texture state{%d}, cache value: {%d}, gl state value: {%d}",
                    i,
                    cacheTexture,
                    texID);
            }
        }
    };
}

void GLStateCache::validateStencilMaskSeparate() {
    if (isStateValidationsEnabled) {
        for (size_t i = 0; i < stencilMaskSeparate.size(); ++i) {
            const auto& mask = stencilMaskSeparate[i];
            if (!mask.isInitialized || toStencilMaskGetType[i] == GL_NONE) {
                continue;
            }

            GLuint cacheMask = static_cast<GLuint>(mask.state);

            GLuint glMask = GL_NONE;
            glGetIntegerv(toStencilMaskGetType[i], reinterpret_cast<GLint*>(&glMask));

            if (cacheMask != glMask) {
                SNAP_RHI_REPORT(
                    validationLayer,
                    snap::rhi::ReportLevel::Error,
                    snap::rhi::ValidationTag::GLStateCacheOp,
                    "[validateStencilMaskSeparate] wrong cache state{%d}, cache value: {%d}, gl state value: {%d}",
                    i,
                    cacheMask,
                    glMask);
            }
        }
    };
}

void GLStateCache::validateStencilFuncSeparate() {
    if (isStateValidationsEnabled) {
        for (size_t i = 0; i < stencilFuncSeparate.size(); ++i) {
            const auto& funcInfo = stencilFuncSeparate[i];
            if (!funcInfo.isInitialized || toStencilFuncGetType[i].isEmpty()) {
                continue;
            }

            const auto& cacheValue = funcInfo.state;

            GLenum func = GL_NONE;
            GLint ref = GL_NONE;
            GLuint mask = GL_NONE;

            glGetIntegerv(toStencilFuncGetType[i].func, reinterpret_cast<GLint*>(&func));
            glGetIntegerv(toStencilFuncGetType[i].ref, reinterpret_cast<GLint*>(&ref));
            glGetIntegerv(toStencilFuncGetType[i].valueMask, reinterpret_cast<GLint*>(&mask));

            if (cacheValue.func != func || cacheValue.ref != ref || cacheValue.mask != mask) {
                SNAP_RHI_REPORT(validationLayer,
                                snap::rhi::ReportLevel::Error,
                                snap::rhi::ValidationTag::GLStateCacheOp,
                                "[validateStencilFuncSeparate] wrong cache state{%d}, gl func: {%d}, gl ref: "
                                "{%d}, gl mask: {%d}, cache func: {%d}, cache ref: {%d}, cache mask: {%d}",
                                i,
                                func,
                                ref,
                                mask,
                                cacheValue.func,
                                cacheValue.ref,
                                cacheValue.mask);
            }
        }
    };
}

void GLStateCache::validateStencilOpSeparate() {
    if (isStateValidationsEnabled) {
        for (size_t i = 0; i < stencilOpSeparate.size(); ++i) {
            const auto& opInfo = stencilOpSeparate[i];
            if (!opInfo.isInitialized || toStencilOpGetType[i].isEmpty()) {
                continue;
            }

            const auto& cacheValue = opInfo.state;

            GLenum sfail = GL_NONE;
            GLenum dpfail = GL_NONE;
            GLenum dppass = GL_NONE;

            glGetIntegerv(toStencilOpGetType[i].sfail, reinterpret_cast<GLint*>(&sfail));
            glGetIntegerv(toStencilOpGetType[i].dpfail, reinterpret_cast<GLint*>(&dpfail));
            glGetIntegerv(toStencilOpGetType[i].dppass, reinterpret_cast<GLint*>(&dppass));

            if (cacheValue.sfail != sfail || cacheValue.dpfail != dpfail || cacheValue.dppass != dppass) {
                SNAP_RHI_REPORT(
                    validationLayer,
                    snap::rhi::ReportLevel::Error,
                    snap::rhi::ValidationTag::GLStateCacheOp,
                    "[validateStencilOpSeparate] wrong cache state{%d}, gl sfail: {%d}, gl dpfail: {%d}, gl dppass: "
                    "{%d}, cache sfail: {%d}, cache dpfail: {%d}, cache dppass: {%d}",
                    i,
                    sfail,
                    dpfail,
                    dppass,
                    cacheValue.sfail,
                    cacheValue.dpfail,
                    cacheValue.dppass);
            }
        }
    };
}

void GLStateCache::validateEnable() {
    if (isStateValidationsEnabled) {
        for (size_t i = 0; i < enable.size(); ++i) {
            const auto& enableInfo = enable[i];
            if (!enableInfo.isInitialized || toEnableGetType[i] == GL_NONE) {
                continue;
            }

            GLboolean cacheValue = enableInfo.state;
            GLboolean glValue = glIsEnabled(toEnableGetType[i]);

            if (cacheValue != glValue) {
                SNAP_RHI_REPORT(validationLayer,
                                snap::rhi::ReportLevel::Error,
                                snap::rhi::ValidationTag::GLStateCacheOp,
                                "[validateEnable] wrong cache state{%d}, cache value: {%d}, gl state value: {%d}",
                                i,
                                cacheValue,
                                glValue);
            }
        }

        const auto& capabilities = device->getCapabilities();
        for (size_t i = 0; i < blending.size(); ++i) {
            const auto& blendSetings = blending[i];

            if (!blendSetings.enabled.isInitialized) {
                continue;
            }

            GLboolean glValue = isBlendingEnabed(capabilities, static_cast<GLuint>(i));
            if (blendSetings.enabled.state.enabled != glValue) {
                SNAP_RHI_REPORT(
                    validationLayer,
                    snap::rhi::ReportLevel::Error,
                    snap::rhi::ValidationTag::GLStateCacheOp,
                    "[validateEnable] wrong cache GL_BLEND[%d] state, cache value: {%d}, gl state value: {%d}",
                    i,
                    blendSetings.enabled.state.enabled,
                    glValue);
            }
        }
    };
}

void GLStateCache::validateBlendColorMask() {
    if (isStateValidationsEnabled) {
        const auto& capabilities = device->getCapabilities();
        for (size_t i = 0; i < blending.size(); ++i) {
            const auto& blendSetings = blending[i];

            if (!blendSetings.colorMask.isInitialized) {
                continue;
            }

            GLboolean writeMask[4]{GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE};
            getColorWriteMask(capabilities, static_cast<GLuint>(i), writeMask);

            const auto& cacheValue = blendSetings.colorMask.state;
            if (cacheValue.red != writeMask[0] || cacheValue.green != writeMask[1] || cacheValue.blue != writeMask[2] ||
                cacheValue.alpha != writeMask[3]) {
                SNAP_RHI_REPORT(validationLayer,
                                snap::rhi::ReportLevel::Error,
                                snap::rhi::ValidationTag::GLStateCacheOp,
                                "[validateBlendColorMask] wrong cache GL_COLOR_WRITEMASK[%d] state, cache "
                                "value: {%d, %d, %d, %d}, gl state value: {%d, %d, %d, %d}",
                                i,
                                cacheValue.red,
                                cacheValue.green,
                                cacheValue.blue,
                                cacheValue.alpha,
                                writeMask[0],
                                writeMask[1],
                                writeMask[2],
                                writeMask[3]);
            }
        }
    };
}

void GLStateCache::validateBlendEquationSeparate() {
    if (isStateValidationsEnabled) {
        const auto& capabilities = device->getCapabilities();
        for (size_t i = 0; i < blending.size(); ++i) {
            const auto& blendSetings = blending[i];

            if (!blendSetings.blendEquationSeparate.isInitialized) {
                continue;
            }

            GLenum blendEquationAlpha = GL_NONE;
            GLenum blendEquationRGB = GL_NONE;
            getBlendEquation(capabilities, static_cast<GLuint>(i), &blendEquationAlpha, &blendEquationRGB);

            const auto& cacheValue = blendSetings.blendEquationSeparate.state;
            if (cacheValue.modeRGB != blendEquationRGB || cacheValue.modeAlpha != blendEquationAlpha) {
                SNAP_RHI_REPORT(validationLayer,
                                snap::rhi::ReportLevel::Error,
                                snap::rhi::ValidationTag::GLStateCacheOp,
                                "[validateBlendEquationSeparate] wrong cache GL_COLOR_WRITEMASK[%d] state, "
                                "cache value: {rgb: %d, alpha: %d}, gl state value: {rgb: %d, alpha: %d}",
                                i,
                                cacheValue.modeRGB,
                                cacheValue.modeAlpha,
                                blendEquationRGB,
                                blendEquationAlpha);
            }
        }
    };
}

void GLStateCache::validateBlendFuncSeparate() {
    if (isStateValidationsEnabled) {
        const auto& capabilities = device->getCapabilities();
        for (size_t i = 0; i < blending.size(); ++i) {
            const auto& blendSetings = blending[i];

            if (!blendSetings.blendFuncSeparate.isInitialized) {
                continue;
            }

            GLenum srcRGB = GL_NONE;
            GLenum dstRGB = GL_NONE;
            GLenum srcAlpha = GL_NONE;
            GLenum dstAlpha = GL_NONE;

            getBlendFuncSeparate(capabilities, static_cast<GLuint>(i), &srcRGB, &dstRGB, &srcAlpha, &dstAlpha);

            const auto& cacheValue = blendSetings.blendFuncSeparate.state;
            if (cacheValue.srcRGB != srcRGB || cacheValue.dstRGB != dstRGB || cacheValue.srcAlpha != srcAlpha ||
                cacheValue.dstAlpha != dstAlpha) {
                SNAP_RHI_REPORT(validationLayer,
                                snap::rhi::ReportLevel::Error,
                                snap::rhi::ValidationTag::GLStateCacheOp,
                                "[validateBlendFuncSeparate] wrong cache GL_COLOR_WRITEMASK[%d] state, cache "
                                "value: {srcRGB: %d, dstRGB: %d, srcAlpha: %d, dstAlpha: %d}, gl state value: "
                                "{srcRGB: %d, dstRGB: %d, srcAlpha: %d, dstAlpha: %d}",
                                i,
                                cacheValue.srcRGB,
                                cacheValue.dstRGB,
                                cacheValue.srcAlpha,
                                cacheValue.dstAlpha,
                                srcRGB,
                                dstRGB,
                                srcAlpha,
                                dstAlpha);
            }
        }
    };
}

void GLStateCache::validatePolygonOffset() {
    if (!polygonOffset.isInitialized) {
        return;
    }

    if (isStateValidationsEnabled) {
        GLfloat factor = 0.0f;
        GLfloat units = 0.0f;

        glGetFloatv(GL_POLYGON_OFFSET_FACTOR, &factor);
        glGetFloatv(GL_POLYGON_OFFSET_UNITS, &units);

        const PolygonOffset& cachePolygonOffset = polygonOffset.state;
        if (!snap::rhi::backend::common::epsilonEqual(cachePolygonOffset.factor, factor, CacheEpsilon) ||
            !snap::rhi::backend::common::epsilonEqual(cachePolygonOffset.units, units, CacheEpsilon)) {
            SNAP_RHI_REPORT(validationLayer,
                            snap::rhi::ReportLevel::Error,
                            snap::rhi::ValidationTag::GLStateCacheOp,
                            "[validatePolygonOffset] wrong cache state, cache value: {factor: %f, units: "
                            "%f}, gl state value: {factor: %f, units: %f}",
                            cachePolygonOffset.factor,
                            cachePolygonOffset.units,
                            factor,
                            units);
        }
    };
}

void GLStateCache::validateVertexAttribArray() {
    if (isStateValidationsEnabled) {
        for (size_t i = 0; i < enableVertexAttribArray.size(); ++i) {
            const auto& attribInfo = enableVertexAttribArray[i];
            if (!attribInfo.isInitialized) {
                continue;
            }

            GLint value = attribInfo.state;

            GLint glValue = GL_FALSE;
            glGetVertexAttribiv(static_cast<GLuint>(i), GL_VERTEX_ATTRIB_ARRAY_ENABLED, &glValue);

            if (value != glValue) {
                SNAP_RHI_REPORT(
                    validationLayer,
                    snap::rhi::ReportLevel::Error,
                    snap::rhi::ValidationTag::GLStateCacheOp,
                    "[validateVertexAttribArray] wrong cache state{%d}, cache value: {%d}, gl state value: {%d}",
                    i,
                    value,
                    glValue);
            }
        }
    };
}

void GLStateCache::validateVertexAttribDivisor() {
    if (isStateValidationsEnabled) {
        for (size_t i = 0; i < vertexAttribDivisor.size(); ++i) {
            GLint value = vertexAttribDivisor[i];
            if (value == UndefinedVertexAttribDiviser) {
                continue;
            }

            GLint glValue = 0;
            glGetVertexAttribiv(static_cast<GLuint>(i), GL_VERTEX_ATTRIB_ARRAY_DIVISOR, &glValue);

            if (value != glValue) {
                SNAP_RHI_REPORT(
                    validationLayer,
                    snap::rhi::ReportLevel::Error,
                    snap::rhi::ValidationTag::GLStateCacheOp,
                    "[validateVertexAttribDivisor] wrong cache state{%d}, cache value: {%d}, gl state value: {%d}",
                    i,
                    value,
                    glValue);
            }
        }
    };
}

void GLStateCache::validateVertexAttribPointer() {
    if (isStateValidationsEnabled) {
        for (size_t i = 0; i < vertexAttribPointer.size(); ++i) {
            const auto& attribInfo = vertexAttribPointer[i];
            if (!attribInfo.isInitialized) {
                continue;
            }

            const auto& value = attribInfo.state;

            GLint size = 0;
            GLint type = GL_NONE;
            GLint normalized = GL_FALSE;
            GLint stride = 0;
            void* pointer = nullptr;
            GLint buffer = GL_NONE;

            glGetVertexAttribiv(static_cast<GLuint>(i), GL_VERTEX_ATTRIB_ARRAY_SIZE, &size);
            glGetVertexAttribiv(static_cast<GLuint>(i), GL_VERTEX_ATTRIB_ARRAY_TYPE, &type);
            glGetVertexAttribiv(static_cast<GLuint>(i), GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, &normalized);
            glGetVertexAttribiv(static_cast<GLuint>(i), GL_VERTEX_ATTRIB_ARRAY_STRIDE, &stride);
            glGetVertexAttribPointerv(static_cast<GLuint>(i), GL_VERTEX_ATTRIB_ARRAY_POINTER, &pointer);
            glGetVertexAttribiv(static_cast<GLuint>(i), GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &buffer);

            if (value.size != size || value.type != type || value.normalized != normalized || value.stride != stride ||
                value.pointer != pointer || value.buffer != buffer) {
                SNAP_RHI_REPORT(
                    validationLayer,
                    snap::rhi::ReportLevel::Error,
                    snap::rhi::ValidationTag::GLStateCacheOp,
                    "[validateVertexAttribPointer] wrong cache state{%d}, cache value: {size: %d, type: %d, "
                    "normalized: %d, stride: %d, pointer: %p, buffer: %d}, gl state value: {size: %d, type: %d, "
                    "normalized: %d, stride: %d, pointer: %p, buffer: %d}",
                    i,
                    value.size,
                    value.type,
                    value.normalized,
                    value.stride,
                    value.pointer,
                    value.buffer,
                    size,
                    type,
                    normalized,
                    stride,
                    pointer,
                    buffer);
            }
        }
    };
}
#endif
} // namespace snap::rhi::backend::opengl
