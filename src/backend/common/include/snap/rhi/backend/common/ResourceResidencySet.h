//
//  ResourceResidencySet.h
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 13.10.2021.
//

#pragma once

#include "snap/rhi/DeviceChild.hpp"
#include "snap/rhi/Enums.h"
#include "snap/rhi/backend/common/Utils.hpp"

#include <memory>
#include <vector>

namespace snap::rhi::backend::common {
class Device;

class ResourceResidencySet final {
public:
    enum class RetentionMode : uint32_t { RetainReferences, NonRetainableReferences };

public:
    explicit ResourceResidencySet(Device* device, RetentionMode retentionMode = RetentionMode::RetainReferences);
    ~ResourceResidencySet() = default;

    SNAP_RHI_ALWAYS_INLINE void track(snap::rhi::DeviceChild* resource) {
        if (retentionMode == RetentionMode::RetainReferences) {
            pushRef(resource);
            return;
        }

#if SNAP_RHI_ENABLE_SLOW_VALIDATIONS()
        if (retentionMode == RetentionMode::NonRetainableReferences) {
            pushDebugInfo(resource);
            return;
        }
#endif
    }
    void setRetentionMode(RetentionMode mode);

#if SNAP_RHI_ENABLE_SLOW_VALIDATIONS()
    void validateResourceLifetimes() const;
#endif
    void clear();

private:
    Device* device = nullptr;

    void pushRef(snap::rhi::DeviceChild* resource);
    std::vector<std::shared_ptr<snap::rhi::DeviceChild>> container;

#if SNAP_RHI_ENABLE_SLOW_VALIDATIONS()
    struct ResourceDebugInfo {
        snap::rhi::DeviceChild* resource = nullptr;
        snap::rhi::ResourceType resourceType = snap::rhi::ResourceType::Undefined;
        std::weak_ptr<snap::rhi::DeviceChild> weakRef;
    };
    void pushDebugInfo(snap::rhi::DeviceChild* resource);

    std::vector<ResourceDebugInfo> debugInfoContainer;
#endif
    RetentionMode retentionMode = RetentionMode::RetainReferences;
};
} // namespace snap::rhi::backend::common
