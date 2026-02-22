//
// Created by Vladyslav Deviatkov on 10/30/25.
//

#pragma once

#include "snap/rhi/Enums.h"
#include <cstdint>

namespace snap::rhi {
struct QueryPoolCreateInfo {
    /**
     * The number of queries to be managed by the pool
     */
    uint32_t queryCount;
};
} // namespace snap::rhi
