#pragma once

#include "snap/rhi/backend/common/TextureInterop.h"
#include "snap/rhi/common/OS.h"
#include <Metal/Metal.h>

namespace snap::rhi::backend::metal {
class TextureInterop : public virtual snap::rhi::backend::common::TextureInterop {
public:
    virtual const id<MTLTexture>& getMetalTexture(const id<MTLDevice>& mtlDevice) = 0;
};
} // namespace snap::rhi::backend::metal
