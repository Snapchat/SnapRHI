#pragma once

/// Correctly stringifies both text and macros (for example, __LINE__) by using a late evaluation technique.
#define SNAP_RHI_STRINGIFY(x) SNAP_RHI_STRINGIFY_IMPL(x)
#define SNAP_RHI_STRINGIFY_IMPL(x) #x

/// Correctly concatenates both text and macros (for example, __LINE__) by using a late evaluation technique.
#define SNAP_RHI_CONCAT(first, ...) SNAP_RHI_CONCAT_IMPL(first, __VA_ARGS__)
#define SNAP_RHI_CONCAT_IMPL(first, ...) first##__VA_ARGS__
