#include "snap/rhi/backend/vulkan/PipelineCache.h"
#include "snap/rhi/backend/vulkan/Device.h"
#include "snap/rhi/backend/vulkan/Utils.hpp"
#include "snap/rhi/backend/vulkan/VkDebugUtils.h"

#include "snap/rhi/backend/common/Logging.hpp"
#include <fstream>
#include <snap/rhi/backend/common/Utils.hpp>
#include <snap/rhi/common/Scope.h>
#include <span>

namespace {
std::vector<uint8_t> readCache(const snap::rhi::PipelineCacheCreateInfo& info) {
    if (info.cachePath.empty() || !std::filesystem::exists(info.cachePath)) {
        return {};
    }

    const size_t fileSize = std::filesystem::file_size(info.cachePath);
    std::vector<uint8_t> serializedData(fileSize, 0);

    {
        std::ifstream fin(info.cachePath, std::ios::binary);
        SNAP_RHI_ON_SCOPE_EXIT {
            fin.close();
        };

        fin.read(reinterpret_cast<char*>(serializedData.data()), fileSize);
        assert(fin);
    }

    return serializedData;
}
} // unnamed namespace

namespace snap::rhi::backend::vulkan {
PipelineCache::PipelineCache(snap::rhi::backend::vulkan::Device* device, const snap::rhi::PipelineCacheCreateInfo& info)
    : snap::rhi::PipelineCache(device, info),
      vkDevice(device->getVkLogicalDevice()),
      validationLayer(device->getValidationLayer()) {
    const auto& physicalDeviceProperties = device->getPhysicalDeviceProperties();
    auto cache = readCache(info);
    VkPipelineCacheCreateFlags flags = 0;

    if (static_cast<bool>(info.createFlags & snap::rhi::PipelineCacheCreateFlags::ExternallySynchronized) &&
        physicalDeviceProperties.apiVersion >= VK_API_VERSION_1_3) {
        flags |= VK_PIPELINE_CACHE_CREATE_EXTERNALLY_SYNCHRONIZED_BIT;
    }

    VkPipelineCacheCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
        .pNext = nullptr,

        /**
         * https://registry.khronos.org/vulkan/specs/latest/man/html/VkPipelineCacheCreateFlagBits.html
         * **/
        .flags = flags,

        .initialDataSize = cache.size(),
        .pInitialData = cache.data()};

    VkResult result = vkCreatePipelineCache(vkDevice, &createInfo, nullptr, &pipelineCache);
    SNAP_RHI_VALIDATE(validationLayer,
                      result == VK_SUCCESS,
                      snap::rhi::ReportLevel::CriticalError,
                      snap::rhi::ValidationTag::PipelineCacheOp,
                      "[Vulkan::PipelineCache] Cannot create PipelineCache");
}

PipelineCache::~PipelineCache() {
    if (pipelineCache == VK_NULL_HANDLE) {
        return;
    }

    vkDestroyPipelineCache(vkDevice, pipelineCache, nullptr);
    pipelineCache = VK_NULL_HANDLE;
}

Result PipelineCache::serializeToFile(const std::filesystem::path& cachePath) const {
    std::vector<uint8_t> serializedData{};
    {
        size_t dataSize = 0;
        VkResult result = vkGetPipelineCacheData(vkDevice, pipelineCache, &dataSize, nullptr);

        if (result != VK_SUCCESS) {
            SNAP_RHI_LOGE("Failed to get pipeline cache data size, error: %d", result);
            return Result::ErrorUnknown;
        }

        if (dataSize == 0) {
            SNAP_RHI_LOGW("Pipeline cache is empty, nothing to serialize");
            return Result::Success;
        }

        serializedData.resize(dataSize);
        vkGetPipelineCacheData(vkDevice, pipelineCache, &dataSize, serializedData.data());

        if (result != VK_SUCCESS) {
            SNAP_RHI_LOGE("Failed to get pipeline cache data, error: %d", result);
            return Result::ErrorUnknown;
        }
    }

    const bool success = snap::rhi::backend::common::writeBinToFile(
        cachePath,
        std::span<const std::byte>(reinterpret_cast<const std::byte*>(serializedData.data()), serializedData.size()));
    return success ? Result::Success : Result::ErrorUnknown;
}

void PipelineCache::setDebugLabel(std::string_view label) {
#if SNAP_RHI_ENABLE_DEBUG_LABELS
    ObjectDebugMarkers::setDebugLabel(label);
    setVkObjectDebugName(
        vkDevice, VK_OBJECT_TYPE_PIPELINE_CACHE, reinterpret_cast<uint64_t>(pipelineCache), label.data());
#endif
}
} // namespace snap::rhi::backend::vulkan
