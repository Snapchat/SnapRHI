#include "snap/rhi/backend/common/FencePool.h"
#include "snap/rhi/backend/common/Device.hpp"

namespace snap::rhi::backend::common {
FencePool::FencePool(FenceCreateFunc fenceCreateFunc, const ValidationLayer& validationLayer)
    : fenceCreateFunc(std::move(fenceCreateFunc)), validationLayer(validationLayer) {}

FencePool::~FencePool() {
    clear();
}

std::shared_ptr<Fence> FencePool::acquireFence() {
    std::lock_guard lock(accessMutex);

    int32_t idx = -1;
    for (size_t i = 0; i < fences.size(); ++i) {
        if (fences[i].use_count() == 1) {
            idx = static_cast<int32_t>(i);
            break;
        }
    }

    if (idx < 0) {
        idx = static_cast<int32_t>(fences.size());

        static snap::rhi::FenceCreateInfo fenceCreateInfo{};
        fences.emplace_back(fenceCreateFunc(fenceCreateInfo));
    }

    fences[idx]->reset();
    return fences[idx];
}

void FencePool::clear() {
    std::lock_guard lock(accessMutex);
    fences.clear();
}
} // namespace snap::rhi::backend::common
