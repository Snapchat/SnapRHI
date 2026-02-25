#pragma once

#include "snap/rhi/backend/opengl/OpenGL.h"
#include <OpenGL/Context.h>

namespace snap::rhi::backend::opengl {
class Context {
public:
    class Guard final {
    public:
        explicit Guard(Context& context);
        // The destructor is force-inlined and delegates to another function to optimize away moves efficiently
        ~Guard() noexcept(false) {
            if (context != nullptr) {
                doDestroy();
            }
        }

        Guard(const Guard&) noexcept = delete;
        Guard& operator=(const Guard&) noexcept = delete;

        Guard(Guard&& other) noexcept
            : context(other.context), recoverContext(other.recoverContext), recoverSurfaces(other.recoverSurfaces) {
            other.context = nullptr;
        }

        Guard& operator=(Guard&&) noexcept = delete;

    private:
        void doDestroy();

        gl::Context context;
        gl::Context recoverContext = nullptr;
        gl::Surfaces recoverSurfaces{};
    };

    enum class ConstructBehavior {
        CreateNewAlways,
        UseBoundOrCreateNew,
    };

    enum class ContextMode : uint8_t {
        Own,
        Validate,
        Manage,
    };
    explicit Context(gl::APIVersion api, ConstructBehavior mode, bool useDebugFlags);
    explicit Context(gl::Context rawContext, ContextMode mode, bool useDebugFlags);
    static Context makeSharedContext(const Context& other) {
        return Context(other);
    }

    Context(Context&& other)
        : context(other.context), contextMode(other.contextMode), useDebugFlags(other.useDebugFlags) {
        other.context = nullptr;
    }
    ~Context() noexcept(false);
    [[nodiscard]] Guard makeCurrent() {
        return Guard(*this);
    }

    bool isContextAttached() const;

    gl::Context getGLContext() const;

private:
    Context(const Context& other); // create new context shared with "other"

    friend class Guard;
    void bindToThread();

    gl::Context context = nullptr;
    ContextMode contextMode;
    bool useDebugFlags;
};
} // namespace snap::rhi::backend::opengl
