#pragma once

#include "../GLES30/Instance.hpp"
#include "snap/rhi/Structs.h"

#if SNAP_RHI_GLES31
namespace snap::rhi::backend::opengl::es31 {
struct Instance : es30::Instance {
public:
    static snap::rhi::backend::opengl::Features buildFeatures(gl::APIVersion realApiVersion);

    static void memoryBarrierByRegion(GLbitfield barriers);
    static void texStorage2DMultisample(snap::rhi::backend::opengl::TextureTarget target,
                                        GLsizei samples,
                                        snap::rhi::backend::opengl::SizedInternalFormat internalformat,
                                        GLsizei width,
                                        GLsizei height,
                                        GLboolean fixedsamplelocations);
    static void bindImageTexture(
        GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format);

    static void getProgramResourceiv(GLuint program,
                                     GLenum programInterface,
                                     GLuint index,
                                     GLsizei propCount,
                                     const GLenum* props,
                                     GLsizei bufSize,
                                     GLsizei* length,
                                     GLint* params);

    static void getProgramInterfaceiv(GLuint program, GLenum programInterface, GLenum pname, GLint* params);

    static void getProgramResourceName(
        GLuint program, GLenum programInterface, GLuint index, GLsizei bufSize, GLsizei* length, char* name);
    static GLuint getProgramResourceIndex(GLuint program, GLenum programInterface, const char* name);

    static void dispatchCompute(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z);

    static void getTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint* params);
};
} // namespace snap::rhi::backend::opengl::es31
#endif // SNAP_RHI_GLES31
