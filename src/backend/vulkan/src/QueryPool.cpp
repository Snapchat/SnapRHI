#include "snap/rhi/backend/vulkan/QueryPool.h"
#include "snap/rhi/backend/vulkan/Device.h"

namespace {
constexpr auto INVALID = std::chrono::nanoseconds::max();

template<typename SpanType>
void validateSpan(const snap::rhi::backend::common::ValidationLayer& validationLayer,
                  SpanType span,
                  uint32_t queryCount,
                  snap::rhi::common::zstring_view errorMsg) {
    SNAP_RHI_VALIDATE(validationLayer,
                      span.size() == queryCount && queryCount >= 1,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::QueryPoolOp,
                      errorMsg);
}
} // anonymous namespace

namespace snap::rhi::backend::vulkan {

QueryPool::QueryPool(Device* device, const snap::rhi::QueryPoolCreateInfo& info)
    : snap::rhi::QueryPool(device, info),
      vkDevice(device->getVkLogicalDevice()),
      validationLayer(device->getValidationLayer()),
      nsPerGpuTick(device->getPhysicalDeviceProperties().limits.timestampPeriod) {
    const uint32_t queryPoolSize = info.queryCount;
    VkQueryPoolCreateInfo queryPoolInfo{.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
                                        .queryType = VK_QUERY_TYPE_TIMESTAMP,
                                        .queryCount = queryPoolSize};
    VkResult result = vkCreateQueryPool(vkDevice, &queryPoolInfo, nullptr, &queryPool);
    SNAP_RHI_VALIDATE(validationLayer,
                      result == VK_SUCCESS,
                      snap::rhi::ReportLevel::CriticalError,
                      snap::rhi::ValidationTag::CreateOp,
                      "[Vulkan::QueryPool] failed to create pool with size %d",
                      static_cast<int>(queryPoolSize));
}

QueryPool::~QueryPool() {
    vkDestroyQueryPool(vkDevice, queryPool, nullptr);
}

VkQueryPool QueryPool::getVkQueryPool() const {
    return queryPool;
}

QueryPool::Result QueryPool::getResults(uint32_t firstQuery,
                                        uint32_t queryCount,
                                        std::span<std::chrono::nanoseconds> queries) {
    validateSpan(validationLayer, queries, queryCount, "[vulkan::QueryPool] queries is the wrong size");
    std::vector<uint64_t>& readout = scratch;
    readout.clear();
    readout.resize(queryCount);
    static constexpr std::size_t kReadSize_t = sizeof(std::decay_t<decltype(readout)>::value_type);
    VkResult result = vkGetQueryPoolResults(vkDevice,
                                            queryPool,
                                            firstQuery,
                                            queryCount,
                                            readout.size() * kReadSize_t,
                                            readout.data(),
                                            kReadSize_t,
                                            VK_QUERY_RESULT_64_BIT);
    SNAP_RHI_VALIDATE(validationLayer,
                      result == VK_SUCCESS,
                      snap::rhi::ReportLevel::Warning,
                      snap::rhi::ValidationTag::QueryPoolOp,
                      "[Vulkan::QueryPool::getResults] failed to get query pool results");
    if (result != VK_SUCCESS) {
        return Result::NotReady;
    }
    for (std::size_t i = 0; i < readout.size(); ++i) {
        const uint64_t ticks = readout[i];
        const auto timestamp = std::chrono::nanoseconds(static_cast<long long>(ticks * nsPerGpuTick));
        queries[i] = timestamp;
    }
    return Result::Available;
}

QueryPool::Result QueryPool::getResultsAndAvailabilities(uint32_t firstQuery,
                                                         uint32_t queryCount,
                                                         std::span<std::chrono::nanoseconds> queries,
                                                         std::span<bool> availabilities) {
    validateSpan(validationLayer, queries, queryCount, "[vulkan::QueryPool] queries is the wrong size");
    validateSpan(validationLayer, availabilities, queryCount, "[vulkan::QueryPool] availabilities is the wrong size");
    std::vector<uint64_t>& readout = scratch;
    readout.clear();
    readout.resize(2 * queryCount);
    static constexpr std::size_t kReadSize_t = sizeof(std::decay_t<decltype(readout)>::value_type);
    VkResult result = vkGetQueryPoolResults(vkDevice,
                                            queryPool,
                                            firstQuery,
                                            queryCount,
                                            readout.size() * kReadSize_t,
                                            readout.data(),
                                            2 * kReadSize_t,
                                            VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WITH_AVAILABILITY_BIT);
    SNAP_RHI_VALIDATE(validationLayer,
                      result == VK_SUCCESS,
                      snap::rhi::ReportLevel::Warning,
                      snap::rhi::ValidationTag::QueryPoolOp,
                      "[Vulkan::QueryPool::getResultsAndAvailabilities] failed to get query pool results");
    if (result != VK_SUCCESS) {
        return Result::Error;
    }
    bool allAvail = true;
    for (std::size_t i = 0; i < readout.size(); i += 2) {
        const bool avail = static_cast<bool>(readout[i + 1]);
        const uint64_t ticks = readout[i];
        const auto timestamp = avail ? std::chrono::nanoseconds(static_cast<long long>(ticks * nsPerGpuTick)) : INVALID;
        availabilities[i / 2] = avail;
        allAvail &= avail;
        queries[i / 2] = timestamp;
    }
    return allAvail ? Result::Available : Result::NotReady;
}

} // namespace snap::rhi::backend::vulkan
