#include "snap/rhi/interop/TextureInteropFactory.h"
#include "TextureInterop.h"

namespace snap::rhi::interop {
std::unique_ptr<snap::rhi::TextureInterop> createTextureInterop(const snap::rhi::TextureInteropCreateInfo& info) {
    return std::make_unique<snap::rhi::interop::Android::TextureInterop>(info);
}
} // namespace snap::rhi::interop
