#include "snap/rhi/backend/opengl/GLObject.h"

namespace snap::rhi::backend::opengl {
GLObject::GLObject() : valid(true) {}

void GLObject::markInvalid() {
    valid = false;
}

bool GLObject::isValid() const {
    return valid;
}
} // namespace snap::rhi::backend::opengl
