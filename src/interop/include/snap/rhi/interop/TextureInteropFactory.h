#pragma once

#include <memory>

#include "snap/rhi/TextureInterop.h"

namespace snap::rhi::interop {
std::unique_ptr<snap::rhi::TextureInterop> createTextureInterop(const snap::rhi::TextureInteropCreateInfo& info);
} // namespace snap::rhi::interop
