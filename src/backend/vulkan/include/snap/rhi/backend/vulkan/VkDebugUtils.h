#pragma once

#include "snap/rhi/backend/vulkan/vulkan.h"

#include <cstdint>
#include <string_view>

namespace snap::rhi::backend::vulkan {

/**
 * @brief Labels a Vulkan object so it appears by name in GPU debuggers (RenderDoc, Nsight, etc.).
 *
 * Uses `vkSetDebugUtilsObjectNameEXT` from `VK_EXT_debug_utils`. The call is a no-op when the
 * extension is not loaded (the function pointer resolves to `nullptr` via volk/meta-loader).
 *
 * @param vkDevice    Logical device that owns the object.
 * @param objectType  Vulkan object type enum (e.g., `VK_OBJECT_TYPE_BUFFER`).
 * @param objectHandle  Raw `uint64_t` handle — cast from the Vulkan handle type.
 * @param name        UTF-8 null-terminated label text.
 */
inline void setVkObjectDebugName(VkDevice vkDevice, VkObjectType objectType, uint64_t objectHandle, const char* name) {
    if (!vkSetDebugUtilsObjectNameEXT) {
        return;
    }

    VkDebugUtilsObjectNameInfoEXT nameInfo{};
    nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    nameInfo.objectType = objectType;
    nameInfo.objectHandle = objectHandle;
    nameInfo.pObjectName = name;

    vkSetDebugUtilsObjectNameEXT(vkDevice, &nameInfo);
}

/**
 * @brief Begins a debug label region on a command buffer.
 *
 * Uses `vkCmdBeginDebugUtilsLabelEXT` from `VK_EXT_debug_utils`.
 */
inline void beginVkDebugLabel(VkCommandBuffer commandBuffer, const char* label) {
    if (!vkCmdBeginDebugUtilsLabelEXT) {
        return;
    }

    VkDebugUtilsLabelEXT labelInfo{};
    labelInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
    labelInfo.pLabelName = label;
    labelInfo.color[0] = labelInfo.color[1] = labelInfo.color[2] = labelInfo.color[3] = 1.0f;

    vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &labelInfo);
}

/**
 * @brief Ends the most recent debug label region on a command buffer.
 */
inline void endVkDebugLabel(VkCommandBuffer commandBuffer) {
    if (!vkCmdEndDebugUtilsLabelEXT) {
        return;
    }
    vkCmdEndDebugUtilsLabelEXT(commandBuffer);
}

} // namespace snap::rhi::backend::vulkan
