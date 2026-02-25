/**
 * Loader for Android Hardware Buffer APIs
 *
 * Generator: Manual
 * APIs:
 *  - android.hardware_buffer
 *
 */

#pragma once

#include <GLAD/gladPlatform.h>

#if GLAD_PLATFORM_ANDROID

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * AHardwareBuffer structures and types
 */

struct AHardwareBuffer;
struct ARect;

typedef struct AHardwareBuffer_Desc {
    uint32_t width;
    uint32_t height;
    uint32_t layers;
    uint32_t format;
    uint64_t usage;
    uint32_t stride;
    uint32_t rfu0;
    uint64_t rfu1;
} AHardwareBuffer_Desc;

typedef struct AHardwareBuffer_Plane {
    void* data;
    uint32_t pixelStride;
    uint32_t rowStride;
} AHardwareBuffer_Plane;

typedef struct AHardwareBuffer_Planes {
    uint32_t planeCount;
    AHardwareBuffer_Plane planes[4];
} AHardwareBuffer_Planes;

/*
 * AHardwareBuffer format constants
 */

#define AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM 1
#define AHARDWAREBUFFER_FORMAT_R8G8B8X8_UNORM 2
#define AHARDWAREBUFFER_FORMAT_R8G8B8_UNORM 3
#define AHARDWAREBUFFER_FORMAT_R5G6B5_UNORM 4
#define AHARDWAREBUFFER_FORMAT_B8G8R8A8_UNORM 5
#define AHARDWAREBUFFER_FORMAT_B5G5R5A1_UNORM 6
#define AHARDWAREBUFFER_FORMAT_B4G4R4A4_UNORM 7
#define AHARDWAREBUFFER_FORMAT_R16G16B16A16_FLOAT 22
#define AHARDWAREBUFFER_FORMAT_R10G10B10A2_UNORM 43
#define AHARDWAREBUFFER_FORMAT_BLOB 33
#define AHARDWAREBUFFER_FORMAT_D16_UNORM 48
#define AHARDWAREBUFFER_FORMAT_D24_UNORM 49
#define AHARDWAREBUFFER_FORMAT_D24_UNORM_S8_UINT 50
#define AHARDWAREBUFFER_FORMAT_D32_FLOAT 51
#define AHARDWAREBUFFER_FORMAT_D32_FLOAT_S8_UINT 52
#define AHARDWAREBUFFER_FORMAT_S8_UINT 53
#define AHARDWAREBUFFER_FORMAT_Y8Cb8Cr8_420 54
#define AHARDWAREBUFFER_FORMAT_YCbCr_P010 54
#define AHARDWAREBUFFER_FORMAT_R8_UNORM 56
#define AHARDWAREBUFFER_FORMAT_R16_UINT 57
#define AHARDWAREBUFFER_FORMAT_R16G16_UINT 58
#define AHARDWAREBUFFER_FORMAT_R10G10B10A10_UNORM 59

/*
 * AHardwareBuffer usage constants
 */

#define AHARDWAREBUFFER_USAGE_CPU_READ_NEVER 0UL
#define AHARDWAREBUFFER_USAGE_CPU_READ_RARELY 2UL
#define AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN 3UL
#define AHARDWAREBUFFER_USAGE_CPU_WRITE_NEVER (0UL << 4)
#define AHARDWAREBUFFER_USAGE_CPU_WRITE_RARELY (2UL << 4)
#define AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN (3UL << 4)
#define AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE (1UL << 8)
#define AHARDWAREBUFFER_USAGE_GPU_FRAMEBUFFER (1UL << 9)
#define AHARDWAREBUFFER_USAGE_GPU_COLOR_OUTPUT AHARDWAREBUFFER_USAGE_GPU_FRAMEBUFFER
#define AHARDWAREBUFFER_USAGE_GPU_CUBE_MAP (1UL << 19)
#define AHARDWAREBUFFER_USAGE_GPU_MIPMAP_COMPLETE (1UL << 20)
#define AHARDWAREBUFFER_USAGE_GPU_DATA_BUFFER (1UL << 24)
#define AHARDWAREBUFFER_USAGE_SENSOR_DIRECT_DATA (1UL << 23)
#define AHARDWAREBUFFER_USAGE_PROTECTED_CONTENT (1UL << 14)
#define AHARDWAREBUFFER_USAGE_VIDEO_ENCODE (1UL << 16)
#define AHARDWAREBUFFER_USAGE_COMPOSER_OVERLAY (1ULL << 11)
#define AHARDWAREBUFFER_USAGE_GPU_RENDER_TARGET AHARDWAREBUFFER_USAGE_GPU_FRAMEBUFFER

/*
 * Function pointer typedefs
 */

typedef int (*PFNAHARDWAREBUFFER_ALLOCATE)(const AHardwareBuffer_Desc* desc, struct AHardwareBuffer** outBuffer);
typedef void (*PFNAHARDWAREBUFFER_RELEASE)(struct AHardwareBuffer* buffer);
typedef void (*PFNAHARDWAREBUFFER_DESCRIBE)(const struct AHardwareBuffer* buffer, AHardwareBuffer_Desc* outDesc);
typedef int (*PFNAHARDWAREBUFFER_LOCK)(
    struct AHardwareBuffer* buffer, uint64_t usage, int32_t fence, const struct ARect* rect, void** outVirtualAddress);
typedef int (*PFNAHARDWAREBUFFER_UNLOCK)(struct AHardwareBuffer* buffer, int32_t* fence);
typedef int (*PFNAHARDWAREBUFFER_LOCKPLANES)(struct AHardwareBuffer* buffer,
                                             uint64_t usage,
                                             int32_t fence,
                                             const struct ARect* rect,
                                             AHardwareBuffer_Planes* outPlanes);

/*
 * Function pointers
 */

GLAD_API_CALL PFNAHARDWAREBUFFER_ALLOCATE sg_glad_AHardwareBuffer_allocate;
#define AHardwareBuffer_allocate sg_glad_AHardwareBuffer_allocate

GLAD_API_CALL PFNAHARDWAREBUFFER_RELEASE sg_glad_AHardwareBuffer_release;
#define AHardwareBuffer_release sg_glad_AHardwareBuffer_release

GLAD_API_CALL PFNAHARDWAREBUFFER_DESCRIBE sg_glad_AHardwareBuffer_describe;
#define AHardwareBuffer_describe sg_glad_AHardwareBuffer_describe

GLAD_API_CALL PFNAHARDWAREBUFFER_LOCK sg_glad_AHardwareBuffer_lock;
#define AHardwareBuffer_lock sg_glad_AHardwareBuffer_lock

GLAD_API_CALL PFNAHARDWAREBUFFER_UNLOCK sg_glad_AHardwareBuffer_unlock;
#define AHardwareBuffer_unlock sg_glad_AHardwareBuffer_unlock

GLAD_API_CALL PFNAHARDWAREBUFFER_LOCKPLANES sg_glad_AHardwareBuffer_lockPlanes;
#define AHardwareBuffer_lockPlanes sg_glad_AHardwareBuffer_lockPlanes

/*
 * Loader function
 */

GLAD_API_CALL int gladLoadAHardwareBufferSafe(void);

#ifdef __cplusplus
}
#endif

#endif /* GLAD_PLATFORM_ANDROID */
