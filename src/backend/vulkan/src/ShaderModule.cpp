#include "snap/rhi/backend/vulkan/ShaderModule.h"

#include "snap/rhi/backend/vulkan/Device.h"
#include "snap/rhi/backend/vulkan/ShaderLibrary.h"
#include "snap/rhi/backend/vulkan/Utils.hpp"
#include "snap/rhi/common/Throw.h"
#include <algorithm>
#include <ranges>

namespace {
VkSpecializationInfo buildSpecializationInfo(const snap::rhi::ShaderModuleCreateInfo& shaderModuleCreateInfo,
                                             std::vector<VkSpecializationMapEntry>& entries,
                                             std::vector<uint8_t>& data) {
    const auto& specializationInfo = shaderModuleCreateInfo.specializationInfo;
    data.insert(data.end(),
                static_cast<const uint8_t*>(specializationInfo.pData),
                static_cast<const uint8_t*>(specializationInfo.pData) + specializationInfo.dataSize);

    entries.resize(specializationInfo.mapEntryCount);
    for (size_t i = 0; i < specializationInfo.mapEntryCount; ++i) {
        entries[i] = VkSpecializationMapEntry{.constantID = specializationInfo.pMapEntries[i].constantID,
                                              .offset = specializationInfo.pMapEntries[i].offset,
                                              .size = static_cast<size_t>(snap::rhi::backend::vulkan::getDataSize(
                                                  specializationInfo.pMapEntries[i].format))};
    }

    return VkSpecializationInfo{.mapEntryCount = static_cast<uint32_t>(entries.size()),
                                .pMapEntries = entries.data(),
                                .dataSize = data.size(),
                                .pData = data.data()};
}
} // unnamed namespace

namespace snap::rhi::backend::vulkan {
ShaderModule::ShaderModule(Device* vkDevice, const snap::rhi::ShaderModuleCreateInfo& info)
    : snap::rhi::ShaderModule(vkDevice, info),
      specializationInfo(buildSpecializationInfo(info, entries, data)),
      entryName(info.name),
      resourceResidencySet(vkDevice,
                           snap::rhi::backend::common::ResourceResidencySet::RetentionMode::RetainReferences) {
    auto shaderLibrary =
        snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::ShaderLibrary>(info.shaderLibrary);
    resourceResidencySet.track(shaderLibrary);

    shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStage.pNext = nullptr;

    /**
     * https://registry.khronos.org/vulkan/specs/latest/man/html/VkPipelineShaderStageCreateFlagBits.html
     * **/
    shaderStage.flags = 0;

    shaderStage.stage = getShaderStageFlags(info.shaderStage);
    shaderStage.module = shaderLibrary->getShaderModule();
    shaderStage.pName = entryName.data();
    shaderStage.pSpecializationInfo = &specializationInfo;

    if (const auto* reflection = getEntryPointReflection(); reflection) {
        shaderReflection = snap::rhi::reflection::ShaderModuleInfo{.vertexAttributes = reflection->vertexAttributes};
    }
}

const EntryPointReflection* ShaderModule::getEntryPointReflection() const {
    auto shaderLibrary =
        snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::ShaderLibrary>(info.shaderLibrary);
    const auto& entryPointReflections = shaderLibrary->getEntryPointReflections();
    auto itr = std::ranges::find(
        entryPointReflections, entryName, [](const auto& entry) { return entry.entryPointInfo.name; });

    auto* vkDevice = snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::Device>(device);
    const auto& validationLayer = vkDevice->getValidationLayer();
    SNAP_RHI_VALIDATE(validationLayer,
                      itr != entryPointReflections.end(),
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::CreateOp,
                      "[Vulkan::ShaderModule] Failed to find entry point reflection for entry point: " +
                          std::string(entryName));

    return &(*itr);
}
} // namespace snap::rhi::backend::vulkan
