#pragma once

#define SNAP_RHI_OS_POSIX() 0       // Linux based systems and Apple
#define SNAP_RHI_OS_LINUX_BASED() 0 // includes Android and other Linux-based systems
#define SNAP_RHI_OS_ANDROID() 0

#define SNAP_RHI_OS_APPLE() 0 // includes macOS, iOS and other systems from Apple
#define SNAP_RHI_OS_MACOS() 0
#define SNAP_RHI_OS_IOS() 0 // includes iOS simulator
#define SNAP_RHI_OS_IOS_SIMULATOR() 0

#define SNAP_RHI_OS_WINDOWS() 0

#if defined(ANDROID) || defined(__ANDROID__)
#undef SNAP_RHI_OS_ANDROID
#define SNAP_RHI_OS_ANDROID() 1
#endif

#if defined(__APPLE__) || defined(__APPLE_CPP__) || defined(__APPLE_CC__) || defined(__MACOS_CLASSIC__)
#undef SNAP_RHI_OS_APPLE
#define SNAP_RHI_OS_APPLE() 1

#include <TargetConditionals.h>

#undef SNAP_RHI_OS_IOS
#define SNAP_RHI_OS_IOS() TARGET_OS_IOS

#undef SNAP_RHI_OS_IOS_SIMULATOR
#define SNAP_RHI_OS_IOS_SIMULATOR() (SNAP_RHI_OS_IOS() && TARGET_OS_SIMULATOR)

#undef SNAP_RHI_OS_MACOS
#define SNAP_RHI_OS_MACOS() TARGET_OS_OSX
#endif

// clang-format off
#if defined(__linux) || defined(__linux__) || defined(linux) || \
defined(__FreeBSD__) || defined(__FreeBSD) || \
defined(__OpenBSD__) || defined(__OpenBSD) || \
defined(__NetBSD__) || defined(__NetBSD) || \
defined(__bsdi__) || \
defined(__DragonFly__)
// clang-format on
#undef SNAP_RHI_OS_LINUX_BASED
#define SNAP_RHI_OS_LINUX_BASED() 1
#endif

#if defined(_WIN32)
#undef SNAP_RHI_OS_WINDOWS
#define SNAP_RHI_OS_WINDOWS() 1
#endif

#if SNAP_RHI_OS_LINUX_BASED() || SNAP_RHI_OS_APPLE()
#undef SNAP_RHI_OS_POSIX
#define SNAP_RHI_OS_POSIX() 1
#endif
