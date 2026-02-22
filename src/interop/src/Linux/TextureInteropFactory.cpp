#include "snap/rhi/interop/TextureInteropFactory.h"
#include "TextureInterop.h"

#if SNAP_RHI_ENABLE_BACKEND_VULKAN
#include "snap/rhi/interop/platform/linux/TextureInteropFactory.h"
#endif

namespace snap::rhi::interop {
std::unique_ptr<snap::rhi::TextureInterop> createTextureInterop(const snap::rhi::TextureInteropCreateInfo& info) {
    return std::make_unique<snap::rhi::interop::Linux::TextureInterop>(info);
}
} // namespace snap::rhi::interop

#if SNAP_RHI_ENABLE_BACKEND_VULKAN
namespace snap::rhi::interop::Linux {
std::unique_ptr<snap::rhi::TextureInterop> createTextureInteropFromExternalFD(
    const snap::rhi::TextureInteropCreateInfo& info, std::shared_ptr<ExternalVulkanResourceInfo> externalResources) {
    return std::make_unique<TextureInterop>(info, std::move(externalResources));
}
} // namespace snap::rhi::interop::Linux
#endif
