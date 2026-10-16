#pragma once
#include "main.h"
#define SHV_NX_PLUGIN_ABI_VERSION 1u
#ifdef __cplusplus
extern "C" {
#endif
typedef struct ShvNxApi {
    uint32_t size,abi,caller_abi,runtime_abi;
    void* plugin;
    bool (*invoke)(uint64_t,void*,uint32_t);
    NativeCallStatus (*native_status)(void);
    uint64_t (*native_hash)(void);
    bool (*native_available)(uint64_t);
    void (*wait)(uint32_t);
    bool (*register_native)(void*,uint64_t,NativeHandler);
    bool (*write_file)(void*,const char*,const void*,size_t);
    bool (*read_file)(void*,const char*,void*,size_t,size_t*);
    void (*raw_init)(uint64_t);
    void (*raw_push)(uint64_t);
    uint64_t* (*raw_call)(void);
    void* (*raw_context)(void);
    uint64_t* (*raw_checked)(uint64_t);
    NativeHandler (*handler)(uint64_t);
} ShvNxApi;
typedef struct ShvNxPlugin {
    uint32_t size,abi,caller_abi,runtime_abi;
    const char* id;
    bool (*initialize)(const ShvNxApi*);
    ScriptFunction run;
} ShvNxPlugin;
// A module exports: const ShvNxPlugin* shvPluginQuery(void).
// Query must have no side effects. initialize must return without waiting.
// run executes as a cooperative script. Modules stay mapped until game exit.
SHV_NX_API bool shvBindApi(const ShvNxApi* api);
#ifdef __cplusplus
}
#endif
