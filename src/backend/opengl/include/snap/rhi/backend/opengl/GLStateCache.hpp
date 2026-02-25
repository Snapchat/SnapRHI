#pragma once

#include "snap/rhi/backend/common/Math.hpp"
#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/common/ValidationLayer.hpp"
#include "snap/rhi/backend/opengl/OpenGL.h"
#include "snap/rhi/backend/opengl/VertexDescriptor.h"

#include <array>
#include <bitset>
#include <compare>
#include <optional>
#include <tuple>
#include <variant>
#include <vector>

namespace snap::rhi::backend::opengl {
class Device;
class Profile;
struct Features;

/**
 * We shouldn't get all these current states, but we have to cache them.
 *
 * The glGet* operations are slow and should not be used.
 * The GPU has to wait for the call because all current operations have to be finished in order to get the correct
 * state.
 *
 * This cache should only be used during CommandQueue::submit[Rendering],
 * as many GL states expect resources to not be deleted.
 *
 * For example: glVertexAttribPointer.
 */
class GLStateCache final {
public:
    enum class BufferType : uint32_t {
        Undefined = 0,

        Array,
        CopyRead,
        CopyWrite,
        DrawIndirect,
        DispathIndirect,
        ElementArray,
        PixelPack,
        PixelUnpack,
        Texture,

        Count
    };

    enum class TextureType : uint32_t {
        Undefined = 0,

        Texture2D,
        Texture2DMultisample,
        Texture2DMultisampleArray,
        Texture3D,
        Texture2DArray,
        TextureCubeMap,
        TextureCubeMapArray,
        TextureBuffer,

        Count
    };

    enum class StencilType : uint32_t {
        Undefined = 0,

        Front,
        Back,

        Count
    };

    enum class EnableType : uint32_t {
        Undefined = 0,

        CullFace,
        DepthTest,
        PolygonOffsetFill,
        PrimitiveRestartFixedIndex,
        RasterizerDiscard,
        SampleAlphaToCoverage,
        SampleCoverage,
        ScissorTest,
        StencilTest,

        ClipDistance0,
        ClipDistance1,
        ClipDistance2,
        ClipDistance3,
        ClipDistance4,
        ClipDistance5,
        ClipDistance6,
        ClipDistance7,

        Count
    };

    enum class FramebufferType : uint32_t {
        Undefined = 0,

        Read,
        Draw,

        Count
    };

    constexpr static inline snap::rhi::backend::opengl::GLStateCache::BufferType toBufferType(const GLenum target) {
        switch (target) {
            case GL_ARRAY_BUFFER:
                return snap::rhi::backend::opengl::GLStateCache::BufferType::Array;
            case GL_COPY_READ_BUFFER:
                return snap::rhi::backend::opengl::GLStateCache::BufferType::CopyRead;
            case GL_COPY_WRITE_BUFFER:
                return snap::rhi::backend::opengl::GLStateCache::BufferType::CopyWrite;
            case GL_DRAW_INDIRECT_BUFFER:
                return snap::rhi::backend::opengl::GLStateCache::BufferType::DrawIndirect;
            case GL_DISPATCH_INDIRECT_BUFFER:
                return snap::rhi::backend::opengl::GLStateCache::BufferType::DispathIndirect;
            case GL_ELEMENT_ARRAY_BUFFER:
                return snap::rhi::backend::opengl::GLStateCache::BufferType::ElementArray;
            case GL_PIXEL_PACK_BUFFER:
                return snap::rhi::backend::opengl::GLStateCache::BufferType::PixelPack;
            case GL_PIXEL_UNPACK_BUFFER:
                return snap::rhi::backend::opengl::GLStateCache::BufferType::PixelUnpack;
            case GL_TEXTURE_BUFFER:
                return snap::rhi::backend::opengl::GLStateCache::BufferType::Texture;

            default:
                return snap::rhi::backend::opengl::GLStateCache::BufferType::Undefined;
        }
    }

    constexpr static inline snap::rhi::backend::opengl::GLStateCache::TextureType toTextureType(const GLenum target) {
        switch (target) {
            case GL_TEXTURE_2D:
                return snap::rhi::backend::opengl::GLStateCache::TextureType::Texture2D;
            case GL_TEXTURE_2D_MULTISAMPLE:
                return snap::rhi::backend::opengl::GLStateCache::TextureType::Texture2DMultisample;
            case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
                return snap::rhi::backend::opengl::GLStateCache::TextureType::Texture2DMultisampleArray;
            case GL_TEXTURE_3D:
                return snap::rhi::backend::opengl::GLStateCache::TextureType::Texture3D;
            case GL_TEXTURE_2D_ARRAY:
                return snap::rhi::backend::opengl::GLStateCache::TextureType::Texture2DArray;
            case GL_TEXTURE_CUBE_MAP:
                return snap::rhi::backend::opengl::GLStateCache::TextureType::TextureCubeMap;
            case GL_TEXTURE_CUBE_MAP_ARRAY:
                return snap::rhi::backend::opengl::GLStateCache::TextureType::TextureCubeMapArray;
            case GL_TEXTURE_BUFFER:
                return snap::rhi::backend::opengl::GLStateCache::TextureType::TextureBuffer;

            default:
                return snap::rhi::backend::opengl::GLStateCache::TextureType::Undefined;
        }
    }

