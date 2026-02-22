#include "snap/rhi/DescriptorSetLayout.hpp"
#include "snap/rhi/common/Throw.h"
#include <algorithm>
#include <ranges>

namespace snap::rhi {
DescriptorSetLayout::DescriptorSetLayout(Device* device, const snap::rhi::DescriptorSetLayoutCreateInfo& info)
    : snap::rhi::DeviceChild(device, snap::rhi::ResourceType::DescriptorSetLayout), info(info) {
#if SNAP_RHI_ENABLE_SLOW_SAFETY_CHECKS
    auto bindings = info.bindings;
    std::ranges::sort(bindings, [](const auto& lhs, const auto& rhs) { return lhs.binding < rhs.binding; });
    bindings.resize(std::unique(bindings.begin(),
                                bindings.end(),
                                [](const auto& lhs, const auto& rhs) { return lhs.binding == rhs.binding; }) -
                    bindings.begin());
    if (bindings != info.bindings) {
        snap::rhi::common::throwException(
            "[SnapRHI] DescriptorSetLayout bindings must be sorted by binding index and contain no duplicates");
    }
#endif
}
} // namespace snap::rhi
