#pragma once
#include "types.h"
#ifndef __cplusplus
#include <stdbool.h>
#endif

#if defined(__GNUC__)
#define SHV_NX_API __attribute__((visibility("default")))
#else
#define SHV_NX_API
#endif
#define SHV_NX_CALLER_ABI_VERSION 1u
#define SHV_NX_RUNTIME_ABI_VERSION 1u

#ifdef __cplusplus
extern "C" {
#endif

// ScriptHookV-NX Core API (Nintendo Switch)
typedef void (*NativeHandler)(void* ctx);
typedef void (*ScriptFunction)(void);

typedef enum NativeCallStatus {
    NativeCall_Ok = 0,
    NativeCall_NotInitialized,
    NativeCall_NoScriptContext,
    NativeCall_WrongThread,
    NativeCall_NoActiveCall,
    NativeCall_ArgumentOverflow,
    NativeCall_MissingHandler,
    NativeCall_InvalidHandler,
    NativeCall_InvalidPointer,
    NativeCall_InvalidVectorCount,
    NativeCall_DepthExceeded,
    NativeCall_HashMismatch,
    NativeCall_InvalidContext,
    NativeCall_ReentrantContext,
    NativeCall_StringTooLong
} NativeCallStatus;

typedef struct NativeCallerSelfTestInfo {
    uint32_t passed, failed;
    NativeCallStatus last_error;
    uint32_t reserved;
    uint64_t last_error_hash;
} NativeCallerSelfTestInfo;

// Raw calls require a running game script thread. Failures return three zero
// result slots; check getNativeCallStatus(). Results last until the same
// nesting depth is reused by another call.
SHV_NX_API void nativeInit(uint64_t hash);
SHV_NX_API void nativePush64(uint64_t value);
SHV_NX_API uint64_t* nativeCall(void);
SHV_NX_API NativeCallStatus getNativeCallStatus(void);
SHV_NX_API uint64_t getLastNativeHash(void);
SHV_NX_API bool nativeIsAvailable(uint64_t hash);
SHV_NX_API void invalidateNativeLookupCache(void);
SHV_NX_API uint32_t getNativeCallerAbiVersion(void);
// Sized, explicit contexts are used by the C++ helpers. Plugins and runtime
// must agree on the caller ABI version and the public NativeContext layout.
SHV_NX_API bool nativeInvokeContext(uint64_t hash, void* context, uint32_t contextBytes);
SHV_NX_API void* nativeGetCallContext(void);
SHV_NX_API uint64_t* nativeCallChecked(uint64_t expectedHash);
// Read-only getter checks, executed only when explicitly requested from a
// running game script. Storage Self-Test invokes this and logs its results.
SHV_NX_API bool nativeCallerSelfTest(NativeCallerSelfTestInfo* result);

SHV_NX_API void scriptRegister(ScriptFunction function);
SHV_NX_API void scriptUnregister(ScriptFunction function);
SHV_NX_API void scriptWait(uint32_t ms);
// Cooperative scripts retain their stack across waits. Registration and control
// belong to the VM host thread once it has been selected. Cancellation discards
// a suspended stack; it does not unwind C++ destructors.
typedef enum ScriptRuntimeStatus {
    ScriptRuntime_Ok = 0, ScriptRuntime_InvalidFunction, ScriptRuntime_Capacity,
    ScriptRuntime_WrongThread, ScriptRuntime_NotRunning, ScriptRuntime_UnsafeYield,
    ScriptRuntime_StackCorrupt, ScriptRuntime_NoContext, ScriptRuntime_Busy
} ScriptRuntimeStatus;
typedef struct ScriptRuntimeInfo {
    uint32_t abi, active, dispatches, resumes, waits, completions, cancellations;
    uint32_t context_rejections, stack_failures, budget_overruns;
    uint32_t self_test_runs, self_test_passed, self_test_failed, self_test_done;
    uint32_t self_test_wait_ms, self_test_peer_steps;
} ScriptRuntimeInfo;
SHV_NX_API bool scriptRegisterChecked(ScriptFunction function);
SHV_NX_API void scriptStopAll(void);
SHV_NX_API ScriptRuntimeStatus getScriptRuntimeStatus(void);
SHV_NX_API bool getScriptRuntimeInfo(ScriptRuntimeInfo* out, uint32_t bytes);
// Explicit, read-only two-script test; writes runtime-report.txt on completion.
SHV_NX_API bool scriptRuntimeSelfTest(void);
SHV_NX_API uint32_t getScriptRuntimeAbiVersion(void);

SHV_NX_API NativeHandler getNativeHandler(uint64_t hash);
SHV_NX_API bool registerCustomNative(uint64_t hash, NativeHandler handler);

// External plugins: the shim isolates flat filenames under
// sdmc:/switch/gtav-scripthook/settings/<plugin-id>-<filename> (64 KiB maximum).
// In-core callers retain the legacy sdmc:/switch/gtav-megatard/ bridge.
SHV_NX_API bool scriptSaveFile(const char* relativePath, const void* data, size_t size);
SHV_NX_API bool scriptReadFile(const char* relativePath, void* buffer, size_t maxBytes, size_t* bytesRead);

SHV_NX_API uint32_t getScriptHookVersion(void);

#ifdef __cplusplus
}
#endif