    constexpr static inline snap::rhi::backend::opengl::GLStateCache::StencilType toStencilType(const GLenum target) {
        switch (target) {
            case GL_FRONT:
                return snap::rhi::backend::opengl::GLStateCache::StencilType::Front;
            case GL_BACK:
                return snap::rhi::backend::opengl::GLStateCache::StencilType::Back;

            default:
                return snap::rhi::backend::opengl::GLStateCache::StencilType::Undefined;
        }
    }

    constexpr static inline snap::rhi::backend::opengl::GLStateCache::EnableType toEnableType(const GLenum target) {
        switch (target) {
            case GL_CULL_FACE:
                return snap::rhi::backend::opengl::GLStateCache::EnableType::CullFace;
            case GL_DEPTH_TEST:
                return snap::rhi::backend::opengl::GLStateCache::EnableType::DepthTest;
            case GL_POLYGON_OFFSET_FILL:
                return snap::rhi::backend::opengl::GLStateCache::EnableType::PolygonOffsetFill;
            case GL_PRIMITIVE_RESTART_FIXED_INDEX:
                return snap::rhi::backend::opengl::GLStateCache::EnableType::PrimitiveRestartFixedIndex;
            case GL_RASTERIZER_DISCARD:
                return snap::rhi::backend::opengl::GLStateCache::EnableType::RasterizerDiscard;
            case GL_SAMPLE_ALPHA_TO_COVERAGE:
                return snap::rhi::backend::opengl::GLStateCache::EnableType::SampleAlphaToCoverage;
            case GL_SAMPLE_COVERAGE:
                return snap::rhi::backend::opengl::GLStateCache::EnableType::SampleCoverage;
            case GL_SCISSOR_TEST:
                return snap::rhi::backend::opengl::GLStateCache::EnableType::ScissorTest;
            case GL_STENCIL_TEST:
                return snap::rhi::backend::opengl::GLStateCache::EnableType::StencilTest;
            case GL_CLIP_DISTANCE0:
                return snap::rhi::backend::opengl::GLStateCache::EnableType::ClipDistance0;
            case GL_CLIP_DISTANCE1:
                return snap::rhi::backend::opengl::GLStateCache::EnableType::ClipDistance1;
            case GL_CLIP_DISTANCE2:
                return snap::rhi::backend::opengl::GLStateCache::EnableType::ClipDistance2;
            case GL_CLIP_DISTANCE3:
                return snap::rhi::backend::opengl::GLStateCache::EnableType::ClipDistance3;
            case GL_CLIP_DISTANCE4:
                return snap::rhi::backend::opengl::GLStateCache::EnableType::ClipDistance4;
            case GL_CLIP_DISTANCE5:
                return snap::rhi::backend::opengl::GLStateCache::EnableType::ClipDistance5;
            case GL_CLIP_DISTANCE6:
                return snap::rhi::backend::opengl::GLStateCache::EnableType::ClipDistance6;
            case GL_CLIP_DISTANCE7:
                return snap::rhi::backend::opengl::GLStateCache::EnableType::ClipDistance7;

            default:
                return snap::rhi::backend::opengl::GLStateCache::EnableType::Undefined;
        }
    }

private:
    static constexpr GLint UndefinedSamplerBinding = -1;
    static constexpr GLint UndefinedTextureBinding = -1;
    static constexpr GLint UndefinedActiveTexture = -1;
    static constexpr GLint UndefinedProgramBinding = -1;
    static constexpr GLint UndefinedFramebufferBinding = -1;
    static constexpr GLint UndefinedBufferBinding = -1;
    static constexpr GLint UndefinedCullFace = -1;
    static constexpr GLint UndefinedDepthFunc = -1;
    static constexpr GLint UndefinedVertexAttribDiviser = -1;

    using TexturesState = std::array<GLint, static_cast<uint32_t>(TextureType::Count)>;

    template<typename T>
    struct GLState {
        T state{};
        bool isInitialized = false;
    };

    struct BlendingState {
        struct Enabled {
            GLboolean enabled = GL_FALSE;

