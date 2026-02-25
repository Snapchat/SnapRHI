#include "snap/rhi/backend/opengl/DeviceContext.hpp"

#include "snap/rhi/backend/common/Logging.hpp"
#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/opengl/Device.hpp"
#include "snap/rhi/backend/opengl/ErrorCheckGuard.hpp"
#include "snap/rhi/backend/opengl/Profile.hpp"
#include <snap/rhi/common/Platform.h>

namespace {
void initOpenGL(const snap::rhi::backend::opengl::Profile& gl) {
    gl.pixelStorei(GL_UNPACK_ALIGNMENT, 1);
    gl.pixelStorei(GL_PACK_ALIGNMENT, 1);

    // https://registry.khronos.org/webgl/specs/latest/2.0/#4.1.4 primitive restart fixed index is always enabled on
    // WebGL. It is not available as a GLenum to be enabled.
    if (!SNAP_RHI_PLATFORM_WEBASSEMBLY() && gl.getFeatures().isPrimitiveRestartIndexSupported) {
        /**
         * Enables primitive restarting.
         * If enabled, any one of the draw commands which transfers a set of generic attribute array elements to the GL
         * will restart the primitive when the index of the vertex is equal to 2 n − 1 where n is 8, 16 or 32 if the
         * type is GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT, or GL_UNSIGNED_INT, respectively.
         */
        gl.enable(GL_PRIMITIVE_RESTART_FIXED_INDEX, nullptr);
    }
}

uint32_t getFBOIndex(const snap::rhi::backend::opengl::FramebufferTarget target) {
    switch (target) {
        case snap::rhi::backend::opengl::FramebufferTarget::Framebuffer:
            return 0;
        case snap::rhi::backend::opengl::FramebufferTarget::ReadFramebuffer:
            return 1;
        case snap::rhi::backend::opengl::FramebufferTarget::DrawFramebuffer:
            return 2;

        default:
            snap::rhi::common::throwException("[getFBOIndex] Invalid FramebufferTarget");
    }

    return 0;
}
} // unnamed namespace

namespace snap::rhi::backend::opengl {
/// @brief This function checks if the framebuffer pool is enabled based on the device and context creation info.
static inline bool isFramebufferPoolEnabled(const Device& device,
                                            const snap::rhi::backend::opengl::DeviceContextCreateInfo& info) {
    GLFramebufferPoolOption framebufferPoolOptions = device.getFramebufferPoolOptions();
    bool isFramebufferPoolSizeSpecified =
        framebufferPoolOptions > snap::rhi::backend::opengl::GLFramebufferPoolOption::None;
    return isFramebufferPoolSizeSpecified && info.internalResourcesAllowed;
}

DeviceContext::DeviceContext(snap::rhi::backend::opengl::Device* device,
                             std::optional<snap::rhi::backend::opengl::Context> context,
                             const snap::rhi::backend::opengl::DeviceContextCreateInfo& info,
                             DeviceContextUUID dcUUID)
    : snap::rhi::DeviceContext(device, info),
      glContext(context.has_value() ?
                    snap::rhi::backend::opengl::Context(std::move(context).value()) :
                    snap::rhi::backend::opengl::Context::makeSharedContext(device->sharingDeviceContext()->glContext)),
      gl(device->getOpenGL()),
      validationLayer(device->getValidationLayer()),
      fboPool(this),
      glCache(device->getOpenGL()),
      dcUUID(dcUUID),
      isFBOPoolEnabled(isFramebufferPoolEnabled(*device, info)),
      explicitFenceFlush(info.explicitFenceFlush) {
    SNAP_RHI_REPORT(device->getValidationLayer(),
                    snap::rhi::ReportLevel::Info,
                    snap::rhi::ValidationTag::CreateOp,
                    "[snap::rhi::backend::opengl::DeviceContext] DeviceContext created");
    fbos.fill(nullptr);
}

DeviceContext::~DeviceContext() noexcept {
    try {
        [[maybe_unused]] auto guard = glContext.makeCurrent();
        deleteDCResources();

        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Info,
                        snap::rhi::ValidationTag::DestroyOp,
                        "[snap::rhi::backend::opengl::~DeviceContext] Destroyed");
    } catch (const snap::rhi::common::Exception& e) {
        SNAP_RHI_LOGE(
            "[snap::rhi::backend::opengl::DeviceContext::~DeviceContext] Caught: %s (possible resource leak).",
            e.what());
    } catch (...) {
        SNAP_RHI_LOGE("[snap::rhi::backend::opengl::DeviceContext::~DeviceContext] Caught unexpected error (possible "
                      "resource leak).");
    }

