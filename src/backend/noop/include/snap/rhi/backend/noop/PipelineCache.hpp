#pragma once

#include "snap/rhi/PipelineCache.hpp"

namespace snap::rhi::backend::common {
class Device;
} // namespace snap::rhi::backend::common

namespace snap::rhi::backend::noop {
class PipelineCache final : public snap::rhi::PipelineCache {
public:
    explicit PipelineCache(snap::rhi::backend::common::Device* device, const snap::rhi::PipelineCacheCreateInfo& info);
    ~PipelineCache() override;

    Result serializeToFile(const std::filesystem::path& cachePath) const override;
};

} // namespace snap::rhi::backend::noop
