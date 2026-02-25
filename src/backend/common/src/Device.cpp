#include "snap/rhi/backend/common/Device.hpp"
#include "snap/rhi/backend/common/Utils.hpp"

#include "snap/rhi/Buffer.hpp"
#include "snap/rhi/CommandBuffer.hpp"
#include "snap/rhi/CommandQueue.hpp"
#include "snap/rhi/ComputePipeline.hpp"
#include "snap/rhi/DescriptorPool.hpp"
#include "snap/rhi/DescriptorSet.hpp"
#include "snap/rhi/DescriptorSetLayout.hpp"
#include "snap/rhi/DeviceContext.hpp"
#include "snap/rhi/Fence.hpp"
#include "snap/rhi/Framebuffer.hpp"
#include "snap/rhi/PipelineCache.hpp"
#include "snap/rhi/PipelineLayout.hpp"
#include "snap/rhi/RenderPass.hpp"
#include "snap/rhi/RenderPipeline.hpp"
#include "snap/rhi/Sampler.hpp"
#include "snap/rhi/Semaphore.hpp"
#include "snap/rhi/ShaderLibrary.hpp"
#include "snap/rhi/ShaderModule.hpp"
#include "snap/rhi/Texture.hpp"

#include <unordered_map>

namespace {
void appendDomainEntry(snap::rhi::MemoryDomainUsage& domain,
                       std::unordered_map<snap::rhi::ResourceType, size_t>& groupIndexByType,
                       const snap::rhi::ResourceType type,
                       const uint64_t sizeInBytes,
                       const std::weak_ptr<snap::rhi::DeviceChild>& weakResource) {
    domain.totalSizeInBytes += sizeInBytes;
    domain.totalResourceCount += 1;

    size_t idx = 0;
    if (auto it = groupIndexByType.find(type); it != groupIndexByType.end()) {
        idx = it->second;
    } else {
        idx = domain.groups.size();
        groupIndexByType.emplace(type, idx);
        domain.groups.push_back(snap::rhi::ResourceTypeGroup{.type = type});
    }

    auto& group = domain.groups[idx];
    group.totalSizeInBytes += sizeInBytes;
    group.entries.push_back(snap::rhi::ResourceMemoryEntry{.sizeInBytes = sizeInBytes, .resource = weakResource});
}
} // namespace

namespace snap::rhi::backend::common {
Device::Device(const DeviceCreateInfo& info)
    : snap::rhi::Device(info), validationLayer(info.enabledTags, info.enabledReportLevel) {}

Device::~Device() noexcept(false) {
    SNAP_RHI_REPORT(validationLayer,
                    snap::rhi::ReportLevel::Info,
                    snap::rhi::ValidationTag::DestroyOp,
                    "[snap::rhi::backend::common::Device::destroy] start");

    {
        SNAP_RHI_VALIDATE(validationLayer,
                          deviceResourceRegistry.empty(),
                          snap::rhi::ReportLevel::CriticalError,
                          snap::rhi::ValidationTag::DestroyOp,
                          "[snap::rhi::backend::common::~Device] @deviceResourceRegistry is not empty! All Snap "
                          "Graphics resources must "
                          "be destroyed before destroying snap::rhi::Device.");
    }
    SNAP_RHI_REPORT(validationLayer,
                    snap::rhi::ReportLevel::Info,
                    snap::rhi::ValidationTag::DestroyOp,
                    "[snap::rhi::backend::common::Device::destroy] end");
}

snap::rhi::CommandQueue* Device::getCommandQueue(const uint32_t queueFamilyIndex, const uint32_t queueIndex) {
    SNAP_RHI_VALIDATE(validationLayer,
                      (queueFamilyIndex < commandQueues.size()) &&
                          (queueIndex < commandQueues[queueFamilyIndex].size()),
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::CommandQueueOp,
                      "[common::Device::getCommandQueue] queueFamilyIndex{%d}, queueIndex{%d}",
                      queueFamilyIndex,
                      queueIndex);

    return commandQueues[queueFamilyIndex][queueIndex].get();
}

void Device::waitForTrackedSubmissions() {
    if (submissionTracker) {
        submissionTracker->reclaim();
    }
}

void Device::validateTextureCreation(const snap::rhi::TextureCreateInfo& info) {
    const auto formatFeatures = caps.formatProperties[static_cast<uint32_t>(info.format)].textureFeatures;

    const bool isFormatSupported = formatFeatures != snap::rhi::FormatFeatures::None;
    SNAP_RHI_VALIDATE(validationLayer,
                      isFormatSupported,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::TextureOp,
                      "[Device::validateTextureCreation] unsupported texture format: %d\n",
                      static_cast<uint32_t>(info.format));
}

DeviceMemorySnapshot Device::captureMemorySnapshot() const {
    DeviceMemorySnapshot snapshot{};

    std::unordered_map<snap::rhi::ResourceType, size_t> cpuGroupIndexByType;
    std::unordered_map<snap::rhi::ResourceType, size_t> gpuGroupIndexByType;

    deviceResourceRegistry.for_each([&](const std::weak_ptr<snap::rhi::DeviceChild>& weakResource) {
        const auto resource = weakResource.lock();
        if (!resource) {
            return;
        }

        const snap::rhi::ResourceType type = resource->getResourceType();
        const uint64_t cpuBytes = resource->getCPUMemoryUsage();
        const uint64_t gpuBytes = resource->getGPUMemoryUsage();

        appendDomainEntry(snapshot.cpu, cpuGroupIndexByType, type, cpuBytes, weakResource);
        appendDomainEntry(snapshot.gpu, gpuGroupIndexByType, type, gpuBytes, weakResource);
    });

    snapshot.gpu.totalSizeInBytes = std::max(snapshot.gpu.totalSizeInBytes, getGPUMemoryUsage());
    snapshot.cpu.totalSizeInBytes = std::max(snapshot.cpu.totalSizeInBytes, getCPUMemoryUsage());

    return snapshot;
}
} // namespace snap::rhi::backend::common
