#include "snap/rhi/backend/opengl/Profile.hpp"
#include "snap/rhi/Enums.h"
#include "snap/rhi/backend/common/Math.hpp"
#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/opengl/Device.hpp"
#include "snap/rhi/backend/opengl/DeviceContext.hpp"
#include "snap/rhi/backend/opengl/OpenGL.h"
#include "snap/rhi/backend/opengl/TextureTarget.h"
#include "snap/rhi/backend/opengl/Utils.hpp"

#include "snap/rhi/backend/common/Logging.hpp"

#include "snap/rhi/common/Throw.h"
#include <algorithm>
#include <cassert>
#include <string_view>
#include <tuple>
#include <vector>

#include "GLES20/Instance.hpp"
#include "GLES30/Instance.hpp"
#include "GLES31/Instance.hpp"
#include "GLES32/Instance.hpp"

#include "GL21/Instance.hpp"
#include "GL41/Instance.hpp"
#include "GL45/Instance.hpp"

#include <cinttypes>

namespace {

template<typename T>
concept RequiresApiVersion = requires { T::buildFeatures(std::declval<gl::APIVersion>()); };

template<typename T>
snap::rhi::backend::opengl::Instance buildInstance(gl::APIVersion realApiVersion) {
    snap::rhi::backend::opengl::Instance result{};

    if constexpr (RequiresApiVersion<T>) {
        result.features = T::buildFeatures(realApiVersion);
    } else {
        result.features = T::buildFeatures();
    }

    result.texSubImage3D = T::texSubImage3D;
    result.texImage3D = T::texImage3D;
    result.texStorage2D = T::texStorage2D;
    result.texStorage3D = T::texStorage3D;
    result.texStorage2DMultisample = T::texStorage2DMultisample;
    result.texStorage3DMultisample = T::texStorage3DMultisample;
    result.copyImageSubData = T::copyImageSubData;
    result.renderbufferStorageMultisample = T::renderbufferStorageMultisample;
    result.drawBuffers = T::drawBuffers;
    result.readBuffer = T::readBuffer;
    result.getActiveUniformBlockName = T::getActiveUniformBlockName;
    result.getUniformBlockIndex = T::getUniformBlockIndex;
    result.getActiveUniformBlockiv = T::getActiveUniformBlockiv;
    result.getActiveUniformsiv = T::getActiveUniformsiv;
    result.uniformBlockBinding = T::uniformBlockBinding;
    result.uniform1uiv = T::uniform1uiv;
    result.uniform2uiv = T::uniform2uiv;
    result.uniform3uiv = T::uniform3uiv;
    result.uniform4uiv = T::uniform4uiv;
    result.bindImageTexture = T::bindImageTexture;
    result.getProgramResourceiv = T::getProgramResourceiv;
    result.getProgramInterfaceiv = T::getProgramInterfaceiv;
    result.getProgramResourceName = T::getProgramResourceName;
    result.getProgramResourceIndex = T::getProgramResourceIndex;
    result.fenceSync = T::fenceSync;
    result.waitSync = T::waitSync;
    result.clientWaitSync = T::clientWaitSync;
    result.deleteSync = T::deleteSync;
    result.getInternalformativ = T::getInternalformativ;
    result.memoryBarrierByRegion = T::memoryBarrierByRegion;
    result.discardFramebuffer = T::discardFramebuffer;
    result.genSamplers = T::genSamplers;
    result.deleteSamplers = T::deleteSamplers;
    result.bindSampler = T::bindSampler;
    result.samplerParameterf = T::samplerParameterf;
    result.samplerParameteri = T::samplerParameteri;
    result.samplerParameterfv = T::samplerParameterfv;
    result.isSampler = T::isSampler;
    result.copyBufferSubData = T::copyBufferSubData;
    result.blitFramebuffer = T::blitFramebuffer;
    result.genVertexArrays = T::genVertexArrays;
    result.bindVertexArray = T::bindVertexArray;
    result.deleteVertexArrays = T::deleteVertexArrays;
    result.clearBufferiv = T::clearBufferiv;
    result.clearBufferuiv = T::clearBufferuiv;
    result.clearBufferfv = T::clearBufferfv;
    result.clearBufferfi = T::clearBufferfi;
    result.bindBufferRange = T::bindBufferRange;
    result.flushMappedBufferRange = T::flushMappedBufferRange;
    result.mapBufferRange = T::mapBufferRange;
    result.unmapBuffer = T::unmapBuffer;
    result.drawArraysInstanced = T::drawArraysInstanced;
    result.drawElementsInstanced = T::drawElementsInstanced;
    result.vertexAttribDivisor = T::vertexAttribDivisor;
    result.vertexAttribI4iv = T::vertexAttribI4iv;
    result.vertexAttribI4uiv = T::vertexAttribI4uiv;
    result.framebufferTextureLayer = T::framebufferTextureLayer;
    result.framebufferTextureMultiviewOVR = T::framebufferTextureMultiviewOVR;
    result.framebufferTextureMultisampleMultiviewOVR = T::framebufferTextureMultisampleMultiviewOVR;
    result.colorMaski = T::colorMaski;
    result.enablei = T::enablei;
    result.disablei = T::disablei;
    result.blendEquationSeparatei = T::blendEquationSeparatei;
    result.blendFuncSeparatei = T::blendFuncSeparatei;
    result.dispatchCompute = T::dispatchCompute;
    result.hint = T::hint;
    result.getTexLevelParameteriv = T::getTexLevelParameteriv;
    result.getTexParameteriv = T::getTexParameteriv;
    result.pixelStorei = T::pixelStorei;
    result.programParameteri = T::programParameteri;
    result.programBinary = T::programBinary;
    result.getProgramBinary = T::getProgramBinary;
    result.copyTexSubImage3D = T::copyTexSubImage3D;
    result.genQueries = T::genQueries;
    result.deleteQueries = T::deleteQueries;
    result.isQuery = T::isQuery;
    result.beginQuery = T::beginQuery;
    result.endQuery = T::endQuery;
    result.getQueryiv = T::getQueryiv;
    result.getQueryObjectiv = T::getQueryObjectiv;
    result.getQueryObjectuiv = T::getQueryObjectuiv;
    result.queryCounter = T::queryCounter;
    result.getQueryObjecti64v = T::getQueryObjecti64v;
    result.getQueryObjectui64v = T::getQueryObjectui64v;
    result.getInteger64v = T::getInteger64v;

    return result;
}

snap::rhi::SampleCount getSampleCount(const snap::rhi::backend::opengl::Instance& gl,
                                      snap::rhi::backend::opengl::TextureTarget target,
                                      snap::rhi::backend::opengl::SizedInternalFormat internalformat) {
    GLint sampleArrSize = 0;
    gl.getInternalformativ(target, internalformat, GL_NUM_SAMPLE_COUNTS, 1, &sampleArrSize);
    std::vector<GLint> samples(sampleArrSize);
    gl.getInternalformativ(target, internalformat, GL_SAMPLES, sampleArrSize, samples.data());
    auto maxPos = std::max_element(samples.begin(), samples.end());
    if (maxPos != samples.end()) {
        GLint sampleCount = *maxPos;

        if (sampleCount >= static_cast<uint32_t>(snap::rhi::SampleCount::Count64)) {
            return snap::rhi::SampleCount::Count64;
        }

        if (sampleCount >= static_cast<uint32_t>(snap::rhi::SampleCount::Count32)) {
            return snap::rhi::SampleCount::Count32;
        }

        if (sampleCount >= static_cast<uint32_t>(snap::rhi::SampleCount::Count16)) {
            return snap::rhi::SampleCount::Count16;
        }

        if (sampleCount >= static_cast<uint32_t>(snap::rhi::SampleCount::Count8)) {
            return snap::rhi::SampleCount::Count8;
        }

        if (sampleCount >= static_cast<uint32_t>(snap::rhi::SampleCount::Count4)) {
            return snap::rhi::SampleCount::Count4;
        }

        if (sampleCount >= static_cast<uint32_t>(snap::rhi::SampleCount::Count2)) {
            return snap::rhi::SampleCount::Count2;
        }
    }
    return snap::rhi::SampleCount::Count1;
}

void initPixelFormatProperties(snap::rhi::Capabilities& capabilities,
                               const snap::rhi::backend::opengl::Instance& glInstance) {
    capabilities.formatProperties.fill({});
    for (uint32_t i = 0; i < static_cast<uint32_t>(snap::rhi::PixelFormat::Count); ++i) {
        auto& formatProperty = capabilities.formatProperties[i];
        auto& formatFeatures = formatProperty.textureFeatures;

        bool isDepth = snap::rhi::hasDepthAspect(static_cast<snap::rhi::PixelFormat>(i));

        if (glInstance.features.textureFormat[i].format != snap::rhi::backend::opengl::FormatGroup::UNKNOWN) {
            if (glInstance.features.textureFormatOpInfo[i].isReadable()) {
                formatFeatures =
                    static_cast<snap::rhi::FormatFeatures>(formatFeatures | snap::rhi::FormatFeatures::TransferSrc);
            }

            if (glInstance.features.textureFormatOpInfo[i].isStorage()) {
                formatFeatures =
                    static_cast<snap::rhi::FormatFeatures>(formatFeatures | snap::rhi::FormatFeatures::Storage);
            }

            if (glInstance.features.textureFormatOpInfo[i].isCopyable()) {
                formatFeatures =
                    static_cast<snap::rhi::FormatFeatures>(formatFeatures | snap::rhi::FormatFeatures::BlitDst);
                formatFeatures =
                    static_cast<snap::rhi::FormatFeatures>(formatFeatures | snap::rhi::FormatFeatures::BlitSrc);
            }

            if (glInstance.features.textureFormatOpInfo[i].isUploadable()) {
                formatFeatures =
                    static_cast<snap::rhi::FormatFeatures>(formatFeatures | snap::rhi::FormatFeatures::TransferDst);
            }

            if (glInstance.features.textureFormatOpInfo[i].isRenderable()) {
                if (isDepth) {
                    formatFeatures = static_cast<snap::rhi::FormatFeatures>(
                        formatFeatures | snap::rhi::FormatFeatures::DepthStencilRenderable);
                } else {
                    formatFeatures = static_cast<snap::rhi::FormatFeatures>(formatFeatures |
                                                                            snap::rhi::FormatFeatures::ColorRenderable);
                }
            }

            if (glInstance.features.textureFormatOpInfo[i].formatFilteringType ==
                snap::rhi::backend::opengl::FormatFilteringType::NearestOnly) {
                formatFeatures =
                    static_cast<snap::rhi::FormatFeatures>(formatFeatures | snap::rhi::FormatFeatures::Sampled);
            } else if (glInstance.features.textureFormatOpInfo[i].formatFilteringType ==
                       snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear) {
                formatFeatures = static_cast<snap::rhi::FormatFeatures>(formatFeatures |
                                                                        snap::rhi::FormatFeatures::SampledFilterLinear);
            }
        } else if (glInstance.features.renderbufferFormat[i].internalFormat !=
                   snap::rhi::backend::opengl::SizedInternalFormat::UNKNOWN) {
            formatFeatures =
                static_cast<snap::rhi::FormatFeatures>(formatFeatures | snap::rhi::FormatFeatures::BlitSrc);

            if (isDepth) {
                formatFeatures = static_cast<snap::rhi::FormatFeatures>(
                    formatFeatures | snap::rhi::FormatFeatures::DepthStencilRenderable);
            } else {
                formatFeatures =
                    static_cast<snap::rhi::FormatFeatures>(formatFeatures | snap::rhi::FormatFeatures::ColorRenderable);
            }
        }

        formatProperty.framebufferSampleCounts = snap::rhi::SampleCount::Count1;
        formatProperty.sampledTexture2DColorSampleCounts = snap::rhi::SampleCount::Count1;
        formatProperty.sampledTexture2DArrayColorSampleCounts = snap::rhi::SampleCount::Count1;

        if (glInstance.features.renderbufferFormat[i].internalFormat !=
            snap::rhi::backend::opengl::SizedInternalFormat::UNKNOWN) {
            formatProperty.framebufferSampleCounts =
                getSampleCount(glInstance,
                               snap::rhi::backend::opengl::TextureTarget::Renderbuffer,
                               glInstance.features.renderbufferFormat[i].internalFormat);
            formatFeatures =
                static_cast<snap::rhi::FormatFeatures>(formatFeatures | snap::rhi::FormatFeatures::Resolve);
        }

        if (glInstance.features.textureFormat[i].format != snap::rhi::backend::opengl::FormatGroup::UNKNOWN &&
            glInstance.features.textureFormatOpInfo[i].formatFilteringType ==
                snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinearWithMSAA) {
            formatProperty.sampledTexture2DColorSampleCounts =
                getSampleCount(glInstance,
                               snap::rhi::backend::opengl::TextureTarget::Texture2DMS,
                               glInstance.features.textureFormat[i].internalFormat);
            formatProperty.sampledTexture2DArrayColorSampleCounts =
                getSampleCount(glInstance,
                               snap::rhi::backend::opengl::TextureTarget::Texture2DMSArray,
                               glInstance.features.textureFormat[i].internalFormat);
            formatFeatures =
                static_cast<snap::rhi::FormatFeatures>(formatFeatures | snap::rhi::FormatFeatures::Resolve);
        }
    }

    { // PixelFormat::Grayscale
        auto& formatProperty = capabilities.formatProperties[static_cast<uint32_t>(snap::rhi::PixelFormat::Grayscale)];
        formatProperty.textureFeatures = static_cast<snap::rhi::FormatFeatures>(
            snap::rhi::FormatFeatures::SampledFilterLinear | snap::rhi::FormatFeatures::BlitDst);
        formatProperty.framebufferSampleCounts = snap::rhi::SampleCount::Count1;
        formatProperty.sampledTexture2DColorSampleCounts = snap::rhi::SampleCount::Count1;
        formatProperty.sampledTexture2DArrayColorSampleCounts = snap::rhi::SampleCount::Count1;
    }
}
} // unnamed namespace

