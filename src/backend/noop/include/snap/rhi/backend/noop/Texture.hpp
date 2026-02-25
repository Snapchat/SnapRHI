#pragma once

#include "snap/rhi/Device.hpp"
#include "snap/rhi/Texture.hpp"

namespace snap::rhi::backend::common {
class Device;
} // namespace snap::rhi::backend::common

namespace snap::rhi::backend::noop {
class Texture final : public snap::rhi::Texture {
public:
    explicit Texture(snap::rhi::backend::common::Device* device, const TextureCreateInfo& info);
    explicit Texture(snap::rhi::backend::common::Device* device, const snap::rhi::TextureViewCreateInfo& info);
};
} // namespace snap::rhi::backend::noop
