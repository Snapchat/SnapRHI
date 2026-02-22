#pragma once

#include "snap/rhi/backend/opengl/OpenGL.h"
#include "snap/rhi/backend/opengl/Texture.hpp"

#include "snap/rhi/Exception.h"
#include "snap/rhi/Limits.h"
#include "snap/rhi/PixelFormat.h"

#include <algorithm>
#include <compare>

namespace snap::rhi::backend::opengl {
struct FramebufferAttachment final {
    TextureUUID texUUID = UndefinedTextureUUID;

    TextureTarget target = TextureTarget::Detached;
    TextureId texId = TextureId::Null;
    uint32_t level = 0;
    uint32_t firstLayer = 0;
    uint32_t viewMask = 0;

    Extent3D size{};
    PixelFormat format = PixelFormat::Undefined;
    SampleCount sampleCount = SampleCount::Undefined; // snap::rhi::SampleCount::Count1 means no MSAA

    // clang-format off
    [[nodiscard]] constexpr bool isMultiview() const noexcept {
        return viewMask != 0;
    }
    [[nodiscard]] constexpr friend auto operator<=>(const FramebufferAttachment&, const FramebufferAttachment&) noexcept = default;
    // clang-format on
};

[[nodiscard]] constexpr bool isFramebufferAttachmentMultiview(const FramebufferAttachment& attachment) noexcept {
    return attachment.isMultiview();
}

struct FramebufferDescription final {
    std::array<FramebufferAttachment, MaxColorAttachments> colorAttachments = {};
    uint32_t numColorAttachments = 0;

    FramebufferAttachment depthStencilAttachment = {};

    [[nodiscard]] constexpr bool isMultiview() const noexcept {
        auto colorIt = colorAttachments.begin();
        auto colorItEnd = colorIt + numColorAttachments;
        bool isMultiview_ = std::any_of(colorIt, colorItEnd, isFramebufferAttachmentMultiview) ||
                            isFramebufferAttachmentMultiview(depthStencilAttachment);
        return isMultiview_;
    }

    [[nodiscard]] constexpr friend bool operator==(const FramebufferDescription& lhs,
                                                   const FramebufferDescription& rhs) noexcept {
        if (lhs.numColorAttachments != rhs.numColorAttachments)
            return false;

        for (uint32_t i = 0; i < lhs.numColorAttachments; ++i) {
            if (!(lhs.colorAttachments[i] == rhs.colorAttachments[i])) {
                return false;
            }
        }
        return lhs.depthStencilAttachment == rhs.depthStencilAttachment;
    };
};
} // namespace snap::rhi::backend::opengl
