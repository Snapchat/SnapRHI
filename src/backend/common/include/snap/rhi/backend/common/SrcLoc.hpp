#pragma once

#include <source_location>

#define SNAP_RHI_SRC_LOCATION_TYPE std::source_location

#if SNAP_RHI_ENABLE_SOURCE_LOCATION
#define SNAP_RHI_SOURCE_LOCATION() std::source_location::current()
#else // #if SNAP_RHI_ENABLE_SOURCE_LOCATION
#define SNAP_RHI_SOURCE_LOCATION() \
    std::source_location {}
#endif // #else // #if SNAP_RHI_ENABLE_SOURCE_LOCATION
