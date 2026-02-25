//
// Created by Vladyslav Deviatkov on 2/11/25.
//

#pragma once

#if defined(_MSC_VER)
// Windows / Microsoft Visual C++
#define SNAP_RHI_ALWAYS_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
// macOS (Xcode) / Linux (GCC)
#define SNAP_RHI_ALWAYS_INLINE __attribute__((always_inline)) inline
#else
// Fallback for other compilers
#define SNAP_RHI_ALWAYS_INLINE inline
#endif

#if defined(_MSC_VER)
// Windows / Microsoft Visual C++
#define SNAP_RHI_NO_INLINE __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
// macOS (Xcode) / Linux (GCC)
#define SNAP_RHI_NO_INLINE __attribute__((noinline))
#else
// Fallback for other compilers
#define SNAP_RHI_NO_INLINE
#endif
