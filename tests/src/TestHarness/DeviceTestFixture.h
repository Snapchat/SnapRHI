//
//  DeviceTestFixture.h
//  unitest-catch2
//
//  Multi-backend device creation helper for Catch2 tests.
//  Copyright © 2026 Snapchat. All rights reserved.
//

#pragma once

#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "snap/rhi/Device.hpp"
#include "snap/rhi/DeviceContext.hpp"
#include "snap/rhi/CommandQueue.hpp"
#include "snap/rhi/backend/common/Logging.hpp"


#if SNAP_RHI_ENABLE_BACKEND_OPENGL
#include <snap/rhi/backend/opengl/DeviceFactory.h>
#endif

#if SNAP_RHI_ENABLE_BACKEND_METAL
#include <snap/rhi/backend/metal/DeviceFactory.h>
#endif

#if SNAP_RHI_ENABLE_BACKEND_VULKAN
#include <snap/rhi/backend/vulkan/DeviceFactory.h>
#endif

#include <snap/rhi/common/OS.h>

/**
 * @brief Convenience macro: logs the current test case + API via SNAP_RHI_LOGI (once per
 *        unique test+API pair), then opens a DYNAMIC_SECTION.
 *
 * Usage (replaces DYNAMIC_SECTION("Backend: " << ctx->apiName)):
 *   SNAP_RHI_TEST_BACKEND_SECTION(ctx->apiName) { ... }
 *
 * Output example:
 *   [I][SnapRHI] [Test] ▶ Buffer — Creation and lifecycle  [Metal]
 */
#define SNAP_RHI_TEST_BACKEND_SECTION(apiNameExpr)                                                         \
    do {                                                                                                   \
        static std::set<std::string> _logged;                                                              \
        auto _key = Catch::getResultCapture().getCurrentTestName() + "|" + std::string(apiNameExpr);       \
        if (_logged.insert(_key).second) {                                                                 \
            SNAP_RHI_LOGI("[Test] ▶ %s  [%s]",                                                            \
                          Catch::getResultCapture().getCurrentTestName().c_str(),                           \
                          std::string(apiNameExpr).c_str());                                               \
        }                                                                                                  \
    } while (false);                                                                                       \
    DYNAMIC_SECTION("Backend: " << (apiNameExpr))

namespace test_harness {

/**
 * @brief Holds all state needed for a backend-specific test.
 */
struct DeviceTestContext {
    std::shared_ptr<snap::rhi::Device> device;
    snap::rhi::CommandQueue* commandQueue = nullptr;
    snap::rhi::API api = snap::rhi::API::Undefined;
    std::string apiName;

private:
    // Keep device context alive via shared_ptr
    std::shared_ptr<snap::rhi::DeviceContext> ownedDC;
    std::unique_ptr<snap::rhi::DeviceContext::Guard> guard;
    friend std::optional<DeviceTestContext> createTestContext(snap::rhi::API api);
};

/**
 * @brief Returns a human-readable name for a given API.
 */
inline std::string getAPIName(snap::rhi::API api) {
    switch (api) {
        case snap::rhi::API::OpenGL: return "OpenGL";
        case snap::rhi::API::Metal: return "Metal";
        case snap::rhi::API::Vulkan: return "Vulkan";
        default: return "Unknown";
    }
}

/**
 * @brief Returns API enum values for all backends enabled at compile time.
 */
inline std::vector<snap::rhi::API> getAvailableAPIs() {
    std::vector<snap::rhi::API> apis;

#if SNAP_RHI_ENABLE_BACKEND_OPENGL
    apis.push_back(snap::rhi::API::OpenGL);
#endif

#if SNAP_RHI_ENABLE_BACKEND_METAL
    apis.push_back(snap::rhi::API::Metal);
#endif

#if SNAP_RHI_ENABLE_BACKEND_VULKAN
    apis.push_back(snap::rhi::API::Vulkan);
#endif

    return apis;
}

/**
 * @brief Creates a device for the specified API and returns a fully initialized test context.
 * @return A valid context, or nullopt if the backend is not available/supported on this platform.
 */
inline std::optional<DeviceTestContext> createTestContext(snap::rhi::API api) {
    const auto name = getAPIName(api);

    snap::rhi::DeviceCreateInfo info{};
    info.enabledReportLevel = snap::rhi::ReportLevel::All;
    info.enabledTags = snap::rhi::ValidationTag::All;

    std::shared_ptr<snap::rhi::Device> device;

    if (api == snap::rhi::API::OpenGL) {
#if SNAP_RHI_ENABLE_BACKEND_OPENGL
        try {
            snap::rhi::backend::opengl::DeviceCreateInfo glInfo{info};
#if SNAP_RHI_OS_MACOS() || SNAP_RHI_OS_WINDOWS() || SNAP_RHI_OS_LINUX_BASED()
            glInfo.apiVersion = gl::APIVersion::GL41;
#else
            glInfo.apiVersion = gl::APIVersion::GLES30;
#endif
            glInfo.dcCreateInfo.isUserTheOwnerOfGLContext = false;
            device = snap::rhi::backend::opengl::createDevice(glInfo);
        } catch (...) {
            SNAP_RHI_LOGE("[Test] Failed to create %s device (exception)", name.c_str());
            return std::nullopt;
        }
#else
        SNAP_RHI_LOGW("[Test] %s backend not compiled in", name.c_str());
        return std::nullopt;
#endif
    } else if (api == snap::rhi::API::Metal) {
#if SNAP_RHI_ENABLE_BACKEND_METAL
        try {
            snap::rhi::backend::metal::DeviceCreateInfo mtlInfo{info};
            device = snap::rhi::backend::metal::createDevice(mtlInfo);
        } catch (...) {
            SNAP_RHI_LOGE("[Test] Failed to create %s device (exception)", name.c_str());
            return std::nullopt;
        }
#else
        SNAP_RHI_LOGW("[Test] %s backend not compiled in", name.c_str());
        return std::nullopt;
#endif
    } else if (api == snap::rhi::API::Vulkan) {
#if SNAP_RHI_ENABLE_BACKEND_VULKAN
        try {
            snap::rhi::backend::vulkan::DeviceCreateInfo vkInfo{info};
            device = snap::rhi::backend::vulkan::createDevice(vkInfo);
        } catch (...) {
            SNAP_RHI_LOGE("[Test] Failed to create %s device (exception)", name.c_str());
            return std::nullopt;
        }
#else
        SNAP_RHI_LOGW("[Test] %s backend not compiled in", name.c_str());
        return std::nullopt;
#endif
    } else {
        return std::nullopt;
    }

    if (!device) {
        SNAP_RHI_LOGE("[Test] %s device creation returned null", name.c_str());
        return std::nullopt;
    }

    DeviceTestContext ctx;
    ctx.device = device;
    ctx.api = api;
    ctx.apiName = getAPIName(api);
    ctx.commandQueue = device->getCommandQueue(0, 0);

    auto* mainDC = device->getMainDeviceContext();
    if (mainDC) {
        ctx.guard = std::make_unique<snap::rhi::DeviceContext::Guard>(device->setCurrent(mainDC));
    } else {
        snap::rhi::DeviceContextCreateInfo dcInfo{};
        ctx.ownedDC = device->createDeviceContext(dcInfo);
        if (ctx.ownedDC) {
            ctx.guard = std::make_unique<snap::rhi::DeviceContext::Guard>(device->setCurrent(ctx.ownedDC.get()));
        }
    }

    return ctx;
}


} // namespace test_harness