            friend auto operator<=>(const Enabled&, const Enabled&) = default;
        };

        struct ColorMask {
            GLboolean red = GL_FALSE;
            GLboolean green = GL_FALSE;
            GLboolean blue = GL_FALSE;
            GLboolean alpha = GL_FALSE;

            friend auto operator<=>(const ColorMask&, const ColorMask&) = default;
        };

        struct BlendEquationSeparate {
            GLenum modeRGB = GL_NONE;
            GLenum modeAlpha = GL_NONE;

            friend auto operator<=>(const BlendEquationSeparate&, const BlendEquationSeparate&) = default;
        };

        struct BlendFuncSeparate {
            GLenum srcRGB = GL_NONE;
            GLenum dstRGB = GL_NONE;

            GLenum srcAlpha = GL_NONE;
            GLenum dstAlpha = GL_NONE;

            friend auto operator<=>(const BlendFuncSeparate&, const BlendFuncSeparate&) = default;
        };

        GLState<Enabled> enabled{};
        GLState<ColorMask> colorMask{};
        GLState<BlendEquationSeparate> blendEquationSeparate{};
        GLState<BlendFuncSeparate> blendFuncSeparate{};
    };

    struct Viewport {
        GLint x = GL_NONE;
        GLint y = GL_NONE;
        GLsizei width = GL_NONE;
        GLsizei height = GL_NONE;

        friend auto operator<=>(const Viewport&, const Viewport&) = default;
    };

    struct StencilFuncSeparate {
        GLenum func = GL_NONE;
        GLint ref = GL_NONE;
        GLuint mask = GL_NONE;

        friend auto operator<=>(const StencilFuncSeparate&, const StencilFuncSeparate&) = default;
    };

    struct StencilOpSeparate {
        GLenum sfail = GL_NONE;
        GLenum dpfail = GL_NONE;
        GLenum dppass = GL_NONE;

        friend auto operator<=>(const StencilOpSeparate&, const StencilOpSeparate&) = default;
    };

    struct PolygonOffset {
        GLfloat factor = 0.0f;
        GLfloat units = 0.0f;

        friend auto operator<=>(const PolygonOffset&, const PolygonOffset&) = default;
    };

    struct VertexAttribPointer {
        GLint size = GL_NONE;
        GLenum type = GL_NONE;
        GLboolean normalized = GL_FALSE;
        GLsizei stride = GL_NONE;
        const void* pointer = nullptr;
        GLuint buffer = GL_NONE;

        friend auto operator<=>(const VertexAttribPointer&, const VertexAttribPointer&) = default;
    };

    struct BlendColor {
        GLfloat red = 0.0f;
        GLfloat green = 0.0f;
        GLfloat blue = 0.0f;
        GLfloat alpha = 0.0f;

        friend auto operator<=>(const BlendColor&, const BlendColor&) = default;
    };

public:
    GLStateCache(const Profile& gl);

    void resetOpenGLClipDistance();
    void reset();

    [[nodiscard]] SNAP_RHI_ALWAYS_INLINE bool shouldBindSampler(GLuint unit, GLuint sampler) {
        if (bindSampler[unit] == sampler) {
#if SNAP_RHI_ENABLE_SLOW_VALIDATIONS()
            validateBindSampler();
#endif
            return false;
        }

        bindSampler[unit] = sampler;
        return true;
    }

    [[nodiscard]] SNAP_RHI_ALWAYS_INLINE bool shouldUseProgram(GLuint program) {
        if (program == useProgram) {
#if SNAP_RHI_ENABLE_SLOW_VALIDATIONS()
            validateProgramState();
#endif
            return false;
        }

        useProgram = program;
        return true;
    }

    [[nodiscard]] SNAP_RHI_ALWAYS_INLINE bool shouldCullFace(GLenum mode) {
        if (mode == cullFace) {
#if SNAP_RHI_ENABLE_SLOW_VALIDATIONS()
            validateCullFaceState();
#endif
            return false;
        }

        cullFace = mode;
        return true;
    }

    [[nodiscard]] SNAP_RHI_ALWAYS_INLINE bool shouldDepthFunc(GLenum func) {
        if (func == depthFunc) {
#if SNAP_RHI_ENABLE_SLOW_VALIDATIONS()
            validateDepthFuncState();
#endif
            return false;
        }

        depthFunc = func;
        return true;
    }

