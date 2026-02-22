#pragma once

#include <GLAD/OpenGL.h>
#include <OpenGL/Context.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

namespace gl {

void setEglContextFlags(ContextBehaviorFlagBits flags);
EGLDisplay getDisplay();
Context createContext(Context sharedContext, bool useDebugFlags);
void bindContext(Context context, EGLSurface drawSurface, EGLSurface readSurface);
void deleteGLContext(Context context);

} // namespace gl