namespace snap::rhi::backend::opengl {
Profile::Profile(Device* device) : device(device), validationLayer(device->getValidationLayer()) {}

std::string Profile::describe(snap::rhi::backend::opengl::FramebufferTarget target,
                              const snap::rhi::backend::opengl::FramebufferId fbo,
                              const snap::rhi::backend::opengl::FramebufferDescription& activeDescription) const {
    std::string framebufferDescription{};
    framebufferDescription += snap::rhi::common::stringFormat("Framebuffer: id=%d target=%s \n",
                                                              static_cast<int32_t>(fbo),
                                                              snap::rhi::backend::opengl::toString(target).data());

    for (uint32_t i = 0; i < activeDescription.numColorAttachments; ++i) {
        auto& colorAtt = activeDescription.colorAttachments[i];
        framebufferDescription += snap::rhi::common::stringFormat(
            "\tc#%u: id=%d w:%d, h:%d, f:%d, target=%s layer=%d level=%d nlayers=%d viewMask=0x%x\n",
            i,
            uint32_t(colorAtt.texId),
            colorAtt.size.width,
            colorAtt.size.height,
            static_cast<uint32_t>(colorAtt.format),
            toString(colorAtt.target).data(),
            colorAtt.firstLayer,
            colorAtt.level,
            colorAtt.size.depth,
            colorAtt.viewMask);
    }

    if (activeDescription.depthStencilAttachment.texId != snap::rhi::backend::opengl::TextureId::Null) {
        framebufferDescription += snap::rhi::common::stringFormat(
            "\t ds: id=%d w:%d, h:%d, f:%d, target=%s layer=%d level=%d nlayers=%d viewMask=0x%x\n",
            uint32_t(activeDescription.depthStencilAttachment.texId),
            activeDescription.depthStencilAttachment.size.width,
            activeDescription.depthStencilAttachment.size.height,
            static_cast<uint32_t>(activeDescription.depthStencilAttachment.format),
            toString(activeDescription.depthStencilAttachment.target).data(),
            activeDescription.depthStencilAttachment.firstLayer,
            activeDescription.depthStencilAttachment.level,
            activeDescription.depthStencilAttachment.size.depth,
            activeDescription.depthStencilAttachment.viewMask);
    }

    return framebufferDescription;
}

void Profile::loadOpenGLInstance(gl::APIVersion logicalApi,
                                 const gl::APIVersion realApi,
                                 std::string_view vendorName,
                                 std::string_view rendererName) {
    using InstanceBuilderFn = snap::rhi::backend::opengl::Instance (*)(gl::APIVersion);
    InstanceBuilderFn builder = nullptr;
    switch (logicalApi) {
#if SNAP_RHI_GLES20
        case gl::APIVersion::GLES20: {
            builder = &buildInstance<opengl::es20::Instance>;
        } break;
#endif

#if SNAP_RHI_GLES30
        case gl::APIVersion::GLES30: {
            builder = &buildInstance<opengl::es30::Instance>;
        } break;
#endif

#if SNAP_RHI_GLES31
        case gl::APIVersion::GLES31: {
            builder = &buildInstance<opengl::es31::Instance>;
        } break;
#endif

#if SNAP_RHI_GLES32
        case gl::APIVersion::GLES32: {
            builder = &buildInstance<opengl::es32::Instance>;
        } break;
#endif

        case gl::APIVersion::GL45:
        case gl::APIVersion::GL46: {
#if SNAP_RHI_GL45
            builder = &buildInstance<opengl45::Instance>;
            logicalApi = gl::APIVersion::GL45;
            break;
#else
            [[fallthrough]];
#endif
        }

        case gl::APIVersion::GL41:
        case gl::APIVersion::GL42:
        case gl::APIVersion::GL43:
        case gl::APIVersion::GL44: {
#if SNAP_RHI_GL41
            builder = &buildInstance<opengl41::Instance>;
            logicalApi = gl::APIVersion::GL41;
            break;
#else
            [[fallthrough]];
#endif
        }

        case gl::APIVersion::GL21:
        case gl::APIVersion::GL30:
        case gl::APIVersion::GL31:
        case gl::APIVersion::GL32:
        case gl::APIVersion::GL33:
        case gl::APIVersion::GL40: {
#if SNAP_RHI_GL21
            builder = &buildInstance<opengl21::Instance>;
            logicalApi = gl::APIVersion::GL21;
            break;
#else
            [[fallthrough]];
#endif
        }

        default: {
            snap::rhi::common::throwException("[Profile] Invalid API version");
        }
    }
    gl = builder(realApi);

    nativeGLVersion = logicalApi;

    vendor = getGPUVendor(vendorName);
    model = getGPUModel(rendererName);
}

void Profile::checkErrors(SNAP_RHI_SRC_LOCATION_TYPE srcLocation,
                          const snap::rhi::ValidationTag tags,
                          const snap::rhi::ReportLevel reportType) const {
    checkGLErrors(srcLocation, device, tags, reportType);
}

void Profile::checkGLErrors(SNAP_RHI_SRC_LOCATION_TYPE srcLocation,
                            snap::rhi::backend::common::Device* device,
                            const snap::rhi::ValidationTag tags,
                            const snap::rhi::ReportLevel reportType) {
    static constexpr int32_t MaxErrorCheckIterations = 64; // Serves to prevent infinite loop on certain gpus

    const auto& validationLayer = device->getValidationLayer();
    GLenum glError = glGetError();
    int32_t errorCheckIterations = 0;
    for (errorCheckIterations = 0; (errorCheckIterations < MaxErrorCheckIterations) && (glError != GL_NO_ERROR);
         errorCheckIterations++, glError = glGetError()) {
        validationLayer.report(reportType,
                               tags,
                               "[Profile::checkErrors] OpenGL error: 0x%x ('%s'), function: '%s', file: '%s:%i, \n",
                               (int)glError,
                               getErrorStr(glError).data(),
                               srcLocation.function_name(),
                               srcLocation.file_name(),
                               srcLocation.line());
    }
    if (errorCheckIterations == MaxErrorCheckIterations) {
        snap::rhi::common::throwException(
            "Profile: OpenGL reached max error check iterations, potential deadlock / opengl misuse\n");
    }
}

snap::rhi::Capabilities Profile::buildCapabilities() const {
    snap::rhi::Capabilities capabilities{};

    // ==========================================================================
    // MARK: - API Description and NDC Layout
    // ==========================================================================
    // OpenGL uses a different coordinate system than Vulkan/Metal

    capabilities.ndcLayout = {snap::rhi::NDCLayout::YAxis::Up, snap::rhi::NDCLayout::DepthRange::MinusOneToOne};
    capabilities.textureConvention = snap::rhi::TextureOriginConvention::BottomLeft;
    capabilities.apiDescription = {snap::rhi::API::OpenGL, static_cast<snap::rhi::APIVersionType>(nativeGLVersion)};

    // ==========================================================================
    // MARK: - Memory Properties
    // ==========================================================================
    // OpenGL is not an explicit-memory API; values are best-effort approximations

    capabilities.nonCoherentAtomSize = 1;
    capabilities.supportsPersistentMapping = false; // gl.features.isMapPersistentSupported;

    capabilities.physicalDeviceMemoryProperties = {};
    capabilities.physicalDeviceMemoryProperties
        .memoryTypes[capabilities.physicalDeviceMemoryProperties.memoryTypeCount++]
        .memoryProperties = snap::rhi::MemoryProperties::DeviceLocal;
    capabilities.physicalDeviceMemoryProperties
        .memoryTypes[capabilities.physicalDeviceMemoryProperties.memoryTypeCount++]
        .memoryProperties = snap::rhi::MemoryProperties::HostVisible | snap::rhi::MemoryProperties::HostCoherent;

    // ==========================================================================
    // MARK: - Core Feature Flags (from gl.features)
    // ==========================================================================

    // Texture generation and filtering
    // Reference: https://docs.gl/es2/glGenerateMipmap
    capabilities.isNPOTGenerateMipmapCmdSupported = gl.features.isNPOTGenMipmapSupported;
    capabilities.isNPOTWrapModeSupported = gl.features.isNPOTWrapModeSupported;
    capabilities.isTextureFormatSwizzleSupported = gl.features.isTexSwizzleSupported;
    capabilities.isSamplerMinMaxLodSupported = gl.features.isTexMinMaxLodSupported;

    // Rendering features
    capabilities.isRasterizerDisableSupported = gl.features.isRasterizationDisableSupported;
    capabilities.isRenderToMipmapSupported = gl.features.isFBORenderToNonZeroMipSupported;
    capabilities.isMinMaxBlendModeSupported = gl.features.isMinMaxBlendModeSupported;

    // Texture types
    capabilities.isTexture3DSupported = gl.features.isTexture3DSupported;
    capabilities.isTextureArraySupported = gl.features.isTextureArraySupported;
    capabilities.isDepthCubeMapSupported = gl.features.isDepthCubemapSupported;

    // Sampler features
    capabilities.isClampToBorderSupported = gl.features.isClampToBorderSupported;
    capabilities.isClampToZeroSupported = false; // Not supported in OpenGL
    capabilities.isSamplerCompareFuncSupported = gl.features.isDepthCompareFuncSupported;

    // Blending and MRT
    capabilities.isMRTBlendSettingsDifferent = gl.features.isDifferentBlendSettingsSupported;

    // Index buffer
    capabilities.isPrimitiveRestartIndexEnabled = gl.features.isPrimitiveRestartIndexSupported;
    capabilities.isUInt32IndexSupported = gl.features.isUInt32IndexSupported;

    // Storage buffers
    capabilities.isShaderStorageBufferSupported = gl.features.isSSBOSupported;

    // Multisampling - SAMPLE_ALPHA_TO_COVERAGE and glSampleCoverage
    capabilities.isAlphaToCoverageSupported = gl.features.isAlphaToCoverageSupported;
    capabilities.isAlphaToOneEnableSupported = false;

    // Instancing
    capabilities.maxVertexAttribDivisor = gl.features.isInstancingSupported ? Unlimited : 1;
    capabilities.isVertexInputRatePerInstanceSupported = gl.features.isInstancingSupported;

    // Framebuffer fetch
    // Reference: https://www.khronos.org/registry/OpenGL/extensions/EXT/EXT_shader_framebuffer_fetch.txt
    capabilities.isFramebufferFetchSupported = gl.features.isFramebufferFetchSupported;

    // ==========================================================================
    // MARK: - Features Not Supported or Not Implemented
    // ==========================================================================

    // Reference: https://www.khronos.org/registry/OpenGL/extensions/ARM/ARM_texture_unnormalized_coordinates.txt
    capabilities.isSamplerUnnormalizedCoordsSupported = false;

    // Reference: https://www.khronos.org/registry/OpenGL/extensions/EXT/EXT_texture_view.txt
    capabilities.isTextureViewSupported = false; // Not implemented

    // Reference: https://www.khronos.org/registry/OpenGL/extensions/EXT/EXT_polygon_offset_clamp.txt
    capabilities.isPolygonOffsetClampSupported = false;

    // Polygon mode (wireframe) - desktop OpenGL only
#if SNAP_RHI_GL_ES
    capabilities.isNonFillPolygonModeSupported = false;
#else
    capabilities.isNonFillPolygonModeSupported = true;
#endif

    // Anisotropic filtering - from GL_EXT_texture_filter_anisotropic
    // Reference: https://www.khronos.org/registry/OpenGL/extensions/EXT/EXT_texture_filter_anisotropic.txt
    capabilities.maxAnisotropic = snap::rhi::AnisotropicFiltering::None;
    if (gl.features.isAnisotropicFilteringSupported) {
        const float maxAnisotropy = gl.features.maxAnisotropy;
        if (maxAnisotropy >= 16.0f) {
            capabilities.maxAnisotropic = snap::rhi::AnisotropicFiltering::Count16;
        } else if (maxAnisotropy >= 8.0f) {
            capabilities.maxAnisotropic = snap::rhi::AnisotropicFiltering::Count8;
        } else if (maxAnisotropy >= 4.0f) {
            capabilities.maxAnisotropic = snap::rhi::AnisotropicFiltering::Count4;
        } else if (maxAnisotropy >= 2.0f) {
            capabilities.maxAnisotropic = snap::rhi::AnisotropicFiltering::Count2;
        }
    }

    // ==========================================================================
    // MARK: - Framebuffer Limits
    // ==========================================================================

    capabilities.maxFramebufferLayers = 1; // OpenGL ES 3.2 only supports layered rendering
    capabilities.maxFramebufferColorAttachmentCount = gl.features.maxFramebufferColorAttachmentCount;
    capabilities.isMultiviewMSAAImplicitResolveEnabled = gl.features.isOVRDepthStencilSupported;

    // Multiview support (OVR_multiview extension)
    capabilities.maxMultiviewViewCount = gl.features.maxOVRViews;

    // Dynamic rendering - OpenGL doesn't require explicit render passes
    capabilities.isDynamicRenderingSupported = true;

    // ==========================================================================
    // MARK: - Constant Input Rate Support
    // ==========================================================================

    capabilities.constantInputRateSupportingBits = FormatDataTypeBits::Float;
    if (gl.features.isIntConstantRateSupported) {
        capabilities.constantInputRateSupportingBits =
            capabilities.constantInputRateSupportingBits | FormatDataTypeBits::Int;
    }

    // ==========================================================================
    // MARK: - Uniform Buffer Limits (from glGetIntegerv)
    // ==========================================================================
    // Reference: https://www.khronos.org/opengl/wiki/Uniform_Buffer_Object

    if (gl.features.isNativeUBOSupported) {
        // Native UBO support (OpenGL ES 3.0+, OpenGL 3.1+)
        // Query combined uniform blocks
        {
            GLint UBOCount = 0;
            getIntegerv(GL_MAX_COMBINED_UNIFORM_BLOCKS, &UBOCount);
            capabilities.maxUniformBuffers = static_cast<uint32_t>(UBOCount);
            assert(capabilities.maxUniformBuffers >= snap::rhi::SupportedLimit::MaxUniformBuffers);
        }

        // Query per-stage uniform blocks
        {
            GLint vertexUBOCount = 0;
            getIntegerv(GL_MAX_VERTEX_UNIFORM_BLOCKS, &vertexUBOCount);

            GLint fragmentUBOCount = 0;
            getIntegerv(GL_MAX_FRAGMENT_UNIFORM_BLOCKS, &fragmentUBOCount);

            capabilities.maxPerStageUniformBuffers = static_cast<uint32_t>(std::min(vertexUBOCount, fragmentUBOCount));
            assert(capabilities.maxPerStageUniformBuffers >= snap::rhi::SupportedLimit::MaxPerStageUniformBuffers);
        }

        // Query total uniform components per stage
        {
            GLint maxSize = 0;
            getIntegerv(GL_MAX_COMBINED_VERTEX_UNIFORM_COMPONENTS, &maxSize);
            capabilities.maxPerStageTotalUniformBufferSize[static_cast<uint32_t>(snap::rhi::ShaderStage::Vertex)] =
                static_cast<uint64_t>(maxSize) * 4;
            assert(
                capabilities.maxPerStageTotalUniformBufferSize[static_cast<uint32_t>(snap::rhi::ShaderStage::Vertex)] >=
                snap::rhi::SupportedLimit::MaxVertexShaderTotalUniformBufferSize);
        }
        {
            GLint maxSize = 0;
            getIntegerv(GL_MAX_COMBINED_FRAGMENT_UNIFORM_COMPONENTS, &maxSize);
            capabilities.maxPerStageTotalUniformBufferSize[static_cast<uint32_t>(snap::rhi::ShaderStage::Fragment)] =
                static_cast<uint64_t>(maxSize) * 4;
            assert(capabilities
                       .maxPerStageTotalUniformBufferSize[static_cast<uint32_t>(snap::rhi::ShaderStage::Fragment)] >=
                   snap::rhi::SupportedLimit::MaxFragmentShaderTotalUniformBufferSize);
        }

        // Query maximum uniform block size
        {
            GLint maxSize = 0;
            getIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxSize);
            capabilities.maxPerStageUniformBufferSize[static_cast<uint32_t>(snap::rhi::ShaderStage::Vertex)] =
                static_cast<uint64_t>(maxSize);
            capabilities.maxPerStageUniformBufferSize[static_cast<uint32_t>(snap::rhi::ShaderStage::Fragment)] =
                static_cast<uint64_t>(maxSize);
            assert(capabilities.maxPerStageUniformBufferSize[static_cast<uint32_t>(snap::rhi::ShaderStage::Vertex)] >=
                   snap::rhi::SupportedLimit::MaxVertexShaderUniformBufferSize);
        }

        // Query uniform buffer offset alignment
        {
            GLint alignment = 1;
            getIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &alignment);
            capabilities.minUniformBufferOffsetAlignment = static_cast<uint64_t>(alignment);
        }
    } else {
        // Emulated UBO for OpenGL ES 2.0 (no native UBO support)
        capabilities.maxPerStageUniformBuffers = 12;
        capabilities.maxUniformBuffers = capabilities.maxPerStageUniformBuffers * 2;

        // Query vertex shader uniform capacity
        {
            GLint maxSize = 0;
#if SNAP_RHI_GL_ES
            getIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS, &maxSize);
            maxSize *= 4 * 4; // Convert vectors to bytes
#else
            getIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &maxSize);
            maxSize *= sizeof(float);
