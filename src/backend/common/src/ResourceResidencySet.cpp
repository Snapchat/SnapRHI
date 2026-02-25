#include "snap/rhi/backend/common/ResourceResidencySet.h"
#include "snap/rhi/backend/common/Device.hpp"
#include "snap/rhi/backend/common/Logging.hpp"
#include "snap/rhi/backend/common/Utils.hpp"

namespace {
} // unnamed namespace

namespace snap::rhi::backend::common {
ResourceResidencySet::ResourceResidencySet(Device* device, RetentionMode retentionMode)
    : device(device), retentionMode(retentionMode) {}

void ResourceResidencySet::setRetentionMode(RetentionMode mode) {
    retentionMode = mode;
}

void ResourceResidencySet::pushRef(snap::rhi::DeviceChild* resource) {
    const auto ref = device->resolveResource(resource);
#if SNAP_RHI_ENABLE_SLOW_VALIDATIONS()
    const auto& validationLayer = device->getValidationLayer();
    SNAP_RHI_VALIDATE(
        validationLayer,
        ref != nullptr,
        snap::rhi::ReportLevel::Error,
        snap::rhi::ValidationTag::CommandBufferOp,
        "[snap::rhi::backend::common::ResourceResidencySet::pushRef] Failed to resolve resource reference.");
#endif
    if (ref) {
        container.push_back(ref);
    }
}

#if SNAP_RHI_ENABLE_SLOW_VALIDATIONS()
void ResourceResidencySet::pushDebugInfo(snap::rhi::DeviceChild* resource) {
    const auto ref = device->resolveResource(resource);
    const auto& validationLayer = device->getValidationLayer();
    SNAP_RHI_VALIDATE(
        validationLayer,
        ref != nullptr,
        snap::rhi::ReportLevel::Error,
        snap::rhi::ValidationTag::CommandBufferOp,
        "[snap::rhi::backend::common::ResourceResidencySet::pushRef] Failed to resolve resource reference.");
    if (ref) {
        debugInfoContainer.push_back(
            ResourceDebugInfo{.resource = resource, .resourceType = resource->getResourceType(), .weakRef = ref});
    }
}
#endif

#if SNAP_RHI_ENABLE_SLOW_VALIDATIONS()
void ResourceResidencySet::validateResourceLifetimes() const {
    [[maybe_unused]] const auto& validationLayer = device->getValidationLayer();
    for (const auto& ref : debugInfoContainer) {
        if (!ref.weakRef.expired()) {
            continue;
        }
        SNAP_RHI_LOGE(
            "[snap::rhi::backend::common::ResourceResidencySet::validateResourceLifetimes] Resource has been destroyed "
            "prematurely (typeName=%s, type=%u, ptr=%p).",
            getResourceTypeStr(ref.resourceType).data(),
            static_cast<uint32_t>(ref.resourceType),
            static_cast<void*>(ref.resource));

#if SNAP_RHI_DEBUG() || SNAP_RHI_THROW_ON_LIFETIME_VIOLATION
        SNAP_RHI_REPORT(
            validationLayer,
            snap::rhi::ReportLevel::Error,
            snap::rhi::ValidationTag::CommandBufferOp,
            "[snap::rhi::backend::common::ResourceResidencySet::validateResourceLifetimes] Resource has been "
            "destroyed prematurely (typeName=%s, type=%u, ptr=%p).",
            getResourceTypeStr(ref.resourceType).data(),
            static_cast<uint32_t>(ref.resourceType),
            static_cast<void*>(ref.resource));
#endif
    }
}
#endif

void ResourceResidencySet::clear() {
    container.clear();
#if SNAP_RHI_ENABLE_SLOW_VALIDATIONS()
    debugInfoContainer.clear();
#endif
}
} // namespace snap::rhi::backend::common
