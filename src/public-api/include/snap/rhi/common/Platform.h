#pragma once

#include "snap/rhi/common/OS.h"

#if defined(__EMSCRIPTEN__)
#define SNAP_RHI_PLATFORM_WEBASSEMBLY() 1
#else
#define SNAP_RHI_PLATFORM_WEBASSEMBLY() 0
#endif

#if !SNAP_RHI_PLATFORM_WEBASSEMBLY() || defined(__EMSCRIPTEN_PTHREADS__)
#define SNAP_RHI_HAS_MULTI_THREADING() 1
#else
#define SNAP_RHI_HAS_MULTI_THREADING() 0
#endif
