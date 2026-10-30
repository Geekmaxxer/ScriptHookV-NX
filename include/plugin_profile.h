#pragma once
#include "main.h"
#define SHV_NX_PROFILE_ABI_VERSION 1u
#define SHV_NX_PROFILE_NAME_BYTES 33u
#ifdef __cplusplus
extern "C" {
#endif
typedef enum ShvNxProfileStatus {
    ShvNxProfile_Ok = 0,
    ShvNxProfile_Unavailable = 1,
    ShvNxProfile_NotInitialized = 2,
    ShvNxProfile_NoOpenUser = 3,
    ShvNxProfile_AmbiguousUser = 4,
    ShvNxProfile_PlatformError = 5,
    ShvNxProfile_InvalidName = 6,
    ShvNxProfile_WrongThread = 7,
    ShvNxProfile_InvalidArgument = 8,
    ShvNxProfile_UnsupportedTarget = 9
} ShvNxProfileStatus;
typedef struct ShvNxUserProfile {
    uint32_t size, abi, status, platform_result;
    char name[SHV_NX_PROFILE_NAME_BYTES];
    uint8_t reserved[7];
} ShvNxUserProfile;
typedef struct ShvNxProfile {
    uint32_t size, abi;
    ShvNxProfileStatus (*get)(void*, ShvNxUserProfile*, uint32_t);
} ShvNxProfile;
SHV_NX_API bool shvPluginBindProfile(const ShvNxProfile* profile);
SHV_NX_API bool scriptProfileAvailable(void);
SHV_NX_API ShvNxProfileStatus scriptGetUserProfile(ShvNxUserProfile* profile);
#ifdef __cplusplus
}
#endif