    [[nodiscard]] SNAP_RHI_ALWAYS_INLINE bool shouldBlendColor(GLfloat red,
                                                               GLfloat green,
                                                               GLfloat blue,
                                                               GLfloat alpha) {
        if (blendColor.isInitialized) {
            const auto& activeColor = blendColor.state;
            if (snap::rhi::backend::common::epsilonEqual(activeColor.red, red, snap::rhi::backend::common::Epsilon) &&
                snap::rhi::backend::common::epsilonEqual(
                    activeColor.green, green, snap::rhi::backend::common::Epsilon) &&
                snap::rhi::backend::common::epsilonEqual(activeColor.blue, blue, snap::rhi::backend::common::Epsilon) &&
                snap::rhi::backend::common::epsilonEqual(
                    activeColor.alpha, alpha, snap::rhi::backend::common::Epsilon)) {
#if SNAP_RHI_ENABLE_SLOW_VALIDATIONS()
                validateBlendColorState();
#endif
                return false;
            }
        }

        blendColor.isInitialized = true;
        blendColor.state = {red, green, blue, alpha};
        return true;
    }

    [[nodiscard]] SNAP_RHI_ALWAYS_INLINE bool shouldBindFramebuffer(FramebufferType framebufferType,
                                                                    GLuint framebuffer) {
        auto& info = bindFramebuffer[static_cast<uint32_t>(framebufferType)];
        if (info == framebuffer) {
#if SNAP_RHI_ENABLE_SLOW_VALIDATIONS()
            validateBindFramebuffer();
#endif
            return false;
        }

        info = framebuffer;
        return true;
    }

    [[nodiscard]] SNAP_RHI_ALWAYS_INLINE bool shouldBindFramebuffer(GLenum target, GLuint framebuffer) {
        if (target == GL_FRAMEBUFFER) {
            const bool readResult = shouldBindFramebuffer(FramebufferType::Read, framebuffer);
            const bool drawResult = shouldBindFramebuffer(FramebufferType::Draw, framebuffer);

            return readResult || drawResult;
        }

        if (target == GL_DRAW_FRAMEBUFFER) {
            return shouldBindFramebuffer(FramebufferType::Draw, framebuffer);
        }

        if (target == GL_READ_FRAMEBUFFER) {
            return shouldBindFramebuffer(FramebufferType::Read, framebuffer);
        }

        return true;
    }

    [[nodiscard]] SNAP_RHI_ALWAYS_INLINE bool shouldViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
        if (viewport.isInitialized) {
            if (viewport.state.x == x && viewport.state.y == y && viewport.state.width == width &&
                viewport.state.height == height) {
#if SNAP_RHI_ENABLE_SLOW_VALIDATIONS()
                validateViewport();
#endif
                return false;
            }
        }

        viewport.isInitialized = true;
        viewport.state = {x, y, width, height};
        return true;
    }

    [[nodiscard]] SNAP_RHI_ALWAYS_INLINE GLuint getBindBufferOrNone(GLenum target) const {
        BufferType type = toBufferType(target);
        if (type == BufferType::Undefined) {
            return GL_NONE;
        }

        const auto& bufferInfo = bindBuffer[static_cast<uint32_t>(type)];
        if (bufferInfo == UndefinedBufferBinding) {
            return GL_NONE;
        }

        return bufferInfo;
    }

    [[nodiscard]] SNAP_RHI_ALWAYS_INLINE bool shouldBindBuffer(GLenum target, GLuint buffer) {
        BufferType type = toBufferType(target);
        if (type == BufferType::Undefined) {
            return true;
        }

        auto& bufferInfo = bindBuffer[static_cast<uint32_t>(type)];

        if (buffer == bufferInfo) {
#if SNAP_RHI_ENABLE_SLOW_VALIDATIONS()
            validateBindBuffer();
#endif
            return false;
        }

        bufferInfo = buffer;
        return true;
    }

    [[nodiscard]] SNAP_RHI_ALWAYS_INLINE bool shouldActiveTexture(GLenum texture) {
        if (texture == activeTexture) {
#if SNAP_RHI_ENABLE_SLOW_VALIDATIONS()
            validateActiveTexture();
#endif
            return false;
        }

        activeTexture = texture;
        return true;
    }

    [[nodiscard]] SNAP_RHI_ALWAYS_INLINE bool shouldBindTexture(GLenum target, GLuint texture) {
        if (activeTexture == UndefinedActiveTexture) {
            return true;
        }

        const GLenum activeTextureIndex = activeTexture - GL_TEXTURE0;
        auto& textureBindingInfo = bindTexture[activeTextureIndex];

        TextureType type = toTextureType(target);
        if (type == TextureType::Undefined) {
            return true;
        }

        auto& textureInfo = textureBindingInfo[static_cast<uint32_t>(type)];
        if (textureInfo == texture) {
#if SNAP_RHI_ENABLE_SLOW_VALIDATIONS()
            validateBindTexture();
#endif
            return false;
        }

        textureInfo = texture;
        return true;
    }

