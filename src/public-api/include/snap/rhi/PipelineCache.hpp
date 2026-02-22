//
//  PipelineCache.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 06.05.2021.
//

#pragma once

#include <cstdint>

#include "snap/rhi/DeviceChild.hpp"
#include "snap/rhi/Enums.h"
#include "snap/rhi/PipelineCacheCreateInfo.h"
#include <span>

namespace snap::rhi {
class Device;

/**
 * @brief Cross-backend pipeline cache.
 *
 * `PipelineCache` stores backend-specific pipeline compilation artifacts that can be reused across application runs
 * to reduce pipeline/shader creation latency.
 *
 * The exact behavior is backend dependent:
 * - Vulkan typically wraps a native pipeline cache object and can serialize it to disk.
 * - OpenGL may cache program binaries (when supported) and serialize them.
 * - Metal may use platform-specific binary archive functionality when available.
 * - Other backends may treat this object as a no-op.
 *
 * @note Pipeline caches are not guaranteed to be portable across:
 * - different devices/GPUs,
 * - different driver versions,
 * - different pipeline creation parameters.
 */
class PipelineCache : public snap::rhi::DeviceChild {
public:
    PipelineCache(Device* device, const PipelineCacheCreateInfo& info);
    ~PipelineCache() override = default;

    /**
     * @brief Serializes the pipeline cache to a file.
     *
     * Implementations typically write backend-specific binary data to @p cachePath, creating parent directories when
     * needed.
     *
     * @param cachePath Target file path.
     * @return Result code indicating success/failure.
     *
     * @note Not all backends support serialization. Unsupported backends may return
     * `Result::Success` (no-op) or `Result::ErrorUnknown`.
     *
     * @warning The produced cache data may be invalid if reused on a different device/driver.
     */
    virtual Result serializeToFile(const std::filesystem::path& cachePath) const = 0;

    /**
     * @brief Returns an estimate of CPU memory usage for this object.
     */
    uint64_t getCPUMemoryUsage() const override {
        return 0;
    }

    /**
     * @brief Returns an estimate of GPU/driver memory usage for this object.
     */
    uint64_t getGPUMemoryUsage() const override {
        return 0;
    }

protected:
    /**
     * @brief Creation flags controlling cache behavior.
     */
    snap::rhi::PipelineCacheCreateFlags createFlags = snap::rhi::PipelineCacheCreateFlags::None;
};
} // namespace snap::rhi
