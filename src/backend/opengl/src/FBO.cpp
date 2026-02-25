#include "snap/rhi/backend/opengl/FBO.hpp"
#include "snap/rhi/backend/common/Logging.hpp"
#include "snap/rhi/backend/opengl/Device.hpp"
#include "snap/rhi/backend/opengl/DeviceContext.hpp"
#include "snap/rhi/backend/opengl/Profile.hpp"
#include <snap/rhi/common/Scope.h>

#include <bit>

namespace {
struct MultiviewInfo {
    uint32_t baseView = 0;
    uint32_t numViews = 0;
};
MultiviewInfo getMultiviewInfo(uint32_t viewMask) {
    uint32_t baseView = std::countr_zero(viewMask);
    uint32_t numViews = std::countr_one(viewMask >> baseView);
    if (viewMask >> (numViews + baseView) != 0) {
        snap::rhi::common::throwException<snap::rhi::backend::opengl::FramebufferException>(
            snap::rhi::common::stringFormat("View mask must have contiguous bits set. Received 0x%x", viewMask));
    }
    return {baseView, numViews};
}

void bindAttachment(const snap::rhi::backend::opengl::Profile& gl,
                    const snap::rhi::backend::opengl::FramebufferTarget target,
                    const snap::rhi::backend::opengl::FramebufferAttachmentTarget attachmentTarget,
                    const snap::rhi::backend::opengl::FramebufferAttachment& attachment) {
    const auto& caps = gl.getDevice()->getCapabilities();

    if (attachment.target == snap::rhi::backend::opengl::TextureTarget::Texture2D ||
        attachment.target == snap::rhi::backend::opengl::TextureTarget::TextureRectangle) {
        assert(!attachment.isMultiview() && "Caught unexpected value.");
        assert(attachment.firstLayer == 0 && "Caught unexpected value.");
        gl.framebufferTexture2D(target, attachmentTarget, attachment.target, attachment.texId, attachment.level);
    } else if (attachment.target == snap::rhi::backend::opengl::TextureTarget::TextureCubeMap) {
        assert(!attachment.isMultiview() && "Caught unexpected value.");
        gl.framebufferTexture2D(target,
                                attachmentTarget,
                                snap::rhi::backend::opengl::getCubeMapSideTextureTarget(attachment.firstLayer),
                                attachment.texId,
                                attachment.level);
    } else if (attachment.target == snap::rhi::backend::opengl::TextureTarget::Renderbuffer) {
        assert(attachment.firstLayer == 0 && "Caught unexpected value.");
        assert(!attachment.isMultiview() && "Caught unexpected value.");

        gl.framebufferRenderbuffer(target, attachmentTarget, attachment.target, attachment.texId);
    } else {
        if (attachment.target != snap::rhi::backend::opengl::TextureTarget::Texture2DArray &&
            attachment.target != snap::rhi::backend::opengl::TextureTarget::Texture3D) {
            snap::rhi::common::throwException<snap::rhi::backend::opengl::FramebufferException>(
                snap::rhi::common::stringFormat(
                    "[snap::rhi::backend::opengl::FBO] Incorrect attachment type: %s, expected: %s or %s.",
                    toString(attachment.target).data(),
                    toString(snap::rhi::backend::opengl::TextureTarget::Texture2DArray).data(),
                    toString(snap::rhi::backend::opengl::TextureTarget::Texture3D).data()));
        }

        if (!attachment.isMultiview()) {
            gl.framebufferTextureLayer(target,
                                       attachmentTarget,
                                       attachment.texId,
                                       static_cast<int32_t>(attachment.level),
                                       static_cast<int32_t>(attachment.firstLayer));

        } else {
            if (target != snap::rhi::backend::opengl::FramebufferTarget::DrawFramebuffer) {
                snap::rhi::common::throwException<snap::rhi::backend::opengl::FramebufferException>(
                    snap::rhi::common::stringFormat(
                        "[snap::rhi::backend::opengl::FBO] Incorrect framebuffer target: %s, expected: %s.",
                        toString(target).data(),
                        toString(snap::rhi::backend::opengl::FramebufferTarget::DrawFramebuffer).data()));
            }

            auto [baseViewIdx, numViews] = getMultiviewInfo(attachment.viewMask);
            if (numViews > caps.maxMultiviewViewCount) {
                snap::rhi::common::throwException<snap::rhi::UnsupportedOperationException>(
                    snap::rhi::common::stringFormat(
                        "[snap::rhi::backend::opengl::FBO] Requested num views %d exceeded max supported count %d",
                        numViews,
                        caps.maxMultiviewViewCount));
            }

            if (attachment.sampleCount == snap::rhi::SampleCount::Count1) {
                gl.framebufferTextureMultiviewOVR(
                    target, attachmentTarget, attachment.texId, attachment.level, baseViewIdx, numViews);
            } else {
                assert(caps.isMultiviewMSAAImplicitResolveEnabled);
                gl.framebufferTextureMultisampleMultiviewOVR(target,
                                                             attachmentTarget,
                                                             attachment.texId,
                                                             attachment.level,
                                                             static_cast<uint32_t>(attachment.sampleCount),
                                                             baseViewIdx,
                                                             numViews);
            }
        }
    }
}

void bindColorAttachments(const snap::rhi::backend::opengl::Profile& gl,
                          snap::rhi::backend::opengl::FramebufferTarget target,
                          const snap::rhi::backend::opengl::FramebufferDescription& description) {
    for (uint32_t i = 0; i < description.numColorAttachments; ++i) {
        bindAttachment(gl,
                       target,
                       snap::rhi::backend::opengl::getFramebufferColorAttachmentTarget(i),
                       description.colorAttachments[i]);
    }
}

void bindDepthStencilAttachment(const snap::rhi::backend::opengl::Profile& gl,
                                snap::rhi::backend::opengl::FramebufferTarget target,
                                const snap::rhi::backend::opengl::FramebufferDescription& description) {
    const auto& attachment = description.depthStencilAttachment;

    if (attachment.texId != snap::rhi::backend::opengl::TextureId::Null) {
        // TODO(vserhiienko)
        //   https://www.khronos.org/registry/OpenGL/extensions/OVR/OVR_multiview.txt,
        //   https://www.khronos.org/registry/OpenGL/extensions/OVR/OVR_multiview2.txt,
        //   Google Pixel (maybe other Adrenos) supports only depth OVR attachments.
        //   Otherwise, FRAMEBUFFER_INCOMPLETE_VIEW_TARGETS_OVR is generated.
        //   I've double-checked via glGetFramebufferAttachmentParameteriv that
        //   DEPTH_ATTACHMENT and STENCIL_ATTACHMENT are not bound to DRAW_FRAMEBUFFER.

        const auto& features = gl.getFeatures();

        bool canUseDepthStencilAttachment =
            attachment.isMultiview() ? features.isOVRDepthStencilSupported : features.isDepthStencilAttachmentSupported;

        auto dsTraits = snap::rhi::getDepthStencilFormatTraits(attachment.format);
        if (dsTraits == snap::rhi::DepthStencilFormatTraits::HasDepthStencilAspects && canUseDepthStencilAttachment) {
            bindAttachment(gl,
                           target,
                           snap::rhi::backend::opengl::FramebufferAttachmentTarget::DepthStencilAttachment,
                           attachment);
        } else {
            if ((dsTraits & snap::rhi::DepthStencilFormatTraits::HasDepthAspect) ==
                snap::rhi::DepthStencilFormatTraits::HasDepthAspect) {
                bindAttachment(
                    gl, target, snap::rhi::backend::opengl::FramebufferAttachmentTarget::DepthAttachment, attachment);
            }
            if ((dsTraits & snap::rhi::DepthStencilFormatTraits::HasStencilAspect) ==
                snap::rhi::DepthStencilFormatTraits::HasStencilAspect) {
                bindAttachment(
                    gl, target, snap::rhi::backend::opengl::FramebufferAttachmentTarget::StencilAttachment, attachment);
            }
        }
    }
}
} // unnamed namespace

