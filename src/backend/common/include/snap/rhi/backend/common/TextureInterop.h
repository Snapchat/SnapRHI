#pragma once

#include "snap/rhi/TextureInterop.h"
#include "snap/rhi/TextureInteropCreateInfo.h"
#include "snap/rhi/backend/common/PlatformSyncHandle.h"
#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/common/OS.h"
#include "snap/rhi/common/Throw.h"

#include <future>
#include <memory>

namespace snap::rhi {
class Device;
} // namespace snap::rhi

namespace snap::rhi::backend::common {
class PlatformSyncHandle;

class TextureInterop : public snap::rhi::TextureInterop {
public:
    explicit TextureInterop(const TextureInteropCreateInfo& info);
    ~TextureInterop() override = default;

    /**
     * @brief Synchronizes texture access for cross-API interoperability (e.g., OpenGL, Metal, Vulkan).
     *
     * @param device The target device context (and API) where the texture will be used next.
     * @param signalSyncHandle The synchronization handle serving as the signal semaphore for the target API.
     * @return std::shared_ptr<PlatformSyncHandle> A handle to a wait semaphore. This must be waited upon
     * before the texture can be safely accessed on the new device.
     */
    virtual std::shared_ptr<PlatformSyncHandle> sync(snap::rhi::Device* device,
                                                     const std::shared_ptr<PlatformSyncHandle>& signalSyncHandle);

    snap::rhi::API getCurrentAPI() const noexcept {
        return currentAPI;
    }

    std::span<const snap::rhi::TextureInterop::ImagePlane> map(
        const snap::rhi::MemoryAccess access, snap::rhi::PlatformSyncHandle* platformSyncHandle) override {
        if (platformSyncHandle) {
            auto* commonHandle = snap::rhi::backend::common::smart_cast<snap::rhi::backend::common::PlatformSyncHandle>(
                platformSyncHandle);
            if (const auto& fence = commonHandle->getFence(); fence) {
                fence->waitForComplete();
            }
        }

        snap::rhi::common::throwException("[TextureInterop::map] CPU-GPU interop not supported on this platform");
    }

    std::unique_ptr<snap::rhi::PlatformSyncHandle> unmap() override {
        return std::make_unique<snap::rhi::backend::common::PlatformSyncHandle>(nullptr);
    }

protected:
    snap::rhi::API currentAPI = snap::rhi::API::Undefined;
    std::shared_ptr<snap::rhi::backend::common::PlatformSyncHandle> waitSignalSyncHandle = nullptr;
};
} // namespace snap::rhi::backend::common
