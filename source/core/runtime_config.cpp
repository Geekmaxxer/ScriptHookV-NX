#include "runtime_config.hpp"
#include "storage.hpp"
#include "../script/script_manager.hpp"
namespace ScriptHook {
namespace {
size_t residents = SHVNX_DEFAULT_RESIDENT_PLUGINS;
size_t scripts = SHVNX_DEFAULT_SCRIPT_SLOTS;
size_t memory_mib = SHVNX_DEFAULT_PLUGIN_MEMORY_MIB;
uint32_t state, error, applied, invalid, stack_error;
bool equal(const char* first, const char* last, const char* key) {
    while (first != last && *key && *first == *key) { ++first; ++key; }
    return first == last && !*key;
}
bool space(char c) { return c == ' ' || c == '\t' || c == '\r'; }
void parse(const char* data, size_t bytes) {
    size_t values[] = {SHVNX_DEFAULT_RESIDENT_PLUGINS, SHVNX_DEFAULT_SCRIPT_SLOTS, SHVNX_DEFAULT_PLUGIN_MEMORY_MIB};
    constexpr size_t ceilings[] = {SHVNX_MAX_RESIDENT_PLUGINS, SHVNX_MAX_SCRIPT_SLOTS, SHVNX_MAX_PLUGIN_MEMORY_MIB};
    const char* names[] = {"resident_limit", "script_slots", "plugin_memory_mib"};
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
        for (size_t i = 0; i < 3; ++i) {
            if (!equal(first, key_end, names[i])) continue;
            const uint32_t bit = 1u << i;
            const bool duplicate = (seen & bit) != 0; seen |= bit;
            const char* p = key_end; while (p != last && space(*p)) ++p;
            bool good = p != last && *p++ == '=';
            while (p != last && space(*p)) ++p;
            size_t value = 0, digits = 0;
            while (p != last && *p >= '0' && *p <= '9') {
                if (++digits > 9) good = false;
                if (digits <= 9) value = value * 10 + size_t(*p - '0');
                ++p;
            }
            good = good && digits && p == last && value >= 1 && value <= ceilings[i] && !duplicate;
            if (!good) invalid |= bit;
            if (good) values[i] = value;
        }
    }
    applied = seen & ~invalid;
    if (applied & 1) residents = values[0];
    if (applied & 2) scripts = values[1];
    if (applied & 4) memory_mib = values[2];
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
    const auto requested = scripts;
    scripts = ScriptManager::ConfigureSlots(requested, stack_error);
    if (scripts != requested) { invalid |= 2; applied &= ~2u; }
}
size_t RuntimeConfig::Residents() { return residents; }
size_t RuntimeConfig::Scripts() { return scripts; }
size_t RuntimeConfig::MemoryBytes() { return memory_mib * 1024 * 1024; }
uint32_t RuntimeConfig::State() { return state; }
uint32_t RuntimeConfig::Error() { return error; }
uint32_t RuntimeConfig::AppliedMask() { return applied; }
uint32_t RuntimeConfig::InvalidMask() { return invalid; }
uint32_t RuntimeConfig::StackError() { return stack_error; }
}
