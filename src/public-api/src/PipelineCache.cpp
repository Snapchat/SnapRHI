#include "snap/rhi/PipelineCache.hpp"

namespace snap::rhi {
PipelineCache::PipelineCache(Device* device, const PipelineCacheCreateInfo& info)
    : snap::rhi::DeviceChild(device, snap::rhi::ResourceType::PipelineCache), createFlags(info.createFlags) {}
} // namespace snap::rhi
