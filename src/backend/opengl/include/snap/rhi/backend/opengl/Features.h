#pragma once

#include "snap/rhi/Enums.h"
#include "snap/rhi/PixelFormat.h"
#include "snap/rhi/VertexAttributeFormat.h"
#include "snap/rhi/backend/opengl/FormatInfo.h"
#include "snap/rhi/backend/opengl/OpenGL.h"

#include <array>
#include <bitset>

namespace snap::rhi::backend::opengl {
struct Features {
    gl::APIVersion apiVersion = gl::APIVersion::None;

    bool isPBOSupported = false;
    bool isBlitFramebufferSupported = false;
    bool isCopyBufferSubDataSupported = false;
    bool isSamplerSupported = false;
    bool isVAOSupported = false;
    bool isDifferentBlendSettingsSupported = false;
    bool isMRTSupported = false;
    bool isStorageTextureSupported = false;
    bool isNativeUBOSupported = false;
    bool isSSBOSupported = false;
    bool isAlphaToCoverageSupported = false;
    bool isDepthStencilAttachmentSupported = false;
    bool isOVRDepthStencilSupported = false;
    bool isOVRMultiviewSupported = false;
    bool isOVRMultiviewMultisampledSupported = false;
    bool isRasterizationDisableSupported = false;
    bool isMinMaxBlendModeSupported = false;
    bool isFBORenderToNonZeroMipSupported = false;
    bool isClampToBorderSupported = false;
    bool isTexMinMaxLodSupported = false;
    bool isTexBaseMaxLevelSupported = false;
    bool isMapUnmapAvailable = false;
    bool isNPOTGenMipmapSupported = false;
    bool isTexSwizzleSupported = false;
    bool isNPOTWrapModeSupported = false;
    bool isDepthCompareFuncSupported = false;
    bool isTexture3DSupported = false;
    bool isTextureArraySupported = false;
    bool isInstancingSupported = false;
    bool isDepthCubemapSupported = false;
    bool isPrimitiveRestartIndexSupported = false;
    bool isFramebufferFetchSupported = false;
    bool isIntConstantRateSupported = false;
    bool isMultisampleSupported = false;
    bool isDiscardFramebufferSupported = false;
    bool isFenceSupported = false;
    bool isSemaphoreSupported = false;
    bool isFragmentPrimitiveIDSupported = false;
    bool isTextureParameterSupported = false;
    bool isCustomTexturePackingSupported = false;
    bool isProgramBinarySupported = false;
    bool isCopyImageSubDataSupported = false;
    bool isUInt32IndexSupported = false;
    bool isS3TCCompressionFormatFamilySupported = false;
    bool isBPTCCompressionFormatFamilySupported = false;
    bool isASTCCompressionFormatFamilySupported = false;
    bool isANDROIDNativeFenceSyncSupported = false;
    // for OpenGL 3+, this occlusion queries always work.
    // for OpenGL ES 2, this designates the inclusion of
    // https://registry.khronos.org/OpenGL/extensions/EXT/EXT_occlusion_query_boolean.txt
    // implementing GenQueriesEXT, DeleteQueriesEXT, IsQueryEXT, BeginQueryEXT, EndQueryEXT, GetQueryivEXT and
    // GetQueryObjectuivEXT which allows usage of ANY_SAMPLES_PASSED_EXT to indicate if an an object is occluded or not.
    bool isOcclusionQuerySupported = false;
    // For all non-ES OpenGLs, this never works.
    // For OpenGL ES 2, this desginates the inclusion of
    // https://registry.khronos.org/OpenGL/extensions/EXT/EXT_disjoint_timer_query.txt implementing GL_GPU_DISJOINT_EXT
    // for glGetIntegerv and similar calls. "Disjoint operations occur whenever a change in the GPU occurs that will
    // make the values returned by this extension unusable for performance metrics. An example can be seen with how
    // mobile GPUs need to proactively try to conserve power, which might cause the GPU to go to sleep at the lower
    // levers."
    bool isDisjointQuerySupported = false;
    // for OpenGL 3.2+, timer and timestamp queries always work.
    // for OpenGL ES 2, this designates the inclusion of
    // https://registry.khronos.org/OpenGL/extensions/EXT/EXT_disjoint_timer_query.txt implementing timer queries
    // GL_TIME_ELAPSED for Begin/EndQuery, and timeSTAMP queries GL_TIMESTAMP for glQueryCounter.
    bool isTimerQuerySupported = false;
    // GL_EXT_texture_filter_anisotropic - anisotropic texture filtering
    // https://registry.khronos.org/OpenGL/extensions/EXT/EXT_texture_filter_anisotropic.txt
    bool isAnisotropicFilteringSupported = false;

    uint32_t maxOVRViews = 0;
    uint32_t maxArrayTextureLayers = 0;
    uint32_t maxTextureDimension3D = 0;
    uint32_t maxFramebufferColorAttachmentCount = 0;
    uint32_t maxDrawBuffers = 0;
    uint32_t maxClipDistances = 0;
    uint32_t maxCullDistances = 0;
    uint32_t maxCombinedClipAndCullDistances = 0;
    uint32_t maxCombinedTextureImageUnits = 0;
    uint32_t maxVertexAttribs = 0;

    uint32_t maxVertexUniformComponents = 0;
    uint32_t maxFragmentUniformComponents = 0;

    uint32_t maxComputeWorkGroupInvocations = 0;
    uint32_t maxComputeWorkGroupCount[3];
    uint32_t maxComputeWorkGroupSize[3];

    // Maximum anisotropic filtering level (from GL_EXT_texture_filter_anisotropic)
    float maxAnisotropy = 1.0f;

    GLuint64 maxClientWaitTimeout = 0;

    std::array<bool, static_cast<uint32_t>(snap::rhi::VertexAttributeFormat::Count)> isVtxAttribSupported{};

    std::string_view shaderVersionHeader;

    std::array<snap::rhi::backend::opengl::RenderbufferFormatInfo, static_cast<uint32_t>(snap::rhi::PixelFormat::Count)>
        renderbufferFormat{};
    std::array<snap::rhi::backend::opengl::CompositeFormat, static_cast<uint32_t>(snap::rhi::PixelFormat::Count)>
        textureFormat{};
    std::array<snap::rhi::backend::opengl::FormatOpInfo, static_cast<uint32_t>(snap::rhi::PixelFormat::Count)>
        textureFormatOpInfo{};
};
} // namespace snap::rhi::backend::opengl
