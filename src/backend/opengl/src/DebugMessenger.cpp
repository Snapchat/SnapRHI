#include "snap/rhi/backend/opengl/DebugMessenger.h"
#include "snap/rhi/backend/opengl/Device.hpp"
#include <snap/rhi/backend/opengl/Profile.hpp>

namespace snap::rhi::backend::opengl {
DebugMessenger::DebugMessenger(snap::rhi::backend::opengl::Device* device, snap::rhi::DebugMessengerCreateInfo&& info)
    : snap::rhi::DebugMessenger(device, std::move(info)), debugMessageHandler(*device->getDebugMessageHandler()) {
    if (!static_cast<bool>(device->getDeviceCreateInfo().deviceCreateFlags &
                           snap::rhi::DeviceCreateFlags::EnableDebugCallback)) {
        return;
    }

    /**
     * OpenGL only allows one callback to be registered,
     * so all callbacks must be stored in a holder and executed through one place.
     *
     * https://www.khronos.org/opengl/wiki/Debug_Output
     * */
    debugMessageHandler.addDebugMessenger(this);
}

DebugMessenger::~DebugMessenger() {
    if (!static_cast<bool>(device->getDeviceCreateInfo().deviceCreateFlags &
                           snap::rhi::DeviceCreateFlags::EnableDebugCallback)) {
        return;
    }

    debugMessageHandler.removeDebugMessenger(this);
}

void DebugMessenger::report(const snap::rhi::DebugCallbackInfo& info) {
    this->info.debugMessengerCallback(info);
}
} // namespace snap::rhi::backend::opengl
