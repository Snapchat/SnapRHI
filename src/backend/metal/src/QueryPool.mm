#include "snap/rhi/backend/metal/QueryPool.h"
#include "snap/rhi/backend/metal/Device.h"

namespace snap::rhi::backend::metal {
QueryPool::QueryPool(snap::rhi::backend::metal::Device* device,
                     const snap::rhi::QueryPoolCreateInfo& info,
                     const id<MTLCounterSet>& counterSet)
    : snap::rhi::QueryPool(device, info) {
    // TODO: implement QueryPool for Metal
}

QueryPool::Result QueryPool::getResultsAndAvailabilities(uint32_t firstQuery,
                                                         uint32_t queryCount,
                                                         std::span<std::chrono::nanoseconds> queries,
                                                         std::span<bool> availabilities) {
    // TODO: implement QueryPool for Metal
    return Result::NotReady;
}
} // namespace snap::rhi::backend::metal
