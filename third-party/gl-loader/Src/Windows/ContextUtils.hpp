#pragma once

#include <OpenGL/Context.h>

namespace gl::Windows {

void* getGLProcAddr(const char* name);
Context getCurrentContext();
void* getCurrentHDC();
Context createContext(Context sharedContext = nullptr);
void bindContext(Context context, void* hdc = nullptr);
void deleteContext(Context context);

} // namespace gl::Windows
