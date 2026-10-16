#include "script_manager.hpp"
#include "../core/memory.hpp"
#include "../../include/nativeCaller.h"
#include "../../include/natives.h"
#include "bindings.hpp"
#include "../plugins/plugin_loader.hpp"
#include "../core/storage.hpp"

namespace ScriptHook {
namespace {
constexpr size_t none = ScriptManager::MaxScripts;
constexpr uint64_t ticks_per_ms = 19200, guard = 0x5348564e5853544bULL;
enum State : uint32_t { Free, Ready, Running, Sleeping, Finished, Reserved };
struct Entry {
    FiberContext context;
    ScriptFunction function;
    uint64_t wake_tick, earliest_dispatch, native_hash;
    NativeCallStatus native_status;
    State state;
    bool cancel;
};
Entry scripts[ScriptManager::MaxScripts];
alignas(16) uint8_t stacks[ScriptManager::BaseScripts][ScriptManager::StackBytes];
uintptr_t extra_stacks;
size_t slot_limit = ScriptManager::BaseScripts;
uint8_t* stack_memory(size_t slot) {
    return slot < ScriptManager::BaseScripts ? stacks[slot] :
        reinterpret_cast<uint8_t*>(extra_stacks) + (slot - ScriptManager::BaseScripts) * ScriptManager::StackBytes;
}
FiberContext scheduler;
size_t current = none, next_slot;
uintptr_t owner_tls, host_pointer;
uint32_t host_id, frame_seen;
bool have_frame, dispatching;
uint64_t serial;
uint32_t mutation_lock, runtime_status;
ScriptRuntimeInfo info;
bool test_pending, test_running, test_main_done, test_peer_done, test_reported;

void set_status(ScriptRuntimeStatus value) { __atomic_store_n(&runtime_status, uint32_t(value), __ATOMIC_RELEASE); }
bool own_thread() { return !owner_tls || owner_tls == sh_get_thread_pointer(); }
struct Mutation {
    bool held;
    Mutation() : held(false) {
        if (!own_thread()) { set_status(ScriptRuntime_WrongThread); return; }
        uint32_t expected = 0;
        held = __atomic_compare_exchange_n(&mutation_lock, &expected, 1, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED);
        if (!held) set_status(ScriptRuntime_Busy);
    }
    ~Mutation() { if (held) __atomic_store_n(&mutation_lock, 0u, __ATOMIC_RELEASE); }
};
// Borrow a live game script thread only between interpreter calls. Validate
// every destination before the first write and restore the exact snapshots.
struct GameContext {
    uintptr_t tls, saved_thread = 0, saved_global = 0, saved_aux = 0;
    uint8_t saved_active = 0;
    bool valid = false;
    GameContext(void* host) : tls(sh_get_thread_pointer()) {
        const uintptr_t base = Memory::s_mainBase;
        if (!base || !tls || !Memory::IsWritable(tls + binding::script_tls_thread, 8) ||
            !Memory::IsWritable(tls + binding::script_tls_active, 1) ||
            !Memory::IsWritable(base + binding::active_script_thread, 8) ||
            !Memory::IsWritable(base + binding::active_script_aux, 8) ||
            !Memory::Read(tls + binding::script_tls_thread, saved_thread) ||
            !Memory::Read(tls + binding::script_tls_active, saved_active) || saved_active ||
            !Memory::Read(base + binding::active_script_thread, saved_global) ||
            !Memory::Read(base + binding::active_script_aux, saved_aux)) return;
        *reinterpret_cast<uintptr_t*>(tls + binding::script_tls_thread) = reinterpret_cast<uintptr_t>(host);
        *reinterpret_cast<uintptr_t*>(base + binding::active_script_thread) = reinterpret_cast<uintptr_t>(host);
        *reinterpret_cast<uintptr_t*>(base + binding::active_script_aux) = reinterpret_cast<uintptr_t>(host) + 0xd4;
        *reinterpret_cast<uint8_t*>(tls + binding::script_tls_active) = 1;
        valid = true;
    }
    ~GameContext() {
        if (!valid) return;
        *reinterpret_cast<uintptr_t*>(tls + binding::script_tls_thread) = saved_thread;
        *reinterpret_cast<uintptr_t*>(Memory::s_mainBase + binding::active_script_thread) = saved_global;
        *reinterpret_cast<uintptr_t*>(Memory::s_mainBase + binding::active_script_aux) = saved_aux;
        *reinterpret_cast<uint8_t*>(tls + binding::script_tls_active) = saved_active;
    }
};
bool guards_ok(size_t slot) {
    const auto* low = reinterpret_cast<const uint64_t*>(stack_memory(slot));
    const auto* high = reinterpret_cast<const uint64_t*>(stack_memory(slot) + ScriptManager::StackBytes - 16);
    if (scripts[slot].context.sp < reinterpret_cast<uintptr_t>(low + 16) ||
        scripts[slot].context.sp > reinterpret_cast<uintptr_t>(high) || scripts[slot].context.sp % 16) return false;
    for (size_t i = 0; i < 16; ++i) if (low[i] != guard) return false;
    return high[0] == guard && high[1] == guard;
}
void release(Entry& entry, bool cancelled) {
    if (cancelled) ++info.cancellations; else ++info.completions;
    entry.state = Free; entry.function = nullptr; entry.cancel = false;
    if (info.active) --info.active;
}
void test_check(bool good) { if (good) ++info.self_test_passed; else ++info.self_test_failed; }
void peer_test() {
    for (uint32_t i = 0; i < 5; ++i) { ++info.self_test_peer_steps; scriptWait(0); }
    test_peer_done = true;
}
void main_test() {
    volatile uint64_t local = 0x12345678abcdef00ULL;
    volatile float fraction = 1.25f;
    const uint32_t first_frame = uint32_t(Natives::MISC::GET_FRAME_COUNT());
    test_check(getNativeCallStatus() == NativeCall_Ok);
    scriptWait(0);
    const auto next_frame = uint32_t(Natives::MISC::GET_FRAME_COUNT());
    test_check(next_frame != first_frame && local == 0x12345678abcdef00ULL && fraction == 1.25f);
    const auto start = sh_get_tick();
    scriptWait(100);
    const auto elapsed = sh_get_tick() - start;
    info.self_test_wait_ms = uint32_t(elapsed / ticks_per_ms);
    test_check(elapsed >= 100 * ticks_per_ms);
    test_check(info.self_test_peer_steps >= 2);
    test_check(uint32_t(Natives::MISC::GET_HASH_KEY("adder")) == 0xb779a091u && getNativeCallStatus() == NativeCall_Ok);
    int32_t width = 0, height = 0;
    Natives::GRAPHICS::GET_SCREEN_RESOLUTION(&width, &height);
    test_check(getNativeCallStatus() == NativeCall_Ok && width > 0 && height > 0);
    scriptWait(0);
    test_check(local == 0x12345678abcdef00ULL && fraction == 1.25f);
    test_main_done = true;
}
void write_test_report() {
    char report[1280]{}; size_t at = 0;
    auto append = [&](const char* s) { while (*s && at + 1 < sizeof(report)) report[at++] = *s++; };
    auto field = [&](const char* name, uint64_t value) {
        append(name); append("=0x"); char digits[16]; size_t n = 0;
        do { digits[n++] = "0123456789abcdef"[value & 15]; value >>= 4; } while (value);
        while (n && at + 1 < sizeof(report)) report[at++] = digits[--n];
        append("\n");
    };
    append("ScriptHookV-NX cooperative runtime test 1\n");
    field("runtime_abi", SHV_NX_RUNTIME_ABI_VERSION);
    field("runtime_test_runs", info.self_test_runs); field("runtime_test_done", info.self_test_done);
    field("runtime_test_passed", info.self_test_passed); field("runtime_test_failed", info.self_test_failed);
    field("runtime_wait_ms", info.self_test_wait_ms); field("runtime_peer_steps", info.self_test_peer_steps);
    field("runtime_dispatches", info.dispatches); field("runtime_resumes", info.resumes);
    field("runtime_waits", info.waits); field("runtime_completions", info.completions);
    field("runtime_active", info.active); field("runtime_context_rejections", info.context_rejections);
    field("runtime_stack_failures", info.stack_failures); field("runtime_budget_overruns", info.budget_overruns);
    field("runtime_host_thread_id", host_id); field("runtime_last_status", runtime_status);
    (void)scriptSaveFile("runtime-report.txt", report, at);
}
} // namespace

void ScriptManager::Initialize() {
    NativeInvoker::Initialize();
    for (auto& entry : scripts) entry = {};
    current = none; next_slot = 0; owner_tls = host_pointer = serial = 0;
    host_id = frame_seen = mutation_lock = 0; have_frame = dispatching = false;
    info = {}; info.abi = SHV_NX_RUNTIME_ABI_VERSION;
    test_pending = test_running = test_main_done = test_peer_done = test_reported = false;
    set_status(ScriptRuntime_Ok);
}
size_t ScriptManager::ConfigureSlots(size_t requested, uint32_t& error) {
    error = 0;
    Mutation lock;
    if (!lock.held || dispatching || requested < 1 || requested > MaxScripts) { error = 0x10021; return slot_limit; }
    // Preserve any script/reservation registered before the first VM tick.
    for (size_t i = requested; i < MaxScripts; ++i)
        if (scripts[i].state != Free) { error = 0x10022; return slot_limit; }
    if (requested > BaseScripts && !extra_stacks) {
        uintptr_t address = 0;
        const size_t bytes = (requested - BaseScripts) * StackBytes;
        error = sdk_call<uint32_t(*)(uintptr_t*,size_t)>(binding::allocate_pages)(&address, bytes);
        if (error || !address || address % 4096) { if (!error) error = 0x10021; return slot_limit; }
        extra_stacks = address;
    }
    slot_limit = requested;
    return slot_limit;
}
size_t ScriptManager::SlotLimit() { return slot_limit; }
size_t ScriptManager::StackAllocationBytes() { return (slot_limit > BaseScripts ? slot_limit : BaseScripts) * StackBytes; }
bool ScriptManager::Register(ScriptFunction function) {
    Mutation lock; if (!lock.held) return false;
    if (!function || !Memory::IsExecutable(reinterpret_cast<uintptr_t>(function))) {
        set_status(ScriptRuntime_InvalidFunction); return false;
    }
    for (const auto& entry : scripts) if (entry.state != Free && entry.function == function) {
        set_status(ScriptRuntime_Ok); return true;
    }
    for (size_t i = 0; i < slot_limit; ++i) if (scripts[i].state == Free) {
        auto& entry = scripts[i]; entry = {};
        auto* low = reinterpret_cast<uint64_t*>(stack_memory(i));
        for (size_t j = 0; j < 16; ++j) low[j] = guard;
        auto* high = reinterpret_cast<uint64_t*>(stack_memory(i) + StackBytes - 16);
        high[0] = high[1] = guard;
        entry.context.sp = reinterpret_cast<uintptr_t>(high);
        entry.context.x19_x30[11] = reinterpret_cast<uintptr_t>(&ScriptManager::RunEntry);
        asm volatile("mrs %0, fpcr" : "=r"(entry.context.fpcr));
        asm volatile("mrs %0, fpsr" : "=r"(entry.context.fpsr));
        entry.function = function; entry.state = Ready; entry.earliest_dispatch = serial + 1;
        ++info.active; set_status(ScriptRuntime_Ok); return true;
    }
    set_status(ScriptRuntime_Capacity); return false;
}
uint32_t ScriptManager::ReserveSlot() {
    Mutation lock; if (!lock.held) return 0;
    for (size_t i = 0; i < slot_limit; ++i) if (scripts[i].state == Free) {
        scripts[i] = {}; scripts[i].state = Reserved;
        set_status(ScriptRuntime_Ok); return uint32_t(i + 1);
    }
    set_status(ScriptRuntime_Capacity); return 0;
}
bool ScriptManager::CommitReserved(uint32_t token, ScriptFunction function) {
    Mutation lock; if (!lock.held) return false;
    if (!token || token > slot_limit || scripts[token - 1].state != Reserved ||
        !function || !Memory::IsExecutable(reinterpret_cast<uintptr_t>(function))) {
        set_status(ScriptRuntime_InvalidFunction); return false;
    }
    const auto i = size_t(token - 1); auto& entry = scripts[i]; entry = {};
    auto* low = reinterpret_cast<uint64_t*>(stack_memory(i));
    for (size_t j = 0; j < 16; ++j) low[j] = guard;
    auto* high = reinterpret_cast<uint64_t*>(stack_memory(i) + StackBytes - 16);
    high[0] = high[1] = guard;
    entry.context.sp = reinterpret_cast<uintptr_t>(high);
    entry.context.x19_x30[11] = reinterpret_cast<uintptr_t>(&ScriptManager::RunEntry);
    asm volatile("mrs %0, fpcr" : "=r"(entry.context.fpcr));
    asm volatile("mrs %0, fpsr" : "=r"(entry.context.fpsr));
    entry.function = function; entry.state = Ready; entry.earliest_dispatch = serial + 1;
    ++info.active; set_status(ScriptRuntime_Ok); return true;
}
void ScriptManager::ReleaseReserved(uint32_t token) {
    Mutation lock; if (!lock.held) return;
    if (token && token <= MaxScripts && scripts[token - 1].state == Reserved) scripts[token - 1] = {};
}
void ScriptManager::Unregister(ScriptFunction function) {
    Mutation lock; if (!lock.held) return;
    for (auto& entry : scripts) if (entry.state != Free && entry.function == function) {
        if (entry.state == Running) entry.cancel = true; else release(entry, true);
        break;
    }
    set_status(ScriptRuntime_Ok);
}
bool ScriptManager::Contains(ScriptFunction function) {
    if(!own_thread()||!function)return false;
    for(const auto& entry:scripts)if(entry.function==function&&
        (entry.state==Ready||entry.state==Running||entry.state==Sleeping))return true;
    return false;
}
void ScriptManager::StopAll() {
    Mutation lock; if (!lock.held) return;
    for (auto& entry : scripts) if (entry.state != Free) {
        if (entry.state == Reserved) { entry = {}; continue; }
        if (entry.state == Running) entry.cancel = true; else release(entry, true);
    }
    if (test_running || test_pending) {
        test_running = test_pending = false;
        ++info.self_test_failed; info.self_test_done = 1;
    }
    set_status(ScriptRuntime_Ok);
}
void ScriptManager::Wait(uint32_t ms) {
    if (!own_thread()) { set_status(ScriptRuntime_WrongThread); return; }
    if (!dispatching || current == none || scripts[current].state != Running) {
        set_status(ScriptRuntime_NotRunning); return;
    }
    if (!NativeInvoker::CanYield()) { set_status(ScriptRuntime_UnsafeYield); return; }
    auto& entry = scripts[current];
    if (!guards_ok(current)) { ++info.stack_failures; entry.cancel = true; set_status(ScriptRuntime_StackCorrupt); }
    else set_status(ScriptRuntime_Ok);
    entry.native_status = getNativeCallStatus(); entry.native_hash = getLastNativeHash();
    entry.wake_tick = sh_get_tick() + uint64_t(ms) * ticks_per_ms;
    entry.earliest_dispatch = serial + 1;
    entry.state = entry.cancel ? Finished : Sleeping; ++info.waits;
    sh_fiber_switch(&entry.context, &scheduler);
}
void ScriptManager::RunEntry() {
    auto& entry = scripts[current]; entry.function();
    if (!NativeInvoker::CanYield()) { NativeInvoker::AbandonCalls(); entry.cancel = true; set_status(ScriptRuntime_UnsafeYield); }
    entry.state = Finished;
    sh_fiber_switch(&entry.context, &scheduler);
    __builtin_trap();
}
void ScriptManager::OnTick(void* host) {
    if ((!info.active && !test_pending && !PluginLoader::Pending()) || !Memory::s_mainBase || dispatching) return;
    if (!own_thread()) { ++info.context_rejections; return; }
    // Every live RAGE script can reach this hook in the same game frame.
    // Once that frame has already dispatched, avoid borrowing its context or
    // invoking a native again. A changed/unreadable counter falls through to
    // the full checks below, and only a successful native query commits it.
    uint32_t observed_frame = 0;
    if (have_frame && Memory::Read(Memory::s_mainBase + binding::game_frame_counter, observed_frame) &&
        observed_frame == frame_seen) return;
    uint32_t hash = 0, id = 0, state = 0;
    const auto pointer = reinterpret_cast<uintptr_t>(host);
    if (!Memory::IsReadable(pointer, 0xe0) || !Memory::Read(pointer + 0xc, hash) || !hash) return;
    if (!Memory::Read(pointer + 8, id) || !id || !Memory::Read(pointer + 0x10, state)) return;
    if (state > 1) {
        return;
    }
    if (!NativeInvoker::CanYield()) { ++info.context_rejections; return; }
    GameContext context(host);
    if (!context.valid) { ++info.context_rejections; return; }
    owner_tls = context.tls; host_pointer = pointer; host_id = id;
    const auto saved_status = getNativeCallStatus(); const auto saved_hash = getLastNativeHash();
    const auto frame = uint32_t(Natives::MISC::GET_FRAME_COUNT());
    if (getNativeCallStatus() != NativeCall_Ok) {
        ++info.context_rejections; NativeInvoker::RestoreStatus(saved_status, saved_hash); return;
    }
    NativeInvoker::RestoreStatus(saved_status, saved_hash);
    if (have_frame && frame_seen == frame) return;
    have_frame = true; frame_seen = frame;
    PluginLoader::Tick();
    if (test_pending) {
        test_pending = false; test_main_done = test_peer_done = test_reported = false;
        info.self_test_passed = info.self_test_failed = info.self_test_done = 0;
        info.self_test_peer_steps = info.self_test_wait_ms = 0; ++info.self_test_runs;
        test_running = Register(main_test) && Register(peer_test);
        if (!test_running) { Unregister(main_test); ++info.self_test_failed; info.self_test_done = 1; }
    }
    dispatching = true; ++serial; ++info.dispatches;
    const auto started = sh_get_tick();
    const auto first = next_slot;
    for (size_t n = 0; n < slot_limit; ++n) {
        const auto i = (first + n) % slot_limit; auto& entry = scripts[i];
        if ((entry.state != Ready && entry.state != Sleeping) || entry.earliest_dispatch > serial ||
            int64_t(sh_get_tick() - entry.wake_tick) < 0) continue;
        if (!guards_ok(i)) { ++info.stack_failures; set_status(ScriptRuntime_StackCorrupt); release(entry, true); continue; }
        current = i; entry.state = Running; ++info.resumes;
        NativeInvoker::RestoreStatus(entry.native_status, entry.native_hash);
        sh_fiber_switch(&scheduler, &entry.context);
        current = none;
        if (!guards_ok(i)) { ++info.stack_failures; entry.cancel = true; set_status(ScriptRuntime_StackCorrupt); }
        if (entry.state == Finished || entry.cancel) release(entry, entry.cancel);
        next_slot = (i + 1) % slot_limit;
        if (sh_get_tick() - started >= 2 * ticks_per_ms) { ++info.budget_overruns; break; }
    }
    dispatching = false;
    NativeInvoker::RestoreStatus(saved_status, saved_hash);
    if (test_running && test_main_done && test_peer_done) {
        test_running = false; info.self_test_done = 1;
        test_check(info.self_test_peer_steps == 5);
    }
    if (info.self_test_done && !test_reported) { test_reported = true; write_test_report(); }
}
bool ScriptManager::RequestSelfTest() {
    Mutation lock; if (!lock.held) return false;
    if (test_pending || test_running || info.active) { set_status(ScriptRuntime_Busy); return false; }
    uintptr_t thread = 0, global = 0; uint8_t active = 0; uint32_t hash = 0;
    const auto tls = sh_get_thread_pointer();
    if (!Memory::Read(tls + binding::script_tls_active, active) || active != 1 ||
        !Memory::Read(tls + binding::script_tls_thread, thread) ||
        !Memory::Read(Memory::s_mainBase + binding::active_script_thread, global) || global != thread ||
        !Memory::Read(thread + 0xc, hash) || !hash) {
        set_status(ScriptRuntime_NoContext); return false;
    }
    test_pending = true; set_status(ScriptRuntime_Ok); return true;
}
bool ScriptManager::Info(ScriptRuntimeInfo* out, uint32_t bytes) {
    if (!own_thread()) { set_status(ScriptRuntime_WrongThread); return false; }
    if (bytes != sizeof(info) || !Memory::IsWritable(reinterpret_cast<uintptr_t>(out), bytes)) return false;
    *out = info; return true;
}
ScriptRuntimeStatus ScriptManager::Status() { return ScriptRuntimeStatus(__atomic_load_n(&runtime_status, __ATOMIC_ACQUIRE)); }
} // namespace ScriptHook

extern "C" {
void scriptRegister(ScriptFunction fn) { (void)ScriptHook::ScriptManager::Register(fn); }
bool scriptRegisterChecked(ScriptFunction fn) { return ScriptHook::ScriptManager::Register(fn); }
void scriptUnregister(ScriptFunction fn) { ScriptHook::ScriptManager::Unregister(fn); }
void scriptStopAll() { ScriptHook::ScriptManager::StopAll(); }
void scriptWait(uint32_t ms) { ScriptHook::ScriptManager::Wait(ms); }
ScriptRuntimeStatus getScriptRuntimeStatus() { return ScriptHook::ScriptManager::Status(); }
bool getScriptRuntimeInfo(ScriptRuntimeInfo* out, uint32_t bytes) { return ScriptHook::ScriptManager::Info(out, bytes); }
bool scriptRuntimeSelfTest() { return ScriptHook::ScriptManager::RequestSelfTest(); }
uint32_t getScriptRuntimeAbiVersion() { return SHV_NX_RUNTIME_ABI_VERSION; }
uint32_t getScriptHookVersion() { return 0x010100; }
}
