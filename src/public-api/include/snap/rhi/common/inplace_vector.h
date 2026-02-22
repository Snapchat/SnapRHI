#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <compare>
#include <cstddef>
#include <initializer_list>
#include <memory>
#include <stdexcept>
#include <type_traits>

namespace snap::rhi::backend::common {
/**
 * @brief A std::vector-like container with fixed capacity and inline storage.
 * * Does not perform dynamic allocation. Elements are stored in-place.
 * Compatible with types that are not default-constructible.
 *
 * TODO: Use std::inplace_vector from C++26 when available.
 */
template<typename T, std::size_t Capacity>
class inplace_vector {
public:
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;
    using iterator = T*;
    using const_iterator = const T*;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    // =========================================================================
    // Constructors & Destructor
    // =========================================================================
    constexpr inplace_vector() noexcept = default;

    constexpr inplace_vector(const inplace_vector& other) {
        for (const auto& item : other) {
            emplace_back(item);
        }
    }

    constexpr inplace_vector(inplace_vector&& other) noexcept(std::is_nothrow_move_constructible_v<T>) {
        for (auto& item : other) {
            emplace_back(std::move(item));
        }
        other.clear();
    }

    constexpr inplace_vector(std::initializer_list<T> list) {
        for (const auto& item : list) {
            push_back(item);
        }
    }

    template<class InputIt>
    constexpr inplace_vector(InputIt first, InputIt last) {
        // Optional: If these are Random Access Iterators (like pointers or vector iterators),
        // we can check the size UP FRONT for a better error message.
        if constexpr (std::random_access_iterator<InputIt>) {
            if (std::distance(first, last) > static_cast<difference_type>(Capacity)) {
                throw std::length_error("InplaceVector: Range exceeds capacity");
            }
        }

        for (; first != last; ++first) {
            if (_size >= Capacity) {
                // If we couldn't check size upfront (e.g. strict input iterator), check here
                // Note: We've already constructed some elements. We must cleanup if we throw.
                clear();
                throw std::length_error("InplaceVector: Range exceeds capacity");
            }
            emplace_back(*first);
        }
    }

    ~inplace_vector() {
        clear();
    }

    constexpr inplace_vector& operator=(const inplace_vector& other) {
        if (this != &other) {
            clear();
            for (const auto& item : other) {
                emplace_back(item);
            }
        }
        return *this;
    }

    constexpr inplace_vector& operator=(inplace_vector&& other) noexcept(std::is_nothrow_move_constructible_v<T>) {
        if (this != &other) {
            clear();
            for (auto& item : other) {
                emplace_back(std::move(item));
            }
            other.clear();
        }
        return *this;
    }

    // =========================================================================
    // Element Access
    // =========================================================================
    [[nodiscard]] constexpr reference operator[](size_type index) noexcept {
        assert(index < _size && "Index out of bounds");
        return *ptr_at(index);
    }

    [[nodiscard]] constexpr const_reference operator[](size_type index) const noexcept {
        assert(index < _size && "Index out of bounds");
        return *ptr_at(index);
    }

    [[nodiscard]] constexpr reference at(size_type index) {
        if (index >= _size)
            throw std::out_of_range("InplaceVector::at");
        return *ptr_at(index);
    }

    [[nodiscard]] constexpr const_reference at(size_type index) const {
        if (index >= _size)
            throw std::out_of_range("InplaceVector::at");
        return *ptr_at(index);
    }

    [[nodiscard]] constexpr reference front() noexcept {
        return *ptr_at(0);
    }
    [[nodiscard]] constexpr const_reference front() const noexcept {
        return *ptr_at(0);
    }
    [[nodiscard]] constexpr reference back() noexcept {
        return *ptr_at(_size - 1);
    }
    [[nodiscard]] constexpr const_reference back() const noexcept {
        return *ptr_at(_size - 1);
    }

    [[nodiscard]] constexpr pointer data() noexcept {
        return ptr_at(0);
    }
    [[nodiscard]] constexpr const_pointer data() const noexcept {
        return ptr_at(0);
    }

    // =========================================================================
    // Iterators
    // =========================================================================

    constexpr iterator begin() noexcept {
        return data();
    }
    constexpr const_iterator begin() const noexcept {
        return data();
    }
    constexpr const_iterator cbegin() const noexcept {
        return data();
    }

