#pragma once

#include <snap/rhi/common/Compiler.h>
#include <snap/rhi/common/Guard.h>
#include <snap/rhi/common/deduce_noexcept.hpp>

#include <snap/rhi/common/Concat.h>

namespace snap::rhi::common {

// Snap-C++ convention of template implementation details to go into Detail
namespace Detail {

// Allows the creation of a `TargetTemplate` instance via `operator<<` and
// deduction guides; useful to create scope-bound variables such as in guards,
// mutex locks, threads (should they be templates) without the syntax of normal
// named-construction
template<template<typename> class TargetTemplate>
struct MakerByInsertion {
    template<typename ToInsert>
    auto operator<<(ToInsert&& fr) const SNAP_RHI_DEDUCE_NOEXCEPT_AND_RETURN(TargetTemplate{std::forward<ToInsert>(fr)})
};
} // namespace Detail

#if !SNAP_RHI_GCC_COMPATIBLE_COMPILER() && !SNAP_RHI_CONFIG_COMPILER_MSVC()
#pragma error Unknown way to generate unique identifiers
#else
#define SNAP_RHI_PP_UNIQUE_IDENTIFIER(prefix) SNAP_RHI_CONCAT(prefix, __COUNTER__)
#endif

// clang-format off

#define SNAP_RHI_PP_IMPL_GUARD_CAPTURING_STATEMENT(flavor) \
    auto SNAP_RHI_PP_UNIQUE_IDENTIFIER(forThisScope) = \
            snap::rhi::common::Detail::MakerByInsertion< \
                SNAP_RHI_CONCAT(snap::rhi::common::Scope, flavor) \
            >{} << [&]()

#define SNAP_RHI_ON_SCOPE_EXIT SNAP_RHI_PP_IMPL_GUARD_CAPTURING_STATEMENT(Guard)
#define SNAP_RHI_ON_SCOPE_SUCCESS SNAP_RHI_PP_IMPL_GUARD_CAPTURING_STATEMENT(Success)
#define SNAP_RHI_ON_SCOPE_FAILURE SNAP_RHI_PP_IMPL_GUARD_CAPTURING_STATEMENT(Failure)

// clang-format on

} // namespace snap::rhi::common