    SNAP_RHI_REPORT(validationLayer,
                    snap::rhi::ReportLevel::Info,
                    snap::rhi::ValidationTag::DestroyOp,
                    "[snap::rhi::backend::opengl::~DeviceContext] DeviceContext destroyed");
}

void DeviceContext::resetGLStateCache() {
    glCache.reset();
}

GLuint DeviceContext::getVAO() {
    if (gl.getFeatures().isVAOSupported && vao == GL_NONE) {
        gl.genVertexArrays(1, &vao);
    }

    return vao;
}

void DeviceContext::deleteDCResources() {
    if (vao != GL_NONE) {
        gl.deleteVertexArrays(1, &vao);
        vao = GL_NONE;
    }

    fbos.fill(nullptr);
    fboPool.clear();
}

thread_local DeviceContext* currentDC = nullptr;

snap::rhi::DeviceContext::Guard DeviceContext::makeCurrent() {
    // We just need to capture underlying context guard into a lambda here.
    // Small object optimization of std::function currently holds 6 void pointers, so with gl context guard we're a bit
    // below the threshold (4 pointers).
    auto oldDC = currentDC;
    currentDC = this;
    static_assert(sizeof(std::decay_t<decltype(glContext.makeCurrent())>) <= 4 * sizeof(void*));
    // initOpenGL? for the initial context, we don't yet have the version in profile determined
    auto contextGuard = glContext.makeCurrent();
    if (!didInitOpenGL) {
        initOpenGL(gl);
        didInitOpenGL = true;
    }
    return snap::rhi::DeviceContext::Guard(snap::rhi::common::move_only_function<void()>(
        [contextGuard = std::move(contextGuard), oldDC] { currentDC = oldDC; }));
}

DeviceContext* DeviceContext::getCurrent() noexcept {
    return currentDC;
}

bool DeviceContext::validateCurrent() const {
    return currentDC == this && glContext.isContextAttached();
}

std::vector<uint32_t>& DeviceContext::getDataAlignmentCache() {
    return dataAlignmentCache;
}

void DeviceContext::clearInternalResources() {
    deleteDCResources();

    snap::rhi::backend::opengl::Device* glDevice =
        snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Device>(device);
    if (glDevice) {
        glDevice->clearLazyDestroyResources();
    }
}

void DeviceContext::validateContext() {
    SNAP_RHI_VALIDATE(
        validationLayer,
        glContext.isContextAttached(),
        snap::rhi::ReportLevel::Error,
        snap::rhi::ValidationTag::DeviceContextOp,
        "[snap::rhi::backend::opengl::DeviceContext::validateContext] Wrong context attached{%p}, expected{%p}",
        gl::getActiveContext(),
        glContext.getGLContext());
}

FramebufferId DeviceContext::bindFramebuffer(FramebufferTarget target, const FramebufferDescription& description) {
    if (!isFBOPoolEnabled) {
        const uint32_t idx = getFBOIndex(target);

        if (!fbos[idx]) {
            fbos[idx] = std::make_unique<FBO>(this);
        }
        return fbos[idx]->assignAndBind(target, description);
    }

    return fboPool.bindFramebuffer(target, description);
}

void DeviceContext::freeUnusedFBOs() {
    /**
     * Driver will have more time to finish FBO related operations if SnapRHI will postpone FBO destroy
     *
     * fbos.fill(nullptr);
     */
    fboPool.freeUnusedFBOs();
#if SNAP_RHI_PLATFORM_WEBASSEMBLY() || SNAP_RHI_OS_WINDOWS()
    fbos.fill(nullptr);
#endif

#if SNAP_RHI_PLATFORM_WEBASSEMBLY()
    if (isFBOPoolEnabled) {
        /**
         * Only if FBO pool enabled, SnapRHI has to reset current framebuffer, since:
         * According to GLES2 spec, textures and renderbuffers
         *  should be detached first before deletion if they are
         *  attached to the currently-bound framebuffer; for any other framebuffers,
         *  the detachment is the responsibility of the application.
         *
         * https://bugs.webkit.org/show_bug.cgi?id=43942
         */
        gl.bindFramebuffer(snap::rhi::backend::opengl::FramebufferTarget::Framebuffer,
                           snap::rhi::backend::opengl::FramebufferId::CurrSurfaceBackbuffer,
                           this);
    }
#endif
}
} // namespace snap::rhi::backend::opengl