    constexpr iterator end() noexcept {
        return data() + _size;
    }
    constexpr const_iterator end() const noexcept {
        return data() + _size;
    }
    constexpr const_iterator cend() const noexcept {
        return data() + _size;
    }

    constexpr reverse_iterator rbegin() noexcept {
        return reverse_iterator(end());
    }
    constexpr const_reverse_iterator rbegin() const noexcept {
        return const_reverse_iterator(end());
    }

    constexpr reverse_iterator rend() noexcept {
        return reverse_iterator(begin());
    }
    constexpr const_reverse_iterator rend() const noexcept {
        return const_reverse_iterator(begin());
    }

    // =========================================================================
    // Capacity
    // =========================================================================

    [[nodiscard]] constexpr bool empty() const noexcept {
        return _size == 0;
    }
    [[nodiscard]] constexpr bool full() const noexcept {
        return _size == Capacity;
    }
    [[nodiscard]] constexpr size_type size() const noexcept {
        return _size;
    }
    [[nodiscard]] constexpr size_type max_size() const noexcept {
        return Capacity;
    }
    [[nodiscard]] constexpr size_type capacity() const noexcept {
        return Capacity;
    }

    // =========================================================================
    // Modifiers
    // =========================================================================

    constexpr void clear() noexcept {
        // Destroy objects in reverse order of creation
        if constexpr (!std::is_trivially_destructible_v<T>) {
            for (size_type i = 0; i < _size; ++i) {
                ptr_at(i)->~T();
            }
        }
        _size = 0;
    }

    constexpr void push_back(const T& value) {
        if (_size >= Capacity)
            throw std::length_error("InplaceVector::push_back: Capacity exceeded");
        // Placement New
        new (static_cast<void*>(ptr_at(_size))) T(value);
        ++_size;
    }

    constexpr void push_back(T&& value) {
        if (_size >= Capacity)
            throw std::length_error("InplaceVector::push_back: Capacity exceeded");
        new (static_cast<void*>(ptr_at(_size))) T(std::move(value));
        ++_size;
    }

    template<typename... Args>
    constexpr reference emplace_back(Args&&... args) {
        if (_size >= Capacity)
            throw std::length_error("InplaceVector::emplace_back: Capacity exceeded");
        T* ptr = ptr_at(_size);
        new (static_cast<void*>(ptr)) T(std::forward<Args>(args)...);
        ++_size;
        return *ptr;
    }

    constexpr void pop_back() {
        assert(_size > 0 && "InplaceVector::pop_back: Vector is empty");
        --_size;
        if constexpr (!std::is_trivially_destructible_v<T>) {
            ptr_at(_size)->~T();
        }
    }

    constexpr void resize(size_type new_size) {
        if (new_size > Capacity)
            throw std::length_error("InplaceVector::resize: Capacity exceeded");

        if (new_size > _size) {
            // Default construct new elements
            for (size_type i = _size; i < new_size; ++i) {
                new (static_cast<void*>(ptr_at(i))) T();
            }
        } else {
            // Destroy excess elements
            for (size_type i = new_size; i < _size; ++i) {
                if constexpr (!std::is_trivially_destructible_v<T>) {
                    ptr_at(i)->~T();
                }
            }
        }
        _size = new_size;
    }

    // =========================================================================
    // Comparison Operators (C++20)
    // =========================================================================

    friend constexpr bool operator==(const inplace_vector& lhs, const inplace_vector& rhs) {
        if (lhs.size() != rhs.size())
            return false;
        return std::equal(lhs.begin(), lhs.end(), rhs.begin());
    }

    friend constexpr auto operator<=>(const inplace_vector& lhs, const inplace_vector& rhs) {
        return std::lexicographical_compare_three_way(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    }

private:
    // Proper alignment for T, raw byte storage
    alignas(T) std::array<std::byte, sizeof(T) * Capacity> _storage;
    size_type _size = 0;

    // Helper to access raw memory as T*
    constexpr T* ptr_at(size_type index) noexcept {
        return reinterpret_cast<T*>(_storage.data()) + index;
    }

    constexpr const T* ptr_at(size_type index) const noexcept {
        return std::launder(reinterpret_cast<const T*>(_storage.data())) + index;
    }
};
} // namespace snap::rhi::backend::common
