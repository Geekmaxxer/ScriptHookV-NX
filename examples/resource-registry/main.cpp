#include "script.h"
#include "shv/memory.h"
#include "shv/text.h"

namespace {
constexpr uint32_t RecordCount = 512;
struct Record {
    uint32_t id, epoch;
    Record(uint32_t index, uint32_t lifetime) noexcept : id(index), epoch(lifetime) {}
};
ShvNxResourceHandle handles[RecordCount];
Record* records[RecordCount];
uint32_t run, failed, completed, registered, verified, destroyed, expected, peak_resources;
bool cleanup_complete;

void Report() {
    ShvNxMemoryStats stats{};
    const bool available = scriptGetMemoryStats(&stats);
    ScriptHook::Util::TextBuffer<384> text;
    text.Add("run=").UInt(run).Add("\nfailed=").UInt(failed)
        .Add("\ncompleted=").UInt(completed).Add("\nregistered=").UInt(registered)
        .Add("\nverified=").UInt(verified).Add("\ndestroyed=").UInt(destroyed)
        .Add("\npeak_resources=").UInt(peak_resources)
        .Add("\nreverse_order_verified=").UInt(!expected && registered == destroyed)
        .Add("\nowner_cleanup_complete=").UInt(cleanup_complete)
        .Add("\nowned_allocations=").UInt(available ? uint32_t(stats.allocations) : UINT32_MAX)
        .Add("\nregistered_resources=").UInt(scriptResourceCount()).Add("\n");
    if (text.Complete()) (void)scriptSaveFile("result.txt", text.CStr(), text.Size());
}

void Audit(void*) {
    ShvNxMemoryStats stats{};
    cleanup_complete = !expected && registered == destroyed && scriptGetMemoryStats(&stats) &&
        !stats.allocations && !scriptResourceCount() && scriptTrimMemory();
    if (!cleanup_complete) ++failed;
    Report();
}

void Cleanup(void* context) {
    auto* record = static_cast<Record*>(context);
    if (record->epoch != run || record->id != expected || !expected) ++failed;
    if (expected) --expected;
    ++destroyed;
    if (!ScriptHook::Memory::Delete(record)) ++failed;
}

bool Initialize() { return scriptMemoryAvailable() && scriptResourcesAvailable(); }

void Main() {
    ++run;
    failed = completed = registered = verified = destroyed = expected = peak_resources = 0;
    cleanup_complete = false;
    if (!scriptResourceRegister(Audit, nullptr)) { ++failed; Report(); return; }
    for (uint32_t i = 0; i < RecordCount; ++i) {
        auto* record = ScriptHook::Memory::TryNew<Record>(i + 1, run);
        if (!record) { ++failed; return; }
        const auto handle = scriptResourceRegister(Cleanup, record);
        if (!handle) {
            (void)ScriptHook::Memory::Delete(record);
            ++failed;
            return;
        }
        records[i] = record;
        handles[i] = handle;
        expected = ++registered;
        peak_resources = scriptResourceCount();
        if (i % 16 == 15) WAIT(0);
    }
    for (uint32_t i = 0; i < RecordCount; ++i) {
        if (!scriptResourceActive(handles[i]) || !scriptResourcePinned(records[i]) ||
            records[i]->id != i + 1 || records[i]->epoch != run) { ++failed; return; }
        ++verified;
        if (i % 16 == 15) WAIT(0);
    }
    completed = 1;
}
}
SHV_NX_PLUGIN_WITH_INIT("resource-registry-example", Main, Initialize)
