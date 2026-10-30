#pragma once
#include <stdint.h>
#include <stddef.h>
#include "main.h"

namespace ScriptHook {

struct CustomNativeEntry {
    uint64_t hash;
    NativeHandler handler;
};

class NativeTable {
public:
    static constexpr size_t MaxCustomNatives = 256;
    static CustomNativeEntry s_customNatives[MaxCustomNatives];
    static size_t s_customNativeCount;

    static bool Initialize();
    static void InvalidateCache();
    static NativeHandler GetHandler(uint64_t hash);
    static bool RegisterCustom(uint64_t hash, NativeHandler handler);
    // Called on the game script thread after the engine initializes its table.
    static bool EnsureEngineRegistration();
    static NativeHandler GetEngineHandler(uint64_t hash);
    static bool RepairScriptNatives(void* thread);
    static uint32_t s_tickCount, s_cacheRepairs, s_cacheRejected;
};

} // namespace ScriptHook
