#pragma once

/// This macro takes an expression  and deduces the `noexcept` specification from it.
/// We use this macro as we might need to disable it in certain platforms.
#define SNAP_RHI_DEDUCE_NOEXCEPT(...) noexcept(noexcept(__VA_ARGS__))

/// This macro takes an expression to return from the function and deduces the `noexcept` specification from it.
#define SNAP_RHI_DEDUCE_NOEXCEPT_AND_RETURN(...) \
    SNAP_RHI_DEDUCE_NOEXCEPT(__VA_ARGS__) {      \
        return __VA_ARGS__;                      \
    }

/// This macro takes an expression to return from the function and deduces the `noexcept` specification and return type
/// from it.
#define SNAP_RHI_DEDUCE_NOEXCEPT_TYPE_AND_RETURN(...)              \
    SNAP_RHI_DEDUCE_NOEXCEPT(__VA_ARGS__)->decltype(__VA_ARGS__) { \
        return __VA_ARGS__;                                        \
    }

/// This macro takes a member initializer list and deduces the `noexcept` specification from it.
/// @note data members aren't supported, only base classes and delegated constructors.
#define SNAP_RHI_DEDUCE_NOEXCEPT_AND_INITIALIZE(...) SNAP_RHI_DEDUCE_NOEXCEPT(__VA_ARGS__) : __VA_ARGS__
