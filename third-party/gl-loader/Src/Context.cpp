#include <OpenGL/Context.h>

#include <functional>
#include <mutex>
#include <stdexcept>

namespace {

gl::LoadAPIHooks glHooks;

// RAII scope guard for exception-safe cleanup
class ScopeGuard {
public:
    explicit ScopeGuard(std::function<void()> onExit) : onExit_(std::move(onExit)) {}
    ~ScopeGuard() {
        if (onExit_) {
            onExit_();
        }
    }

    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;
    ScopeGuard(ScopeGuard&&) = delete;
    ScopeGuard& operator=(ScopeGuard&&) = delete;

private:
    std::function<void()> onExit_;
};

} // namespace

namespace gl {

void installLoadAPIHooks(const LoadAPIHooks& hooks) {
    if (glHooks.preHook != nullptr || glHooks.inHook != nullptr || glHooks.postHook != nullptr) {
        throw std::runtime_error("OpenGL initialization hooks were already installed");
    }
    glHooks = hooks;
}

void loadAPIWithHooks() {
    static std::once_flag of;
    std::call_once(of, [] {
        if (glHooks.preHook != nullptr) {
            glHooks.preHook();
        }

        ScopeGuard postHookGuard([] {
            if (glHooks.postHook != nullptr) {
                glHooks.postHook();
            }
        });

        loadAPI();

        if (glHooks.inHook != nullptr) {
            glHooks.inHook();
        }
    });
}

} // namespace gl
