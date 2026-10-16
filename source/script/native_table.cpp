#include "native_table.hpp"
#include "native_manifest.hpp"
#include "../core/memory.hpp"
#include "bindings.hpp"

namespace ScriptHook {
namespace {
struct LookupCache {
    uint32_t sequence;
    uint64_t hash, base, head;
    NativeHandler handler;
};
constexpr size_t cache_size = 128;
LookupCache lookup_cache[cache_size]{};
size_t registered_count;
uintptr_t registered_head;
LookupCache& cache_slot(uint64_t hash) { return lookup_cache[(hash ^ (hash >> 32)) & (cache_size - 1)]; }
NativeHandler cached(uint64_t hash, uintptr_t head) {
    auto& entry = cache_slot(hash);
    const auto before = __atomic_load_n(&entry.sequence, __ATOMIC_ACQUIRE);
    if (before & 1) return nullptr;
    const auto key = __atomic_load_n(&entry.hash, __ATOMIC_RELAXED);
    const auto base = __atomic_load_n(&entry.base, __ATOMIC_RELAXED);
    const auto node = __atomic_load_n(&entry.head, __ATOMIC_RELAXED);
    const auto handler = __atomic_load_n(&entry.handler, __ATOMIC_RELAXED);
    if (before != __atomic_load_n(&entry.sequence, __ATOMIC_ACQUIRE)) return nullptr;
    return key == hash && base == Memory::s_mainBase && node == head ? handler : nullptr;
}
void remember(uint64_t hash, uintptr_t head, NativeHandler handler) {
    if (!handler || reinterpret_cast<uintptr_t>(handler) == Memory::s_mainBase + binding::missing_native ||
        !Memory::IsExecutable(reinterpret_cast<uintptr_t>(handler))) return;
    auto& entry = cache_slot(hash);
    auto before = __atomic_load_n(&entry.sequence, __ATOMIC_RELAXED);
    if ((before & 1) || !__atomic_compare_exchange_n(&entry.sequence, &before, before + 1, false,
                                                    __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) return;
    __atomic_store_n(&entry.hash, hash, __ATOMIC_RELAXED);
    __atomic_store_n(&entry.base, Memory::s_mainBase, __ATOMIC_RELAXED);
    __atomic_store_n(&entry.head, head, __ATOMIC_RELAXED);
    __atomic_store_n(&entry.handler, handler, __ATOMIC_RELAXED);
    __atomic_store_n(&entry.sequence, before + 2, __ATOMIC_RELEASE);
}
}

CustomNativeEntry NativeTable::s_customNatives[MaxCustomNatives]{};
size_t NativeTable::s_customNativeCount = 0;
uint32_t NativeTable::s_tickCount = 0, NativeTable::s_cacheRepairs = 0, NativeTable::s_cacheRejected = 0;

bool NativeTable::Initialize() {
    InvalidateCache();
    return true;
}

void NativeTable::InvalidateCache() {
    for (auto& entry : lookup_cache) {
        auto before = __atomic_load_n(&entry.sequence, __ATOMIC_RELAXED);
        if ((before & 1) || !__atomic_compare_exchange_n(&entry.sequence, &before, before + 1, false,
                                                        __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) continue;
        __atomic_store_n(&entry.hash, uint64_t(0), __ATOMIC_RELAXED);
        __atomic_store_n(&entry.handler, NativeHandler(nullptr), __ATOMIC_RELAXED);
        __atomic_store_n(&entry.sequence, before + 2, __ATOMIC_RELEASE);
    }
}

bool NativeTable::RegisterCustom(uint64_t hash, NativeHandler handler) {
    if (!hash || !handler) return false;

    // Check if already registered
    for (size_t i = 0; i < s_customNativeCount; ++i) {
        if (s_customNatives[i].hash == hash) {
            if (s_customNatives[i].handler != handler && GetEngineHandler(hash)) return false;
            s_customNatives[i].handler = handler;
            return true;
        }
    }

    // Custom hashes must not shadow an existing engine native.
    const auto engine=GetEngineHandler(hash);
    if(engine && reinterpret_cast<uintptr_t>(engine)!=Memory::s_mainBase+binding::missing_native) return false;

    if (s_customNativeCount < MaxCustomNatives) {
        s_customNatives[s_customNativeCount].hash = hash;
        s_customNatives[s_customNativeCount].handler = handler;
        s_customNativeCount++;
        return true;
    }

    return false;
}

NativeHandler NativeTable::GetHandler(uint64_t hash) {
    // 1. Check custom natives first
    for (size_t i = 0; i < s_customNativeCount; ++i) {
        if (s_customNatives[i].hash == hash) {
            return s_customNatives[i].handler;
        }
    }

    return GetEngineHandler(hash);
}

NativeHandler NativeTable::GetEngineHandler(uint64_t hash) {
    if (!Memory::s_mainBase || !hash) return nullptr;
    uintptr_t node = 0;
    if (!Memory::Read(Memory::s_mainBase + binding::native_registry + (hash & 255) * 8, node)) return nullptr;
    if (!node) return nullptr;
    const uintptr_t head = node;
    if (auto handler = cached(hash, head)) return handler;
    // Exact count/hash/next decoding used by main+0x2633318 and +0x2637a94.
    // Each engine node holds at most seven handlers. Never follow a corrupt
    // chain without bounds or treat the standard allocator as a native table.
    for (unsigned depth = 0; node && depth < 256; ++depth) {
        uint32_t count_a = 0, count_b = 0;
        if (!Memory::Read(node + 0x48, count_a) || !Memory::Read(node + 0x4c, count_b)) return nullptr;
        const uint32_t count = count_a ^ count_b ^ uint32_t(node + 0x48);
        if (count > 7) return nullptr;
        for (uint32_t i = 0; i < count; ++i) {
            const uintptr_t address = node + 0x54 + i * 16;
            uint32_t low = 0, high = 0, salt = 0;
            if (!Memory::Read(address, low) || !Memory::Read(address + 4, high) ||
                !Memory::Read(address + 8, salt)) return nullptr;
            const uint32_t key = salt ^ uint32_t(address);
            if ((uint64_t(low ^ key) | uint64_t(high ^ key) << 32) == hash) {
                NativeHandler handler = nullptr;
                if (!Memory::Read(node + 0x10 + i * 8, handler)) return nullptr;
                remember(hash, head, handler);
                return handler;
            }
        }
        uint32_t low = 0, high = 0, salt = 0;
        if (!Memory::Read(node, low) || !Memory::Read(node + 4, high) || !Memory::Read(node + 8, salt)) return nullptr;
        const uint32_t key = salt ^ uint32_t(node);
        const uintptr_t next = uint64_t(low ^ key) | uint64_t(high ^ key) << 32;
        if (next == node) return nullptr;
        node = next;
    }
    return nullptr;
}

bool NativeTable::EnsureEngineRegistration() {
    // The game clears the registry at main+0x1255230. Registering from DT_INIT
    // would therefore be lost. A guarded VM tick runs after that initialization
    // and before achievement_controller can request/start ragemenu.
    if (!Memory::s_mainBase || !s_customNativeCount) return false;
    uintptr_t head=0;
    if (!Memory::Read(Memory::s_mainBase+binding::native_registry+(s_customNatives[0].hash&255)*8,head)) return false;
    if (registered_count==s_customNativeCount && head && head==registered_head) return true;
    bool populated = false;
    for (unsigned bucket = 0; bucket < 256 && !populated; ++bucket) {
        uintptr_t head = 0;
        if (!Memory::Read(Memory::s_mainBase + binding::native_registry + bucket * 8, head)) return false;
        populated = head != 0;
    }
    if (!populated) return false;
    bool ready = true;
    for (size_t i = 0; i < s_customNativeCount; ++i) {
        const auto& custom = s_customNatives[i];
        NativeHandler found = GetEngineHandler(custom.hash);
        if (!found) {
            using Register = void (*)(uint64_t, NativeHandler);
            reinterpret_cast<Register>(Memory::s_mainBase + binding::native_register)(custom.hash, custom.handler);
            found = GetEngineHandler(custom.hash);
            if (found == custom.handler) Memory::Log("[ScriptHookV-NX] Custom native registered in game VM.\n");
        }
        if (found != custom.handler) ready = false;
    }
    if (ready) {
        registered_count=s_customNativeCount;
        (void)Memory::Read(Memory::s_mainBase+binding::native_registry+(s_customNatives[0].hash&255)*8,registered_head);
    }
    return ready;
}

bool NativeTable::RepairScriptNatives(void* thread) {
    return NativeManifest::Repair(thread);
}

} // namespace ScriptHook

// C API Wrappers
extern "C" {

NativeHandler getNativeHandler(uint64_t hash) {
    return ScriptHook::NativeTable::GetHandler(hash);
}

bool registerCustomNative(uint64_t hash, NativeHandler handler) {
    return ScriptHook::NativeTable::RegisterCustom(hash, handler);
}

void invalidateNativeLookupCache() { ScriptHook::NativeTable::InvalidateCache(); }

}
