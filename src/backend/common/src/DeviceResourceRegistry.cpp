//
// Created by Vladyslav Deviatkov on 12/9/25.
//

#include "snap/rhi/backend/common/DeviceResourceRegistry.h"
#include "snap/rhi/common/Throw.h"
#include <algorithm>
#include <mutex>
#include <ranges>

namespace snap::rhi::backend::common {
void DeviceResourceRegistry::for_each(
    const std::function<void(const std::weak_ptr<snap::rhi::DeviceChild>&)>& func) const {
    std::lock_guard lock(accessMutex);
    for (const auto& weakPtr : holder | std::views::values) {
        func(weakPtr);
    }
}

void DeviceResourceRegistry::insert(const std::shared_ptr<snap::rhi::DeviceChild>& resource) {
    std::lock_guard lock(accessMutex);
    assert(!holder.contains(resource.get()) &&
           "[snap::rhi::backend::common::DeviceResourceRegistry::insert] attempt to insert already existing resource!");
    holder[resource.get()] = resource;

#if SNAP_RHI_ENABLE_SLOW_VALIDATIONS()
    resourceList.push_back(resource.get());
#endif
}

std::weak_ptr<snap::rhi::DeviceChild> DeviceResourceRegistry::find(snap::rhi::DeviceChild* resource) const {
    std::lock_guard lock(accessMutex);
    if (auto itr = holder.find(resource); itr != holder.end()) {
        return itr->second;
    }

    return {};
}

void DeviceResourceRegistry::erase(snap::rhi::DeviceChild* resource) {
    std::lock_guard lock(accessMutex);
    holder.erase(resource);

#if SNAP_RHI_ENABLE_SLOW_VALIDATIONS()
    resourceList.erase(std::find(resourceList.begin(), resourceList.end(), resource));
#endif
}

bool DeviceResourceRegistry::empty() {
    std::lock_guard lock(accessMutex);
    return holder.empty();
}

void DeviceResourceRegistry::clear() {
    std::lock_guard lock(accessMutex);
    holder.clear();

#if SNAP_RHI_ENABLE_SLOW_VALIDATIONS()
    resourceList.clear();
#endif
}
} // namespace snap::rhi::backend::common