namespace snap::rhi::backend::opengl {
FramebufferIncompleteException::FramebufferIncompleteException(FramebufferStatus status)
    : FramebufferIncompleteException(status, "") {}

FramebufferIncompleteException::FramebufferIncompleteException(FramebufferStatus status, const std::string& desc)
    : FramebufferException(snap::rhi::common::stringFormat("glCheckFramebufferStatus() failed with %s (0x%x).%s%s\n",
                                                           toString(status).data(),
                                                           uint32_t(status),
                                                           desc.empty() ? "" : "\n",
                                                           desc.c_str())),
      status(status) {}

FBO::FBO(DeviceContext* dc) : dc(dc), gl(dc->getOpenGL()) {}

FBO::~FBO() {
    try {
        if (fbo != FramebufferId::CurrSurfaceBackbuffer) {
            gl.deleteFramebuffers(1, &fbo);
        }
    } catch (const snap::rhi::common::Exception& e) {
        SNAP_RHI_LOGE("[snap::rhi::backend::opengl::FBO::~FBO] Caught: %s, (possible resource leak).", e.what());
    } catch (...) {
        SNAP_RHI_LOGE("[snap::rhi::backend::opengl::FBO::~FBO] Caught unexpected error (possible resource leak).");
    }
}

FBO::FBO(FBO&& other) : dc(other.dc), gl(other.gl), fbo(other.fbo), activeDescription(other.activeDescription) {
    other.fbo = FramebufferId::CurrSurfaceBackbuffer;
}

void FBO::tryInitialize() {
    if (fbo == FramebufferId::CurrSurfaceBackbuffer) {
        gl.genFramebuffers(1, &fbo);
    }
}

void FBO::bindAttachments(FramebufferTarget target, const FramebufferDescription& description) {
    bindColorAttachments(gl, target, description);
    bindDepthStencilAttachment(gl, target, description);

    activeDescription = description;
}

void FBO::setFramebufferDescription(FramebufferTarget target, const FramebufferDescription& description) {
    /**
     * External code have to init proper draw/read buffer settings
     */
    //    if (target != FramebufferTarget::ReadFramebuffer) {
    //        gl.drawBuffers(0, nullptr);
    //    }
    //
    //    if (target != FramebufferTarget::DrawFramebuffer) {
    //        gl.readBuffer(FramebufferAttachmentTarget::Detached);
    //    }

    if (description.numColorAttachments < activeDescription.numColorAttachments) {
        for (uint32_t i = description.numColorAttachments; i < activeDescription.numColorAttachments; ++i) {
            gl.framebufferTexture2D(
                target, getFramebufferColorAttachmentTarget(i), TextureTarget::Texture2D, TextureId::Null, 0);
            activeDescription.colorAttachments[i].texId = TextureId::Null;
        }
    }

    /**
     * These is issue on some Samsung devices with reusing the same FBO with MTR.
     * To prevent this issue, SnapRHI needs to reset the DepthStencil attachments and then rebind the correct
     * attachments.
     */
    if ((description.numColorAttachments > 1) ||
        ((description.depthStencilAttachment.texId == TextureId::Null) &&
         (activeDescription.depthStencilAttachment.texId != TextureId::Null))) {
        gl.framebufferTexture2D(target,
                                FramebufferAttachmentTarget::DepthStencilAttachment,
                                TextureTarget::Texture2D,
                                snap::rhi::backend::opengl::TextureId::Null,
                                0);
        activeDescription.depthStencilAttachment.texId = TextureId::Null;
    }

    bindAttachments(target, description);
}

FramebufferId FBO::assignAndBind(FramebufferTarget target, const FramebufferDescription& description) {
    if (isDefaultBackBuffer(description)) {
        gl.bindFramebuffer(target, FramebufferId::CurrSurfaceBackbuffer, dc);
        activeDescription = description;
        return FramebufferId::CurrSurfaceBackbuffer;
    }

    tryInitialize();
    gl.bindFramebuffer(target, fbo, dc);
    setFramebufferDescription(target, description);
    return fbo;
}

FramebufferId FBO::bind(FramebufferTarget target) {
    if (isDefaultBackBuffer(activeDescription)) {
        gl.bindFramebuffer(target, FramebufferId::CurrSurfaceBackbuffer, dc);
        return FramebufferId::CurrSurfaceBackbuffer;
    }

    tryInitialize();
    gl.bindFramebuffer(target, fbo, dc);

    /**
     * External code have to init proper draw/read buffer settings
     */
    //    if (target != FramebufferTarget::ReadFramebuffer) {
    //        gl.drawBuffers(0, nullptr);
    //    }
    //
    //    if (target != FramebufferTarget::DrawFramebuffer) {
    //        gl.readBuffer(FramebufferAttachmentTarget::Detached);
    //    }

    /**
     * These is issue on some Samsung devices with reusing the same FBO with MTR.
     * To prevent this issue, SnapRHI needs to reset the DepthStencil attachments and then rebind the correct
     * attachments.
     */
    if (activeDescription.numColorAttachments > 1 &&
        (activeDescription.depthStencilAttachment.texId != TextureId::Null)) {
        gl.framebufferTexture2D(target,
                                FramebufferAttachmentTarget::DepthStencilAttachment,
                                TextureTarget::Texture2D,
                                snap::rhi::backend::opengl::TextureId::Null,
                                0);
        bindDepthStencilAttachment(gl, target, activeDescription);
    }

    return fbo;
}

bool isDefaultBackBuffer(const FramebufferDescription& description) {
    return ((description.numColorAttachments == 1) &&
            (description.colorAttachments[0].texId == TextureId::CurrentSurfaceBackBuffer) &&
            (description.depthStencilAttachment.texId == TextureId::Null));
}
} // namespace snap::rhi::backend::opengl
