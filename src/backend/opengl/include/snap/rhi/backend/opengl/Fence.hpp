//
//  Fence.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 12.10.2021.
//

#pragma once

#include "snap/rhi/Fence.hpp"
#include "snap/rhi/backend/opengl/OpenGL.h"

namespace snap::rhi::backend::opengl {
class Profile;
class Device;

class Fence final : public snap::rhi::Fence {
public:
    explicit Fence(Device* device, const snap::rhi::FenceCreateInfo& info);
    ~Fence() override;

    void setDebugLabel(std::string_view label) override;

    /**
     * Don't forget glFlush() https://www.khronos.org/opengl/wiki/Sync_Object
     */
    void init();

    std::unique_ptr<snap::rhi::PlatformSyncHandle> exportPlatformSyncHandle() override;

    snap::rhi::FenceStatus getStatus(uint64_t generationID) override;
    void waitForComplete() override;
    void waitForScheduled() override;
    void reset() override;

private:
    const Profile& gl;
    SNAP_RHI_GLsync fence = nullptr;
    FenceStatus status = snap::rhi::FenceStatus::NotReady;
};
} // namespace snap::rhi::backend::opengl
