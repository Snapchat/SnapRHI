//
//  Framebuffer.h
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 1/21/20.
//  Copyright © 2020 Snapchat. All rights reserved.
//

#pragma once

#include "snap/rhi/ClearValue.h"
#include "snap/rhi/Framebuffer.hpp"
#include <Metal/Metal.h>
#include <array>
#include <span>

namespace snap::rhi {
class Texture;
} // namespace snap::rhi

namespace snap::rhi::backend::metal {
class Device;
class Texture;

class Framebuffer final : public snap::rhi::Framebuffer {
public:
    explicit Framebuffer(Device* mtlDevice, const snap::rhi::FramebufferCreateInfo& info);
    ~Framebuffer() override = default;

    bool hasDepthStencilAttachment() const;

private:
    bool doesFramebufferHasDepthStencilAttachment = false;
};
} // namespace snap::rhi::backend::metal
