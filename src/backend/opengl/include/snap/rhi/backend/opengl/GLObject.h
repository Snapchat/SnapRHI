#pragma once

#include "snap/rhi/backend/opengl/OpenGL.h"
#include <snap/rhi/common/Platform.h>

namespace snap::rhi::backend::opengl {
class GLObject final {
public:
    explicit GLObject();
    ~GLObject() = default;

    void markInvalid();
    bool isValid() const;

private:
    bool valid = true;
};
} // namespace snap::rhi::backend::opengl
