#pragma once

#include "snap/rhi/DebugMessenger.h"
#include "snap/rhi/Guard.h"

namespace snap::rhi::backend::opengl {
class Device;
class DebugMessageHandler;

class DebugMessenger final : public snap::rhi::DebugMessenger {
public:
    DebugMessenger(snap::rhi::backend::opengl::Device* device, snap::rhi::DebugMessengerCreateInfo&& info);
    ~DebugMessenger() override;

    void report(const snap::rhi::DebugCallbackInfo& info);

private:
    snap::rhi::backend::opengl::DebugMessageHandler& debugMessageHandler;
};
} // namespace snap::rhi::backend::opengl
