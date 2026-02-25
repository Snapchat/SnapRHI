#include "snap/rhi/backend/common/PipelineLayout.h"

#include "snap/rhi/DescriptorSetLayout.hpp"
#include "snap/rhi/backend/common/Device.hpp"

namespace snap::rhi::backend::common {
PipelineLayout::PipelineLayout(snap::rhi::backend::common::Device* device,
                               const snap::rhi::PipelineLayoutCreateInfo& info)
    : snap::rhi::PipelineLayout(device, info) {
    setLayouts.resize(info.setLayouts.size());
    for (size_t i = 0; i < setLayouts.size(); ++i) {
        if (const auto* dsLayout = info.setLayouts[i]; dsLayout) {
            setLayouts[i] = dsLayout->getCreateInfo();
        }
    }
}
} // namespace snap::rhi::backend::common
