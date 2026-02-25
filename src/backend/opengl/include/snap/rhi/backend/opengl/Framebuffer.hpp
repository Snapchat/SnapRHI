//
//  Framebuffer.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 21.10.2021.
//

#pragma once

#include <span>

#include "snap/rhi/Framebuffer.hpp"
#include "snap/rhi/backend/opengl/OpenGL.h"

#include <array>
#include <span>

namespace snap::rhi {
class Texture;
} // namespace snap::rhi

namespace snap::rhi::backend::opengl {
class Device;
class Profile;

/// \brief Represents framebuffer description and retains \c Texture attachments.
/// \details In OpenGL backends, framebuffers do not contain actual GL framebuffers.
///          The reason for such decision is that framebuffer object names and the corresponding
///          framebuffer object contents are local to the shared object space of the current
///          GL rendering context. Each GL rendering context is \c SnapRHI is represented
///          by \c DeviceContext. Each \c DeviceContext instance contains \c FramebufferPool
///          that is populated and used during command execution in \c CommandRecorder.
/// \sa FramebufferPool, CommandRecorder, DeviceContext, Texture.
/// https://www.khronos.org/opengl/wiki/Framebuffer
/// https://arm-software.github.io/opengl-es-sdk-for-android/multiview.html
class Framebuffer final : public snap::rhi::Framebuffer {
public:
    explicit Framebuffer(Device* glesDevice, const snap::rhi::FramebufferCreateInfo& info);
    ~Framebuffer() override = default;

private:
};
} // namespace snap::rhi::backend::opengl
