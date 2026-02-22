#include "snap/rhi/backend/opengl/CmdPerformer.hpp"
#include "snap/rhi/backend/opengl/Device.hpp"

namespace snap::rhi::backend::opengl {
CmdPerformer::CmdPerformer(snap::rhi::backend::opengl::Device* device)
    : device(device), gl(device->getOpenGL()), validationLayer(device->getValidationLayer()) {}

void CmdPerformer::enableCache(DeviceContext* dc) {
    this->dc = dc;
}

void CmdPerformer::disableCache() {
    this->dc = nullptr;
}
} // namespace snap::rhi::backend::opengl
