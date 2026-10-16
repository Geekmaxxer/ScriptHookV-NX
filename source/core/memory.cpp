#include "memory.hpp"
#include "bindings.hpp"

namespace ScriptHook {
namespace {
template<size_t Count> bool matches(uintptr_t base, const binding::Fingerprint (&fingerprints)[Count]) {
    for (const auto& f : fingerprints) {
        if (!Memory::IsReadable(base + f.offset, sizeof(f.bytes))) return false;
        const auto* code = reinterpret_cast<const uint8_t*>(base + f.offset);
        for (size_t i = 0; i < sizeof(f.bytes); ++i) if (code[i] != f.bytes[i]) return false;
    }
    return true;
}
}

uintptr_t Memory::s_mainBase = 0;
uintptr_t Memory::s_sdkBase = 0;

void Memory::Log(const char* msg) {
    if (!msg) return;
    size_t len = 0;
    while (msg[len] && len < 1024) len++;
    sh_debug_print(msg, len);
}

bool Memory::IsReadable(uintptr_t addr, size_t size) {
    if (!addr || size == 0 || addr > UINT64_MAX - size) return false;
    MemoryInfo m{};
    uint32_t page = 0;
    if (sh_query_memory(&m, &page, addr) != 0) return false;
    return (m.permission & 1) && (addr >= m.address) && (addr + size <= m.address + m.size);
}

bool Memory::IsWritable(uintptr_t addr, size_t size) {
    if (!addr || size == 0 || addr > UINT64_MAX - size) return false;
    MemoryInfo m{};
    uint32_t page = 0;
    if (sh_query_memory(&m, &page, addr) != 0) return false;
    return (m.permission & 2) && (addr >= m.address) && (addr + size <= m.address + m.size);
}

bool Memory::Initialize() {
    if (s_mainBase) return true;
    uint64_t address = 0;
    for (unsigned i = 0; i < 512; ++i) {
        MemoryInfo info{}; uint32_t page = 0;
        if (sh_query_memory(&info, &page, address) || !info.size) break;
        if ((info.type & 0xff) == 3 && info.permission == 5 &&
            info.size == binding::main_text_pages && matches(info.address, binding::main_fingerprints))
            s_mainBase = info.address;
        const uint64_t next = info.address + info.size;
        if (next <= address) break;
        address = next;
    }
    if (!s_mainBase) { Log("[ScriptHookV-NX] Target main fingerprints not found; hooks skipped.\n"); return false; }
    Log("[ScriptHookV-NX] Main verified; SDK resolution deferred until filesystem use.\n");
    return true;
}

bool Memory::IsExecutable(uintptr_t addr) {
    if (!addr || addr % 4) return false;
    MemoryInfo m{}; uint32_t page = 0;
    return sh_query_memory(&m, &page, addr) == 0 && (m.permission & 5) == 5 &&
           addr >= m.address && addr <= UINT64_MAX - 4 && addr + 4 <= m.address + m.size;
}

bool Memory::ResolveSdk() {
    if (s_sdkBase) return true;
    if (!s_mainBase) return false;
    const uint32_t slots[] = {binding::sdk_anchor_slot, binding::sdk_fallback_slot};
    const uint32_t offsets[] = {binding::sdk_anchor_offset, binding::sdk_fallback_offset};
    for (unsigned i = 0; i < 2; ++i) {
        uintptr_t anchor = 0;
        if (!Read(s_mainBase + slots[i], anchor) || anchor < offsets[i]) continue;
        const uintptr_t candidate = anchor - offsets[i];
        MemoryInfo info{}; uint32_t page = 0;
        if (sh_query_memory(&info, &page, candidate) || info.address != candidate ||
            info.permission != 5 || info.size != binding::sdk_text_pages ||
            !matches(candidate, binding::sdk_fingerprints)) continue;
        s_sdkBase = candidate; return true;
    }
    return false;
}

} // namespace ScriptHook
