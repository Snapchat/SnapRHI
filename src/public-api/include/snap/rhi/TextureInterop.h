#pragma once

#include <memory>
#include <span>
#include <vector>

#include "snap/rhi/DeviceChild.hpp"
#include "snap/rhi/Enums.h"
#include "snap/rhi/TextureCreateInfo.h"
#include "snap/rhi/TextureInteropCreateInfo.h"

namespace snap::rhi {
class Device;
class PlatformSyncHandle;

/**
 * @brief Cross-API texture wrapper used for interoperability.
 *
 * A TextureInterop represents a platform/back-end specific texture object (for example, a native Metal/Vulkan/OpenGL
 * texture) that can be wrapped by SnapRHI and shared between APIs.
 *
 * Typical usage:
 * - Create a `TextureInterop` instance (backend/platform specific).
 * - Wrap it into an `snap::rhi::Texture` via `Device::createTexture(textureInterop)`.
 * - Use synchronization handles returned by interop operations to safely transition the resource between CPU and GPU
 *   and/or across different graphics APIs.
 *
 * @note CPU mapping (`map`/`unmap`) is platform/backend dependent. The default backend implementation may not support
 *       CPU-visible interop textures.
 */
class TextureInterop {
public:
    /**
     * @brief Describes the memory layout of a single image plane.
     *
     * Multi-planar formats (e.g., YUV) may return multiple planes.
     */
    struct ImagePlane {
        /// Number of bytes between two adjacent rows.
        uint32_t bytesPerRow = 0;
        /// Number of bytes that represent a single pixel in this plane.
        uint32_t bytesPerPixel = 0;
        /// Pointer to the start of the plane data.
        void* pixels = nullptr;
    };

    TextureInterop(const snap::rhi::TextureInteropCreateInfo& info);
    virtual ~TextureInterop() = default;

    /**
     * @brief Returns the derived `TextureCreateInfo` for this interop texture.
     */
    [[nodiscard]] const snap::rhi::TextureCreateInfo& getTextureCreateInfo() const {
        return textureCreateInfo;
    }

    /**
     * @brief Returns the list of APIs that can share this texture.
     */
    [[nodiscard]] const std::vector<snap::rhi::API>& getCompatibleAPIs() const {
        return compatibleAPIs;
    }

    /**
     * @brief Returns the list of APIs that have used (imported) this texture.
     */
    [[nodiscard]] const std::vector<snap::rhi::API>& getUsedAPIs() const {
        return usedAPIs;
    }

    /**
     * @brief Returns the creation parameters used to set up this interop texture.
     */
    [[nodiscard]] const snap::rhi::TextureInteropCreateInfo& getCreateInfo() const {
        return textureInteropCreateInfo;
    }

    /**
     * @brief Maps the texture memory for CPU access.
     *
     * Implementations must ensure that GPU writes are complete (or properly synchronized) before exposing the memory to
     * the CPU. If @p platformSyncHandle is provided, it represents GPU work that must be completed before the mapping
     * becomes safe.
     *
     * @param usage Intended CPU operation (read/write).
     * @param platformSyncHandle Optional platform-specific synchronization handle associated with prior GPU work.
     * @return A span of planes describing the mapped memory.
     */
    [[nodiscard]] virtual std::span<const ImagePlane> map(const snap::rhi::MemoryAccess access,
                                                          snap::rhi::PlatformSyncHandle* platformSyncHandle) = 0;

    /**
     * @brief Unmaps the texture memory, ending CPU access.
     *
     * Some platforms require explicit synchronization from CPU back to GPU. The returned handle (if non-null)
     * represents completion of CPU-side writes and must be waited upon before the next GPU submission that accesses the
     * texture.
     *
     * @return Platform-specific synchronization handle, or null depending on platform/backend.
     *
     * @note If a non-null handle is returned, the caller should create an `snap::rhi::Semaphore` from it (when
     * supported) and include it as a wait semaphore in the next queue submission that uses this texture.
     */
    [[nodiscard]] virtual std::unique_ptr<snap::rhi::PlatformSyncHandle> unmap() = 0;

protected:
    snap::rhi::TextureCreateInfo textureCreateInfo{};
    snap::rhi::TextureInteropCreateInfo textureInteropCreateInfo{};
    std::vector<snap::rhi::API> compatibleAPIs;
    std::vector<snap::rhi::API> usedAPIs;
    std::vector<ImagePlane> imagePlanes;
};
} // namespace snap::rhi
