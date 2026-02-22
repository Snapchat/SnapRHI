#include "hardware_buffer.h"

#if GLAD_PLATFORM_ANDROID

#include <dlfcn.h>
#include <mutex>
#include <stdio.h>

/*
 * Function pointers
 */

PFNAHARDWAREBUFFER_ALLOCATE sg_glad_AHardwareBuffer_allocate = NULL;
PFNAHARDWAREBUFFER_RELEASE sg_glad_AHardwareBuffer_release = NULL;
PFNAHARDWAREBUFFER_DESCRIBE sg_glad_AHardwareBuffer_describe = NULL;
PFNAHARDWAREBUFFER_LOCK sg_glad_AHardwareBuffer_lock = NULL;
PFNAHARDWAREBUFFER_UNLOCK sg_glad_AHardwareBuffer_unlock = NULL;
PFNAHARDWAREBUFFER_LOCKPLANES sg_glad_AHardwareBuffer_lockPlanes = NULL;

static void* sg_libandroid_handle = NULL;

static void sg_glad_ahardwarebuffer_free(void) {
    if (sg_libandroid_handle != NULL) {
        dlclose(sg_libandroid_handle);
        sg_libandroid_handle = NULL;
    }
}

static int sg_glad_ahardwarebuffer_open_libandroid(void) {
    static std::once_flag init_flag;
    static int result = 0;

    std::call_once(init_flag, []() {
        sg_libandroid_handle = dlopen("libandroid.so", RTLD_LAZY | RTLD_LOCAL);
        if (sg_libandroid_handle != NULL) {
            result = 1;
            atexit(sg_glad_ahardwarebuffer_free);
        } else {
            fprintf(stderr, "[GLAD] Failed to load libandroid.so: %s\n", dlerror());
            result = 0;
        }
    });

    return result;
}

static void* sg_glad_ahardwarebuffer_dlsym(const char* name) {
    if (sg_libandroid_handle == NULL) {
        return NULL;
    }

    void* symbol = dlsym(sg_libandroid_handle, name);
    if (symbol == NULL) {
        fprintf(stderr, "[GLAD] Failed to load symbol %s: %s\n", name, dlerror());
    }

    return symbol;
}

static int sg_gladLoadAHardwareBufferImpl(void) {
    if (!sg_glad_ahardwarebuffer_open_libandroid()) {
        return 0;
    }

    sg_glad_AHardwareBuffer_allocate =
        (PFNAHARDWAREBUFFER_ALLOCATE)sg_glad_ahardwarebuffer_dlsym("AHardwareBuffer_allocate");
    sg_glad_AHardwareBuffer_release =
        (PFNAHARDWAREBUFFER_RELEASE)sg_glad_ahardwarebuffer_dlsym("AHardwareBuffer_release");
    sg_glad_AHardwareBuffer_describe =
        (PFNAHARDWAREBUFFER_DESCRIBE)sg_glad_ahardwarebuffer_dlsym("AHardwareBuffer_describe");
    sg_glad_AHardwareBuffer_lock = (PFNAHARDWAREBUFFER_LOCK)sg_glad_ahardwarebuffer_dlsym("AHardwareBuffer_lock");
    sg_glad_AHardwareBuffer_unlock = (PFNAHARDWAREBUFFER_UNLOCK)sg_glad_ahardwarebuffer_dlsym("AHardwareBuffer_unlock");
    sg_glad_AHardwareBuffer_lockPlanes =
        (PFNAHARDWAREBUFFER_LOCKPLANES)sg_glad_ahardwarebuffer_dlsym("AHardwareBuffer_lockPlanes");

    // Check if critical functions loaded successfully (allocate and release are required)
    if (sg_glad_AHardwareBuffer_allocate == NULL || sg_glad_AHardwareBuffer_release == NULL) {
        return 0;
    }

    return 1;
}

int gladLoadAHardwareBufferSafe(void) {
    static std::once_flag of;
    static int retValue = 0;
    std::call_once(of, [] { retValue = sg_gladLoadAHardwareBufferImpl(); });
    return retValue;
}

#endif /* GLAD_PLATFORM_ANDROID */
