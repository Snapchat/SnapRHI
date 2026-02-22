#pragma once

#include "snap/rhi/ShaderModule.hpp"
#include "snap/rhi/backend/common/ResourceResidencySet.h"
#include "snap/rhi/backend/vulkan/Utils.hpp"
#include "snap/rhi/backend/vulkan/vulkan.h"

namespace snap::rhi::backend::vulkan {
class Device;

class ShaderModule final : public snap::rhi::ShaderModule {
public:
    explicit ShaderModule(Device* vkDevice, const snap::rhi::ShaderModuleCreateInfo& info);
    ~ShaderModule() final = default;

    const VkPipelineShaderStageCreateInfo& getShaderStage() const {
        return shaderStage;
    }

    const EntryPointReflection* getEntryPointReflection() const;

private:
    VkPipelineShaderStageCreateInfo shaderStage{};
    std::vector<VkSpecializationMapEntry> entries;
    std::vector<uint8_t> data;
    VkSpecializationInfo specializationInfo;
    std::string entryName;

    snap::rhi::backend::common::ResourceResidencySet resourceResidencySet;
};
} // namespace snap::rhi::backend::vulkan
