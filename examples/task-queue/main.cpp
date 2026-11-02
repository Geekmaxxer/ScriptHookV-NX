#include "script.h"
#include "shv/deque.h"
#include "shv/function.h"
#include "shv/containers.h"
#include "shv/managed.h"
#include "shv/text.h"

using ScriptHook::Callbacks::Function;
using ScriptHook::Containers::Deque;
namespace {
uint32_t run, failed, completed, queued, executed, next_id, live_captures, peak_queue;
uint64_t sum;
bool wrapped_verified, relocation_verified, cleanup_complete;

struct Action {
    ScriptHook::Containers::String label;
    uint32_t id, epoch;
    explicit Action(uint32_t value) noexcept : id(value), epoch(run) { ++live_captures; }
    Action(const Action&) = delete;
    Action(Action&& other) noexcept : label(std::move(other.label)), id(other.id), epoch(other.epoch) { ++live_captures; }
    ~Action() noexcept { --live_captures; }
    void operator()() noexcept {
        if (id != next_id || epoch != run || strcmp(label.CStr(), "queued-action")) ++failed;
        ++next_id; ++executed; sum += id;
    }
};
struct alignas(256) LargeAction : Action {
    unsigned char padding[96]{};
    explicit LargeAction(uint32_t id) noexcept : Action(id) {}
    LargeAction(LargeAction&&) noexcept = default;
};
struct State { Deque<Function<void()>> tasks; };

void Report() {
    ShvNxMemoryStats stats{};
    const bool available = scriptGetMemoryStats(&stats);
    ScriptHook::Util::TextBuffer<448> text;
    text.Add("run=").UInt(run).Add("\nfailed=").UInt(failed)
        .Add("\ncompleted=").UInt(completed).Add("\nqueued=").UInt(queued)
        .Add("\nexecuted=").UInt(executed).Add("\nsum=").UInt(uint32_t(sum))
        .Add("\npeak_queue=").UInt(peak_queue)
        .Add("\nwrapped_verified=").UInt(wrapped_verified)
        .Add("\nrelocation_verified=").UInt(relocation_verified)
        .Add("\nlive_captures=").UInt(live_captures)
        .Add("\nowner_cleanup_complete=").UInt(cleanup_complete)
        .Add("\nowned_allocations=").UInt(available ? uint32_t(stats.allocations) : UINT32_MAX)
        .Add("\nregistered_resources=").UInt(scriptResourceCount()).Add("\n");
    if (text.Complete()) (void)scriptSaveFile("result.txt", text.CStr(), text.Size());
}
void Audit(void*) {
    ShvNxMemoryStats stats{};
    cleanup_complete = !live_captures && scriptGetMemoryStats(&stats) &&
        !stats.allocations && !scriptResourceCount() && scriptTrimMemory();
    if (!cleanup_complete) ++failed;
    Report();
}
template<class T> bool Enqueue(State& state, uint32_t id) {
    T action(id);
    Function<void()> function;
    if (!action.label.TryAssign("queued-action") || !function.TryAssign(std::move(action)) ||
        !state.tasks.TryPushBack(std::move(function))) return false;
    ++queued;
    if (state.tasks.Size() > peak_queue) peak_queue = uint32_t(state.tasks.Size());
    return true;
}
bool RunOne(State& state) {
    if (state.tasks.Empty()) return false;
    Function<void()> action(std::move(*state.tasks.Front()));
    return state.tasks.PopFront() && action.TryInvoke();
}
bool Initialize() { return scriptMemoryAvailable() && scriptResourcesAvailable(); }
void Main() {
    ++run;
    failed = completed = queued = executed = live_captures = peak_queue = 0;
    next_id = 1; sum = 0;
    wrapped_verified = relocation_verified = cleanup_complete = false;
    if (!scriptResourceRegister(Audit, nullptr)) { ++failed; Report(); return; }
    auto owner = ScriptHook::Memory::Managed<State>::TryMake();
    if (!owner) { ++failed; return; }
    for (uint32_t id = 1; id <= 512; ++id) {
        {
            auto* state = owner.Get();
            if (!state || !(id % 2 ? Enqueue<Action>(*state, id) : Enqueue<LargeAction>(*state, id))) { ++failed; return; }
        }
        if (id % 16 == 0) WAIT(0);
    }
    for (uint32_t i = 0; i < 256; ++i) {
        { auto* state = owner.Get(); if (!state || !RunOne(*state)) { ++failed; return; } }
        if (i % 16 == 15) WAIT(0);
    }
    for (uint32_t id = 513; id <= 768; ++id) {
        {
            auto* state = owner.Get();
            if (!state || !(id % 2 ? Enqueue<Action>(*state, id) : Enqueue<LargeAction>(*state, id))) { ++failed; return; }
        }
        if (id % 16 == 0) WAIT(0);
    }
    {
        auto* state = owner.Get();
        if (!state || state->tasks.Size() != 512 || state->tasks.Capacity() != 512) { ++failed; return; }
        wrapped_verified = true;
        if (!state->tasks.TryReserve(1024)) { ++failed; return; }
        relocation_verified = true;
    }
    for (uint32_t i = 0; i < 512; ++i) {
        { auto* state = owner.Get(); if (!state || !RunOne(*state)) { ++failed; return; } }
        if (i % 16 == 15) WAIT(0);
    }
    if (executed != 768 || sum != 295296 || next_id != 769) { ++failed; return; }
    completed = 1;
}
}
SHV_NX_PLUGIN_WITH_INIT("task-queue-example", Main, Initialize)
