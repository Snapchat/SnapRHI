#pragma once

#include "snap/rhi/ComputePipelineCreateInfo.h"
#include "snap/rhi/PipelineCache.hpp"
#include "snap/rhi/RenderPassCreateInfo.h"
#include "snap/rhi/RenderPipelineCreateInfo.h"
#include "snap/rhi/backend/common/ValidationLayer.hpp"

#include "snap/rhi/backend/vulkan/vulkan.h"
#include <mutex>

namespace snap::rhi::backend::vulkan {
class Device;

class PipelineCache final : public snap::rhi::PipelineCache {
public:
    explicit PipelineCache(snap::rhi::backend::vulkan::Device* device, const snap::rhi::PipelineCacheCreateInfo& info);
    ~PipelineCache() final;

    void setDebugLabel(std::string_view label) override;

    VkPipelineCache getPipelineCache() const {
        return pipelineCache;
    }

    Result serializeToFile(const std::filesystem::path& cachePath) const final;

private:
    VkDevice vkDevice = VK_NULL_HANDLE;
    const snap::rhi::backend::common::ValidationLayer& validationLayer;

    VkPipelineCache pipelineCache = VK_NULL_HANDLE;
};
} // namespace snap::rhi::backend::vulkan
