#include "plugin_resources.hpp"
#include "plugin_heap.hpp"
#include "plugin_loader.hpp"
#include "../core/memory.hpp"
#include "../core/slot_store.hpp"
#include "../script/script_manager.hpp"

namespace ScriptHook {
namespace {
struct Entry {
    ShvNxResourceCleanup cleanup;
    void* context;
    uint64_t serial;
    Entry* previous;
    Entry* next;
};
struct Owner {
    void* token;
    SlotStore<Entry, SHV_NX_RESOURCE_INITIAL_CAPACITY, 64> entries;
    Entry* newest;
    Entry* available;
    size_t used;
    uint64_t failures;
    uint32_t count, depth;
    bool draining;
};
SlotStore<Owner, 16> owners;
uint64_t next_serial = 1;
size_t metadata_bytes;

template<class Store> bool ensure(Store& store, size_t index) {
    if (index < store.Capacity()) return true;
    constexpr size_t bytes = (sizeof(typename Store::Page) + 4095) & ~size_t(4095);
    if (!PluginLoader::ReserveExtraMemory(bytes)) return false;
    const auto before = RuntimeConfig::MetadataBytes();
    const bool ready = store.Ensure(index);
    if (RuntimeConfig::MetadataBytes() == before) PluginLoader::ReleaseExtraMemory(bytes);
    else metadata_bytes += bytes;
    return ready;
}

Owner* owner(void* token, bool create = false) {
    if (!token) return nullptr;
    for (auto& value : owners) if (value.token == token) return &value;
    if (!create) return nullptr;
    for (auto& value : owners) if (!value.token) {
        value.token = token;
        return &value;
    }
    const auto next = owners.Capacity();
    if (!ensure(owners, next)) return nullptr;
    owners[next].token = token;
    return &owners[next];
}

Entry* find(Owner* state, ShvNxResourceHandle handle) {
    if (state && handle) for (auto* entry = state->newest; entry; entry = entry->next) {
        if (entry->serial == handle) return entry;
    }
    return nullptr;
}

void failure(Owner* state) {
    if (state && state->failures != UINT64_MAX) ++state->failures;
}

bool remove(Owner& state, Entry& entry) {
    // Clear the heap pin before consumption. An invariant failure preserves
    // the registration and its allocation for diagnosis.
    if (entry.context && !PluginHeap::SetResourcePin(state.token, entry.context, false)) {
        failure(&state);
        return false;
    }
    if (entry.previous) entry.previous->next = entry.next;
    else state.newest = entry.next;
    if (entry.next) entry.next->previous = entry.previous;
    entry = {};
    entry.next = state.available;
    state.available = &entry;
    --state.count;
    return true;
}

bool release(Owner& state, Entry& entry) {
    if (state.depth >= SHV_NX_RESOURCE_CLEANUP_DEPTH_LIMIT ||
        !ScriptManager::CanRunResourceCleanup(entry.cleanup)) return false;
    const auto snapshot = entry;
    if (!remove(state, entry)) return false;
    ++state.depth;
    const auto started = sh_get_tick();
    const bool ok = ScriptManager::RunResourceCleanup(snapshot.cleanup, snapshot.context);
    PluginLoader::RecordTime(state.token, 2, sh_get_tick() - started);
    --state.depth;
    if (!ok) failure(&state);
    return ok;
}
}

uint32_t PluginResources::Count(void* token) {
    const auto* state = owner(token);
    return state ? state->count : 0;
}

bool PluginResources::Active(void* token, ShvNxResourceHandle handle) {
    return find(owner(token), handle) != nullptr;
}

bool PluginResources::Pinned(void* token, const void* pointer) {
    return PluginHeap::ResourcePinned(token, pointer);
}

uint64_t PluginResources::Failures(void* token) {
    const auto* state = owner(token);
    return state ? state->failures : 0;
}

ShvNxResourceHandle PluginResources::Register(void* token, ShvNxResourceCleanup cleanup, void* context) {
    auto* state = owner(token, true);
    if (!state || state->draining || state->depth || !cleanup || !next_serial ||
        state->count == UINT32_MAX ||
        (context && (!PluginHeap::Size(token, context) || Pinned(token, context)))) {
        failure(state);
        return 0;
    }
    Entry* entry = state->available;
    if (!entry) {
        if (!ensure(state->entries, state->used)) {
            failure(state);
            return 0;
        }
        entry = &state->entries[state->used];
    }
    if (context && !PluginHeap::SetResourcePin(token, context, true)) {
        failure(state);
        return 0;
    }
    if (state->available) state->available = entry->next;
    else ++state->used;
    const auto handle = next_serial++;
    *entry = {cleanup, context, handle, nullptr, state->newest};
    if (state->newest) state->newest->previous = entry;
    state->newest = entry;
    ++state->count;
    return handle;
}

bool PluginResources::Forget(void* token, ShvNxResourceHandle handle) {
    auto* state = owner(token);
    auto* entry = find(state, handle);
    return entry && remove(*state, *entry);
}

bool PluginResources::Release(void* token, ShvNxResourceHandle handle) {
    auto* state = owner(token);
    auto* entry = find(state, handle);
    return entry && release(*state, *entry);
}

bool PluginResources::Drain(void* token) {
    auto* state = owner(token);
    if (!state) return true;
    if (state->draining || state->depth) return false;
    state->draining = true;
    bool ok = true;
    // Callbacks can consume other records. Reacquire the newest live entry
    // after each invocation; registration is refused throughout draining.
    while (state->newest) {
        auto* entry = state->newest;
        const auto handle = entry->serial;
        if (!release(*state, *entry)) {
            ok = false;
            if (entry->serial == handle) break;
        }
    }
    state->draining = false;
    return ok && !state->count;
}

size_t PluginResources::MetadataBytes() { return metadata_bytes; }

size_t PluginResources::Capacity(void* token) {
    const auto* state = owner(token);
    return state ? state->entries.Capacity() : 0;
}
}
