#include "snap/rhi/backend/vulkan/ShaderLibrary.h"

#include "snap/rhi/backend/vulkan/Device.h"
#include "snap/rhi/backend/vulkan/Utils.hpp"
#include "snap/rhi/backend/vulkan/VkDebugUtils.h"
#include "snap/rhi/reflection/ShaderModuleInfo.h"

#include <algorithm>
#include <ranges>
#include <snap/rhi/common/Scope.h>
#include <spirv_reflect.h>

namespace snap::rhi::backend::vulkan {
ShaderLibrary::ShaderLibrary(Device* vkDevice, const snap::rhi::ShaderLibraryCreateInfo& info)
    : snap::rhi::ShaderLibrary(vkDevice, info), device(vkDevice->getVkLogicalDevice()) {
    const auto& validationLayer = vkDevice->getValidationLayer();

    SNAP_RHI_VALIDATE(
        validationLayer,
        info.libCompileFlag == snap::rhi::ShaderLibraryCreateFlag::CompileFromBinary,
        snap::rhi::ReportLevel::CriticalError,
        snap::rhi::ValidationTag::CreateOp,
        "[Vulkan::ShaderLibrary] Vulkan can't compile ShaderLibrary from source, only SPIR-V binary supported!");

    std::span<const uint32_t> codeSpan{reinterpret_cast<const uint32_t*>(info.code.data()),
                                       static_cast<uint32_t>(info.code.size()) / sizeof(uint32_t)};
    {
        const VkShaderModuleCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pNext = nullptr,
            /**
             * https://registry.khronos.org/vulkan/specs/latest/man/html/VkShaderModuleCreateFlags.html
             * **/
            .flags = 0,
            // Must be the total size in bytes (i.e., WordCount * 4).
            .codeSize = static_cast<uint32_t>(codeSpan.size()) * sizeof(uint32_t),
            .pCode = codeSpan.data(),
        };

        VkResult result = vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);
        SNAP_RHI_VALIDATE(validationLayer,
                          result == VK_SUCCESS,
                          snap::rhi::ReportLevel::CriticalError,
                          snap::rhi::ValidationTag::CreateOp,
                          "[Vulkan::ShaderLibrary] Cannot create ShaderLibrary");
    }

    { // build reflection info
        SpvReflectShaderModule spvModule{};
        SpvReflectResult spvResult = spvReflectCreateShaderModule(info.code.size(), info.code.data(), &spvModule);
        SNAP_RHI_VALIDATE(validationLayer,
                          spvResult == SPV_REFLECT_RESULT_SUCCESS,
                          snap::rhi::ReportLevel::Warning,
                          snap::rhi::ValidationTag::CreateOp,
                          "[Vulkan::ShaderLibrary] SPIRV-Reflect failed to parse SPIR-V code");
        if (spvResult != SPV_REFLECT_RESULT_SUCCESS) {
            return;
        }

        SNAP_RHI_ON_SCOPE_EXIT {
            spvReflectDestroyShaderModule(&spvModule);
        };

        entryPointReflections = buildEntryPointReflection(codeSpan, spvModule);
        shaderLibraryInfo = std::make_optional<snap::rhi::reflection::ShaderLibraryInfo>();
        std::ranges::copy(entryPointReflections |
                              std::views::transform([](const auto& info) { return info.entryPointInfo; }),
                          std::back_inserter(shaderLibraryInfo->entryPoints));
    }
}

ShaderLibrary::~ShaderLibrary() {
    if (shaderModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(device, shaderModule, nullptr);
    }

    shaderModule = VK_NULL_HANDLE;
}

void ShaderLibrary::setDebugLabel(std::string_view label) {
#if SNAP_RHI_ENABLE_DEBUG_LABELS
    ObjectDebugMarkers::setDebugLabel(label);
    setVkObjectDebugName(device, VK_OBJECT_TYPE_SHADER_MODULE, reinterpret_cast<uint64_t>(shaderModule), label.data());
#endif
}
} // namespace snap::rhi::backend::vulkan
