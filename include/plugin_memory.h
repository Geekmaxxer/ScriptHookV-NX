#pragma once
#include "main.h"
#define SHV_NX_MEMORY_ABI_VERSION 1u
#define SHV_NX_MEMORY_ZERO 1u
#ifdef __cplusplus
extern "C" {
#endif
// Requested bytes are live payload; charged bytes include pooled page capacity.
typedef struct ShvNxMemoryStats {
    uint32_t size,abi;
    uint64_t requested_bytes,charged_bytes,allocations,arenas;
    uint64_t peak_requested_bytes,peak_charged_bytes,failures,release_failures;
} ShvNxMemoryStats;
// Optional independent table; the base plugin/extension ABI 1 layouts are intact.
typedef struct ShvNxMemory {
    uint32_t size,abi;
    void* (*allocate)(void*,size_t,size_t,uint32_t);
    void* (*reallocate)(void*,void*,size_t);
    bool (*free)(void*,void*);
    size_t (*allocation_size)(void*,const void*);
    bool (*stats)(void*,ShvNxMemoryStats*);
    bool (*trim)(void*);
} ShvNxMemory;
SHV_NX_API bool shvPluginBindMemory(const ShvNxMemory* memory);
SHV_NX_API bool scriptMemoryAvailable(void);
// Owner thread only, including bounded initialization, callbacks and cleanup.
// bytes=0 returns null. Alignment is power-of-two 1..4096 (minimum returned 16).
SHV_NX_API void* scriptAllocate(size_t bytes,size_t alignment,uint32_t flags);
SHV_NX_API void* scriptCalloc(size_t count,size_t elementBytes);
// Growth failure preserves the original block. Null allocates; size=0 frees.
// Existing alignment is preserved. No Windows/CRT allocator is substituted.
SHV_NX_API void* scriptReallocate(void* pointer,size_t bytes);
SHV_NX_API bool scriptFree(void* pointer);
SHV_NX_API size_t scriptAllocationSize(const void* pointer);
SHV_NX_API bool scriptGetMemoryStats(ShvNxMemoryStats* stats);
// Release unused pools. Live allocations survive Kill/Load until explicitly freed.
SHV_NX_API bool scriptTrimMemory(void);
#ifdef __cplusplus
}
#endif
