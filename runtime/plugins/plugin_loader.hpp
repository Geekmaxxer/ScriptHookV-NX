#pragma once
#include "main.h"
namespace ScriptHook {
class PluginLoader {
public:
    static void Tick();
    static void Report();
    static uint32_t Loaded();
    static bool Pending();

    // In-Menu Plugin Manager API
    static uint32_t Count();
    static const char* GetFilename(uint32_t idx);
    static const char* GetId(uint32_t idx);
    static bool GetInfo(uint32_t idx, uint32_t& status, uint32_t& memKb, uint32_t& step, uint32_t& err);
    static const char* GetStatusText(uint32_t idx);
    static bool Toggle(uint32_t idx);
    static bool RescanAndReload();
    // Startup policy is durable via filename suffix: Disabled=0, Manual=1,
    // Auto=2. Changing policy never invalidates an existing module mapping.
    static uint32_t GetPolicy(uint32_t idx);
    static bool SetPolicy(uint32_t idx, uint32_t policy);
    static bool RequestLoad(uint32_t idx);
    static bool Kill(uint32_t idx);
    static bool IsRunning(uint32_t idx);
    static uint32_t LastActionError();
    // Elapsed Horizon time: scripts=0, callbacks=1, cleanup=2, initialize=3.
    static void RecordScriptTime(ScriptFunction function,uint64_t ticks);
    static void RecordTime(void* owner,uint32_t kind,uint64_t ticks);
    static bool GetTiming(uint32_t idx,uint32_t kind,uint64_t& calls,uint64_t& totalUs,uint64_t& worstUs);
    static void CallbackFailed();
    // Shared accounting for transient image decoding and retained GPU textures.
    static bool ReserveExtraMemory(size_t bytes);
    static void ReleaseExtraMemory(size_t bytes);
};
}
