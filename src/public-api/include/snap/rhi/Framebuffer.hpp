//
//  Framebuffer.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 21.09.2020.
//  Copyright © 2021 Snapchat. All rights reserved.
//

#pragma once

#include "snap/rhi/DeviceChild.hpp"
#include "snap/rhi/FramebufferCreateInfo.h"
#include "snap/rhi/RenderPass.hpp"

#include <cstdint>
#include <span>

namespace snap::rhi {
class Device;
class Texture;

/**
 * @brief Framebuffer object describing a set of render pass attachments.
 *
 * A framebuffer binds a @ref snap::rhi::RenderPass to a concrete set of attachment textures (color/depth-stencil) and
 * specifies the render area dimensions (width/height/layers).
 *
 * This is an API-agnostic object:
 * - Vulkan typically creates a native `VkFramebuffer` from the provided attachment views.
 * - Metal and OpenGL mostly treat this as a lightweight description used when encoding a render pass.
 * - WebGPU uses it to build a `wgpu::RenderPassDescriptor` with attachment views.
 *
 * @note The textures referenced by this framebuffer are *not* owned by the framebuffer. The application must keep
 * them alive while the framebuffer is in use (until GPU work that references it has completed).
 */
class Framebuffer : public snap::rhi::DeviceChild {
public:
    Framebuffer(Device* device, const FramebufferCreateInfo& info);
    ~Framebuffer() override = default;

    /**
     * @brief Returns the immutable create info used to build this framebuffer.
     */
    const FramebufferCreateInfo& getCreateInfo() const noexcept {
        return info;
    }

    /**
     * @brief Returns an estimate of CPU memory usage for this object.
     */
    uint64_t getCPUMemoryUsage() const override {
        return 0;
    }

    /**
     * @brief Returns an estimate of GPU memory usage for this object.
     */
    uint64_t getGPUMemoryUsage() const override {
        return 0;
    }

protected:
    FramebufferCreateInfo info{};
};
} // namespace snap::rhi
