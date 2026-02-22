//
//  Instance.cpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 6/30/22.
//

#include "Instance.hpp"

#include "snap/rhi/common/Throw.h"

#if SNAP_RHI_GLES31
namespace {
void initTexFormatProperties(snap::rhi::backend::opengl::Features& features, const std::string& glExtensionsList) {
    { // snap::rhi::PixelFormat::R8G8B8A8Unorm
        auto& formatInfo = features.textureFormatOpInfo[static_cast<uint32_t>(snap::rhi::PixelFormat::R8G8B8A8Unorm)];
        formatInfo.features = snap::rhi::backend::opengl::FormatFeatures::All;
    }
}

void initRenderbufferFormatProperties(snap::rhi::backend::opengl::Features& features,
                                      const std::string& glExtensionsList) {}
} // unnamed namespace

namespace snap::rhi::backend::opengl::es31 {
snap::rhi::backend::opengl::Features Instance::buildFeatures(gl::APIVersion realApiVersion) {
    snap::rhi::backend::opengl::Features features = es30::Instance::buildFeatures(realApiVersion);

    std::string glExtensionsList = (char*)glGetString(GL_EXTENSIONS);
    initTexFormatProperties(features, glExtensionsList);
    initRenderbufferFormatProperties(features, glExtensionsList);

    features.apiVersion = gl::APIVersion::GLES31;
    features.isSSBOSupported = true;
    features.isTextureParameterSupported = true;

    // https://en.wikipedia.org/wiki/OpenGL_Shading_Language
    features.shaderVersionHeader = "#version 310 es\n";

    return features;
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

void Instance::getProgramInterfaceiv(GLuint program, GLenum programInterface, GLenum pname, GLint* params) {
    glGetProgramInterfaceiv(program, programInterface, pname, params);
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

void Instance::getTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint* params) {
    glGetTexLevelParameteriv(target, level, pname, params);
}
} // namespace snap::rhi::backend::opengl::es31
#endif // SNAP_RHI_GLES31
