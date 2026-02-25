//
//  Instance.cpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 6/30/22.
//

#include "Instance.hpp"
#include "snap/rhi/common/Throw.h"

#if SNAP_RHI_GL45
namespace {
void initTexFormatProperties(snap::rhi::backend::opengl::Features& features) {
    for (auto format : std::array{
             // clang-format off
             snap::rhi::PixelFormat::R8Unorm,
             snap::rhi::PixelFormat::R8G8Unorm,
             snap::rhi::PixelFormat::R8G8B8A8Unorm,
             snap::rhi::PixelFormat::R16Unorm,
             snap::rhi::PixelFormat::R16G16Unorm,
             snap::rhi::PixelFormat::R16G16B16A16Unorm,
             snap::rhi::PixelFormat::R8Snorm,
             snap::rhi::PixelFormat::R8G8Snorm,
             snap::rhi::PixelFormat::R8G8B8A8Snorm,
             snap::rhi::PixelFormat::R16Snorm,
             snap::rhi::PixelFormat::R16G16Snorm,
             snap::rhi::PixelFormat::R16G16B16A16Snorm,
             snap::rhi::PixelFormat::R8Uint,
             snap::rhi::PixelFormat::R8G8Uint,
             snap::rhi::PixelFormat::R8G8B8A8Uint,
             snap::rhi::PixelFormat::R16Uint,
             snap::rhi::PixelFormat::R16G16Uint,
             snap::rhi::PixelFormat::R16G16B16A16Uint,
             snap::rhi::PixelFormat::R32Uint,
             snap::rhi::PixelFormat::R32G32Uint,
             snap::rhi::PixelFormat::R32G32B32A32Uint,
             snap::rhi::PixelFormat::R8Sint,
             snap::rhi::PixelFormat::R8G8Sint,
             snap::rhi::PixelFormat::R8G8B8A8Sint,
             snap::rhi::PixelFormat::R16Sint,
             snap::rhi::PixelFormat::R16G16Sint,
             snap::rhi::PixelFormat::R16G16B16A16Sint,
             snap::rhi::PixelFormat::R32Sint,
             snap::rhi::PixelFormat::R32G32Sint,
             snap::rhi::PixelFormat::R32G32B32A32Sint,
             snap::rhi::PixelFormat::R16Float,
             snap::rhi::PixelFormat::R16G16Float,
             snap::rhi::PixelFormat::R16G16B16A16Float,
             snap::rhi::PixelFormat::R32Float,
             snap::rhi::PixelFormat::R32G32Float,
             snap::rhi::PixelFormat::R32G32B32A32Float,
             snap::rhi::PixelFormat::R10G10B10A2Unorm,
             snap::rhi::PixelFormat::R10G10B10A2Uint,
             snap::rhi::PixelFormat::R11G11B10Float,
             // clang-format on
         }) {
        auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(format)];
        formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::All;
    }

    for (auto format : std::array{
             // clang-format off
             snap::rhi::PixelFormat::R32Float,
             snap::rhi::PixelFormat::R32G32Float,
             snap::rhi::PixelFormat::R32G32B32A32Float,
             // clang-format on
         }) {
        auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(format)];
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

    { // snap::rhi::PixelFormat::DepthStencil
        auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::DepthStencil)];
        formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::AllNonStorage;
        formatInfo.formatFilteringType = snap::rhi::backend::opengl::FormatFilteringType::NearestAndLinear;
    }

    snap::rhi::backend::opengl::addCompressedFormat(features);
}

