#include "snap/rhi/backend/opengl/QueryPool.h"
#include "snap/rhi/backend/opengl/Device.hpp"

#include <cassert>

namespace {
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
} // namespace

namespace snap::rhi::backend::opengl {
QueryPool::QueryPool(snap::rhi::backend::opengl::Device* device, const snap::rhi::QueryPoolCreateInfo& info)
    : snap::rhi::QueryPool(device, info),
      validationLayer(device->getValidationLayer()),
      gl(device->getOpenGL()),
      queryPool(std::vector<uint32_t>(info.queryCount)),
      queryDisjoint(std::vector<bool>(info.queryCount)) {
    gl.genQueries(info.queryCount, queryPool.data());
}

QueryPool::~QueryPool() {
    gl.deleteQueries(queryPool.size(), queryPool.data());
}

void QueryPool::writeTimestamp(uint32_t query) {
    const GLuint queryNative = queryPool[query];
    gl.queryCounter(queryNative, GL_TIMESTAMP);
}

void QueryPool::checkDisjoint() {
    GLint disjointError = 0;
    gl.getIntegerv(GL_GPU_DISJOINT, &disjointError);
    if (disjointError) {
        for (size_t i = 0; i < queryPool.size(); ++i) {
            queryDisjoint[i] = true;
        }
    }
}

void QueryPool::reset(uint32_t firstQuery, uint32_t queryCount) {
    for (size_t i = 0; i < queryCount; ++i) {
        queryDisjoint[firstQuery + i] = false;
        queryPool[firstQuery + i] = Invalid;
    }
    // we don't touch the actual queries themselves.
    // glBeginQuery and glQueryCounter will do it implicitly.
    // "When glBeginQuery is executed, the query object's primitives-written counter is reset to 0." -- GL ES 3 spec
    // page. glQueryCounter is not an accumulation operation and does not need a reset because it statelessly writes the
    // current timestamp.
}

QueryPool::Result QueryPool::getResults(uint32_t firstQuery,
                                        uint32_t queryCount,
                                        std::span<std::chrono::nanoseconds> queries) {
    validateSpan(
        validationLayer, queries, queryCount, "[QueryPool::getResultsAndAvailabilities] queries is the wrong size");
    checkDisjoint();
    const bool anyDisjoint = getAnyDisjoint(firstQuery, queryCount);
    if (anyDisjoint) {
        return Result::Disjoint;
    }
    const bool anyError = getAnyInternalError(firstQuery, queryCount);
    if (anyError) {
        return Result::Error;
    }
    for (size_t i = 0; i < queryCount; ++i) {
        const GLuint query = queryPool[firstQuery + i];
        GLuint64 timestamp = 0;
        // this is a BLOCKING call if the query is not available
        gl.getQueryObjectui64v(query, GL_QUERY_RESULT, &timestamp);
        queries[i] = std::chrono::nanoseconds(timestamp);
    }
    reset(firstQuery, queryCount);
    return Result::Available;
}

QueryPool::Result QueryPool::getResultsAndAvailabilities(uint32_t firstQuery,
                                                         uint32_t queryCount,
                                                         std::span<std::chrono::nanoseconds> queries,
                                                         std::span<bool> availabilities) {
    validateSpan(
        validationLayer, queries, queryCount, "[QueryPool::getResultsAndAvailabilities] queries is the wrong size");
    validateSpan(validationLayer,
                 availabilities,
                 queryCount,
                 "[QueryPool::getResultsAndAvailabilities] availabilities is the wrong size");
    checkDisjoint();
    const bool anyDisjoint = getAnyDisjoint(firstQuery, queryCount);
    if (anyDisjoint) {
        return Result::Disjoint;
    }
    const bool anyError = getAnyInternalError(firstQuery, queryCount);
    if (anyError) {
        return Result::Error;
    }
    bool allAvail = true;
    for (size_t i = 0; i < queryCount; ++i) {
        const GLuint query = queryPool[firstQuery + i];
        GLint available = 0;
        gl.getQueryObjectiv(query, GL_QUERY_RESULT_AVAILABLE, &available);
        availabilities[i] = available;
        allAvail &= available;
        if (available) {
            GLuint64 timestamp = 0;
            gl.getQueryObjectui64v(query, GL_QUERY_RESULT, &timestamp);
            queries[i] = std::chrono::nanoseconds(timestamp);
        }
    }
    reset(firstQuery, queryCount);
    if (allAvail) {
        return Result::Available;
    } else {
        return Result::NotReady;
    }
}

bool QueryPool::getAnyDisjoint(uint32_t firstQuery, uint32_t queryCount) {
    return std::any_of(
        queryDisjoint.begin() + firstQuery, queryDisjoint.begin() + firstQuery + queryCount, [](bool b) { return b; });
}

bool QueryPool::getAnyInternalError(uint32_t firstQuery, uint32_t queryCount) {
    return std::any_of(queryPool.begin() + firstQuery, queryPool.begin() + firstQuery + queryCount, [](GLuint id) {
        return id == Invalid;
    });
}

} // namespace snap::rhi::backend::opengl
