#include "snap/rhi/backend/opengl/DeviceFactory.h"
#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/opengl/Device.hpp"

namespace snap::rhi::backend::opengl {
std::shared_ptr<snap::rhi::Device> createDevice(const snap::rhi::backend::opengl::DeviceCreateInfo& info) {
    return snap::rhi::backend::common::createDeviceSafe(std::make_shared<snap::rhi::backend::opengl::Device>(info));
}
} // namespace snap::rhi::backend::opengl
