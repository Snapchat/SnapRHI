#pragma once

#include "snap/rhi/backend/opengl/OpenGL.h"

#include <cassert>
#include <cstdint>
#include <snap/rhi/common/zstring_view.h>
#include <string_view>

namespace snap::rhi::backend::opengl {

/// \brief Completeness status of a framebuffer object.
enum class FramebufferStatus {
    /// \brief The return value is GL_FRAMEBUFFER_COMPLETE if the specified framebuffer is complete.
    Complete = GL_FRAMEBUFFER_COMPLETE,
    // Otherwise, the return value is determined as follows:
    /// \brief Undefined: the specified framebuffer is the default read or draw framebuffer,
    ///        but the default framebuffer does not exist.
    Undefined = GL_FRAMEBUFFER_UNDEFINED,
    /// \brief Unsupported: the combination of internal formats of the attached images
    ///        violates an implementation-dependent set of restrictions.
    Unsupported = GL_FRAMEBUFFER_UNSUPPORTED,
    /// \brief IncompleteAttachment: any of the framebuffer attachment points are framebuffer incomplete.
    IncompleteAttachment = GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT,
    /// \brief IncompleteMissingAttachment: the framebuffer does not have at least one image attached to it.
    IncompleteMissingAttachment = GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT,
    /// \brief IncompleteDrawBuffer, IncompleteReadBuffer: attachment type mismatches are detected.
    ///        The value of GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE is GL_NONE for any color
    ///        attachment point(s) named by GL_DRAW_BUFFERi.
    IncompleteDrawBuffer = GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER,
    /// \brief IncompleteReadBuffer: attachment type mismatches are detected.
    ///        The value of GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE is GL_NONE
    ///        for the color attachment point named by GL_READ_BUFFER
    ///        (+ GL_READ_BUFFER is not GL_NONE).
    IncompleteReadBuffer = GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER,
    /// \brief IncompleteDimensions: Not all attached images have the same width and height.
    IncompleteDimensions = GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS,
    /// \brief IncompleteMultisample: number of samples is different on the attached renderbuffers.
    IncompleteMultisample = GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE,
    /// \brief IncompleteLayersTarget: any framebuffer attachment is layered, and any populated attachment is not
    /// layered,
    // or if all populated color attachments are not from textures of the same target.
    IncompleteLayersTarget = GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS,
    /// \brief IncompleteViewTargetsOVR: baseViewIndex is not the same for all framebuffer attachment
    ///        points where the value of GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE is not GL_NONE.
    ///        If you see this error, you may have mixed OVR and non-OVR attachments.
    ///        We do not unbind OVR attachments because of bugs on Adreno GPUs, so use FramebufferPool.
    /// \sa FramebufferPool, GLESFramebuffer::resetAttachment()
    IncompleteViewTargetsOVR = GL_FRAMEBUFFER_INCOMPLETE_VIEW_TARGETS_OVR
};

[[nodiscard]] static inline constexpr snap::rhi::common::zstring_view toString(FramebufferStatus v) noexcept {
    switch (v) {
        case FramebufferStatus::Complete:
            return "Complete";
        case FramebufferStatus::Undefined:
            return "Undefined";
        case FramebufferStatus::Unsupported:
            return "Unsupported";
        case FramebufferStatus::IncompleteAttachment:
            return "IncompleteAttachment";
        case FramebufferStatus::IncompleteMissingAttachment:
            return "IncompleteMissingAttachment";
        case FramebufferStatus::IncompleteDrawBuffer:
            return "IncompleteDrawBuffer";
        case FramebufferStatus::IncompleteReadBuffer:
            return "IncompleteReadBuffer";
        case FramebufferStatus::IncompleteDimensions:
            return "IncompleteDimensions";
        case FramebufferStatus::IncompleteMultisample:
            return "IncompleteMultisample";
        case FramebufferStatus::IncompleteLayersTarget:
            return "IncompleteLayersTarget";
        case FramebufferStatus::IncompleteViewTargetsOVR:
            return "IncompleteViewTargetsOVR";
        default:
            assert(false && "Caught unknown framebuffer status.");
            return "<Error>";
    }
}

} // namespace snap::rhi::backend::opengl
