#include "snap/rhi/backend/metal/Semaphore.h"
#include "snap/rhi/backend/metal/Device.h"

#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/common/Throw.h"

namespace snap::rhi::backend::metal {
Semaphore::Semaphore(Device* mtlDevice, const snap::rhi::SemaphoreCreateInfo& info)
    : snap::rhi::Semaphore(mtlDevice, info) {
    // Use a timeline primitive (MTLSharedEvent) to implement binary semaphore semantics robustly.
    // Each signal uses a new value; waits target that value and we enforce a single pending signal.
    if (@available(macOS 10.14, ios 12.0, tvos 12.0, *)) {
        event = [mtlDevice->getMtlDevice() newSharedEvent];
        if (event != nil) {
            event.signaledValue = 0;
        }
    }

    if (event == nil) {
        snap::rhi::common::throwException(
            "[metal::Semaphore] MTLSharedEvent is unavailable; Metal semaphore is unsupported on this OS/device");
    }
}

void Semaphore::wait(const id<MTLCommandBuffer>& commandBuffer) {
    uint64_t valueToWait = 0;
    {
        std::lock_guard lock(stateMutex);

        // Binary semaphore rule: must be signaled before it can be waited.
        auto* mtlDevice = common::smart_cast<Device>(getDevice());
        const auto& validationLayer = mtlDevice->getValidationLayer();
        SNAP_RHI_VALIDATE(validationLayer,
                          hasPendingSignal,
                          snap::rhi::ReportLevel::Error,
                          snap::rhi::ValidationTag::SemaphoreOp,
                          "[metal::Semaphore::wait] invalid wait: semaphore has no pending signal");

        valueToWait = pendingSignalValue;
        hasPendingSignal = false; // consume
        pendingSignalValue = 0;
    }

    if (@available(macOS 10.14, ios 12.0, tvos 12.0, *)) {
        [commandBuffer encodeWaitForEvent:event value:valueToWait];
        return;
    }

    snap::rhi::common::throwException("[metal::Semaphore::wait] MTLSharedEvent encodeWait is unavailable");
}

void Semaphore::signal(const id<MTLCommandBuffer>& commandBuffer) {
    uint64_t valueToSignal = 0;
    {
        std::lock_guard lock(stateMutex);

        // Binary semaphore rule: can't signal twice without an intervening wait.
        auto* mtlDevice = common::smart_cast<Device>(getDevice());
        const auto& validationLayer = mtlDevice->getValidationLayer();
        SNAP_RHI_VALIDATE(validationLayer,
                          !hasPendingSignal,
                          snap::rhi::ReportLevel::Error,
                          snap::rhi::ValidationTag::SemaphoreOp,
                          "[metal::Semaphore::signal] invalid signal: semaphore already has a pending signal");

        valueToSignal = nextSignalValue++;
        pendingSignalValue = valueToSignal;
        hasPendingSignal = true;
    }

    if (@available(macOS 10.14, ios 12.0, tvos 12.0, *)) {
        [commandBuffer encodeSignalEvent:event value:valueToSignal];
        return;
    }

    snap::rhi::common::throwException("[metal::Semaphore::signal] MTLSharedEvent encodeSignal is unavailable");
}

void Semaphore::setDebugLabel(std::string_view label) {
#if SNAP_RHI_ENABLE_DEBUG_LABELS
    ObjectDebugMarkers::setDebugLabel(label);
    event.label = [NSString stringWithUTF8String:label.data()];
#endif
}
} // namespace snap::rhi::backend::metal
