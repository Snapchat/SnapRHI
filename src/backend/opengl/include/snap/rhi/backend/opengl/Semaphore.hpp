//
//  Semaphore.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 12.10.2021.
//

#pragma once

#include "snap/rhi/Semaphore.hpp"
#include "snap/rhi/backend/opengl/OpenGL.h"

#include <condition_variable>
#include <future>
#include <mutex>

namespace snap::rhi::backend::opengl {
class Device;
class Profile;

/**
 * VUID-vkQueueSubmit-pSignalSemaphores-00067
 * Each binary semaphore element of the pSignalSemaphores member of any element of pSubmits must be unsignaled when the
 semaphore signal operation it defines is executed on the device

 * VUID-vkQueueSubmit-pWaitSemaphores-00068
 * When a semaphore wait operation referring to a binary semaphore defined by any element of the pWaitSemaphores member
 of any element of pSubmits executes on queue, there must be no other queues waiting on the same semaphore
 *
 * VUID-vkQueueSubmit-pWaitSemaphores-03238
 * All elements of the pWaitSemaphores member of all elements of pSubmits created with a VkSemaphoreType of
 VK_SEMAPHORE_TYPE_BINARY must reference a semaphore signal operation that has been submitted for execution and any
 semaphore signal operations on which it depends must have also been submitted for execution
 */
class Semaphore : public snap::rhi::Semaphore {
public:
    explicit Semaphore(Device* device, const SemaphoreCreateInfo& info);
    ~Semaphore() override;

    void setDebugLabel(std::string_view label) override;

    /**
     *  Don't forget glFlush() https://www.khronos.org/opengl/wiki/Sync_Object
     */
    void signal();
    void wait();

protected:
    void init();

    const Profile& gl;
    SNAP_RHI_GLsync fence = nullptr;

    std::mutex signalMutex;
    std::condition_variable cv;
};
} // namespace snap::rhi::backend::opengl
