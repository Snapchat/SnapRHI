#pragma once

#include "snap/rhi/DeviceContext.hpp"
#include "snap/rhi/backend/common/Device.hpp"

namespace snap::rhi::backend::common {
class DeviceContextless : public snap::rhi::backend::common::Device {
public:
    explicit DeviceContextless(const snap::rhi::DeviceCreateInfo& info) : snap::rhi::backend::common::Device(info) {}
    ~DeviceContextless() noexcept(false) override = default;

    bool isNativeContextAttached() const override {
        return true;
    }

    snap::rhi::DeviceContext* getMainDeviceContext() final {
        return &contextInstance;
    }

    std::shared_ptr<snap::rhi::DeviceContext> createDeviceContext(
        const snap::rhi::DeviceContextCreateInfo& dcCreateInfo) final {
        return createResource<snap::rhi::DeviceContext, DeviceContextNoOp>(this, dcCreateInfo);
    }

    snap::rhi::DeviceContext::Guard setCurrent(snap::rhi::DeviceContext* ptr) final {
        return contextInstance.makeCurrent();
    }

    snap::rhi::DeviceContext* getCurrentDeviceContext() final {
        return contextInstance.currentRef();
    }

public:
    class DeviceContextNoOp : public snap::rhi::DeviceContext {
    public:
        static inline DeviceContextNoOp*& currentRef() {
            static thread_local DeviceContextNoOp* current = nullptr;
            return current;
        }

        using DeviceContext::DeviceContext;
        snap::rhi::DeviceContext::Guard makeCurrent() {
            auto old = currentRef();
            currentRef() = this;
            return snap::rhi::DeviceContext::Guard([old] { currentRef() = old; });
        }

        bool validateCurrent() const override {
            return true;
        }

        void* getNativeContext() const override {
            return nullptr;
        }

        void clearInternalResources() override {}
    };

private:
    DeviceContextNoOp contextInstance{this, {}};
};
} // namespace snap::rhi::backend::common
