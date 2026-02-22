#include "egl.h"
#include <mutex>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef GLAD_IMPL_UTIL_C_
#define GLAD_IMPL_UTIL_C_

#ifdef _MSC_VER
#define GLAD_IMPL_UTIL_SSCANF sscanf_s
#else
#define GLAD_IMPL_UTIL_SSCANF sscanf
#endif

#endif /* GLAD_IMPL_UTIL_C_ */

#if (GLAD_PLATFORM_ANDROID || GLAD_PLATFORM_EMSCRIPTEN)

int SNAP_RHI_GLAD_EGL_ANDROID_get_native_client_buffer = 0;
int SNAP_RHI_GLAD_EGL_ANDROID_native_fence_sync = 0;
int SNAP_RHI_GLAD_EGL_KHR_image_base = 0;

PFNEGLGETNATIVECLIENTBUFFERANDROIDPROC snap_rhi_glad_eglGetNativeClientBufferANDROID = NULL;
PFNEGLDUPNATIVEFENCEFDANDROIDPROC snap_rhi_glad_eglDupNativeFenceFDANDROID = NULL;
PFNEGLCREATEIMAGEKHRPROC snap_rhi_glad_eglCreateImageKHR = NULL;
PFNEGLDESTROYIMAGEKHRPROC snap_rhi_glad_eglDestroyImageKHR = NULL;

static void snap_rhi_glad_egl_load_EGL_ANDROID_get_native_client_buffer() {
    if (!SNAP_RHI_GLAD_EGL_ANDROID_get_native_client_buffer)
        return;
    snap_rhi_glad_eglGetNativeClientBufferANDROID =
        (PFNEGLGETNATIVECLIENTBUFFERANDROIDPROC)eglGetProcAddress("eglGetNativeClientBufferANDROID");
}

static void snap_rhi_glad_egl_load_EGL_ANDROID_native_fence_sync() {
    if (!SNAP_RHI_GLAD_EGL_ANDROID_native_fence_sync)
        return;
    snap_rhi_glad_eglDupNativeFenceFDANDROID =
        (PFNEGLDUPNATIVEFENCEFDANDROIDPROC)eglGetProcAddress("eglDupNativeFenceFDANDROID");
}

static void snap_rhi_glad_egl_load_EGL_KHR_image_base() {
    if (!SNAP_RHI_GLAD_EGL_KHR_image_base)
        return;
    snap_rhi_glad_eglCreateImageKHR = (PFNEGLCREATEIMAGEKHRPROC)eglGetProcAddress("eglCreateImageKHR");
    snap_rhi_glad_eglDestroyImageKHR = (PFNEGLDESTROYIMAGEKHRPROC)eglGetProcAddress("eglDestroyImageKHR");
}

static int snap_rhi_glad_egl_get_extensions(EGLDisplay display, const char** extensions) {
    *extensions = eglQueryString(display, EGL_EXTENSIONS);

    return extensions != NULL;
}

static int snap_rhi_glad_egl_has_extension(const char* extensions, const char* ext) {
    const char* loc;
    const char* terminator;
    if (extensions == NULL) {
        return 0;
    }
    while (1) {
        loc = strstr(extensions, ext);
        if (loc == NULL) {
            return 0;
        }
        terminator = loc + strlen(ext);
        if ((loc == extensions || *(loc - 1) == ' ') && (*terminator == ' ' || *terminator == '\0')) {
            return 1;
        }
        extensions = terminator;
    }
}

static int snap_rhi_glad_egl_find_extensions_egl(EGLDisplay display) {
    const char* extensions;
    if (!snap_rhi_glad_egl_get_extensions(display, &extensions))
        return 0;

    SNAP_RHI_GLAD_EGL_ANDROID_get_native_client_buffer =
        snap_rhi_glad_egl_has_extension(extensions, "EGL_ANDROID_get_native_client_buffer");
    SNAP_RHI_GLAD_EGL_ANDROID_native_fence_sync = snap_rhi_glad_egl_has_extension(extensions, "EGL_ANDROID_native_fence_sync");
    SNAP_RHI_GLAD_EGL_KHR_image_base = snap_rhi_glad_egl_has_extension(extensions, "EGL_KHR_image_base");

    return 1;
}

int gladLoadEGLExtensions(EGLDisplay display) {
    if (!snap_rhi_glad_egl_find_extensions_egl(display))
        return 0;
    snap_rhi_glad_egl_load_EGL_ANDROID_get_native_client_buffer();
    snap_rhi_glad_egl_load_EGL_ANDROID_native_fence_sync();
    snap_rhi_glad_egl_load_EGL_KHR_image_base();

    return 1;
}

int gladLoadEGLExtensionsSafe(EGLDisplay display) {
    static std::once_flag of;
    static int retValue = 0;
    std::call_once(of, [&] { retValue = gladLoadEGLExtensions(display); });
    return retValue;
}

#endif //(GLAD_PLATFORM_ANDROID || GLAD_PLATFORM_EMSCRIPTEN)