void initRenderbufferFormatProperties(snap::rhi::backend::opengl::Features& features) {
    // note: all renderable formats except for snorm
    for (auto format : std::array{
             // clang-format off
             snap::rhi::PixelFormat::R8Unorm,
             snap::rhi::PixelFormat::R8G8Unorm,
             snap::rhi::PixelFormat::R8G8B8A8Unorm,
             snap::rhi::PixelFormat::R16Unorm,
             snap::rhi::PixelFormat::R16G16Unorm,
             snap::rhi::PixelFormat::R16G16B16A16Unorm,
             snap::rhi::PixelFormat::R8Uint,
             snap::rhi::PixelFormat::R8G8Uint,
             snap::rhi::PixelFormat::R8G8B8A8Uint,
             snap::rhi::PixelFormat::R16Uint,
             snap::rhi::PixelFormat::R16G16Uint,
             snap::rhi::PixelFormat::R16G16B16A16Uint,
             snap::rhi::PixelFormat::R32Uint,
             snap::rhi::PixelFormat::R32G32Uint,
             snap::rhi::PixelFormat::R32G32B32A32Uint,
             snap::rhi::PixelFormat::R8Sint,
             snap::rhi::PixelFormat::R8G8Sint,
             snap::rhi::PixelFormat::R8G8B8A8Sint,
             snap::rhi::PixelFormat::R16Sint,
             snap::rhi::PixelFormat::R16G16Sint,
             snap::rhi::PixelFormat::R16G16B16A16Sint,
             snap::rhi::PixelFormat::R32Sint,
             snap::rhi::PixelFormat::R32G32Sint,
             snap::rhi::PixelFormat::R32G32B32A32Sint,
             snap::rhi::PixelFormat::R16Float,
             snap::rhi::PixelFormat::R16G16Float,
             snap::rhi::PixelFormat::R16G16B16A16Float,
             snap::rhi::PixelFormat::R32Float,
             snap::rhi::PixelFormat::R32G32Float,
             snap::rhi::PixelFormat::R32G32B32A32Float,
             snap::rhi::PixelFormat::R10G10B10A2Unorm,
             snap::rhi::PixelFormat::R10G10B10A2Uint,
             snap::rhi::PixelFormat::R11G11B10Float,
             // clang-format on
         }) {
        const auto& textureFormat = features.textureFormat[static_cast<uint32_t>(format)];
        features.renderbufferFormat[static_cast<uint32_t>(format)] = {textureFormat.internalFormat};
    }
}
} // unnamed namespace

namespace snap::rhi::backend::opengl45 {
snap::rhi::backend::opengl::Features Instance::buildFeatures() {
    snap::rhi::backend::opengl::Features features = opengl41::Instance::buildFeatures();

    features.isBPTCCompressionFormatFamilySupported = true;

    initTexFormatProperties(features);
    initRenderbufferFormatProperties(features);

    features.apiVersion = gl::APIVersion::GL45;
    features.isSSBOSupported = true;
    features.isClampToBorderSupported = true;
    features.isDiscardFramebufferSupported = true;
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
    features.shaderVersionHeader = "#version 450 core\n";

    return features;
}

void Instance::discardFramebuffer(snap::rhi::backend::opengl::FramebufferTarget target,
                                  GLsizei numAttachments,
                                  const snap::rhi::backend::opengl::FramebufferAttachmentTarget* attachments) {
    glInvalidateFramebuffer(static_cast<GLenum>(target), numAttachments, reinterpret_cast<const GLenum*>(attachments));
}

void Instance::getInternalformativ(snap::rhi::backend::opengl::TextureTarget target,
                                   snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                                   GLenum pname,
                                   GLsizei bufSize,
                                   GLint* params) {
    glGetInternalformativ(static_cast<GLenum>(target), static_cast<GLenum>(internalformat), pname, bufSize, params);
}

void Instance::texStorage2DMultisample(snap::rhi::backend::opengl::TextureTarget target,
                                       GLsizei samples,
                                       snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                                       GLsizei width,
                                       GLsizei height,
                                       GLboolean fixedsamplelocations) {
    glTexStorage2DMultisample(
        static_cast<GLenum>(target), samples, static_cast<GLenum>(internalformat), width, height, fixedsamplelocations);
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

void Instance::getProgramInterfaceiv(GLuint program, GLenum programInterface, GLenum pname, GLint* params) {
    glGetProgramInterfaceiv(program, programInterface, pname, params);
}

void Instance::bindImageTexture(
    GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format) {
    glBindImageTexture(unit, texture, level, layered, layer, access, format);
}

void Instance::getProgramResourceiv(GLuint program,
                                    GLenum programInterface,
                                    GLuint index,
                                    GLsizei propCount,
                                    const GLenum* props,
                                    GLsizei bufSize,
                                    GLsizei* length,
                                    GLint* params) {
    glGetProgramResourceiv(program, programInterface, index, propCount, props, bufSize, length, params);
}

void Instance::getProgramResourceName(
    GLuint program, GLenum programInterface, GLuint index, GLsizei bufSize, GLsizei* length, char* name) {
    glGetProgramResourceName(program, programInterface, index, bufSize, length, name);
}

GLuint Instance::getProgramResourceIndex(GLuint program, GLenum programInterface, const char* name) {
    return glGetProgramResourceIndex(program, programInterface, name);
}

void Instance::memoryBarrierByRegion(GLbitfield barriers) {
    glMemoryBarrierByRegion(barriers);
}

void Instance::dispatchCompute(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z) {
    glDispatchCompute(num_groups_x, num_groups_y, num_groups_z);
}
} // namespace snap::rhi::backend::opengl45
#endif // SNAP_RHI_GL45
