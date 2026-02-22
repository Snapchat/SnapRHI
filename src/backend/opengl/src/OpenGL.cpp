#include "snap/rhi/backend/opengl/OpenGL.h"
#include "snap/rhi/backend/common/Logging.hpp"

#include "snap/rhi/common/StringFormat.h"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <snap/rhi/common/Platform.h>
#include <string>

namespace snap::rhi::backend::opengl {
gl::APIVersion computeVersion(const char* versionString) {
    gl::APIVersion version = gl::APIVersion::None;
    const char* glVersionString =
        versionString ? versionString : reinterpret_cast<char const*>(glGetString(GL_VERSION));
    const bool isES = [glVersionString] {
#if SNAP_RHI_OS_ANDROID() || SNAP_RHI_OS_IOS() || SNAP_RHI_PLATFORM_WEBASSEMBLY()
        return true;
#else
        return strstr(glVersionString, "OpenGL ES") != nullptr;
#endif
    }();

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glGetString.xhtml
    // https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGetString.xhtml
    while (!isdigit(*glVersionString)) {
        glVersionString++;
    }

    int versionMajor = 0, versionMinor = 0;
    sscanf(glVersionString, "%d.%d", &versionMajor, &versionMinor);
    version = static_cast<gl::APIVersion>(versionMajor * 10 + versionMinor +
                                          (isES ? static_cast<int>(gl::APIVersion::GL_ES_START) : 0));

    SNAP_RHI_LOGI("[computeVersion] OpenGL Version string: %s", reinterpret_cast<char const*>(glVersionString));
    SNAP_RHI_LOGI("[computeVersion] SnapRHI GL Version: %d", static_cast<int32_t>(version));
    return version;
}

bool isExtensionSupported(std::string_view extensionsList, std::string_view extName) {
    auto pos = extensionsList.find(extName);
    return pos != std::string::npos &&
           (pos + extName.size() == extensionsList.size() || extensionsList[pos + extName.size()] == ' ');
}

std::string getGLErrorsString() {
    GLenum glError = glGetError();

    std::string message = "";
    constexpr int MaxErrorCheckIterations = 64; // Serves to prevent infinite loop on certain gpus
    int errorCheckIterations = 0;
    for (errorCheckIterations = 0; (errorCheckIterations < MaxErrorCheckIterations) && (glError != GL_NO_ERROR);
         errorCheckIterations++, glError = glGetError()) {
        message +=
            snap::rhi::common::stringFormat("OpenGL error: 0x%x ('%s')", (int)glError, getErrorStr(glError).data());
    }
    if (errorCheckIterations == MaxErrorCheckIterations) {
        SNAP_RHI_LOGE("GLErrorGuard: OpenGL reached max error check iterations, potential deadlock / opengl misuse\n");
    }

    return message;
}
} // namespace snap::rhi::backend::opengl