    [[nodiscard]] SNAP_RHI_ALWAYS_INLINE GLuint getBindTextureOrNone(GLenum target) const {
        if (activeTexture == UndefinedActiveTexture) {
            return GL_NONE;
        }

        const GLenum activeTextureIndex = activeTexture - GL_TEXTURE0;
        auto& textureBindingInfo = bindTexture[activeTextureIndex];

        TextureType type = toTextureType(target);
        if (type == TextureType::Undefined) {
            return GL_NONE;
        }

        auto& textureInfo = textureBindingInfo[static_cast<uint32_t>(type)];
        if (textureInfo == UndefinedTextureBinding) {
            return GL_NONE;
        }

        return textureInfo;
    }

    [[nodiscard]] SNAP_RHI_ALWAYS_INLINE bool shouldStencilMaskSeparate(StencilType type, GLuint mask) {
        if (type == StencilType::Undefined) {
            return true;
        }

        auto& info = stencilMaskSeparate[static_cast<uint32_t>(type)];
        if (info.isInitialized && info.state == mask) {
#if SNAP_RHI_ENABLE_SLOW_VALIDATIONS()
            validateStencilMaskSeparate();
#endif
            return false;
        }

        info.isInitialized = true;
        info.state = mask;
        return true;
    }

    [[nodiscard]] SNAP_RHI_ALWAYS_INLINE bool shouldStencilMaskSeparate(GLenum face, GLuint mask) {
        if (face == GL_FRONT_AND_BACK) {
            const bool frontResult = shouldStencilMaskSeparate(StencilType::Front, mask);
            const bool backResult = shouldStencilMaskSeparate(StencilType::Back, mask);

            return frontResult || backResult;
        }

        const StencilType type = toStencilType(face);
        return shouldStencilMaskSeparate(type, mask);
    }

    [[nodiscard]] SNAP_RHI_ALWAYS_INLINE bool shouldStencilFuncSeparate(StencilType type,
                                                                        GLenum func,
                                                                        GLint ref,
                                                                        GLuint mask) {
        if (type == StencilType::Undefined) {
            return true;
        }

        StencilFuncSeparate value{func, ref, mask};

        auto& info = stencilFuncSeparate[static_cast<uint32_t>(type)];
        if (info.isInitialized && info.state == value) {
#if SNAP_RHI_ENABLE_SLOW_VALIDATIONS()
            validateStencilFuncSeparate();
#endif
            return false;
        }

        info.isInitialized = true;
        info.state = value;
        return true;
    }

    [[nodiscard]] SNAP_RHI_ALWAYS_INLINE bool shouldStencilFuncSeparate(GLenum face,
                                                                        GLenum func,
                                                                        GLint ref,
                                                                        GLuint mask) {
        if (face == GL_FRONT_AND_BACK) {
            const bool frontResult = shouldStencilFuncSeparate(StencilType::Front, func, ref, mask);
            const bool backResult = shouldStencilFuncSeparate(StencilType::Back, func, ref, mask);

            return frontResult || backResult;
        }

        const StencilType type = toStencilType(face);
        return shouldStencilFuncSeparate(type, func, ref, mask);
    }

    [[nodiscard]] SNAP_RHI_ALWAYS_INLINE bool shouldStencilFunc(GLenum func, GLint ref, GLuint mask) {
        const bool frontResult = shouldStencilFuncSeparate(StencilType::Front, func, ref, mask);
        const bool backResult = shouldStencilFuncSeparate(StencilType::Back, func, ref, mask);

        return frontResult || backResult;
    }

    [[nodiscard]] SNAP_RHI_ALWAYS_INLINE bool shouldStencilOpSeparate(StencilType type,
                                                                      GLenum sfail,
                                                                      GLenum dpfail,
                                                                      GLenum dppass) {
        if (type == StencilType::Undefined) {
            return true;
        }

        StencilOpSeparate value{sfail, dpfail, dppass};

        auto& info = stencilOpSeparate[static_cast<uint32_t>(type)];
        if (info.isInitialized && info.state == value) {
#if SNAP_RHI_ENABLE_SLOW_VALIDATIONS()
            validateStencilOpSeparate();
#endif
            return false;
        }

        info.isInitialized = true;
        info.state = value;
        return true;
    }

    [[nodiscard]] SNAP_RHI_ALWAYS_INLINE bool shouldStencilOpSeparate(GLenum face,
                                                                      GLenum sfail,
                                                                      GLenum dpfail,
                                                                      GLenum dppass) {
        if (face == GL_FRONT_AND_BACK) {
            const bool frontResult = shouldStencilOpSeparate(StencilType::Front, sfail, dpfail, dppass);
            const bool backResult = shouldStencilOpSeparate(StencilType::Back, sfail, dpfail, dppass);

            return frontResult || backResult;
        }

        const StencilType type = toStencilType(face);
        return shouldStencilOpSeparate(type, sfail, dpfail, dppass);
    }

