//
// Created by Vladyslav Deviatkov on 10/30/25.
//

#pragma once

#include "snap/rhi/DeviceChild.hpp"
#include "snap/rhi/QueryPoolCreateInfo.h"

#include <chrono>
#include <cstdint>
#include <span>

namespace snap::rhi {
class Device;

class QueryPool : public snap::rhi::DeviceChild {
public:
    enum class Result : uint32_t {
        /**
         * @brief The query has finished successfully, and the timing data is valid.
         * @details You can safely read the timestamp and use it for profiling or statistics.
         */
        Available,

        /**
         * @brief The GPU is still processing the commands associated with queries.
         * @details The result is not yet available. The caller should retry polling this query later.
         */
        NotReady,

        /**
         * @brief The query finished, but the timing data is invalid due to a disjoint event.
         * @details This occurs if the GPU was throttled, the context was switched, or
         * an OS-level interruption occurred during execution. The associated time
         * value should be discarded to prevent skewing profiling averages.
         */
        Disjoint,

        /**
         * @brief The query failed due to an internal error.
         * @details This generally indicates an invalid query ID, or due to any internal failure.
         */
        Error
    };

public:
    QueryPool(Device* device, const QueryPoolCreateInfo& info);
    ~QueryPool() override = default;

    /**
     * @brief Gets the results for a range of queries. Results are in nanoseconds.
     * This may block until the results are available.
     * @param firstQuery The index of the first query.
     * @param queryCount The number of queries.
     * @param queries A span to write the 64-bit timestamp results (in nanoseconds).
     */
    virtual Result getResults(uint32_t firstQuery, uint32_t queryCount, std::span<std::chrono::nanoseconds> queries) {
        return getResultsAndAvailabilities(firstQuery, queryCount, queries, {});
    }

    /**
     * @brief Gets both results and availability for a range of queries.
     * Results will be in nanoseconds.
     * @param firstQuery The index of the first query.
     * @param queryCount The number of queries.
     * @param queries (Optional) A span to write the 64-bit timestamp results.
     * @param availabilities (Optional) A span to write availability results.
     */
    virtual Result getResultsAndAvailabilities(uint32_t firstQuery,
                                               uint32_t queryCount,
                                               std::span<std::chrono::nanoseconds> queries,
                                               std::span<bool> availabilities) = 0;

    const QueryPoolCreateInfo& getCreateInfo() const noexcept {
        return info;
    }

    uint64_t getCPUMemoryUsage() const override {
        return sizeof(QueryPoolCreateInfo);
    }

    uint64_t getGPUMemoryUsage() const override {
        return 0;
    }

protected:
    QueryPoolCreateInfo info{};

private:
};
} // namespace snap::rhi
