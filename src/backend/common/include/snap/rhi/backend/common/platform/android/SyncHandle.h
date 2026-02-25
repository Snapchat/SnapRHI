#pragma once

#include "snap/rhi/backend/common/PlatformSyncHandle.h"
#include <snap/rhi/common/Platform.h>

#if SNAP_RHI_OS_ANDROID()
#include <unistd.h>

namespace snap::rhi::backend::common::platform::android {
class SyncHandle : public snap::rhi::backend::common::PlatformSyncHandle {
public:
    explicit SyncHandle(const std::shared_ptr<snap::rhi::Fence>& fence, const int32_t fenceFd = -1)
        : snap::rhi::backend::common::PlatformSyncHandle(fence), fenceFd(fenceFd) {}

    ~SyncHandle() override {
        if (fenceFd >= 0) {
            close(fenceFd);
        }
    }

    int32_t releaseFenceFd() {
        const int32_t fd = this->fenceFd;
        this->fenceFd = -1;
        return fd;
    }

protected:
    int32_t fenceFd = -1;
};
} // namespace snap::rhi::backend::common::platform::android
#endif
