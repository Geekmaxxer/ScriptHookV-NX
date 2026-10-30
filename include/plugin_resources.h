#pragma once
#include "main.h"

#define SHV_NX_RESOURCES_ABI_VERSION 1u
#define SHV_NX_RESOURCES_PER_OWNER 64u

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t ShvNxResourceHandle;
typedef void (*ShvNxResourceCleanup)(void* context);

// Optional independent contract. A nonnull context must be the exact start of
// an owned heap allocation; registration pins that allocation until removal.
typedef struct ShvNxResources {
    uint32_t size, abi;
    ShvNxResourceHandle (*register_resource)(void*, ShvNxResourceCleanup, void*);
    bool (*release)(void*, ShvNxResourceHandle);
    bool (*forget)(void*, ShvNxResourceHandle);
    bool (*active)(void*, ShvNxResourceHandle);
    bool (*pinned)(void*, const void*);
    uint32_t (*count)(void*);
} ShvNxResources;

SHV_NX_API bool shvPluginBindResources(const ShvNxResources* resources);
SHV_NX_API bool scriptResourcesAvailable(void);
SHV_NX_API ShvNxResourceHandle scriptResourceRegister(ShvNxResourceCleanup cleanup, void* context);
// Release consumes a valid registration before invoking its callback. False
// can diagnose a callback violation after consumption; inspect Active to retry.
SHV_NX_API bool scriptResourceRelease(ShvNxResourceHandle handle);
SHV_NX_API bool scriptResourceForget(ShvNxResourceHandle handle);
SHV_NX_API bool scriptResourceActive(ShvNxResourceHandle handle);
SHV_NX_API bool scriptResourcePinned(const void* pointer);
SHV_NX_API uint32_t scriptResourceCount(void);

#ifdef __cplusplus
}
#endif
