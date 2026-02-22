#include "snap/rhi/backend/opengl/Context.h"

#include "snap/rhi/backend/common/Logging.hpp"

#include "snap/rhi/common/OS.h"
#include "snap/rhi/common/Throw.h"
#include <cstdio>
#include <snap/rhi/common/Scope.h>
#include <snap/rhi/common/StringFormat.h>

namespace snap::rhi::backend::opengl {
Context::Context(gl::APIVersion api, ConstructBehavior behavior, bool useDebugFlags) : useDebugFlags(useDebugFlags) {
    switch (behavior) {
        case ConstructBehavior::UseBoundOrCreateNew: {
            auto currentContext = gl::getActiveContext();
            if (currentContext != nullptr) {
                context = currentContext;
                gl::retainContext(context);
                contextMode = ContextMode::Validate;
                break;
            }
            [[fallthrough]];
        }
        case ConstructBehavior::CreateNewAlways: {
            context = gl::createContext(api, useDebugFlags);
            contextMode = ContextMode::Own;
            break;
        }
    }
}

Context::Context(gl::Context rawContext, ContextMode contextMode, bool useDebugFlags)
    : context(rawContext), contextMode(contextMode), useDebugFlags(useDebugFlags) {
    gl::retainContext(context);
}

Context::Context(const Context& other) : contextMode(ContextMode::Own), useDebugFlags(other.useDebugFlags) {
    context = gl::createSharedContext(other.context, useDebugFlags);
}

Context::~Context() noexcept(false) {
    if (context != nullptr) {
        if (contextMode == ContextMode::Own) {
            gl::destroyContext(context);
        } else {
            gl::releaseContext(context);
        }
    }
}

Context::Guard::Guard(Context& context) : context(context.getGLContext()) {
    recoverContext = gl::getActiveContext();
    recoverSurfaces = gl::getActiveSurfaces();

    if (recoverContext != context.getGLContext()) {
        gl::retainContext(recoverContext);

        context.bindToThread();
    }
}

void Context::Guard::doDestroy() {
    if (auto activeContext = gl::getActiveContext(); activeContext != context && activeContext != nullptr) {
        snap::rhi::common::throwException(snap::rhi::common::stringFormat(
            "[ContextChangeGuard::~ContextChangeGuard] Invalid current context; expected %p real %p",
            context,
            activeContext));
    }

    if (recoverContext != context) {
        // We don't need to unbind here if we're just about to bind new one instead?..
        // Should we try to unbind only if trying to bind a new context failed?

        gl::bindContextWithSurfaces(recoverContext, recoverSurfaces);
        gl::releaseContext(recoverContext);
    }
}

bool Context::isContextAttached() const {
    auto currentContext = gl::getActiveContext();
    return context == currentContext;
}

void Context::bindToThread() {
    if (contextMode == ContextMode::Own || contextMode == ContextMode::Manage) {
        gl::bindContext(context);
    } else {
        if (!isContextAttached()) {
            snap::rhi::common::throwException(snap::rhi::common::stringFormat(
                "[Context::bindToThread] Invalid context attached, current cotext: %p, target context: %p\n",
                gl::getActiveContext(),
                context));
        }
    }
}

gl::Context Context::getGLContext() const {
    return context;
}
} // namespace snap::rhi::backend::opengl
