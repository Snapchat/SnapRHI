#pragma once

#include "OpenGL/runtime.h"

#include <string_view>

namespace gl {
#if GLAD_PLATFORM_EMSCRIPTEN
Context createContext(std::string_view canvasTarget);
#endif

void setContextBehaviorFlagBits(const ContextBehaviorFlagBits flags);

Context createContext(const APIVersion api, bool useDebugFlags = false);
Context createSharedContext(const Context context, bool useDebugFlags = false);
void destroyContext(const Context context);

void bindContext(const Context context);
void bindContextWithSurfaces(const Context context, const Surfaces& surfaces);

void retainContext(const Context context);
void releaseContext(const Context context);

Context getActiveContext();
Display getDefaultDisplay();
Surfaces getActiveSurfaces();

APIProc getProcAddress(char const* procName);

void loadAPI();

using LoadAPIHook = void (*)(void);
struct LoadAPIHooks {
    LoadAPIHook preHook;
    LoadAPIHook inHook;
    LoadAPIHook postHook;
};

void installLoadAPIHooks(const LoadAPIHooks& hooks);
void loadAPIWithHooks();
} // namespace gl
