#include "memory.hpp"
#include "../script/native_table.hpp"
#include "../script/script_manager.hpp"
#include "../script/scr_thread.hpp"
#include "../bridge/persistence.hpp"
#include "../bridge/public_natives.hpp"

extern "C" void* memset(void* p, int v, size_t n) {
    auto* out = static_cast<uint8_t*>(p);
    for (size_t i = 0; i < n; ++i) out[i] = static_cast<uint8_t>(v);
    return p;
}

extern "C" void* memcpy(void* p, const void* q, size_t n) {
    auto* out = static_cast<uint8_t*>(p);
    auto* in = static_cast<const uint8_t*>(q);
    for (size_t i = 0; i < n; ++i) out[i] = in[i];
    return p;
}

extern "C" void* memmove(void* p, const void* q, size_t n) {
    auto* out = static_cast<uint8_t*>(p);
    auto* in = static_cast<const uint8_t*>(q);
    if (reinterpret_cast<uintptr_t>(out) <= reinterpret_cast<uintptr_t>(in)) {
        for (size_t i = 0; i < n; ++i) out[i] = in[i];
    } else {
        while (n) { --n; out[n] = in[n]; }
    }
    return p;
}

extern "C" void scripthook_module_init() {
    ScriptHook::Memory::Log("\n========================================\n");
    ScriptHook::Memory::Log("   ScriptHookV-NX configurable limits test 1\n");
    ScriptHook::Memory::Log("========================================\n");

    if (!ScriptHook::Memory::Initialize()) {
        ScriptHook::Memory::Log("[ScriptHookV-NX] Initialization halted: Main base unverified.\n");
        return;
    }

    ScriptHook::NativeTable::Initialize();
    ScriptHook::Persistence::Initialize();
    ScriptHook::RegisterPublicNatives();
    ScriptHook::ScriptManager::Initialize();

    // Native table registration is deferred until the real game script thread
    // is running. No SD access, user plugin or arbitrary code scan at startup.
    if (!ScriptHook::ScriptThreadHook::Install()) {
        ScriptHook::Memory::Log("[ScriptHookV-NX] VM hook conflict; bridge inactive.\n");
        return;
    }

    ScriptHook::Memory::Log("[ScriptHookV-NX] Core subsystems ready!\n");
}
