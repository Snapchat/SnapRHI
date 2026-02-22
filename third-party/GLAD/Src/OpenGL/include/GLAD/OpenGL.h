#pragma once

#include <GLAD/gladPlatform.h>

#define GLAD_PLATFORM_OPENGL() (GLAD_OPENGL)
#define GLAD_PLATFORM_OPENGL_ES() (GLAD_PLATFORM_EMSCRIPTEN || GLAD_OPENGL_ES)

#if GLAD_OPENGL
#include "GLAD/gl.h"
#elif GLAD_OPENGL_ES
#include "GLAD/gles.h"
#elif GLAD_PLATFORM_EMSCRIPTEN
#include "GLES/gl.h"
#include "GLES3/gl3.h"
#define GL_GLEXT_PROTOTYPES
#include "GLES3/gl2ext.h"
#endif

namespace glad {
int loadOpenGL(GLADloadfunc load);
int loadOpenGLSafe(GLADloadfunc load);
} // namespace glad
