#pragma once

#include "snap/rhi/common/Inlining.h"
#include "snap/rhi/common/Platform.h"

#include <stdexcept>
#include <type_traits>
#include <utility>

namespace snap::rhi::common {

#if SNAP_RHI_PLATFORM_WEBASSEMBLY()
// setjmp/longjmp on webassembly currently works by throwing an exception
// and so if we make the function noexcept, the exception cannot exit the
// function. See https://emscripten.org/docs/porting/setjmp-longjmp.html
// for details.
#define SNAP_RHI_ERROR_CONTINUATION_NOEXCEPT()
#else
#define SNAP_RHI_ERROR_CONTINUATION_NOEXCEPT() noexcept
#endif

// https://gcc.gnu.org/onlinedocs/libstdc++/manual/using_exceptions.html
// Note, for MSVC we don't have this flag.
#if defined(__cpp_exceptions) || defined(__EXCEPTIONS)
#define SNAP_RHI_CONFIG_USE_EXCEPTIONS() 1
#else
#define SNAP_RHI_CONFIG_USE_EXCEPTIONS() 0
#endif

using ErrorContinuation = void (*)(std::exception&&) SNAP_RHI_ERROR_CONTINUATION_NOEXCEPT();
void setErrorContinuation(ErrorContinuation) noexcept;
ErrorContinuation getErrorContinuation() noexcept;

namespace Detail {

/// \brief overload of last resort to indicate conversion
/// Here, we rely on the fact that variadic functions are the last ones
/// considered in overload resolution. Hence, the additional overloads we have
/// below for char[] would cover all value categories for the type.
/// The alternative of using type traits for the same would require careful
/// specializations to cover all value categories and is thus error-prone.
/// This is why we use a function instead of type trait.
std::false_type isConvertedForExceptionThrowing(...);

template<std::size_t L>
std::true_type isConvertedForExceptionThrowing(const char (&)[L]);

template<typename T>
constexpr auto convertedForExceptionThrowing() {
    return decltype(isConvertedForExceptionThrowing(std::declval<T>()))::value;
}

template<typename T>
struct PassByValueToBuildException {
    static constexpr bool value = std::is_trivial_v<std::remove_reference_t<T>>;
};

template<typename T>
inline constexpr bool PassByValueToBuildException_v = PassByValueToBuildException<T>::value;

// The following piece of code is inspired from
// https://github.com/facebook/folly/blob/master/folly/lang/Exception.h#L54
// clang-format off
template<typename T>
constexpr auto toExceptionArg(T&& val) ->
    std::enable_if_t<
        !convertedForExceptionThrowing<T>() && !PassByValueToBuildException_v<T>,
        T&&
    >
{ return std::forward<T>(val); }

template<typename T>
constexpr auto toExceptionArg(T&& val) ->
    std::enable_if_t<
        !convertedForExceptionThrowing<T>() && PassByValueToBuildException_v<T>,
        std::remove_reference_t<T>
    >
{ return val; }
// clang-format on

template<std::size_t N>
constexpr auto toExceptionArg(char const (&array)[N]) {
    return array;
}

template<typename Ex, typename... Args>
[[noreturn]] SNAP_RHI_NO_INLINE void throwExceptionBackend(Args... args) {
#if SNAP_RHI_CONFIG_USE_EXCEPTIONS()
    throw Ex(std::forward<Args>(args)...);
#else
    getErrorContinuation()(Ex(std::forward<Args>(args)...));
    // because function pointers can not have the attribute [[noreturn]]
    // we have to indicate somehow this function does not return
    // calling "abort" because of that
    abort();
#endif
}

}; // namespace Detail

struct Exception : std::runtime_error {
    using std::runtime_error::runtime_error;
};

// clang-format off
template<class Ex = Exception, class... Args>
[[noreturn]] SNAP_RHI_ALWAYS_INLINE auto throwException(Args&&... args) ->
    std::enable_if_t<std::is_base_of_v<std::exception, Ex>>
{
    Detail::throwExceptionBackend<
        Ex,
        decltype(Detail::toExceptionArg(std::forward<Args>(args)))...
    >(Detail::toExceptionArg(std::forward<Args>(args))...);
}

struct BadAllocation: Exception {
    using Exception::Exception;
};
} // namespace snap::rhi::common
