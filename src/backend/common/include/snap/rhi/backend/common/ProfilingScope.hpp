// Copyright © 2023 Snap, Inc. All rights reserved.

#pragma once

#include <cstddef>
#include <cstdint>

#include "snap/rhi/Enums.h"
#include "snap/rhi/common/NonCopyable.h"
#include <string>

namespace snap::rhi {
class Device;
} // namespace snap::rhi

#if SNAP_RHI_ENABLE_CUSTOM_PROFILING_LABELS
namespace snap::rhi::backend::common {
class ProfilingScope final : public snap::rhi::common::NonCopyable {
public:
    ProfilingScope(snap::rhi::Device* device, std::string_view label);
    ~ProfilingScope();

private:
    snap::rhi::Device* device = nullptr;
    std::string label;
};
} // namespace snap::rhi::backend::common
#endif