#endif
            capabilities.maxPerStageTotalUniformBufferSize[static_cast<uint32_t>(snap::rhi::ShaderStage::Vertex)] =
                static_cast<uint64_t>(maxSize);
            assert(
                capabilities.maxPerStageTotalUniformBufferSize[static_cast<uint32_t>(snap::rhi::ShaderStage::Vertex)] >=
                snap::rhi::SupportedLimit::MaxVertexShaderTotalUniformBufferSize);
        }

        // Query fragment shader uniform capacity
        {
            GLint maxSize = 0;
#if SNAP_RHI_GL_ES
            getIntegerv(GL_MAX_FRAGMENT_UNIFORM_VECTORS, &maxSize);
            maxSize *= 4 * 4; // Convert vectors to bytes
#else
            getIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &maxSize);
            maxSize *= sizeof(float);
#endif
            capabilities.maxPerStageTotalUniformBufferSize[static_cast<uint32_t>(snap::rhi::ShaderStage::Fragment)] =
                static_cast<uint64_t>(maxSize);
            assert(capabilities
                       .maxPerStageTotalUniformBufferSize[static_cast<uint32_t>(snap::rhi::ShaderStage::Fragment)] >=
                   snap::rhi::SupportedLimit::MaxFragmentShaderTotalUniformBufferSize);
        }

        // Per-stage uniform buffer size equals total (no block subdivision in ES 2.0)
        capabilities.maxPerStageUniformBufferSize[static_cast<uint32_t>(snap::rhi::ShaderStage::Vertex)] =
            capabilities.maxPerStageTotalUniformBufferSize[static_cast<uint32_t>(snap::rhi::ShaderStage::Vertex)];
        capabilities.maxPerStageUniformBufferSize[static_cast<uint32_t>(snap::rhi::ShaderStage::Fragment)] =
            capabilities.maxPerStageTotalUniformBufferSize[static_cast<uint32_t>(snap::rhi::ShaderStage::Fragment)];

        // No alignment requirement for emulated UBOs
        capabilities.minUniformBufferOffsetAlignment = 1;
    }

    // ==========================================================================
    // MARK: - Texture/Sampler Binding Limits (from glGetIntegerv)
    // ==========================================================================

    // Query vertex texture units
    // GPU_BUG_019: PowerVR SGX falsely reports GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS as "8"
    {
        GLint textureCount = 0;
        if (model == GPUModel::PowerVR_SGX) {
            // GPU_BUG_019
            // 01/08/2020
            // PowerVR SGX
            // This GPU falsely reports GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS as “8” on Android when in fact
            // it doesn’t support vertex texture fetch at all. When tex2DLod() is called in the vertex
            // shader, the compiler outputs a useless “Compile failed” error with zero indication of
            // where or what the problem is. We force vertex texture unit caps to 0 for this GPU.
            textureCount = 0;
        } else {
            getIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &textureCount);
        }
        capabilities.maxPerStageTextures[static_cast<uint32_t>(snap::rhi::ShaderStage::Vertex)] =
            static_cast<uint32_t>(textureCount);
        capabilities.maxPerStageSamplers[static_cast<uint32_t>(snap::rhi::ShaderStage::Vertex)] =
            static_cast<uint32_t>(textureCount);
        assert(capabilities.maxPerStageTextures[static_cast<uint32_t>(snap::rhi::ShaderStage::Vertex)] >=
               snap::rhi::SupportedLimit::MaxVertexShaderTextures);
    }

    // Query fragment texture units
    {
        GLint textureCount = 0;
        getIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &textureCount);
        capabilities.maxPerStageTextures[static_cast<uint32_t>(snap::rhi::ShaderStage::Fragment)] =
            static_cast<uint32_t>(textureCount);
        capabilities.maxPerStageSamplers[static_cast<uint32_t>(snap::rhi::ShaderStage::Fragment)] =
            static_cast<uint32_t>(textureCount);
        assert(capabilities.maxPerStageTextures[static_cast<uint32_t>(snap::rhi::ShaderStage::Fragment)] >=
               snap::rhi::SupportedLimit::MaxFragmentShaderTextures);
    }

    // Query combined texture units
    {
        GLint textureCount = 0;
        getIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &textureCount);
        capabilities.maxTextures = static_cast<uint32_t>(textureCount);
        capabilities.maxSamplers = static_cast<uint32_t>(textureCount);
        assert(capabilities.maxTextures >= snap::rhi::SupportedLimit::MaxTextures);
    }

    // ==========================================================================
    // MARK: - Vertex Input Limits (from glGetIntegerv)
    // ==========================================================================

    {
        GLint vertexAttribs = 0;
        getIntegerv(GL_MAX_VERTEX_ATTRIBS, &vertexAttribs);
        capabilities.maxVertexInputAttributes = static_cast<uint32_t>(vertexAttribs);
        assert(capabilities.maxVertexInputAttributes >= snap::rhi::SupportedLimit::MaxVertexInputAttributes);
    }

    // Vertex input bindings equals vertex attributes in OpenGL
    capabilities.maxVertexInputBindings = capabilities.maxVertexInputAttributes;

    // ==========================================================================
    // MARK: - Texture Dimension Limits (from glGetIntegerv)
    // ==========================================================================

    // Query maximum 2D/Cube texture size
    // Reference: https://docs.gl/es2/glGet - minimum is 64 for 2D, 16 for cube
    {
        constexpr uint32_t minTexture2DSize = 64;
        constexpr uint32_t minTextureCubeSize = 16;

        GLint textureSize = 0;
        getIntegerv(GL_MAX_TEXTURE_SIZE, &textureSize);

        assert(static_cast<uint32_t>(textureSize) >= minTexture2DSize);
        capabilities.maxTextureDimension2D = std::max(minTexture2DSize, static_cast<uint32_t>(textureSize));

        assert(static_cast<uint32_t>(textureSize) >= minTextureCubeSize);
        capabilities.maxTextureDimensionCube = std::max(minTextureCubeSize, static_cast<uint32_t>(textureSize));
    }

    // 3D texture size and array layers from features
    capabilities.maxTextureDimension3D = gl.features.maxTextureDimension3D;
    capabilities.maxTextureArrayLayers = gl.features.maxArrayTextureLayers;

    // ==========================================================================
    // MARK: - Clip/Cull Distance Limits (from gl.features)
    // ==========================================================================

    capabilities.maxClipDistances = gl.features.maxClipDistances;
    capabilities.maxCullDistances = gl.features.maxCullDistances;
    capabilities.maxCombinedClipAndCullDistances = gl.features.maxCombinedClipAndCullDistances;

    // ==========================================================================
    // MARK: - Compute Shader Limits (from gl.features)
    // ==========================================================================
    // OpenGL ES 3.1+ requires at least 128 threads per work group

    capabilities.maxComputeWorkGroupInvocations = gl.features.maxComputeWorkGroupInvocations;
    capabilities.threadExecutionWidth = capabilities.maxComputeWorkGroupInvocations;

    capabilities.maxComputeWorkGroupCount[0] = gl.features.maxComputeWorkGroupCount[0];
    capabilities.maxComputeWorkGroupCount[1] = gl.features.maxComputeWorkGroupCount[1];
    capabilities.maxComputeWorkGroupCount[2] = gl.features.maxComputeWorkGroupCount[2];

    capabilities.maxComputeWorkGroupSize[0] = gl.features.maxComputeWorkGroupSize[0];
    capabilities.maxComputeWorkGroupSize[1] = gl.features.maxComputeWorkGroupSize[1];
    capabilities.maxComputeWorkGroupSize[2] = gl.features.maxComputeWorkGroupSize[2];

    // ==========================================================================
    // MARK: - Other Limits
    // ==========================================================================

    capabilities.maxSpecializationConstants = snap::rhi::SupportedLimit::MaxSpecializationConstants;
    capabilities.maxVertexInputAttributeOffset = snap::rhi::SupportedLimit::MaxVertexInputAttributeOffset;
    capabilities.maxVertexInputBindingStride = snap::rhi::SupportedLimit::MaxVertexInputBindingStride;

    // ==========================================================================
    // MARK: - Fragment Shader Features
    // ==========================================================================

    // OpenGL 3.3+ / OpenGL ES 3.2+ support gl_PrimitiveID
    // GPU_BUG: Mali-G76 has issues with gl_PrimitiveID
    capabilities.isFragmentPrimitiveIDSupported =
        gl.features.isFragmentPrimitiveIDSupported && model != GPUModel::MaliG76;

    // Reference: https://registry.khronos.org/OpenGL/extensions/NV/NV_fragment_shader_barycentric.txt
    capabilities.isFragmentBarycentricCoordinatesSupported = true;

    // ==========================================================================
    // MARK: - Vertex Attribute Format Support
    // ==========================================================================

    capabilities.vertexAttributeFormatProperties = gl.features.isVtxAttribSupported;

    // ==========================================================================
    // MARK: - Queue Family Properties
    // ==========================================================================

    capabilities.queueFamiliesCount = 1;
    capabilities.queueFamilyProperties.fill({});
    capabilities.queueFamilyProperties[0].queueCount = 1;
    capabilities.queueFamilyProperties[0].queueFlags =
        snap::rhi::CommandQueueFeatures::Graphics | snap::rhi::CommandQueueFeatures::Transfer;
    capabilities.queueFamilyProperties[0].isTimestampQuerySupported = gl.features.isTimerQuerySupported;

    if (capabilities.maxComputeWorkGroupInvocations > 0) {
        capabilities.queueFamilyProperties[0].queueFlags =
            capabilities.queueFamilyProperties[0].queueFlags | snap::rhi::CommandQueueFeatures::Compute;
    }

    // ==========================================================================
    // MARK: - Pixel Format Properties (queried per-format)
    // ==========================================================================

    initPixelFormatProperties(capabilities, gl);

    return capabilities;
}
} // namespace snap::rhi::backend::opengl
