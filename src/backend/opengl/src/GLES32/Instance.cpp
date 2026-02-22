#include "Instance.hpp"
#include "snap/rhi/common/Throw.h"

#if SNAP_RHI_GLES32
namespace {
void initTexFormatProperties(snap::rhi::backend::opengl::Features& features, const std::string& glExtensionsList) {
    { // snap::rhi::PixelFormat::DepthStencil
        auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::DepthStencil)];
        formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
        formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
    }

    { // snap::rhi::PixelFormat::Depth16Unorm
        auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::Depth16Unorm)];
        formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
        formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
    }

    { // snap::rhi::PixelFormat::DepthFloat
        auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::DepthFloat)];
        formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
        formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
    }

    for (auto format : {snap::rhi::PixelFormat::R16Float,
                        snap::rhi::PixelFormat::R16G16Float,
                        snap::rhi::PixelFormat::R16G16B16A16Float,
                        snap::rhi::PixelFormat::R32Float,
                        snap::rhi::PixelFormat::R32G32Float,
                        snap::rhi::PixelFormat::R32G32B32A32Float,
                        snap::rhi::PixelFormat::R11G11B10Float}) {
        auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(format)];
        formatInfo.features |= snap::rhi::backend::opengl::FormatFeatures::Renderable;
    }
}

void initRenderbufferFormatProperties(snap::rhi::backend::opengl::Features& features,
                                      const std::string& glExtensionsList) {}
} // unnamed namespace

namespace snap::rhi::backend::opengl::es32 {
snap::rhi::backend::opengl::Features Instance::buildFeatures(gl::APIVersion realApiVersion) {
    snap::rhi::backend::opengl::Features features = es31::Instance::buildFeatures(realApiVersion);

    std::string glExtensionsList = (char*)glGetString(GL_EXTENSIONS);
    initTexFormatProperties(features, glExtensionsList);
    initRenderbufferFormatProperties(features, glExtensionsList);

    features.apiVersion = gl::APIVersion::GLES32;
    features.isFragmentPrimitiveIDSupported = true;
    features.isDifferentBlendSettingsSupported = true;
    features.isClampToBorderSupported = true;
    features.isCopyImageSubDataSupported = true;

    {
        GLint value = 0;

        glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &value);
        features.maxComputeWorkGroupInvocations = static_cast<uint32_t>(value);

        for (GLuint i = 0; i < 3; ++i) {
            glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, i, &value);
            features.maxComputeWorkGroupCount[i] = static_cast<uint32_t>(value);
        }

        for (GLuint i = 0; i < 3; ++i) {
            glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, i, &value);
            features.maxComputeWorkGroupSize[i] = static_cast<uint32_t>(value);
        }
    }

    // https://en.wikipedia.org/wiki/OpenGL_Shading_Language
    features.shaderVersionHeader = "#version 320 es\n";

    return features;
}

void Instance::texStorage3DMultisample(snap::rhi::backend::opengl::TextureTarget target,
                                       GLsizei samples,
                                       snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                                       GLsizei width,
                                       GLsizei height,
                                       GLsizei depth,
                                       GLboolean fixedsamplelocations) {
    glTexStorage3DMultisample(static_cast<GLenum>(target),
                              samples,
                              static_cast<GLenum>(internalformat),
                              width,
                              height,
                              depth,
                              fixedsamplelocations);
}

void Instance::copyImageSubData(GLuint srcName,
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
                                GLsizei srcDepth) {
    glCopyImageSubData(srcName,
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
}

void Instance::colorMaski(GLuint buf, GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
    glColorMaski(buf, red, green, blue, alpha);
}

void Instance::enablei(GLenum cap, GLuint index) {
    glEnablei(cap, index);
}

void Instance::disablei(GLenum cap, GLuint index) {
    glDisablei(cap, index);
}

void Instance::blendEquationSeparatei(GLuint buf, GLenum modeRGB, GLenum modeAlpha) {
    glBlendEquationSeparatei(buf, modeRGB, modeAlpha);
}

void Instance::blendFuncSeparatei(GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) {
    glBlendFuncSeparatei(buf, srcRGB, dstRGB, srcAlpha, dstAlpha);
}
} // namespace snap::rhi::backend::opengl::es32
#endif // SNAP_RHI_GLES32
