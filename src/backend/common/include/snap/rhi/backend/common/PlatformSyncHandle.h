#pragma once

#include "snap/rhi/Fence.hpp"
#include "snap/rhi/PlatformSyncHandle.h"
#include <memory>

namespace snap::rhi::backend::common {
class Device;

class PlatformSyncHandle : public snap::rhi::PlatformSyncHandle {
public:
    explicit PlatformSyncHandle(const std::shared_ptr<snap::rhi::Fence>& fence);
    ~PlatformSyncHandle() override = default;

    const std::shared_ptr<snap::rhi::Fence>& getFence() const {
        return fence;
    }

protected:
    std::shared_ptr<snap::rhi::Fence> fence = nullptr;
};
} // namespace snap::rhi::backend::common
