#pragma once
#include "main.h"
#include "runtime_defaults.h"

namespace ScriptHook {
struct alignas(16) FiberContext {
    uint64_t x19_x30[12], sp, d8_d15[8], fpcr, fpsr;
};
static_assert(offsetof(FiberContext, sp) == 96);
static_assert(offsetof(FiberContext, d8_d15) == 104);
static_assert(offsetof(FiberContext, fpcr) == 168);
static_assert(offsetof(FiberContext, fpsr) == 176);
static_assert(sizeof(FiberContext) == 192);
extern "C" void sh_fiber_switch(FiberContext* from, const FiberContext* to);

class ScriptManager {
public:
    static constexpr size_t MaxScripts = SHVNX_MAX_SCRIPT_SLOTS, StackBytes = 64 * 1024;
    static constexpr size_t BaseScripts = SHVNX_DEFAULT_SCRIPT_SLOTS;
    static size_t ConfigureSlots(size_t requested, uint32_t& error);
    static size_t SlotLimit();
    static size_t StackAllocationBytes();
    static void Initialize();
    static bool Register(ScriptFunction function);
    // Internal loader reservation. Tokens are one-based slots; zero fails.
    // Reserve before an initializer can publish callbacks into its image.
    static uint32_t ReserveSlot();
    static bool CommitReserved(uint32_t token, ScriptFunction function);
    static void ReleaseReserved(uint32_t token);
    static void Unregister(ScriptFunction function);
    static bool Contains(ScriptFunction function);
    static void StopAll();
    static void Wait(uint32_t ms);
    static void OnTick(void* host);
    static bool RequestSelfTest();
    static bool Info(ScriptRuntimeInfo* out, uint32_t bytes);
    static ScriptRuntimeStatus Status();
    static void RunEntry();
};
} // namespace ScriptHook