    [[nodiscard]] SNAP_RHI_ALWAYS_INLINE bool shouldEnable(GLenum cap) {
        return applyEnable(cap, GL_TRUE);
    }

    [[nodiscard]] SNAP_RHI_ALWAYS_INLINE bool shouldDisable(GLenum cap) {
        return applyEnable(cap, GL_FALSE);
    }

    [[nodiscard]] SNAP_RHI_ALWAYS_INLINE bool shouldColorMask(GLboolean red,
                                                              GLboolean green,
                                                              GLboolean blue,
                                                              GLboolean alpha) {
        BlendingState::ColorMask value{red, green, blue, alpha};

        bool isEqual = true;
        for (auto& blending : blending) {
            if (!(blending.colorMask.isInitialized && blending.colorMask.state == value)) {
                isEqual = false;
            }

            blending.colorMask.isInitialized = true;
            blending.colorMask.state = value;
        }

        if (isEqual) {
#if SNAP_RHI_ENABLE_SLOW_VALIDATIONS()
            validateBlendColorMask();
#endif
            return false;
        }
        return true;
    }

    [[nodiscard]] SNAP_RHI_ALWAYS_INLINE bool shouldBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha) {
        BlendingState::BlendEquationSeparate value{modeRGB, modeAlpha};

        bool isEqual = true;
        for (auto& blending : blending) {
            if (!(blending.blendEquationSeparate.isInitialized && blending.blendEquationSeparate.state == value)) {
                isEqual = false;
            }

            blending.blendEquationSeparate.isInitialized = true;
            blending.blendEquationSeparate.state = value;
        }

        if (isEqual) {
#if SNAP_RHI_ENABLE_SLOW_VALIDATIONS()
            validateBlendEquationSeparate();
#endif
            return false;
        }
        return true;
    }

    [[nodiscard]] SNAP_RHI_ALWAYS_INLINE bool shouldBlendFuncSeparate(GLenum srcRGB,
                                                                      GLenum dstRGB,
                                                                      GLenum srcAlpha,
                                                                      GLenum dstAlpha) {
        BlendingState::BlendFuncSeparate value{srcRGB, dstRGB, srcAlpha, dstAlpha};

        bool isEqual = true;
        for (auto& blending : blending) {
            if (!(blending.blendFuncSeparate.isInitialized && blending.blendFuncSeparate.state == value)) {
                isEqual = false;
            }

            blending.blendFuncSeparate.isInitialized = true;
            blending.blendFuncSeparate.state = value;
        }

        if (isEqual) {
#if SNAP_RHI_ENABLE_SLOW_VALIDATIONS()
            validateBlendFuncSeparate();
#endif
            return false;
        }

        return true;
    }

    [[nodiscard]] SNAP_RHI_ALWAYS_INLINE bool shouldEnablei(GLenum cap, GLuint index) {
        return applyEnablei(cap, index, GL_TRUE);
    }

    [[nodiscard]] SNAP_RHI_ALWAYS_INLINE bool shouldDisablei(GLenum cap, GLuint index) {
        return applyEnablei(cap, index, GL_FALSE);
    }

    [[nodiscard]] SNAP_RHI_ALWAYS_INLINE bool shouldColorMaski(
        GLuint buf, GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
        assert(blending.size() > buf);

        BlendingState::ColorMask value{red, green, blue, alpha};
        if (blending[buf].colorMask.isInitialized && blending[buf].colorMask.state == value) {
#if SNAP_RHI_ENABLE_SLOW_VALIDATIONS()
            validateBlendColorMask();
#endif
            return false;
        }

        blending[buf].colorMask.isInitialized = true;
        blending[buf].colorMask.state = value;
        return true;
    }

    [[nodiscard]] SNAP_RHI_ALWAYS_INLINE bool shouldBlendEquationSeparatei(GLuint buf,
                                                                           GLenum modeRGB,
                                                                           GLenum modeAlpha) {
        assert(blending.size() > buf);

        BlendingState::BlendEquationSeparate value{modeRGB, modeAlpha};
        if (blending[buf].blendEquationSeparate.isInitialized && blending[buf].blendEquationSeparate.state == value) {
#if SNAP_RHI_ENABLE_SLOW_VALIDATIONS()
            validateBlendEquationSeparate();
#endif
            return false;
        }

        blending[buf].blendEquationSeparate.isInitialized = true;
        blending[buf].blendEquationSeparate.state = value;
        return true;
    }

    [[nodiscard]] SNAP_RHI_ALWAYS_INLINE bool shouldBlendFuncSeparatei(
        GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) {
        assert(blending.size() > buf);

        BlendingState::BlendFuncSeparate value{srcRGB, dstRGB, srcAlpha, dstAlpha};
        if (blending[buf].blendFuncSeparate.isInitialized && blending[buf].blendFuncSeparate.state == value) {
#if SNAP_RHI_ENABLE_SLOW_VALIDATIONS()
            validateBlendFuncSeparate();
#endif
            return false;
        }

        blending[buf].blendFuncSeparate.isInitialized = true;
        blending[buf].blendFuncSeparate.state = value;
        return true;
    }

    [[nodiscard]] SNAP_RHI_ALWAYS_INLINE bool shouldPolygonOffset(GLfloat factor, GLfloat units) {
        if (polygonOffset.isInitialized) {
            const auto& activePolygonOffset = polygonOffset.state;
            if (snap::rhi::backend::common::epsilonEqual(
                    activePolygonOffset.factor, factor, snap::rhi::backend::common::Epsilon) &&
                snap::rhi::backend::common::epsilonEqual(
                    activePolygonOffset.units, units, snap::rhi::backend::common::Epsilon)) {
#if SNAP_RHI_ENABLE_SLOW_VALIDATIONS()
                validatePolygonOffset();
#endif
                return false;
            }
        }

        polygonOffset.isInitialized = true;
        polygonOffset.state = {factor, units};
        return true;
    }

    [[nodiscard]] SNAP_RHI_ALWAYS_INLINE bool shouldEnableVertexAttribArray(GLuint index, GLboolean value) {
        assert(enableVertexAttribArray.size() > index);

        auto& info = enableVertexAttribArray[index];
        if (info.isInitialized && info.state == value) {
#if SNAP_RHI_ENABLE_SLOW_VALIDATIONS()
            validateVertexAttribArray();
#endif
            return false;
        }

        info.isInitialized = true;
        info.state = value;
        return true;
    }

    [[nodiscard]] SNAP_RHI_ALWAYS_INLINE bool shouldEnableVertexAttribArray(GLuint index) {
        return shouldEnableVertexAttribArray(index, GL_TRUE);
    }

    [[nodiscard]] SNAP_RHI_ALWAYS_INLINE bool shouldDisableVertexAttribArray(GLuint index) {
        return shouldEnableVertexAttribArray(index, GL_FALSE);
    }

    [[nodiscard]] SNAP_RHI_ALWAYS_INLINE bool shouldVertexAttrib(GLuint index,
                                                                 GLint size,
                                                                 GLenum type,
                                                                 GLboolean normalized,
                                                                 GLsizei stride,
                                                                 const void* pointer,
                                                                 GLuint buffer) {
        assert(vertexAttribPointer.size() > index);
        auto& info = vertexAttribPointer[index];

        VertexAttribPointer vertexAttribPointerInfo{size, type, normalized, stride, pointer, buffer};
        if (info.isInitialized && info.state == vertexAttribPointerInfo) {
#if SNAP_RHI_ENABLE_SLOW_VALIDATIONS()
            validateVertexAttribPointer();
#endif
            return false;
        }

        info.isInitialized = true;
        info.state = vertexAttribPointerInfo;
        return true;
    }

    [[nodiscard]] SNAP_RHI_ALWAYS_INLINE bool shouldVertexAttribDivisor(GLuint index, GLuint divisor) {
        assert(vertexAttribDivisor.size() > index);

        auto& info = vertexAttribDivisor[index];
        if (info == divisor) {
            // Disabled because on some Android devices getting GL state natively will have incorrect results
#if SNAP_RHI_ENABLE_SLOW_VALIDATIONS() && 0
            validateVertexAttribDivisor();
#endif
            return false;
        }

        info = divisor;
        return true;
    }

    [[nodiscard]] SNAP_RHI_ALWAYS_INLINE bool shouldBindBufferRange(GLenum target) {
        /**
         * Caching of this function is not yet supported, so we need to reset the buffer cache.
         *
         * https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBindBufferRange.xhtml
         * target must be one of GL_ATOMIC_COUNTER_BUFFER, GL_TRANSFORM_FEEDBACK_BUFFER, GL_UNIFORM_BUFFER, or
         * GL_SHADER_STORAGE_BUFFER.
         */
        const BufferType type = toBufferType(target);
        if (type != BufferType::Undefined) {
            bindBuffer[static_cast<uint32_t>(type)] = UndefinedBufferBinding;
        }

        return true;
    }

