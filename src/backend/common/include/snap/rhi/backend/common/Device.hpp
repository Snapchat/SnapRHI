#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_set>

#include "snap/rhi/common/OS.h"

#include "snap/rhi/CommandQueue.hpp"
#include "snap/rhi/Device.hpp"

#include "snap/rhi/DeviceContext.hpp"
#include "snap/rhi/backend/common/DeviceResourceRegistry.h"
#include "snap/rhi/backend/common/FencePool.h"
#include "snap/rhi/backend/common/ProfilingScope.hpp"
#include "snap/rhi/backend/common/SubmissionTracker.h"
#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/common/ValidationLayer.hpp"

#include <snap/rhi/common/Concat.h>

#include <span>

#if SNAP_RHI_ENABLE_CUSTOM_PROFILING_LABELS
#define SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, labelName)                                                     \
    [[maybe_unused]] snap::rhi::backend::common::ProfilingScope SNAP_RHI_CONCAT(rhiInternalPerfEvent,                  \
                                                                                __LINE__)(device, labelName)
#else
#define SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, labelName)
#endif

namespace snap::rhi::backend::common {
class Device : public snap::rhi::Device {
public:
    explicit Device(const snap::rhi::DeviceCreateInfo& info);
    ~Device() noexcept(false) override;

    const ValidationLayer& getValidationLayer() const {
        return validationLayer;
    }

    template<typename PublicType, typename ImplType, typename... Args>
    std::shared_ptr<PublicType> createResource(Args&&... args) {
        auto resource = std::shared_ptr<PublicType>(
            new ImplType(std::forward<Args>(args)...), [deviceBase = shared_from_this()](auto* ptr) {
                auto* device =
                    snap::rhi::backend::common::smart_cast<snap::rhi::backend::common::Device>(deviceBase.get());
                device->deviceResourceRegistry.erase(ptr);

                SNAP_RHI_VALIDATE_RESOURCE_LIFETIMES(device);

                delete ptr;
            });
        deviceResourceRegistry.insert(resource);
        return std::move(resource);
    }

    template<typename PublicType, typename ImplType, typename... Args>
    std::shared_ptr<PublicType> createResourceNoDeviceRetain(Args&&... args) {
        auto resource =
            std::shared_ptr<PublicType>(new ImplType(std::forward<Args>(args)...), [device = this](auto* ptr) {
                auto* deviceImpl = snap::rhi::backend::common::smart_cast<snap::rhi::backend::common::Device>(device);
                deviceImpl->deviceResourceRegistry.erase(ptr);

                SNAP_RHI_VALIDATE_RESOURCE_LIFETIMES(deviceImpl);

                delete ptr;
            });
        deviceResourceRegistry.insert(resource);
        return std::move(resource);
    }

    std::shared_ptr<snap::rhi::DeviceChild> resolveResource(snap::rhi::DeviceChild* resource) const {
        if (const auto weakPtr = deviceResourceRegistry.find(resource); !weakPtr.expired()) {
            return weakPtr.lock();
        }

        assert(false);
        return nullptr;
    }

    uint64_t getCPUMemoryUsage() const override {
        uint64_t totalSize = 0;
        deviceResourceRegistry.for_each([&totalSize](const std::weak_ptr<snap::rhi::DeviceChild>& resource) {
            if (const auto ptr = resource.lock(); ptr) {
                totalSize += ptr->getCPUMemoryUsage();
            }
        });

        return totalSize;
    }

    uint64_t getGPUMemoryUsage() const override {
        uint64_t totalSize = 0;
        deviceResourceRegistry.for_each([&totalSize](const std::weak_ptr<snap::rhi::DeviceChild>& resource) {
            if (const auto ptr = resource.lock(); ptr) {
                totalSize += ptr->getGPUMemoryUsage();
            }
        });

        return totalSize;
    }

    const std::unique_ptr<snap::rhi::backend::common::SubmissionTracker>& getSubmissionTrackerPtr() const {
        return submissionTracker;
    }

    snap::rhi::backend::common::SubmissionTracker& getSubmissionTracker() const {
        assert(submissionTracker);
        return *submissionTracker;
    }

    FencePool& getFencePool() const {
        assert(fencePool);
        return *fencePool;
    }

    snap::rhi::CommandQueue* getCommandQueue(const uint32_t queueFamilyIndex, const uint32_t queueIndex) override;

    DeviceMemorySnapshot captureMemorySnapshot() const final;

    void waitForTrackedSubmissions() final;

protected:
    void validateTextureCreation(const snap::rhi::TextureCreateInfo& info);

    ValidationLayer validationLayer;
    snap::rhi::backend::common::DeviceResourceRegistry deviceResourceRegistry;

    /**
     * std::unique_ptr is used to destroy and init it in derived class
     * So all resources and DeviceContext can be initialized/released properly
     */
    std::unique_ptr<snap::rhi::backend::common::SubmissionTracker> submissionTracker = nullptr;
    std::unique_ptr<snap::rhi::backend::common::FencePool> fencePool = nullptr;
    std::vector<std::vector<std::unique_ptr<snap::rhi::CommandQueue>>> commandQueues;
};
} // namespace snap::rhi::backend::common
