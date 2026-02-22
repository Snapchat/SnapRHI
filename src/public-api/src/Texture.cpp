#include "snap/rhi/Texture.hpp"
#include "snap/rhi/Common.h"
#include "snap/rhi/TextureInterop.h"
#include <algorithm>

namespace {
} // unnamed namespace

namespace snap::rhi {
Texture::Texture(Device* device, const TextureCreateInfo& info)
    : snap::rhi::DeviceChild(device, snap::rhi::ResourceType::Texture),
      info(info),
      viewInfo({.viewInfo = defaultTextureViewInfo(info), .texture = nullptr}) {}

Texture::Texture(Device* device, const TextureCreateInfo& info, const TextureViewCreateInfo& view)
    : snap::rhi::DeviceChild(device, snap::rhi::ResourceType::Texture), info(info), viewInfo(view) {}

Texture::Texture(Device* device, const std::shared_ptr<snap::rhi::TextureInterop>& textureInterop)
    : Texture(device, textureInterop->getTextureCreateInfo()) {
    this->textureInterop = textureInterop;
}

uint64_t Texture::getGPUMemoryUsage() const {
    uint64_t depth = info.size.depth;
    if (info.textureType == snap::rhi::TextureType::TextureCubemap) {
        depth = depth * 6; // For cubemap array
    }

    const uint64_t textureSize =
        static_cast<uint64_t>(snap::rhi::bytesPerSlice(info.size.width, info.size.height, info.format)) * depth;
    return textureSize;
}
} // namespace snap::rhi
