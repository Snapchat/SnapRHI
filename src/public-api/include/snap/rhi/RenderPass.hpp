#pragma once

#include "snap/rhi/DeviceChild.hpp"
#include "snap/rhi/RenderPassCreateInfo.h"

#include <cstdint>
#include <span>

namespace snap::rhi {
class Device;

/**
 * @brief Immutable description of render pass state shared by framebuffers and render command encoders.
 *
 * A RenderPass defines how render targets are used within one or more subpasses: attachment formats, load/store
 * operations, and subpass structure.
 *
 * Backends may represent this object differently:
 * - Vulkan typically creates a native render pass object.
 * - Other backends may treat it as a lightweight description used during command encoding.
 */
class RenderPass : public snap::rhi::DeviceChild {
public:
    RenderPass(Device* device, const RenderPassCreateInfo& info);
    ~RenderPass() override = default;

    /**
     * @brief Returns the creation parameters used to create this render pass.
     */
    const RenderPassCreateInfo& getCreateInfo() const noexcept {
        return info;
    }

    /**
     * @brief Returns an estimate of CPU-side memory used by this object.
     */
    uint64_t getCPUMemoryUsage() const override {
        return sizeof(RenderPassCreateInfo);
    }

    /**
     * @brief Returns an estimate of GPU-side memory used by this object.
     */
    uint64_t getGPUMemoryUsage() const override {
        return 0;
    }

protected:
    RenderPassCreateInfo info{};

private:
};
} // namespace snap::rhi
