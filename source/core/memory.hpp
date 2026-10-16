#pragma once
#include <stdint.h>
#include <stddef.h>

struct MemoryInfo {
    uint64_t address;
    uint64_t size;
    uint32_t type;
    uint32_t attribute;
    uint32_t permission;
    uint32_t ipc_refs;
    uint32_t device_refs;
    uint32_t padding;
};

extern "C" uint32_t sh_query_memory(MemoryInfo* info, uint32_t* page, uint64_t address);
extern "C" void sh_debug_print(const char* str, uint64_t length);
extern "C" uint64_t sh_get_tick();
extern "C" uintptr_t sh_get_thread_pointer();
extern "C" void sh_sleep_thread(uint64_t nanoseconds);

namespace ScriptHook {

class Memory {
public:
    static uintptr_t s_mainBase;
    static uintptr_t s_sdkBase;

    static bool Initialize();
    static bool ResolveSdk();
    static bool IsReadable(uintptr_t addr, size_t size);
    static bool IsWritable(uintptr_t addr, size_t size);
    static bool IsExecutable(uintptr_t addr);

    template <typename T>
    static bool Read(uintptr_t addr, T& out) {
        if (!IsReadable(addr, sizeof(T))) return false;
        const auto* src = reinterpret_cast<const uint8_t*>(addr);
        auto* dst = reinterpret_cast<uint8_t*>(&out);
        for (size_t i = 0; i < sizeof(T); ++i) dst[i] = src[i];
        return true;
    }

    template <typename T>
    static bool Write(uintptr_t addr, const T& val) {
        if (!IsWritable(addr, sizeof(T))) return false;
        auto* dst = reinterpret_cast<uint8_t*>(addr);
        const auto* src = reinterpret_cast<const uint8_t*>(&val);
        for (size_t i = 0; i < sizeof(T); ++i) dst[i] = src[i];
        return true;
    }

    static void Log(const char* msg);
};

} // namespace ScriptHook
