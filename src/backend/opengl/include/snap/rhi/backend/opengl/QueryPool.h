#pragma once

#include "snap/rhi/QueryPool.h"
#include "snap/rhi/backend/opengl/OpenGL.h"

#include "snap/rhi/backend/common/ValidationLayer.hpp"

namespace snap::rhi::backend::opengl {
class Device;
class Profile;

class QueryPool final : public snap::rhi::QueryPool {
public:
    QueryPool(snap::rhi::backend::opengl::Device* device, const snap::rhi::QueryPoolCreateInfo& info);
    ~QueryPool() override;

    void writeTimestamp(uint32_t query);

    void reset(uint32_t firstQuery, uint32_t queryCount);

    Result getResults(uint32_t firstQuery, uint32_t queryCount, std::span<std::chrono::nanoseconds> queries) override;

    Result getResultsAndAvailabilities(uint32_t firstQuery,
                                       uint32_t queryCount,
                                       std::span<std::chrono::nanoseconds> queries,
                                       std::span<bool> availabilities) override;

private:
    const snap::rhi::backend::common::ValidationLayer& validationLayer;
    snap::rhi::backend::opengl::Profile& gl;

    void checkDisjoint();
    bool getAnyDisjoint(uint32_t firstQuery, uint32_t queryCount);
    bool getAnyInternalError(uint32_t firstQuery, uint32_t queryCount);

    static constexpr GLuint Invalid = std::numeric_limits<uint32_t>::max();
    std::vector<GLuint> queryPool;
    std::vector<bool> queryDisjoint;
};
} // namespace snap::rhi::backend::opengl
