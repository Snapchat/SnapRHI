#pragma once

#include <cassert>
#include <cstdint>
#include <string_view>

#include "snap/rhi/backend/opengl/OpenGL.h"

namespace snap::rhi::backend::opengl {

/// \brief Typed identifier for framebuffer object names.
enum class FramebufferId : uint32_t {
    /// \brief The value zero is reserved to represent the default
    ///        framebuffer provided by the windowing system.
    CurrSurfaceBackbuffer = 0
};

enum class FramebufferTarget : uint32_t {
    Detached = 0,
    Framebuffer = GL_FRAMEBUFFER,
    ReadFramebuffer = GL_READ_FRAMEBUFFER, // Src
    DrawFramebuffer = GL_DRAW_FRAMEBUFFER, // Dst
    Count = 3
};

constexpr bool isGood(FramebufferTarget target) {
    switch (target) {
        case FramebufferTarget::Framebuffer:
        case FramebufferTarget::DrawFramebuffer:
        case FramebufferTarget::ReadFramebuffer:
            return true;
        default:
            return false;
    }
}

constexpr uint32_t NumFramebufferTargets = static_cast<uint32_t>(FramebufferTarget::Count);

[[nodiscard]] static inline constexpr std::string_view toString(FramebufferTarget target) noexcept {
    switch (target) {
        case FramebufferTarget::Detached:
            return "Detached";
        case FramebufferTarget::Framebuffer:
            return "Framebuffer";
        case FramebufferTarget::ReadFramebuffer:
            return "ReadFramebuffer";
        case FramebufferTarget::DrawFramebuffer:
            return "DrawFramebuffer";
        default:
            assert(false && "Caught unknown framebuffer target.");
            return "<Error>";
    }
}

enum class FramebufferTargetIndex : uint8_t {
    Detached = 0,
    Framebuffer,
    ReadFramebuffer,
    DrawFramebuffer,
    Error,
};

[[nodiscard]] static inline constexpr FramebufferTargetIndex toIndex(FramebufferTarget target) noexcept {
    switch (target) {
        case FramebufferTarget::Detached:
            return FramebufferTargetIndex::Detached;
        case FramebufferTarget::Framebuffer:
            return FramebufferTargetIndex::Framebuffer;
        case FramebufferTarget::ReadFramebuffer:
            return FramebufferTargetIndex::ReadFramebuffer;
        case FramebufferTarget::DrawFramebuffer:
            return FramebufferTargetIndex::DrawFramebuffer;
        default:
            assert(false && "Caught unknown framebuffer target.");
            return FramebufferTargetIndex::Error;
    }
}

} // namespace snap::rhi::backend::opengl