private:
    [[nodiscard]] SNAP_RHI_ALWAYS_INLINE bool applyEnable(GLenum cap, GLboolean value) {
        if (cap == GL_BLEND) {
            bool result = false;

            for (size_t index = 0; index < blending.size(); ++index) {
                if (!(blending[index].enabled.isInitialized && blending[index].enabled.state.enabled == value)) {
                    result = true;
                }

                blending[index].enabled.isInitialized = true;
                blending[index].enabled.state.enabled = value;
            }

            return result;
        } else {
            const EnableType type = toEnableType(cap);
            if (type == EnableType::Undefined) {
                return true;
            }

            auto& info = enable[static_cast<uint32_t>(type)];
            if (info.isInitialized && info.state == value) {
                return false;
            }

            info.isInitialized = true;
            info.state = value;
        }
        return true;
    }

    [[nodiscard]] SNAP_RHI_ALWAYS_INLINE bool applyEnablei(GLenum cap, GLuint index, GLboolean value) {
        if (cap == GL_BLEND) {
            assert(blending.size() > index);

            if (blending[index].enabled.isInitialized && blending[index].enabled.state.enabled == value) {
                return false;
            }

            blending[index].enabled.isInitialized = true;
            blending[index].enabled.state.enabled = value;
        } else {
            const EnableType type = toEnableType(cap);
            if (type == EnableType::Undefined) {
                return true;
            }

            auto& info = enable[static_cast<uint32_t>(type)];
            if (index == 0) {
                if (info.isInitialized && info.state == value) {
                    return false;
                }

                info.isInitialized = true;
                info.state = value;
            } else {
                // we do not support cache for this case
                info.isInitialized = false;
            }
        }
        return true;
    }

