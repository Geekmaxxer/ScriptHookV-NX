#include "script.h"
#include "shv/hash_map.h"
#include "shv/managed.h"
#include "shv/text.h"

using namespace ScriptHook;
using namespace ScriptHook::Containers;
namespace {
struct CollisionHash { size_t operator()(uint32_t) const noexcept { return 7; } };
struct Registry {
    HashMap<uint32_t, uint32_t> entities;
    HashMap<String, uint32_t> names;
    HashMap<uint32_t, uint32_t, CollisionHash> collisions;
};
uint32_t run, failed, completed, records, names, collision_records;
bool cleanup_complete;

void SaveReport() {
    ShvNxMemoryStats stats{};
    const bool memory_available = scriptGetMemoryStats(&stats);
    Util::TextBuffer<384> report;
    report.Add("run=").UInt(run).Add("\nfailed=").UInt(failed)
        .Add("\ncompleted=").UInt(completed).Add("\nrecords=").UInt(records)
        .Add("\nnames=").UInt(names).Add("\ncollision_records=").UInt(collision_records)
        .Add("\nowner_cleanup_complete=").UInt(cleanup_complete)
        .Add("\nowned_allocations=").UInt(memory_available ? uint32_t(stats.allocations) : UINT32_MAX)
        .Add("\nregistered_resources=").UInt(scriptResourceCount()).Add("\n");
    if (report.Complete()) (void)scriptSaveFile("result.txt", report.CStr(), report.Size());
}

void FinalAudit(void*) {
    ShvNxMemoryStats stats{};
    cleanup_complete = scriptGetMemoryStats(&stats) && !stats.allocations &&
                       !scriptResourceCount() && scriptTrimMemory();
    if (!cleanup_complete) ++failed;
    SaveReport();
}

bool Initialize() { return scriptMemoryAvailable() && scriptResourcesAvailable(); }

void Main() {
    ++run;
    failed = completed = records = names = collision_records = 0;
    cleanup_complete = false;
    if (!scriptResourceRegister(FinalAudit, nullptr)) { ++failed; SaveReport(); return; }
    auto owner = Memory::Managed<Registry>::TryMake();
    if (!owner) { ++failed; return; }
    for (uint32_t i = 0; i < 2048; ++i) {
        {
            auto* state = owner.Get();
            if (!state || state->entities.TryInsert(i, i * 17 + 3) != HashInsertResult::Inserted) { ++failed; return; }
        }
        if (i % 64 == 63) WAIT(0);
    }
    for (uint32_t i = 0; i < 2048; ++i) {
        {
            auto* state = owner.Get();
            if (!state) { ++failed; return; }
            if (i % 3 == 0 && !state->entities.Erase(i)) { ++failed; return; }
            const auto* value = state->entities.Find(i);
            if ((i % 3 == 0 && value) || (i % 3 != 0 && (!value || *value != i * 17 + 3))) { ++failed; return; }
            records = uint32_t(state->entities.Size());
        }
        if (i % 64 == 63) WAIT(0);
    }
    for (uint32_t i = 0; i < 128; ++i) {
        {
            auto* state = owner.Get();
            Util::TextBuffer<32> label;
            label.Add("registry-").UInt(i);
            String key;
            if (!state || !key.TryAssign(label.CStr(), label.Size()) ||
                state->names.TryInsert(std::move(key), i) != HashInsertResult::Inserted ||
                state->collisions.TryInsert(i, i + 2699) != HashInsertResult::Inserted) { ++failed; return; }
            const auto* found = state->names.FindAs(StringKeyView{label.CStr(), label.Size()});
            if (!found || *found != i) { ++failed; return; }
            names = uint32_t(state->names.Size());
        }
        if (i % 16 == 15) WAIT(0);
    }
    for (uint32_t i = 0; i < 128; ++i) {
        {
            auto* state = owner.Get();
            if (!state) { ++failed; return; }
            if (i % 2 && !state->collisions.Erase(i)) { ++failed; return; }
            const auto* found = state->collisions.Find(i);
            if ((i % 2 && found) || (!(i % 2) && (!found || *found != i + 2699))) { ++failed; return; }
            collision_records = uint32_t(state->collisions.Size());
        }
        if (i % 16 == 15) WAIT(0);
    }
    completed = 1;
    // Scope exit releases the managed registry before the final audit drains.
}
}
SHV_NX_PLUGIN_WITH_INIT("hash-registry-example", Main, Initialize)
