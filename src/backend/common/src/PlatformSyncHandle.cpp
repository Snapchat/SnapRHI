#include "snap/rhi/backend/common/PlatformSyncHandle.h"

namespace snap::rhi::backend::common {
PlatformSyncHandle::PlatformSyncHandle(const std::shared_ptr<snap::rhi::Fence>& fence) : fence(fence) {}
} // namespace snap::rhi::backend::common