#if SNAP_RHI_ENABLE_SLOW_VALIDATIONS()
    void validateProgramState();
    void validateCullFaceState();
    void validateDepthFuncState();
    void validateBlendColorState();

    void validateBindFramebuffer();
    void validateBindBuffer();

    void validateActiveTexture();
    void validateBindTexture();
    void validateBindSampler();

    void validateStencilMaskSeparate();
    void validateStencilFuncSeparate();
    void validateStencilOpSeparate();

    void validateEnable();
    void validateBlendColorMask();
    void validateBlendEquationSeparate();
    void validateBlendFuncSeparate();
    void validateViewport();

    void validatePolygonOffset();
    void validateVertexAttribArray();
    void validateVertexAttribDivisor();
    void validateVertexAttribPointer();
#endif

private:
    snap::rhi::backend::opengl::Device* device = nullptr;
    const snap::rhi::backend::common::ValidationLayer& validationLayer;
    const bool isStateValidationsEnabled = false;

    std::vector<GLint> bindSampler{};

    /**
     * If a program object is in use as part of current rendering state, it will be flagged for deletion,
     * but it will not be deleted until it is no longer part of current state for any rendering context.
     */
    GLint useProgram = UndefinedProgramBinding;

    /**
     * If a framebuffer object that is currently bound is deleted, the binding reverts to 0 (the window-system-provided
     * framebuffer).
     */
    std::array<GLint, static_cast<uint32_t>(FramebufferType::Count)> bindFramebuffer{};

    /**
     * If a buffer object that is currently bound is deleted, the binding reverts to 0 (the absence of any buffer
     * object).
     */
    std::array<GLint, static_cast<uint32_t>(BufferType::Count)> bindBuffer{};

    /**
     * If a texture that is currently bound is deleted, the binding reverts to 0 (the default texture).
     */
    std::vector<TexturesState> bindTexture{};
    GLint activeTexture = UndefinedActiveTexture;

    GLint cullFace = UndefinedCullFace;
    GLint depthFunc = UndefinedDepthFunc;
    GLState<BlendColor> blendColor{};
    GLState<Viewport> viewport{};

    std::array<GLState<GLint>, static_cast<uint32_t>(StencilType::Count)> stencilMaskSeparate{};
    std::array<GLState<StencilFuncSeparate>, static_cast<uint32_t>(StencilType::Count)> stencilFuncSeparate{};
    std::array<GLState<StencilOpSeparate>, static_cast<uint32_t>(StencilType::Count)> stencilOpSeparate{};

    std::array<GLState<GLboolean>, static_cast<uint32_t>(EnableType::Count)> enable{};
    std::vector<BlendingState> blending{};

    GLState<PolygonOffset> polygonOffset{};

    std::vector<GLState<GLboolean>> enableVertexAttribArray{};
    std::vector<GLint> vertexAttribDivisor{};
    std::vector<GLState<VertexAttribPointer>> vertexAttribPointer{};

    const Features& features;
};
} // namespace snap::rhi::backend::opengl
