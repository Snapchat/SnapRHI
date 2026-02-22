#pragma once

// clang-format off

#include <utility> // for std::forward
#include <exception>
#include <type_traits>

namespace snap::rhi::common {

namespace Detail {

/// \brief Components for scope guards
///
/// The three components here are a simplification and slightly more strict versions of the scope guards
/// first implemented by Alexandrescu and in the process of becoming standard.
///
/// Their purpose is to emulate the functionality of \c finally in other programming languages, code
/// that is executed regardless of how the control flow escapes a code block (ScopeExit), whenever
/// a code block is completed successfully (ScopeSuccess) or as a mechanism to "undo" things when
/// a code block is exited due to a failure (an exception is thrown).

template<typename Callable>
struct ScopeGuard {
    Callable callable_;

    ScopeGuard() = delete;
    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard(ScopeGuard&&) = delete;

    template<
        typename T,
        typename =
            std::enable_if_t<
                std::is_nothrow_destructible_v<
                    std::remove_cv_t<std::remove_reference_t<T>
                >
            >
        >
    >
    // note: the noexceptness of the construction of these guards
    // is a moot point since they can only occur in scopes
    ScopeGuard(T&& t) : callable_(std::forward<T>(t)) {}

    ~ScopeGuard() {}

protected:
    template<typename... Args>
    ScopeGuard(Args&&... args) : callable_(std::forward<Args>(args)...) {}
};

template<typename T>
ScopeGuard(T &&) ->
    ScopeGuard<std::remove_cv_t<std::remove_reference_t<T>>>;

}

/// \brief close equivalent to the upcoming std::scope_exit
///
/// https://en.cppreference.com/w/cpp/experimental/scope_exit
/// This component must not be allocated dynamically, it should be used only within
/// scopes.
template<typename Callable>
struct ScopeGuard : Detail::ScopeGuard<Callable> {
    using Detail::ScopeGuard<Callable>::ScopeGuard;

    ~ScopeGuard() noexcept(std::is_nothrow_invocable_r<void, Callable>::value) {
        this->callable_();
    }

private:
    template<typename... Args>
    friend ScopeGuard makeGuard(Args&&...);
};
template<typename T>
ScopeGuard(T &&)->ScopeGuard<std::remove_cv_t<std::remove_reference_t<T>>>;

template<typename T, typename... Args>
// note: this relies on return value copy elision since ScopeGuard is not
// movable or copiable
ScopeGuard<T> makeGuard(Args&&... args) {
    return ScopeGuard<T>(std::forward<Args>(args)...);
}

/// \brief ScopeSuccess Stricter version of std::scope_success
///
/// https://en.cppreference.com/w/cpp/experimental/scope_success
/// This component is not movable, and does not have a release mechanism.
/// The intention is for the component to not mislead the reader or force them to prove "release" is not
/// called because the user could trivially include a boolean flag in their callable to deactivate the clean up.
/// Also, the name "release" generally means to destroy resources, not their deactivation.
///
/// This component should not be used in destructors because of the technicalities involved in handling
/// exceptions unwound while the object being destroyed might be unwinding itself
template<typename Callable>
struct ScopeSuccess : Detail::ScopeGuard<Callable> {
    using Detail::ScopeGuard<Callable>::ScopeGuard;

    ~ScopeSuccess() noexcept(std::is_nothrow_invocable_r<void, Callable>::value) {
        if (std::uncaught_exceptions() <= priorUncaughtExceptions_) {
            this->callable_();
        }
    }

private:
    int priorUncaughtExceptions_ = std::uncaught_exceptions();
};
template<typename T>
ScopeSuccess(T &&)->
    ScopeSuccess<std::remove_cv_t<std::remove_reference_t<T>>>;

/// \brief Stricter version of scope_failure, refer to ScopeSuccess above
///
/// https://en.cppreference.com/w/cpp/experimental/scope_fail
/// \note The callable must be noexcept to prevent unwinding of the exception that
/// activates the callable and what the callable could throw
template<typename Callable>
struct ScopeFailure : Detail::ScopeGuard<Callable> {
    template<
        typename T,
        typename = std::enable_if_t<
            std::is_nothrow_destructible_v<
                std::remove_cv_t<std::remove_reference_t<T>>
            > && std::is_nothrow_invocable_r_v<void, T>
        >
    >
    ScopeFailure(T&& t):
        Detail::ScopeGuard<Callable>(std::forward<T>(t))
    {}

    ~ScopeFailure() {
        if (priorUncaughtExceptions_ < std::uncaught_exceptions()) {
            this->callable_();
        }
    }

private:
    int priorUncaughtExceptions_ = std::uncaught_exceptions();
};
template<typename T>
ScopeFailure(T&&)->
    ScopeFailure<std::remove_cv_t<std::remove_reference_t<T>>>;

}
