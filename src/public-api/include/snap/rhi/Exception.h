#pragma once

#include "snap/rhi/common/Throw.h"

namespace snap::rhi {
/// \brief Serves as a base class for all runtime exceptions thrown from SnapRHI and
///        can be used to filter out SnapRHI exceptions caught by client code.
class Exception : public snap::rhi::common::Exception {
    using base_type = snap::rhi::common::Exception;
    using base_type::base_type;
};

/// \brief Thrown to indicate that the requested operation is not supported.
class UnsupportedOperationException : public Exception {
    using Exception::Exception;
};

/// \brief Thrown to indicate that the argument did not pass validation.
class InvalidArgumentException : public Exception {
    using Exception::Exception;
};

/// \brief Thrown to indicate that no \c DeviceContext was bound to \c Device during command execution.
class NoBoundDeviceContextException : public Exception {
    using Exception::Exception;
};

/// \brief Thrown to indicate that \c CommandAllocator returned a command with an unknown command type.
class UnexpectedCommandException : public Exception {
    using Exception::Exception;
};

} // namespace snap::rhi
