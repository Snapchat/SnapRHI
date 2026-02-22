#pragma once

#include "snap/rhi/Enums.h"

#include <functional>
#include <snap/rhi/common/Scope.h>

namespace snap::rhi::backend::opengl {
class Device;

class ErrorCheckGuard final {
public:
    ErrorCheckGuard(snap::rhi::backend::opengl::Device* glDevice, bool isEnabled, std::function<void()>&& callback);
    ~ErrorCheckGuard() noexcept(false);

private:
    snap::rhi::backend::opengl::Device* device = nullptr;
    std::function<void()> onErrorCallback;
};
} // namespace snap::rhi::backend::opengl
