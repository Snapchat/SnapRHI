#pragma once

#include "snap/rhi/backend/common/ValidationLayer.hpp"
#include "snap/rhi/backend/opengl/FBO.hpp"
#include "snap/rhi/backend/opengl/FramebufferDescription.h"
#include "snap/rhi/backend/opengl/FramebufferStatus.h"
#include "snap/rhi/backend/opengl/FramebufferTarget.h"
#include "snap/rhi/backend/opengl/OpenGL.h"

#include "snap/rhi/Exception.h"
#include "snap/rhi/Limits.h"

#include "snap/rhi/common/NonCopyable.h"
#include "snap/rhi/common/OS.h"
#include <span>

#include <memory>
#include <utility>
#include <vector>

namespace snap::rhi::backend::opengl {
class DeviceContext;
class Profile;

struct FramebufferPoolException : snap::rhi::Exception {
    using Exception::Exception;
};

/// \brief Represents a pool of \c GLESFramebuffer per \c DeviceContext.
/// \note Use \c FramebufferDescBuilder for creating proper \c FramebufferDesc instances.
/// \details In the legacy renderer, we have a fixed number of framebuffers and always detach
/// framebuffer attachments, and this helped GPU drivers to manage framebuffer objects, mark used
/// tile memory as dirty, etc. This is not working well for us in the case with OVR
/// attachments (texture arrays), as we see either corrupted output or output rendered only to one
/// half of the texture array (in stereo terms, only one stereo view/eye). To overcome this issue
/// in OVR, we designed a framebuffer pool that does need to detach attachments and instead manages
/// a pool of framebuffers with a limited capacity that creates framebuffers and bind attachments
/// once during framebuffer creation and then only rebinds read/draw buffers.
class FramebufferPool final {
    struct Key {
        Key(const FramebufferDescription& description);

        /**
         * A class can define operator== as defaulted, with a return value of bool.
         * This will generate an equality comparison of each base class
         * and member subobject, in their declaration order.
         * Two objects are equal if the values of their base classes and members are equal.
         * The test will short-circuit if an inequality is found in members
         * or base classes earlier in declaration order.
         * https://en.cppreference.com/w/cpp/language/default_comparisons
         *
         * It means that hash will compare first hash, and stop compare if hash is different, but we have to declare
         * HASH as first field.
         */
        constexpr friend bool operator==(const Key&, const Key&) noexcept = default;

        struct HashFunction {
            size_t operator()(const Key& key) const {
                return key.hash;
            }
        };

    public:
        size_t hash = 0;
        FramebufferDescription description{};
    };

    struct Value {
        Value(DeviceContext* dc);

    public:
        uint64_t useCount = 0;
        std::unique_ptr<FBO> fbo;
    };

public:
    explicit FramebufferPool(DeviceContext* dc) noexcept;
    ~FramebufferPool() noexcept;

    FramebufferId bindFramebuffer(FramebufferTarget target, const FramebufferDescription& description);
    void freeUnusedFBOs();
    void clear();

private:
    const snap::rhi::backend::common::ValidationLayer& validationLayer;
    DeviceContext* dc = nullptr;
    Profile& gl;

    std::vector<std::pair<Key, Value>> fbos;

    const uint32_t ClearFBOPoolSize = 0;
};
} // namespace snap::rhi::backend::opengl
