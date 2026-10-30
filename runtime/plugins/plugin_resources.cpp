#include "plugin_resources.hpp"
#include "plugin_heap.hpp"
#include "plugin_loader.hpp"
#include "../core/memory.hpp"
#include "../script/script_manager.hpp"

namespace ScriptHook {
namespace {
constexpr size_t OwnerLimit = 16;
struct Entry {
    ShvNxResourceCleanup cleanup;
    void* context;
    uint64_t serial;
};
struct Owner {
    void* token;
    Entry entries[SHV_NX_RESOURCES_PER_OWNER];
    uint64_t failures;
    uint32_t count, depth;
    bool draining;
};
Owner owners[OwnerLimit];
uint64_t next_serial = 1;

Owner* owner(void* token, bool create = false) {
    if (!token) return nullptr;
    for (auto& value : owners) if (value.token == token) return &value;
    if (create) for (auto& value : owners) if (!value.token) { value.token = token; return &value; }
    return nullptr;
}
Entry* find(void* token, ShvNxResourceHandle handle) {
    if (!handle) return nullptr;
    auto* state = owner(token);
    if (state && state->count) for (auto& entry : state->entries) if (entry.serial == handle) return &entry;
    return nullptr;
}
void failure(Owner* value) { if (value && value->failures != UINT64_MAX) ++value->failures; }
}

uint32_t PluginResources::Count(void* token) {
    const auto* state = owner(token);
    return state ? state->count : 0;
}
bool PluginResources::Active(void* token, ShvNxResourceHandle handle) { return find(token, handle) != nullptr; }
bool PluginResources::Pinned(void* token, const void* pointer) {
    if (!pointer) return false;
    const auto* state = owner(token);
    if (state && state->count) for (const auto& entry : state->entries) if (entry.serial && entry.context == pointer) return true;
    return false;
}
uint64_t PluginResources::Failures(void* token) { const auto* value = owner(token); return value ? value->failures : 0; }

ShvNxResourceHandle PluginResources::Register(void* token, ShvNxResourceCleanup cleanup, void* context) {
    auto* state = owner(token, true);
    if (!state || state->draining || state->depth || !cleanup || !next_serial ||
        Count(token) >= SHV_NX_RESOURCES_PER_OWNER ||
        (context && (!PluginHeap::Size(token, context) || Pinned(token, context)))) {
        failure(state); return 0;
    }
    for (auto& entry : state->entries) if (!entry.serial) {
        const auto handle = next_serial++;
        entry = {cleanup, context, handle};
        ++state->count;
        return handle;
    }
    failure(state);
    return 0;
}
bool PluginResources::Forget(void* token, ShvNxResourceHandle handle) {
    auto* entry = find(token, handle);
    if (!entry) return false;
    *entry = {};
    --owner(token)->count;
    return true;
}
bool PluginResources::Release(void* token, ShvNxResourceHandle handle) {
    auto* entry = find(token, handle);
    auto* state = owner(token);
    if (!entry || !state || !ScriptManager::CanRunResourceCleanup(entry->cleanup)) return false;
    const auto snapshot = *entry;
    *entry = {}; // Unpin before invoking; its callback may destroy/free context.
    --state->count;
    ++state->depth;
    const auto started = sh_get_tick();
    const bool ok = ScriptManager::RunResourceCleanup(snapshot.cleanup, snapshot.context);
    PluginLoader::RecordTime(token, 2, sh_get_tick() - started);
    --state->depth;
    if (!ok) failure(state);
    return ok;
}
bool PluginResources::Drain(void* token) {
    auto* state = owner(token);
    if (!state) return true;
    if (state->draining || state->depth) return false;
    state->draining = true;
    bool ok = true;
    for (uint32_t n = 0; n < SHV_NX_RESOURCES_PER_OWNER; ++n) {
        uint64_t newest = 0;
        for (const auto& entry : state->entries) if (entry.serial > newest) newest = entry.serial;
        if (!newest) break;
        if (!Release(token, newest)) {
            ok = false;
            // Failed preconditions leave the entry pinned for later inspection.
            if (Active(token, newest)) break;
        }
    }
    state->draining = false;
    return ok && !Count(token);
}
}
