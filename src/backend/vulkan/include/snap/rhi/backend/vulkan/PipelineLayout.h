#pragma once

#include "snap/rhi/backend/common/PipelineLayout.h"
#include "snap/rhi/backend/vulkan/vulkan.h"

namespace snap::rhi::backend::vulkan {
class Device;

class PipelineLayout final : public snap::rhi::backend::common::PipelineLayout {
public:
    explicit PipelineLayout(snap::rhi::backend::vulkan::Device* device,
                            const snap::rhi::PipelineLayoutCreateInfo& info);
    ~PipelineLayout() override;

    void setDebugLabel(std::string_view label) override;

    VkPipelineLayout getPipelineLayout() const {
        return pipelineLayout;
    }

private:
    VkDevice vkDevice = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
};
} // namespace snap::rhi::backend::vulkan
