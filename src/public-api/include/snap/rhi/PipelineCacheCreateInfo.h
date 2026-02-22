#pragma once

#include "snap/rhi/Enums.h"
#include <filesystem>

namespace snap::rhi {
/**
 * @brief Pipeline cache creation parameters.
 *
 * This structure configures how a backend pipeline cache is created and (optionally) persisted to disk.
 *
 * Backends may use @ref cachePath in two ways:
 * - as an input: load initial cache data from an existing file (when available);
 * - as an output: a suggested destination path when calling `PipelineCache::serializeToFile()`.
 */
struct PipelineCacheCreateInfo {
    /**
     * @brief Flags controlling pipeline cache behavior.
     *
     * Notably, @ref snap::rhi::PipelineCacheCreateFlags::ExternallySynchronized communicates that the application will
     * provide external synchronization for concurrent access to the cache.
     *
     * @note When this flag is not set, some backends guard cache access internally (e.g., using a mutex).
     */
    snap::rhi::PipelineCacheCreateFlags createFlags = snap::rhi::PipelineCacheCreateFlags::None;

    /**
     * @brief Optional filesystem path for cache persistence.
     *
     * If empty, backends will not attempt to load initial cache data from disk.
     *
     * @warning Cache data is generally not portable across different GPUs, driver versions, or OS versions.
     */
    std::filesystem::path cachePath;
};
} // namespace snap::rhi
