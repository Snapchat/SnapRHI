//
// Created by Vladyslav Deviatkov on 10/30/25.
//

#include "snap/rhi/QueryPool.h"

namespace snap::rhi {
QueryPool::QueryPool(Device* device, const QueryPoolCreateInfo& info)
    : snap::rhi::DeviceChild(device, snap::rhi::ResourceType::QueryPool), info(info) {}
} // namespace snap::rhi
