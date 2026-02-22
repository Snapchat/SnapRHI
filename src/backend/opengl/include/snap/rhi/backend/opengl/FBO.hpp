// Copyright © 2023 Snap, Inc. All rights reserved.

#pragma once

#include "snap/rhi/backend/opengl/FramebufferDescription.h"
#include "snap/rhi/backend/opengl/FramebufferStatus.h"
#include "snap/rhi/backend/opengl/FramebufferTarget.h"

namespace snap::rhi::backend::opengl {
class DeviceContext;
class Profile;

/// \brief Serves as a base class for exceptions raised from \c FramebufferException and related classes.
struct FramebufferException : public snap::rhi::Exception {
    using Exception::Exception;
};

/// \brief Raised when \c SnapRHI hits incomplete framebuffer check status.
/// \details Contains \c FramebufferStatus for error handling and logging purposes.
struct FramebufferIncompleteException : FramebufferException {
    const FramebufferStatus status = FramebufferStatus::Complete;
    FramebufferIncompleteException(FramebufferStatus status);
    FramebufferIncompleteException(FramebufferStatus status, const std::string& desc);
};

bool isDefaultBackBuffer(const FramebufferDescription& description);

class FBO final {
public:
    explicit FBO(DeviceContext* dc);
    ~FBO();

    FBO(FBO&&);
    FBO& operator=(FBO&&) = delete;

    FBO(const FBO&) = delete;
    FBO& operator=(const FBO&) = delete;

    FramebufferId assignAndBind(FramebufferTarget target, const FramebufferDescription& description);
    FramebufferId bind(FramebufferTarget target);

private:
    void bindAttachments(FramebufferTarget target, const FramebufferDescription& description);
    void setFramebufferDescription(FramebufferTarget target, const FramebufferDescription& description);

    void tryInitialize();

    DeviceContext* dc = nullptr;
    Profile& gl;

    FramebufferId fbo = FramebufferId::CurrSurfaceBackbuffer;
    FramebufferDescription activeDescription{};
};
} // namespace snap::rhi::backend::opengl
