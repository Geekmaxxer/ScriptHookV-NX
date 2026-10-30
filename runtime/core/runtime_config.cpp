#include "runtime_config.hpp"
#include "storage.hpp"
#include "memory.hpp"
#include "../script/script_manager.hpp"
#include "../plugins/plugin_loader.hpp"
namespace ScriptHook {
namespace {
size_t residents = SHVNX_DEFAULT_RESIDENT_PLUGINS;
size_t scripts = SHVNX_DEFAULT_SCRIPT_SLOTS;
size_t memory_mib = SHVNX_DEFAULT_PLUGIN_MEMORY_MIB;
size_t image_mib = SHVNX_DEFAULT_MODULE_IMAGE_MIB;
size_t bss_mib = SHVNX_DEFAULT_MODULE_BSS_MIB;
uint32_t state, error, applied, invalid, stack_error, automatic;
size_t metadata_bytes;
uint32_t metadata_error;
bool equal(const char* first, const char* last, const char* key) {
    while (first != last && *key && *first == *key) { ++first; ++key; }
    return first == last && !*key;
}
bool space(char c) { return c == ' ' || c == '\t' || c == '\r'; }
void parse(const char* data, size_t bytes) {
    size_t values[] = {SHVNX_DEFAULT_RESIDENT_PLUGINS, SHVNX_DEFAULT_SCRIPT_SLOTS, SHVNX_DEFAULT_PLUGIN_MEMORY_MIB, SHVNX_DEFAULT_MODULE_IMAGE_MIB, SHVNX_DEFAULT_MODULE_BSS_MIB};
    constexpr size_t ceilings[] = {SHVNX_MAX_RESIDENT_PLUGINS, SHVNX_MAX_SCRIPT_SLOTS, SHVNX_MAX_PLUGIN_MEMORY_MIB, SHVNX_MAX_MODULE_IMAGE_MIB, SHVNX_MAX_MODULE_BSS_MIB};
    const char* names[] = {"resident_limit", "script_slots", "plugin_memory_mib", "module_image_mib", "module_bss_mib"};
    uint32_t seen = 0;
    size_t at = bytes >= 3 && uint8_t(data[0]) == 0xef && uint8_t(data[1]) == 0xbb && uint8_t(data[2]) == 0xbf ? 3 : 0;
    while (at < bytes) {
        const char* first = data + at;
        while (at < bytes && data[at] != '\n') ++at;
        const char* last = data + at; if (at < bytes) ++at;
        while (first != last && space(*first)) ++first;
        for (const char* p = first; p != last; ++p) if (*p == '#' || *p == ';') { last = p; break; }
        while (last != first && space(last[-1])) --last;
        if (first == last) continue;
        const char* key_end = first;
        while (key_end != last && !space(*key_end) && *key_end != '=') ++key_end;
        for (size_t i = 0; i < sizeof(values)/sizeof(values[0]); ++i) {
            if (!equal(first, key_end, names[i])) continue;
            const uint32_t bit = 1u << i;
            const bool duplicate = (seen & bit) != 0; seen |= bit;
            const char* p = key_end; while (p != last && space(*p)) ++p;
            bool good = p != last && *p++ == '=';
            while (p != last && space(*p)) ++p;
            size_t value = 0, digits = 0;
            while (p != last && *p >= '0' && *p <= '9') {
                ++digits;
                const size_t digit = size_t(*p - '0');
                if (digit > ceilings[i] || value > (ceilings[i] - digit) / 10) good = false;
                else value = value * 10 + digit;
                ++p;
            }
            good = good && digits && p == last && value <= ceilings[i] && !duplicate;
            if (!good) invalid |= bit;
            if (good) values[i] = value;
        }
    }
    applied = seen & ~invalid;
    if (applied & 1) residents = values[0];
    if (applied & 2) scripts = values[1];
    if (applied & 4) memory_mib = values[2];
    if (applied & 8) image_mib = values[3];
    if (applied & 16) bss_mib = values[4];
}
}
void RuntimeConfig::LoadOnce() {
    if (state) return;
    // Fixed-size read; no background polling, file creation or config writes.
    {
        Storage disk; char data[4096]{}; size_t bytes = 0;
        if (disk.Read("runtime.ini", data, sizeof(data), bytes)) { state = 2; parse(data, bytes); }
        else if (disk.error == 0x202) state = 1;
        else { state = 3; error = disk.error ? disk.error : 0x10020; }
    }
    automatic = (!residents ? 1u : 0u) | (!scripts ? 2u : 0u) | (!memory_mib ? 4u : 0u) |
                (!image_mib ? 8u : 0u) | (!bss_mib ? 16u : 0u);
    scripts = ScriptManager::ConfigureSlots(scripts, stack_error);
    if (stack_error) { invalid |= 2; applied &= ~2u; }
}
size_t RuntimeConfig::Residents() { return residents ? residents : SHVNX_MAX_RESIDENT_PLUGINS; }
size_t RuntimeConfig::Scripts() { return scripts; }
size_t RuntimeConfig::MemoryBytes() { return memory_mib ? memory_mib * 1024 * 1024 : SIZE_MAX; }
size_t RuntimeConfig::ModuleImageBytes() { return image_mib ? image_mib * 1024 * 1024 : SIZE_MAX; }
size_t RuntimeConfig::ModuleBssBytes() { return bss_mib ? bss_mib * 1024 * 1024 : SIZE_MAX; }
uint32_t RuntimeConfig::State() { return state; }
uint32_t RuntimeConfig::Error() { return error; }
uint32_t RuntimeConfig::AppliedMask() { return applied; }
uint32_t RuntimeConfig::InvalidMask() { return invalid; }
uint32_t RuntimeConfig::StackError() { return stack_error; }
void RuntimeConfig::SetStackError(uint32_t value) { if(stack_error!=value){stack_error=value;PluginLoader::InvalidateReport();} }
size_t RuntimeConfig::MetadataBytes() { return metadata_bytes; }
uint32_t RuntimeConfig::MetadataError() { return metadata_error; }
uint32_t RuntimeConfig::AutomaticMask() {
    return automatic;
}
void* RuntimeConfig::AllocateMetadata(size_t bytes) {
    PluginLoader::InvalidateReport();
    if (!Memory::ResolveSdk()) { metadata_error = 0x1000b; return nullptr; }
    if (!bytes || bytes > SIZE_MAX - 4095) { metadata_error = 0x10021; return nullptr; }
    bytes = (bytes + 4095) & ~size_t(4095);
    if (metadata_bytes > SIZE_MAX - bytes) { metadata_error = 0x10021; return nullptr; }
    uintptr_t address = 0;
    metadata_error = sdk_call<uint32_t(*)(uintptr_t*, size_t)>(binding::allocate_pages)(&address, bytes);
    if (metadata_error || !address) { if (!metadata_error) metadata_error = 0x10021; return nullptr; }
    metadata_bytes += bytes;
    if ((address & 4095) || address > UINTPTR_MAX - bytes || !Memory::IsWritable(address, bytes)) {
        metadata_error = 0x10021; return nullptr;
    }
    return reinterpret_cast<void*>(address);
}
}
