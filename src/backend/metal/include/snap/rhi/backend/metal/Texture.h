#pragma once

#include "snap/rhi/Texture.hpp"
#include "snap/rhi/TextureInterop.h"
#include "snap/rhi/TextureViewCreateInfo.h"
#include "snap/rhi/backend/metal/TextureViewCache.h"

#include <Metal/Metal.h>
#include <vector>

namespace snap::rhi::backend::metal {
class Device;

class Texture final : public snap::rhi::Texture {
public:
    explicit Texture(Device* mtlDevice, const snap::rhi::TextureViewCreateInfo& info);
    explicit Texture(Device* mtlDevice, const std::shared_ptr<snap::rhi::TextureInterop>& textureInterop);
    explicit Texture(Device* mtlDevice, const id<MTLTexture>& mtlTexture, const snap::rhi::TextureCreateInfo& info);
    explicit Texture(Device* mtlDevice, const snap::rhi::TextureCreateInfo& info);
    ~Texture() override = default;

    [[nodiscard]] uint64_t getGPUResourceID() const {
        return gpuResourceID;
    }

    const id<MTLTexture>& getTexture() const {
        return mtlTexture;
    }

    void setDebugLabel(std::string_view label) override;

    const std::shared_ptr<TextureViewCache>& getTextureViewCache() const {
        return textureViewCache;
    }

private:
    id<MTLTexture> mtlTexture = nil;
    uint64_t gpuResourceID = 0;

    std::shared_ptr<TextureViewCache> textureViewCache = nullptr;
};
} // namespace snap::rhi::backend::metal
