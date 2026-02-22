#pragma once

#include "snap/rhi/Structs.h"
#include "snap/rhi/backend/opengl/OpenGL.h"
#include "snap/rhi/common/NonCopyable.h"
#include <mutex>
#include <set>

namespace snap::rhi::backend::opengl {
class Device;
class DebugMessenger;
class Profile;

// https://www.khronos.org/opengl/wiki/Debug_Output
class DebugMessageHandler final : public snap::rhi::common::NonCopyable {
public:
    explicit DebugMessageHandler(snap::rhi::backend::opengl::Device* device);
    ~DebugMessageHandler();

    void addDebugMessenger(DebugMessenger* debugMessenger);
    void removeDebugMessenger(DebugMessenger* debugMessenger);

    void report(const snap::rhi::DebugCallbackInfo& info) const;

private:
    snap::rhi::backend::opengl::Device* device = nullptr;
    snap::rhi::backend::opengl::Profile& gl;

    bool debugMessageEnabled = false;
    std::set<DebugMessenger*> messengers;
    mutable std::mutex accessMutex;
};
} // namespace snap::rhi::backend::opengl
