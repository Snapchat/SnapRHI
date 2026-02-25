#pragma once
// clang-format off

/// \file Compiler.h
/// \brief Introspection of the compiler
///
/// Defines
///     SNAP_RHI_CONFIG_COMPILER and
///     SNAP_RHI_CONFIG_COMPILER_GCC SNAP_RHI_CONFIG_COMPILER_CLANG SNAP_RHI_CONFIG_COMPILER_MSVC
/// Override by definining any of
/// SNAP_RHI_CONFIG_OVERRIDE_COMPILER_GCC
/// SNAP_RHI_CONFIG_OVERRIDE_COMPILER_CLANG
/// SNAP_RHI_CONFIG_OVERRIDE_COMPILER_MSVC

#define SNAP_RHI_CONFIG_COMPILER_GCC() 0
#define SNAP_RHI_CONFIG_COMPILER_CLANG() 0
#define SNAP_RHI_CONFIG_COMPILER_MSVC() 0

#if defined(SNAP_RHI_CONFIG_OVERRIDE_COMPILER_GCC)
    #define SNAP_RHI_CONFIG_COMPILER() GCC
    #undef SNAP_RHI_CONFIG_COMPILER_GCC
    #define SNAP_RHI_CONFIG_COMPILER_GCC() 1
#elif defined(SNAP_RHI_CONFIG_OVERRIDE_COMPILER_CLANG)
    #define SNAP_RHI_CONFIG_COMPILER() CLANG
    #undef SNAP_RHI_CONFIG_COMPILER_CLANG
    #define SNAP_RHI_CONFIG_COMPILER_CLANG() 1
#elif defined(SNAP_RHI_CONFIG_OVERRIDE_COMPILER_MSVC)
    #define SNAP_RHI_CONFIG_COMPILER() MSVC
    #undef SNAP_RHI_CONFIG_COMPILER_MSVC
    #define SNAP_RHI_CONFIG_COMPILER_MSVC() 1
#else
    // automatic discovery of the compiler
    #if defined(__clang__)
        #define SNAP_RHI_CONFIG_COMPILER clang
        #undef SNAP_RHI_CONFIG_COMPILER_CLANG
        #define SNAP_RHI_CONFIG_COMPILER_CLANG() 1
    #elif defined(__GNUC__)
        #define SNAP_RHI_CONFIG_COMPILER GCC
        #undef SNAP_RHI_CONFIG_COMPILER_GCC
        #define SNAP_RHI_CONFIG_COMPILER_GCC() 1
    #elif defined(_MSC_VER)
        #define SNAP_RHI_CONFIG_COMPILER MSVC
        #undef SNAP_RHI_CONFIG_COMPILER_MSVC
        #define SNAP_RHI_CONFIG_COMPILER_MSVC() 1

        #if (_MSC_VER < 1920) // MSVC 2017 does not support __forceinline on lambda
            // TODO(fedir.poliakov): Fix MSVC 2019 once it supports forceinline on lambda
            #define SNAP_RHI_CONFIG_MSVC_BEFORE_1920() 1
        #else
            #define SNAP_RHI_CONFIG_MSVC_BEFORE_1920() 0
        #endif
    #endif
#endif

#define SNAP_RHI_GCC_COMPATIBLE_COMPILER() (SNAP_RHI_CONFIG_COMPILER_GCC() || SNAP_RHI_CONFIG_COMPILER_CLANG())
