#pragma once

#include <Metal/Metal.h>
#include <snap/rhi/backend/common/TextureViewCache.h>

namespace snap::rhi::backend::metal {
class Device;

struct TextureWithResourceID {
    id<MTLTexture> texture = nil;
    uint64_t resourceID = 0;
};

/**
 * @brief Per-texture cache for Metal texture views.
 *
 * Creating `MTLTexture` views can be relatively expensive and is often repeated (e.g. binding storage textures).
 * This cache is intended to be owned by the underlying metal texture and shared between all
 * `snap::rhi::backend::metal::Texture` wrappers that reference the same `id<MTLTexture>`.
 */
class TextureViewCache final : public common::TextureViewCache<id<MTLTexture>, TextureWithResourceID> {
public:
    TextureViewCache(Device* mtlDevice, const id<MTLTexture>& texture);
    ~TextureViewCache() override;

private:
    TextureWithResourceID createView(const snap::rhi::TextureViewInfo& key) const override;
};
} // namespace snap::rhi::backend::metal
