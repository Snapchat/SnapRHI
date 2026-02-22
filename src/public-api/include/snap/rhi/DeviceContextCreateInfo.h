//
//  DeviceContextCreateInfo.h
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 21.09.2020.
//  Copyright © 2021 Snapchat. All rights reserved.
//

#pragma once

namespace snap::rhi {
/**
 * @brief Parameters controlling `snap::rhi::DeviceContext` creation.
 */
struct DeviceContextCreateInfo {
    /**
     * @brief Allows the backend to allocate additional context-dependent internal resources.
     *
     * When enabled, SnapRHI may allocate extra caches/pools that can improve performance but increase memory
     * usage.
     *
     * Backend notes:
     * - OpenGL: allows enabling certain context-scoped caches (for example framebuffer pooling, depending on device
     *   settings).
     * - Internal/background contexts created by SnapRHI may keep this disabled to avoid unbounded growth when a
     *   dedicated clean-up path is not available.
     */
    bool internalResourcesAllowed = false;
};
} // namespace snap::rhi
