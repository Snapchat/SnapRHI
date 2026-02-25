#include "snap/rhi/backend/noop/PipelineCache.hpp"

#include "snap/rhi/backend/noop/Device.hpp"

namespace snap::rhi::backend::noop {
PipelineCache::PipelineCache(snap::rhi::backend::common::Device* device, const snap::rhi::PipelineCacheCreateInfo& info)
    : snap::rhi::PipelineCache(device, info) {}

PipelineCache::~PipelineCache() {}

Result PipelineCache::serializeToFile(const std::filesystem::path& cachePath) const {
    return snap::rhi::Result::Success;
}
} // namespace snap::rhi::backend::noop
