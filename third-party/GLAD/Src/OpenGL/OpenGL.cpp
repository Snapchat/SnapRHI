#include "GLAD/OpenGL.h"

namespace glad {
int loadOpenGL(GLADloadfunc load) {
#if GLAD_PLATFORM_WIN32 || (GLAD_OPENGL && SNAP_GLAD_LINUX_BASED())      // Desktop
    return gladLoadGL(load);
#elif GLAD_PLATFORM_ANDROID || (GLAD_OPENGL_ES && SNAP_GLAD_LINUX_BASED()) // Mobile
    return gladLoadGLES(load);
#else
    return 0;
#endif
}

int loadOpenGLSafe(GLADloadfunc load) {
#if GLAD_PLATFORM_WIN32 || (GLAD_OPENGL && SNAP_GLAD_LINUX_BASED())      // Desktop
    return gladLoadGLSafe(load);
#elif GLAD_PLATFORM_ANDROID || (GLAD_OPENGL_ES && SNAP_GLAD_LINUX_BASED()) // Mobile
    return gladLoadGLESSafe(load);
#else
    return 0;
#endif
}
} // namespace glad
