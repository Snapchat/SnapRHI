//
//  SemaphoreCreateInfo.h
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 21.09.2020.
//  Copyright © 2021 Snapchat. All rights reserved.
//

#pragma once

namespace snap::rhi {

/**
 * @brief Parameters controlling `snap::rhi::Semaphore` creation.
 *
 * This structure is currently empty because SnapRHI only exposes a single default semaphore type (a binary
 * semaphore suitable for queue submission wait/signal lists).
 *
 * External synchronization:
 * - Importing an external/native sync primitive (when supported) is performed via the `platformSyncHandle` parameter
 *   of `snap::rhi::Device::createSemaphore()` rather than fields in this struct.
 *
 * @note This struct is reserved for future extensions (for example timeline semaphores or additional backend options).
 */
struct SemaphoreCreateInfo {
    // Intentionally empty.
};

} // namespace snap::rhi
