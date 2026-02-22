#pragma once

#include <cstdint>
#include <limits>

namespace gl {
enum class APIVersion : uint32_t {
    None = 0,

    // OpenGL version
    GL_START = 10,
    GL20 = 20,
    GL21 = 21,

    GL30 = 30,
    GL31 = 31,
    GL32 = 32,
    GL33 = 33,

    GL40 = 40,
    GL41 = 41,
    GL42 = 42,
    GL43 = 43,
    GL44 = 44,
    GL45 = 45,
    GL46 = 46,
    GL_END = 100,

    // OpenGL ES version
    GL_ES_START = 1000,
    GLES20 = 1020,
    GLES30 = 1030,
    GLES31 = 1031,
    GLES32 = 1032,
    GL_ES_END = 1100,

    Latest = std::numeric_limits<uint32_t>::max(),
};

using Context = void*;
using Surface = void*;
using Display = void*;
using APIProc = void (*)(void);

struct Surfaces {
    Surface readSurface = nullptr;
    Surface drawSurface = nullptr;
};

enum class ContextBehaviorFlagBits : uint32_t {
    None = 0,
    NoConfig = 1 << 0,   // GL_KHR_no_config_context_enabled
    SurfaceLess = 1 << 1 // GL_KHR_surfaceless_context_enabled
};
} // namespace gl
