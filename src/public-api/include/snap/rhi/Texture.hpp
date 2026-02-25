//
//  Texture.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 21.09.2020.
//  Copyright © 2021 Snapchat. All rights reserved.
//

#pragma once

#include <cstdint>

#include "snap/rhi/DeviceChild.hpp"
#include "snap/rhi/TextureCreateInfo.h"
#include "snap/rhi/TextureViewCreateInfo.h"

namespace snap::rhi {
class Device;
class TextureInterop;

/**
 * @brief GPU texture resource.
 *
 * A Texture represents an image resource that can be sampled, rendered to, or used for storage operations depending on
 * its usage flags.
 *
 * Textures are typically created via `snap::rhi::Device::createTexture()`.
 *
 * Interop:
 * - A texture may be backed by a platform or external API object via `snap::rhi::TextureInterop`.
 * - When created from `TextureInterop`, the create info is derived from the interop object.
 *
 * Memory usage:
 * - `getGPUMemoryUsage()` returns an estimate derived from the create info.
 * - Backends may allocate additional metadata or compression/tiling resources; the value is approximate.
 */
class Texture : public snap::rhi::DeviceChild {
public:
    Texture(Device* device, const TextureCreateInfo& info);
    Texture(Device* device, const TextureCreateInfo& info, const TextureViewCreateInfo& view);
    Texture(Device* device, const std::shared_ptr<snap::rhi::TextureInterop>& textureInterop);
    ~Texture() override = default;

    /**
     * @brief Returns the immutable creation parameters used for this texture.
     */
    [[nodiscard]] const TextureCreateInfo& getCreateInfo() const {
        return info;
    }

    /**
     * @brief Returns the immutable view creation parameters used for this texture, if any.
     */
    [[nodiscard]] const TextureViewCreateInfo& getViewCreateInfo() const {
        return viewInfo;
    }

    /**
     * @brief Returns the interop object backing this texture, if any.
     *
     * @note When non-null, the interop object participates in resource lifetime management. Synchronization between
     * external producers/consumers and SnapRHI remains the caller's responsibility.
     */
    [[nodiscard]] const std::shared_ptr<snap::rhi::TextureInterop>& getTextureInterop() const {
        return textureInterop;
    }

    /**
     * @brief Returns an estimate of CPU-side memory used by this object.
     */
    [[nodiscard]] uint64_t getCPUMemoryUsage() const override {
        return 0;
    }

    /**
     * @brief Returns an estimate of GPU-side memory used by this texture.
     */
    [[nodiscard]] uint64_t getGPUMemoryUsage() const override;

protected:
    TextureCreateInfo info{};
    TextureViewCreateInfo viewInfo{};
    std::shared_ptr<TextureInterop> textureInterop = nullptr;

private:
};
} // namespace snap::rhi
