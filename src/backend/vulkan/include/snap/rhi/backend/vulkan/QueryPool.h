#pragma once

#include "snap/rhi/QueryPool.h"
#include "snap/rhi/backend/common/ValidationLayer.hpp"
#include "snap/rhi/backend/vulkan/vulkan.h"

namespace snap::rhi::backend::vulkan {
class Device;

class QueryPool final : public snap::rhi::QueryPool {
public:
    QueryPool(Device* device, const snap::rhi::QueryPoolCreateInfo& info);
    ~QueryPool() override;

    VkQueryPool getVkQueryPool() const;

    Result getResults(uint32_t firstQuery, uint32_t queryCount, std::span<std::chrono::nanoseconds> queries) override;

    Result getResultsAndAvailabilities(uint32_t firstQuery,
                                       uint32_t queryCount,
                                       std::span<std::chrono::nanoseconds> queries,
                                       std::span<bool> availabilities) override;

private:
    VkDevice vkDevice = VK_NULL_HANDLE;
    const snap::rhi::backend::common::ValidationLayer& validationLayer;
    const float nsPerGpuTick = 0;
    VkQueryPool queryPool = VK_NULL_HANDLE;
    std::vector<uint64_t> scratch;
};

} // namespace snap::rhi::backend::vulkan
