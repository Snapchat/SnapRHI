//
// Created by Vladyslav Deviatkov on 11/10/25.
//

#pragma once

#include "snap/rhi/DeviceChild.hpp"
#include "snap/rhi/Enums.h"
#include "snap/rhi/common/NonCopyable.h"

namespace snap::rhi {

/**
 * @brief Opaque, platform-dependent synchronization handle.
 *
 * PlatformSyncHandle is used to transfer GPU/OS synchronization objects across API or process boundaries in an
 * API-agnostic way.
 *
 * Typical use cases:
 * - Exporting a fence from one backend and importing it when creating another synchronization primitive.
 * - Bridging backend-specific external synchronization representations (for example, an OS handle on platforms that
 *   support exporting native sync objects).
 *
 * The concrete type of the handle is backend- and platform-specific.
 */
class PlatformSyncHandle : public snap::rhi::common::NonCopyable {
public:
    virtual ~PlatformSyncHandle() = default;

protected:
    /**
     * @brief Protected default constructor.
     *
     * This is a non-instantiable base; only backend/platform implementations can construct derived types.
     */
    PlatformSyncHandle() = default;
};
} // namespace snap::rhi
