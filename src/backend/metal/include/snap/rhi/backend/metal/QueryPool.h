#pragma once

#include "snap/rhi/QueryPool.h"
#include <Metal/Metal.h>

namespace snap::rhi::backend::metal {
class Device;
class Profile;

class API_AVAILABLE(macos(10.15), ios(14.0)) QueryPool final : public snap::rhi::QueryPool {
public:
    QueryPool(snap::rhi::backend::metal::Device* device,
              const snap::rhi::QueryPoolCreateInfo& info,
              const id<MTLCounterSet>& counterSet);
    ~QueryPool() override = default;

    Result getResultsAndAvailabilities(uint32_t firstQuery,
                                       uint32_t queryCount,
                                       std::span<std::chrono::nanoseconds> queries,
                                       std::span<bool> availabilities) override;

private:
    id<MTLCounterSampleBuffer> sampler = nil;
};
} // namespace snap::rhi::backend::metal
