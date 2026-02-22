// Copyright © 2023 Snap, Inc. All rights reserved.

#pragma once

#include "snap/rhi/backend/common/ValidationLayer.hpp"
#include "snap/rhi/backend/opengl/Profile.hpp"
#include "snap/rhi/common/NonCopyable.h"

namespace snap::rhi::backend::opengl {
class Device;
class DeviceContext;

class CmdPerformer : public snap::rhi::common::NonCopyable {
public:
    CmdPerformer(snap::rhi::backend::opengl::Device* device);
    ~CmdPerformer() = default;

    void enableCache(DeviceContext* dc);
    void disableCache();

protected:
    snap::rhi::backend::opengl::Device* device = nullptr;
    snap::rhi::backend::opengl::Profile& gl;
    const snap::rhi::backend::common::ValidationLayer& validationLayer;

    DeviceContext* dc = nullptr;
};
} // namespace snap::rhi::backend::opengl
