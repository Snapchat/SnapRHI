#include "snap/rhi/backend/noop/Texture.hpp"

#include "snap/rhi/Device.hpp"
#include "snap/rhi/Sampler.hpp"
#include "snap/rhi/Texture.hpp"

#include "snap/rhi/backend/noop/Device.hpp"

namespace snap::rhi::backend::noop {
Texture::Texture(snap::rhi::backend::common::Device* device, const TextureCreateInfo& info)
    : snap::rhi::Texture(device, info) {}

Texture::Texture(snap::rhi::backend::common::Device* device, const snap::rhi::TextureViewCreateInfo& info)
    : snap::rhi::Texture(device, info.texture->getCreateInfo()) {}
} // namespace snap::rhi::backend::noop
