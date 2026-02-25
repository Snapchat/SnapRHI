#include "snap/rhi/backend/opengl/Sampler.hpp"
#include "snap/rhi/Common.h"
#include "snap/rhi/backend/common/Logging.hpp"
#include "snap/rhi/backend/common/Math.hpp"
#include "snap/rhi/backend/opengl/Context.h"
#include "snap/rhi/backend/opengl/Device.hpp"
#include "snap/rhi/backend/opengl/ErrorCheckGuard.hpp"
#include "snap/rhi/backend/opengl/OpenGL.h"
#include "snap/rhi/backend/opengl/Profile.hpp"
#include "snap/rhi/backend/opengl/Texture.hpp"

#include <cassert>

namespace {
void setupMagFilter(const snap::rhi::backend::opengl::Profile& gl,
                    snap::rhi::SamplerMinMagFilter magFilter,
                    const GLuint sampler) {
    switch (magFilter) {
        case snap::rhi::SamplerMinMagFilter::Nearest: {
            gl.samplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        } break;

        case snap::rhi::SamplerMinMagFilter::Linear: {
            gl.samplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        } break;

        default:
            snap::rhi::common::throwException("invalid magFilter");
    }
}

void setupMinMipFilter(const snap::rhi::backend::opengl::Profile& gl,
                       snap::rhi::SamplerMinMagFilter minFilter,
                       snap::rhi::SamplerMipFilter mipFilter,
                       const GLuint sampler) {
    if (mipFilter == snap::rhi::SamplerMipFilter::NotMipmapped) {
        switch (minFilter) {
            case snap::rhi::SamplerMinMagFilter::Nearest: {
                gl.samplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            } break;

            case snap::rhi::SamplerMinMagFilter::Linear: {
                gl.samplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            } break;

            default:
                snap::rhi::common::throwException("invalid SamplerMinMagFilter");
        }
    } else if (mipFilter == snap::rhi::SamplerMipFilter::Nearest) {
        switch (minFilter) {
            case snap::rhi::SamplerMinMagFilter::Nearest: {
                gl.samplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
            } break;

            case snap::rhi::SamplerMinMagFilter::Linear: {
                gl.samplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
            } break;

            default:
                snap::rhi::common::throwException("invalid SamplerMinMagFilter");
        }
    } else if (mipFilter == snap::rhi::SamplerMipFilter::Linear) {
        switch (minFilter) {
            case snap::rhi::SamplerMinMagFilter::Nearest: {
                gl.samplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
            } break;

            case snap::rhi::SamplerMinMagFilter::Linear: {
                gl.samplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            } break;

            default:
                snap::rhi::common::throwException("invalid SamplerMinMagFilter");
        }
    } else {
        snap::rhi::common::throwException("invalid SamplerMipFilter");
    }
}

GLenum convertToGL(const snap::rhi::WrapMode mode) {
    switch (mode) {
        case snap::rhi::WrapMode::ClampToEdge:
            return GL_CLAMP_TO_EDGE;
        case snap::rhi::WrapMode::ClampToBorderColor:
            return GL_CLAMP_TO_BORDER; // only with extension
        case snap::rhi::WrapMode::Repeat:
            return GL_REPEAT;
        case snap::rhi::WrapMode::MirrorRepeat:
            return GL_MIRRORED_REPEAT;

        default:
            snap::rhi::common::throwException("invalid WrapMode");
    }
}

void setupWrapModes(const snap::rhi::backend::opengl::Profile& gl,
                    const snap::rhi::WrapMode wrapU,
                    const snap::rhi::WrapMode wrapV,
                    const snap::rhi::WrapMode wrapW,
                    const snap::rhi::SamplerBorderColor borderColor,
                    const GLuint sampler) {
    gl.samplerParameteri(sampler, GL_TEXTURE_WRAP_S, convertToGL(wrapU));
    gl.samplerParameteri(sampler, GL_TEXTURE_WRAP_T, convertToGL(wrapV));
    gl.samplerParameteri(sampler, GL_TEXTURE_WRAP_R, convertToGL(wrapW));

    // https://www.khronos.org/registry/OpenGL/extensions/OES/OES_texture_border_clamp.txt
    const bool needBorderColor = wrapU == snap::rhi::WrapMode::ClampToBorderColor ||
                                 wrapV == snap::rhi::WrapMode::ClampToBorderColor ||
                                 wrapW == snap::rhi::WrapMode::ClampToBorderColor;
    if (needBorderColor) {
        switch (borderColor) {
            case snap::rhi::SamplerBorderColor::TransparentBlack: {
                static constexpr std::array<float, 4> TransparentBlack{0.0f, 0.0f, 0.0f, 0.0f};
                gl.samplerParameterfv(sampler, GL_TEXTURE_BORDER_COLOR, TransparentBlack.data());
            } break;

            case snap::rhi::SamplerBorderColor::OpaqueBlack: {
                static constexpr std::array<float, 4> OpaqueBlack{0.0f, 0.0f, 0.0f, 1.0f};
                gl.samplerParameterfv(sampler, GL_TEXTURE_BORDER_COLOR, OpaqueBlack.data());
            } break;

            case snap::rhi::SamplerBorderColor::OpaqueWhite: {
                static constexpr std::array<float, 4> OpaqueWhite{1.0f, 1.0f, 1.0f, 1.0f};
                gl.samplerParameterfv(sampler, GL_TEXTURE_BORDER_COLOR, OpaqueWhite.data());
            } break;

            default:
                snap::rhi::common::throwException("[setupWrapModes] invalid borderColor");
        }
    }
}

void setupMinMaxLod(const snap::rhi::backend::opengl::Profile& gl,
                    const float lodMin,
                    const float lodMax,
                    const GLuint sampler) {
    gl.samplerParameterf(sampler, GL_TEXTURE_MIN_LOD, lodMin);
    gl.samplerParameterf(sampler, GL_TEXTURE_MAX_LOD, lodMax);
}

GLenum convertToGL(const snap::rhi::CompareFunction compareFunction) {
    switch (compareFunction) {
        case snap::rhi::CompareFunction::Never:
            return GL_NEVER;

        case snap::rhi::CompareFunction::Less:
            return GL_LESS;

        case snap::rhi::CompareFunction::Equal:
            return GL_EQUAL;

        case snap::rhi::CompareFunction::LessEqual:
            return GL_LEQUAL;

        case snap::rhi::CompareFunction::Greater:
            return GL_GREATER;

        case snap::rhi::CompareFunction::NotEqual:
            return GL_NOTEQUAL;

        case snap::rhi::CompareFunction::GreaterEqual:
            return GL_GEQUAL;

        case snap::rhi::CompareFunction::Always:
            return GL_ALWAYS;

        default:
            snap::rhi::common::throwException("invalid WrapMode");
    }
}

void setupCompare(const snap::rhi::backend::opengl::Profile& gl,
                  const bool compareEnable,
                  const snap::rhi::CompareFunction compareFunction,
                  const GLuint sampler) {
    if (compareEnable) {
        gl.samplerParameteri(sampler, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        gl.samplerParameteri(sampler, GL_TEXTURE_COMPARE_FUNC, convertToGL(compareFunction));
    } else {
        gl.samplerParameteri(sampler, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    }
}

void initializeSampler(const snap::rhi::backend::opengl::Profile& gl,
                       const GLuint sampler,
                       const snap::rhi::SamplerCreateInfo& info) {
    setupMagFilter(gl, info.magFilter, sampler);
    setupMinMipFilter(gl, info.minFilter, info.mipFilter, sampler);
    setupWrapModes(gl, info.wrapU, info.wrapV, info.wrapW, info.borderColor, sampler);
    setupMinMaxLod(gl, info.lodMin, info.lodMax, sampler);
    setupCompare(gl, info.compareEnable, info.compareFunction, sampler);

    const auto& caps = gl.getDevice()->getCapabilities();
    SNAP_RHI_VALIDATE(gl.getDevice()->getValidationLayer(),
                      !info.anisotropyEnable,
                      snap::rhi::ReportLevel::Warning,
                      snap::rhi::ValidationTag::SamplerOp,
                      "[initializeSampler] OpenGL doesn't supproted AnisotropicFiltering");
    SNAP_RHI_VALIDATE(gl.getDevice()->getValidationLayer(),
                      caps.isSamplerUnnormalizedCoordsSupported || !info.unnormalizedCoordinates,
                      snap::rhi::ReportLevel::Warning,
                      snap::rhi::ValidationTag::SamplerOp,
                      "[initializeSampler] OpenGL doesn't supproted unnormalized coordinates");
}

void applyFilteringMode(const snap::rhi::backend::opengl::Profile& gl,
                        const snap::rhi::Capabilities& caps,
                        snap::rhi::SamplerMipFilter mipFilter,
                        snap::rhi::SamplerMinMagFilter minFilter,
                        snap::rhi::SamplerMinMagFilter magFilter,
                        const snap::rhi::backend::opengl::TextureTarget target,
                        const bool isTextureUseMipMap,
                        bool isPowOfTwo) {
    switch (magFilter) {
        case snap::rhi::SamplerMinMagFilter::Nearest: {
            gl.texParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        } break;

        case snap::rhi::SamplerMinMagFilter::Linear: {
            gl.texParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        } break;

        default:
            snap::rhi::common::throwException("invalid WrapMode");
    }

    if (mipFilter == snap::rhi::SamplerMipFilter::NotMipmapped) {
        switch (minFilter) {
            case snap::rhi::SamplerMinMagFilter::Nearest: {
                gl.texParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            } break;

            case snap::rhi::SamplerMinMagFilter::Linear: {
                gl.texParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            } break;

            default:
                snap::rhi::common::throwException("invalid SamplerMinMagFilter");
        }
    } else if (mipFilter == snap::rhi::SamplerMipFilter::Nearest) {
        switch (minFilter) {
            case snap::rhi::SamplerMinMagFilter::Nearest: {
                gl.texParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
            } break;

            case snap::rhi::SamplerMinMagFilter::Linear: {
                gl.texParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
            } break;

            default:
                snap::rhi::common::throwException("invalid SamplerMinMagFilter");
        }
    } else if (mipFilter == snap::rhi::SamplerMipFilter::Linear) {
        switch (minFilter) {
            case snap::rhi::SamplerMinMagFilter::Nearest: {
                gl.texParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
            } break;

            case snap::rhi::SamplerMinMagFilter::Linear: {
                gl.texParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            } break;

            default:
                snap::rhi::common::throwException("invalid SamplerMinMagFilter");
        }
    } else {
        snap::rhi::common::throwException("invalid SamplerMipFilter");
    }
}

void applyWrapMode(const snap::rhi::backend::opengl::Profile& gl,
                   const snap::rhi::Capabilities& caps,
                   snap::rhi::backend::opengl::TextureTarget target,
                   GLenum side,
                   GLint wrapMode,
                   bool isPowOfTwo) {
    if (wrapMode == GL_CLAMP_TO_EDGE) {
        gl.texParameteri(target, side, GL_CLAMP_TO_EDGE);
        return;
    }

    if (!isPowOfTwo && !caps.isNPOTWrapModeSupported) {
        // ES2 only
        // https://www.khronos.org/registry/OpenGL-Refpages/es2.0/xhtml/glTexParameter.xml
        // Similarly, if the width or height of a texture image are not powers of two and either the
        // GL_TEXTURE_MIN_FILTER is set to one of the functions that requires mipmaps or the GL_TEXTURE_WRAP_S or
        // GL_TEXTURE_WRAP_T is not set to GL_CLAMP_TO_EDGE, then the texture image unit will return (R, G, B, A) = (0,
        // 0, 0, 1).
        SNAP_RHI_REPORT(gl.getDevice()->getValidationLayer(),
                        snap::rhi::ReportLevel::Warning,
                        snap::rhi::ValidationTag::SamplerOp,
                        "NPOT texture used with repeat wrap mode");
    }

    gl.texParameteri(target, side, wrapMode);
}

void applyWrapModes(const snap::rhi::backend::opengl::Profile& gl,
                    const snap::rhi::Capabilities& caps,
                    const snap::rhi::backend::opengl::TextureTarget target,
                    const bool isPowOfTwo,
                    const bool is3DTexture,
                    const snap::rhi::SamplerCreateInfo& samplerInfo) {
    applyWrapMode(gl, caps, target, GL_TEXTURE_WRAP_S, convertToGL(samplerInfo.wrapU), isPowOfTwo);
    applyWrapMode(gl, caps, target, GL_TEXTURE_WRAP_T, convertToGL(samplerInfo.wrapV), isPowOfTwo);
    if (is3DTexture) {
        applyWrapMode(gl, caps, target, GL_TEXTURE_WRAP_R, convertToGL(samplerInfo.wrapW), isPowOfTwo);
    }

    const bool needBorderColor = samplerInfo.wrapU == snap::rhi::WrapMode::ClampToBorderColor ||
                                 samplerInfo.wrapV == snap::rhi::WrapMode::ClampToBorderColor ||
                                 samplerInfo.wrapW == snap::rhi::WrapMode::ClampToBorderColor;
    if (needBorderColor) {
        switch (samplerInfo.borderColor) {
            case snap::rhi::SamplerBorderColor::TransparentBlack: {
                static constexpr std::array<float, 4> TransparentBlack{0.0f, 0.0f, 0.0f, 0.0f};
                gl.texParameterfv(target, GL_TEXTURE_BORDER_COLOR, TransparentBlack.data());
            } break;

            case snap::rhi::SamplerBorderColor::OpaqueBlack: {
                static constexpr std::array<float, 4> OpaqueBlack{0.0f, 0.0f, 0.0f, 1.0f};
                gl.texParameterfv(target, GL_TEXTURE_BORDER_COLOR, OpaqueBlack.data());
            } break;

            case snap::rhi::SamplerBorderColor::OpaqueWhite: {
                static constexpr std::array<float, 4> OpaqueWhite{1.0f, 1.0f, 1.0f, 1.0f};
                gl.texParameterfv(target, GL_TEXTURE_BORDER_COLOR, OpaqueWhite.data());
            } break;

            default:
                snap::rhi::common::throwException("[setWrapModes] invalid borderColor");
        }
    }
}

void applyTextureLod(const snap::rhi::backend::opengl::Profile& gl,
                     snap::rhi::backend::opengl::TextureTarget target,
                     float lodMin,
                     float lodMax) {
    const auto& caps = gl.getDevice()->getCapabilities();
    if (caps.isSamplerMinMaxLodSupported) {
        gl.texParameterf(target, GL_TEXTURE_MIN_LOD, lodMin);
        gl.texParameterf(target, GL_TEXTURE_MAX_LOD, lodMax);
        gl.texParameterf(target, GL_TEXTURE_MAX_LEVEL, lodMax);
    }
}

/**
 * Corner cases:
 *
 * snap::rhi::WrapMode::ClampToBorderColor supporting
 *
 * Non PowOfTwo textures and WrapMode(REPEAT and MIRRORED_REPEAT, ClampToBorderColor) and GL_OES_texture_npot
 * By default:
 * If the width or height of a texture image are not powers of two and either the GL_TEXTURE_MIN_FILTER is set to one
 * of the functions that requires mipmaps or the GL_TEXTURE_WRAP_S or GL_TEXTURE_WRAP_T is not set to GL_CLAMP_TO_EDGE,
 * then the texture image unit will return (R, G, B, A) = (0, 0, 0, 1).
 *
 * Texture3D/Texture2DArray supporting
 *
 * DepthCompareFunc supporting ES3.0 => (c++: GL_TEXTURE_COMPARE_MODE, glsl: sampler2DShadow + texture func)
 * OpenGL ES 2.0 : OES_depth_texture(DepthCompareFunc not supported)
 * //https://gamedev.stackexchange.com/questions/74508/shadow-map-depth-texture-always-returns-0
 *
 * minLod/maxLod not suppored for OpenGL ES 2.0
 **/
void applySamplerForActiveTexture(snap::rhi::backend::opengl::Profile& gl,
                                  const snap::rhi::SamplerCreateInfo& info,
                                  const bool isPowOfTwo,
                                  const bool isTextureUseMipMap,
                                  const bool is3DTexture,
                                  const snap::rhi::backend::opengl::TextureTarget target) {
    const auto& caps = gl.getDevice()->getCapabilities();

    applyFilteringMode(
        gl, caps, info.mipFilter, info.minFilter, info.magFilter, target, isTextureUseMipMap, isPowOfTwo);
    applyWrapModes(gl, caps, target, isPowOfTwo, is3DTexture, info);
    applyTextureLod(gl, target, info.lodMin, info.lodMax);

    SNAP_RHI_VALIDATE(gl.getDevice()->getValidationLayer(),
                      !info.anisotropyEnable,
                      snap::rhi::ReportLevel::Warning,
                      snap::rhi::ValidationTag::SamplerOp,
                      "[PipelineTextureBindingState::setState] Legacy OpenGL(ES 2.0/ GL "
                      "2.1) doesnt supported anisotropyEnable = true");
    SNAP_RHI_VALIDATE(
        gl.getDevice()->getValidationLayer(),
        caps.isSamplerCompareFuncSupported || !info.compareEnable,
        snap::rhi::ReportLevel::Warning,
        snap::rhi::ValidationTag::SamplerOp,
        "[PipelineTextureBindingState::setState] Legacy OpenGL(ES 2.0/ GL 2.1) supported only CompareFunction::Always");
    SNAP_RHI_VALIDATE(
        gl.getDevice()->getValidationLayer(),
        caps.isSamplerMinMaxLodSupported ||
            (snap::rhi::backend::common::epsilonEqual(info.lodMin, 0.0f, snap::rhi::backend::common::Epsilon) &&
             snap::rhi::backend::common::epsilonEqual(
                 info.lodMax, snap::rhi::DefaultSamplerLodMax, snap::rhi::backend::common::Epsilon)),
        snap::rhi::ReportLevel::Warning,
        snap::rhi::ValidationTag::SamplerOp,
        "[PipelineTextureBindingState::setState] Legacy OpenGL(ES 2.0/ GL 2.1) supported only lodMin = 0.0f, lodMax = "
        "1000.0f(DefaulSamplerLodMax)");
    SNAP_RHI_VALIDATE(gl.getDevice()->getValidationLayer(),
                      caps.isSamplerUnnormalizedCoordsSupported || !info.unnormalizedCoordinates,
                      snap::rhi::ReportLevel::Warning,
                      snap::rhi::ValidationTag::SamplerOp,
                      "[PipelineTextureBindingState::setState] Legacy OpenGL(ES 2.0/ GL "
                      "2.1) doesn't supported [unnormalizedCoordinates = true]");
}
} // unnamed namespace

namespace snap::rhi::backend::opengl {
Sampler::Sampler(snap::rhi::backend::opengl::Device* device, const snap::rhi::SamplerCreateInfo& info)
    : snap::rhi::Sampler(device, info), gl(device->getOpenGL()) {
    if (!device->areResourcesLazyAllocationsEnabled()) {
        tryAllocate();
    }
}

Sampler::~Sampler() {
    try {
        auto* glDevice = snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Device>(device);

        SNAP_RHI_REPORT(glDevice->getValidationLayer(),
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::DestroyOp,
                        "[snap::rhi::backend::opengl::~Sampler] start destruction");
        SNAP_RHI_VALIDATE(glDevice->getValidationLayer(),
                          device->getCurrentDeviceContext(),
                          snap::rhi::ReportLevel::Warning,
                          snap::rhi::ValidationTag::DestroyOp,
                          "[snap::rhi::backend::opengl::~Sampler] DeviceContext isn't attached to thread");
        SNAP_RHI_VALIDATE(glDevice->getValidationLayer(),
                          glDevice->isNativeContextAttached(),
                          snap::rhi::ReportLevel::CriticalError,
                          snap::rhi::ValidationTag::DestroyOp,
                          "[snap::rhi::backend::opengl::~Sampler] GL context isn't attached to thread");
        if (sampler != GL_NONE) {
            gl.deleteSamplers(1, &sampler);
        }

        sampler = GL_NONE;
        SNAP_RHI_REPORT(glDevice->getValidationLayer(),
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::DestroyOp,
                        "[snap::rhi::backend::opengl::~Sampler] end of destruction");
    } catch (const snap::rhi::common::Exception& e) {
        SNAP_RHI_LOGE("[snap::rhi::backend::opengl::Sampler::~Sampler] Caught: %s, (possible resource leak).",
                      e.what());
    } catch (...) {
        SNAP_RHI_LOGE(
            "[snap::rhi::backend::opengl::Sampler::~Sampler] Caught unexpected error (possible resource leak).");
    }
}

void Sampler::tryAllocate() const {
    if ((sampler != GL_NONE) || !gl.getFeatures().isSamplerSupported) {
        return;
    }

    if (gl.getFeatures().isSamplerSupported) {
        gl.genSamplers(1, &sampler);
        initializeSampler(gl, sampler, info);
    }
}

void Sampler::bindTextureSampler(GLuint unit,
                                 const snap::rhi::backend::opengl::Texture* texture,
                                 const TextureTarget target,
                                 DeviceContext* dc) const {
    tryAllocate();

    gl.activeTexture(GL_TEXTURE0 + unit, dc);
    gl.bindTexture(target, texture->getTextureID(dc), dc);

    if (sampler) {
        gl.bindSampler(unit, sampler, dc);
    } else {
        const auto& textureInfo = texture->getCreateInfo();
        const bool isPowOfTwo = isSizePowerOfTwo(textureInfo);
        const bool isTextureUseMipMap = textureInfo.mipLevels > 1;
        const bool is3DTexture = textureInfo.textureType == snap::rhi::TextureType::Texture3D;

        applySamplerForActiveTexture(gl, info, isPowOfTwo, isTextureUseMipMap, is3DTexture, target);
    }
}

void Sampler::setDebugLabel(std::string_view label) {
#if SNAP_RHI_ENABLE_DEBUG_LABELS
    ObjectDebugMarkers::setDebugLabel(label);
    if (sampler != GL_NONE) {
        gl.objectLabel(GL_SAMPLER, sampler, label);
    }
#endif
}

} // namespace snap::rhi::backend::opengl
