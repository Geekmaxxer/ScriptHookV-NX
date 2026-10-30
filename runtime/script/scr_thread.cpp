#include "scr_thread.hpp"
#include "script_manager.hpp"
#include "../core/memory.hpp"
#include "native_table.hpp"
#include "bindings.hpp"

extern "C" uint32_t sh_vm_tick(void* thread) {
    // Preserve the real interpreter call, return value and TLS management.
    ++ScriptHook::NativeTable::s_tickCount;
    const bool registered = ScriptHook::NativeTable::EnsureEngineRegistration();
    const bool repaired = registered && ScriptHook::NativeTable::RepairScriptNatives(thread);
    using Tick = uint32_t (*)(void*);
    // A manifest declares custom-native dependencies. Delay its script while
    // a plugin is still registering them, rather than execute the stock trap.
    if (registered && !repaired) { ScriptHook::ScriptManager::OnTick(thread); return 1; }
    const auto result = reinterpret_cast<Tick>(ScriptHook::ScriptThreadHook::s_originalTick)(thread);
    if (repaired) ScriptHook::ScriptManager::OnTick(thread);
    return result;
}

namespace ScriptHook {

bool ScriptThreadHook::s_hooked = false;
uintptr_t ScriptThreadHook::s_originalTick = 0;

bool ScriptThreadHook::Install() {
    if (s_hooked) return true;
    if (!Memory::s_mainBase) return false;

    const uintptr_t original = Memory::s_mainBase + binding::vm_tick;
    const uint32_t offsets[] = {binding::vm_slot_base, binding::vm_slot_derived};
    // Both table slots are proven by target relocations. Validate all slots
    // before writing either; leave other mods' hooks alone.
    for (uint32_t offset : offsets) {
        uintptr_t value = 0;
        if (!Memory::IsWritable(Memory::s_mainBase + offset, sizeof(value)) ||
            !Memory::Read(Memory::s_mainBase + offset, value) || value != original) return false;
    }
    s_originalTick = original;
    unsigned changed = 0;
    for (uint32_t offset : offsets) {
        auto* slot = reinterpret_cast<uintptr_t*>(Memory::s_mainBase + offset);
        uintptr_t expected = original;
        if (!__atomic_compare_exchange_n(slot, &expected, reinterpret_cast<uintptr_t>(sh_vm_tick),
                                         false, __ATOMIC_RELEASE, __ATOMIC_RELAXED)) {
            for (unsigned i = 0; i < changed; ++i) {
                auto* prior = reinterpret_cast<uintptr_t*>(Memory::s_mainBase + offsets[i]);
                expected = reinterpret_cast<uintptr_t>(sh_vm_tick);
                (void)__atomic_compare_exchange_n(prior, &expected, original, false, __ATOMIC_RELEASE, __ATOMIC_RELAXED);
            }
            return false;
        }
        ++changed;
    }
    s_hooked = true;
    Memory::Log("[ScriptHookV-NX] Guarded script VM tick hooks installed.\n");
    return true;
}

} // namespace ScriptHook
