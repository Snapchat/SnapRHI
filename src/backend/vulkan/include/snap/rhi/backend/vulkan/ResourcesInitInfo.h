#pragma once

#include "snap/rhi/backend/vulkan/vulkan.h"
#include <vector>

namespace snap::rhi::backend::vulkan {
class Texture;

/**
 * Some resources in Vulkan SnapRHI require their states to be initialized.
 *
 * However, this cannot be done efficiently by creating and sending a command buffer during resource creation,
 * as SnapRHI would then either have to wait for the command buffer to complete,
 * or would have to maintain a reference to the command buffer and only release it when it completes.
 *
 * SnapRHI textures require changing their layout
 * from Undefined to a layout that is used as the entry point for each command buffer.
 * */
struct ResourcesInitInfo {
    std::vector<snap::rhi::backend::vulkan::Texture*> textures;

    void clear() {
        textures.clear();
    }

    bool empty() {
        return textures.empty();
    }
};
} // namespace snap::rhi::backend::vulkan
