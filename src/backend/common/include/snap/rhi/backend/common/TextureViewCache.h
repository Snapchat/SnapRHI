#pragma once

#include "snap/rhi/TextureViewCreateInfo.h"
#include "snap/rhi/backend/common/Device.hpp"
#include "snap/rhi/backend/common/ValidationLayer.hpp"
#include "snap/rhi/common/HashCombine.h"
#include "snap/rhi/common/NonCopyable.h"

#include <mutex>
#include <unordered_map>

namespace snap::rhi::backend::common {
class Device;
/**
 * @brief Per-view cache.
 *
 * Image views are expensive to create and are frequently duplicated when creating multiple snap::rhi::Texture objects
 * referencing the same Texture (e.g., texture views).
 *
 * This cache is intended to be owned by the underlying texture resource and shared via std::shared_ptr
 * among all Texture wrappers referencing that texture.
 */
template<typename TextureType, typename TextureViewType>
class TextureViewCache : snap::rhi::common::NonCopyable {
public:
    TextureViewCache(Device* device, TextureType texture)
        : device(device), texture(texture), validationLayer(device->getValidationLayer()) {}
    virtual ~TextureViewCache() = default;

    const TextureType& getTexture() const {
        return texture;
    }

    TextureViewType acquire(const snap::rhi::TextureViewInfo& key) {
        std::lock_guard lock(mutex);

        if (const auto it = views.find(key); it != views.end()) {
            return it->second;
        }

        auto view = createView(key);
        views.emplace(key, view);
        return view;
    }

protected:
    virtual TextureViewType createView(const snap::rhi::TextureViewInfo& key) const = 0;

    snap::rhi::backend::common::Device* device = nullptr;
    TextureType texture;
    const snap::rhi::backend::common::ValidationLayer& validationLayer;

    std::mutex mutex;
    std::unordered_map<snap::rhi::TextureViewInfo, TextureViewType> views;
};

} // namespace snap::rhi::backend::common
