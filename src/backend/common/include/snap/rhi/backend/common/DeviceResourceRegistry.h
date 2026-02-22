//
//  DeviceResourceRegistry.h
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 13.10.2025.
//

#pragma once

#include "snap/rhi/DeviceChild.hpp"
#include "snap/rhi/backend/common/Utils.hpp"
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace snap::rhi::backend::common {
class DeviceResourceRegistry final {
public:
    DeviceResourceRegistry() = default;
    ~DeviceResourceRegistry() = default;

    void for_each(const std::function<void(const std::weak_ptr<snap::rhi::DeviceChild>&)>& func) const;

    void insert(const std::shared_ptr<snap::rhi::DeviceChild>& resource);
    std::weak_ptr<snap::rhi::DeviceChild> find(snap::rhi::DeviceChild* resource) const;
    void erase(snap::rhi::DeviceChild* resource);

    bool empty();
    void clear();

private:
    mutable std::mutex accessMutex;
    std::unordered_map<snap::rhi::DeviceChild*, std::weak_ptr<snap::rhi::DeviceChild>> holder;

#if SNAP_RHI_ENABLE_SLOW_VALIDATIONS()
    /**
     * Used only for debug purposes to track live resources
     */
    std::vector<snap::rhi::DeviceChild*> resourceList;
#endif
};
} // namespace snap::rhi::backend::common
