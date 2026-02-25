#pragma once

#include "snap/rhi/DeviceContext.hpp"
#include "snap/rhi/backend/opengl/Context.h"
#include "snap/rhi/backend/opengl/DeviceCreateInfo.h"
#include "snap/rhi/backend/opengl/FBO.hpp"
#include "snap/rhi/backend/opengl/Framebuffer.hpp"
#include "snap/rhi/backend/opengl/FramebufferPool.h"
#include "snap/rhi/backend/opengl/GLStateCache.hpp"

#include "snap/rhi/backend/common/Utils.hpp"

#include <memory>
#include <vector>

namespace snap::rhi::backend::opengl {
class Device;
class Profile;

using DeviceContextUUID = uint64_t;
constexpr DeviceContextUUID UndefinedDeviceContextUUID = 0;

class DeviceContext final : public snap::rhi::DeviceContext {
public:
    /**
     * Creates a new DeviceContext. This DeviceContext may either create a new OpenGL context, or reuse existing
     * context. This behavior is controlled by glApiVersion and by info. If glApiVersion is set to anything other than
     * None, it means that we want to create the root DeviceContext for the device. In such a case, dependent on the
     * flag in info, DeviceContext will either pick the existing context and validate it against the API version
     * requested, or create a new context with the requested API version. If glApiVersion is set to None, then we expect
     * the parent Device to already own the root context, and we'll create a new GL DeviceContext shared with it.
     */
    DeviceContext(snap::rhi::backend::opengl::Device* device,
                  std::optional<snap::rhi::backend::opengl::Context> context,
                  const snap::rhi::backend::opengl::DeviceContextCreateInfo& info,
                  DeviceContextUUID dcUUID);
    ~DeviceContext() noexcept override;

    void resetGLStateCache();

    const Profile& getOpenGL() const noexcept {
        return gl;
    }

    Profile& getOpenGL() noexcept {
        return gl;
    }

    SNAP_RHI_ALWAYS_INLINE GLStateCache& getGLStateCache() {
        return glCache;
    }

    GLuint getVAO();

    DeviceContextUUID getDeviceContextUUID() const {
        return dcUUID;
    }

    snap::rhi::DeviceContext::Guard makeCurrent();
    virtual void* getNativeContext() const override {
        return glContext.getGLContext();
    }

    bool validateCurrent() const override;

    static DeviceContext* getCurrent() noexcept;

    void clearInternalResources() override;

    FramebufferId bindFramebuffer(FramebufferTarget target, const FramebufferDescription& description);
    void freeUnusedFBOs();

    std::vector<uint32_t>& getDataAlignmentCache();

    const snap::rhi::backend::common::ValidationLayer& getValidationLayer() {
        return validationLayer;
    }

    void validateContext();

    [[nodiscard]] bool shouldFlushOnFenceCreation() const {
        return !explicitFenceFlush;
    }

private:
    void deleteDCResources();

    snap::rhi::backend::opengl::Context glContext;
    snap::rhi::backend::opengl::Profile& gl;
    const snap::rhi::backend::common::ValidationLayer& validationLayer;

    GLuint vao = GL_NONE;
    std::array<std::shared_ptr<FBO>, static_cast<uint32_t>(FramebufferTarget::Count)> fbos;
    std::vector<uint32_t> dataAlignmentCache;
    FramebufferPool fboPool;
    GLStateCache glCache;

    const DeviceContextUUID dcUUID;
    const bool isFBOPoolEnabled = true;
    bool didInitOpenGL = false;
    bool explicitFenceFlush = false;
};
} // namespace snap::rhi::backend::opengl
