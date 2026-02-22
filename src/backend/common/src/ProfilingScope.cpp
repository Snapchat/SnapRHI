#include "snap/rhi/backend/common/ProfilingScope.hpp"
#include "snap/rhi/backend/common/Device.hpp"

#if SNAP_RHI_ENABLE_CUSTOM_PROFILING_LABELS
namespace snap::rhi::backend::common {
ProfilingScope::ProfilingScope(snap::rhi::Device* device, std::string_view label) : device(device), label(label) {
    const auto& deviceCreateInfo = device->getDeviceCreateInfo();
    const auto& profilingCreateInfo = deviceCreateInfo.profilingCreateInfo;

    if (profilingCreateInfo.onStartScope) {
        profilingCreateInfo.onStartScope(label);
    }
}

ProfilingScope::~ProfilingScope() {
    const auto& deviceCreateInfo = device->getDeviceCreateInfo();
    const auto& profilingCreateInfo = deviceCreateInfo.profilingCreateInfo;

    if (profilingCreateInfo.onEndScope) {
        profilingCreateInfo.onEndScope(label);
    }
}
} // namespace snap::rhi::backend::common
#endif
