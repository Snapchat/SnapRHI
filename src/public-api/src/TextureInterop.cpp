#include "snap/rhi/TextureInterop.h"

namespace snap::rhi {
TextureInterop::TextureInterop(const snap::rhi::TextureInteropCreateInfo& info)
    : textureCreateInfo({.size = {.width = info.size.width, .height = info.size.height, .depth = info.size.depth},
                         .mipLevels = 1,
                         .textureType = info.textureType,
                         .textureUsage = info.textureUsage,
                         .sampleCount = snap::rhi::SampleCount::Count1,
                         .format = info.format,
                         .components = snap::rhi::DefaultComponentMapping}),
      textureInteropCreateInfo(info) {}
} // namespace snap::rhi
