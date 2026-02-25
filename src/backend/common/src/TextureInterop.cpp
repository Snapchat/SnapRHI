#include <cassert>
#include <snap/rhi/Device.hpp>
#include <snap/rhi/backend/common/TextureInterop.h>

namespace snap::rhi::backend::common {
TextureInterop::TextureInterop(const TextureInteropCreateInfo& info) : snap::rhi::TextureInterop(info) {}

std::shared_ptr<snap::rhi::backend::common::PlatformSyncHandle> TextureInterop::sync(
    snap::rhi::Device* device,
    const std::shared_ptr<snap::rhi::backend::common::PlatformSyncHandle>& signalSyncHandle) {
    assert(signalSyncHandle);
    currentAPI = device->getCapabilities().apiDescription.api;
    auto returnedSignalSyncHandle = waitSignalSyncHandle;
    waitSignalSyncHandle = signalSyncHandle;
    return returnedSignalSyncHandle;
}
} // namespace snap::rhi::backend::common
