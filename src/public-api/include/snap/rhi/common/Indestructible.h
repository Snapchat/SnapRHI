#pragma once

#include <utility>

namespace snap::rhi::common {
// Wrapper that constructs an object but NEVER calls its destructor.
template<typename T>
class Indestructible {
public:
    // 1. Constructor: Forwards all arguments to T's constructor.
    // We use placement new syntax implicitly via the union member.
    template<typename... Args>
    constexpr explicit Indestructible(Args&&... args) : value(std::forward<Args>(args)...) {}

    // 2. Destructor: Does NOTHING.
    // Because 'value' is in a union, the compiler does not know which member is active,
    // so it will NOT generate a call to T::~T() automatically.
    ~Indestructible() {}

    // 3. Accessors: transparent access to the underlying object.
    T& operator*() {
        return value;
    }
    const T& operator*() const {
        return value;
    }
    T* operator->() {
        return &value;
    }
    const T* operator->() const {
        return &value;
    }

private:
    // Using an anonymous union prevents the compiler from automatically
    // destroying 'value' when the Indestructible object goes out of scope.
    union {
        T value;
    };
};
} // namespace snap::rhi::common
