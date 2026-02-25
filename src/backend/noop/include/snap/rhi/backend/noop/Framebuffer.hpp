#pragma once

#include "snap/rhi/Framebuffer.hpp"

#include <span>

namespace snap::rhi {
class Texture;
} // namespace snap::rhi

namespace snap::rhi::backend::common {
class Device;
} // namespace snap::rhi::backend::common

namespace snap::rhi::backend::noop {
class Framebuffer final : public snap::rhi::Framebuffer {
public:
    explicit Framebuffer(snap::rhi::backend::common::Device* device, const snap::rhi::FramebufferCreateInfo& info);

    ~Framebuffer() override = default;
};
} // namespace snap::rhi::backend::noop
