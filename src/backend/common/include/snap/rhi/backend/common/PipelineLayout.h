#pragma once

#include <optional>
#include <vector>

#include "snap/rhi/DescriptorSetLayoutCreateInfo.h"
#include "snap/rhi/PipelineLayout.hpp"

namespace snap::rhi::backend::common {
class Device;

class PipelineLayout : public snap::rhi::PipelineLayout {
    using SetLayouts = std::vector<std::optional<snap::rhi::DescriptorSetLayoutCreateInfo>>;

public:
    explicit PipelineLayout(snap::rhi::backend::common::Device* device,
                            const snap::rhi::PipelineLayoutCreateInfo& info);

    [[nodiscard]] const SetLayouts& getSetLayouts() const {
        return setLayouts;
    }

protected:
    SetLayouts setLayouts;
};
} // namespace snap::rhi::backend::common
