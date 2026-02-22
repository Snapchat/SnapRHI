#pragma once

#include <snap/rhi/common/move_only_function.h>

namespace snap::rhi {

struct Guard {
    using GuardDestroyFn = snap::rhi::common::move_only_function<void()>;
    Guard() : guardDestoyFn(nullptr) {}
    Guard(GuardDestroyFn&& g) : guardDestoyFn(std::move(g)) {}
    Guard(const Guard& other) = delete;
    Guard(Guard&& other) noexcept {
        guardDestoyFn = std::move(other.guardDestoyFn);
        other.guardDestoyFn = nullptr;
    }
    Guard& operator=(Guard&&) = delete;
    ~Guard() noexcept(false) {
        if (guardDestoyFn) {
            guardDestoyFn();
        }
    }

private:
    GuardDestroyFn guardDestoyFn;
};

} // namespace snap::rhi
