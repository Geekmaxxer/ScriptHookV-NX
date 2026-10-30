#include "script.h"
#include "plugin_manager.h"
#include "shv/crt.h"
#include "shv/managed.h"
#include "shv/text.h"

namespace {
constexpr unsigned Workers = 36, Blocks = 24;
constexpr size_t Extent = 2 * 1024 * 1024 + 4096;
// Read through a volatile view so the complete image payload survives linking.
const uint8_t image_payload[Extent] = {0xa5};
volatile uint8_t bss_payload[Extent];
struct Counters {
    uint32_t run, error, admitted, cycles, heartbeats, render_frames;
    uint32_t children_created, children_destroyed, children_before_parent, sessions_destroyed, stop_callbacks;
    uint32_t report_failures, cleanup_order_errors, image_pages, bss_pages;
    uint64_t worker_ticks[Workers];
    bool startup_complete, cleanup_reclaimed, trim_succeeded;
    ShvNxMemoryStats baseline;
};
Counters state{};
uint32_t next_run, self_slot = UINT32_MAX;
extern "C" uint64_t capacity_diagnostic_tick();
constexpr uint64_t TicksPerSecond = 19200000;
enum Phase { Setup, ExtentCheck, Admission, VerifyBlock, AllocateBlock, ReportFormat, ReportWrite, SessionCleanup, HeapTrim, PhaseCount };
constexpr const char* PhaseNames[] = {"setup","extents","admission","verify_block","allocate_block","report_format","report","cleanup","trim"};
struct Samples {
    uint64_t count, total_ticks, worst_ticks, worst_at_ticks, over_2ms;
    uint64_t buckets[7];
    void Add(uint64_t ticks, uint64_t at) {
        ++count; total_ticks += ticks;
        if (ticks > worst_ticks) { worst_ticks = ticks; worst_at_ticks = at; }
        if (ticks > 38400) ++over_2ms;
        constexpr uint64_t limits[] = {1920,9600,19200,38400,192000,960000};
        unsigned bucket = 0;
        while (bucket < 6 && ticks > limits[bucket]) ++bucket;
        ++buckets[bucket];
    }
};
Samples samples[PhaseCount];
uint64_t run_started, session_started, last_render_tick, render_total_ticks, render_worst_ticks;
uint64_t render_intervals, render_gaps[7];
uint32_t clock_errors;
ScriptHook::Plugins::Timing timing_baseline{};
bool timing_baseline_available;
uint64_t Microseconds(uint64_t ticks) { return ticks / 96 * 5 + ticks % 96 * 5 / 96; }
struct Measure {
    Phase phase; uint64_t start;
    explicit Measure(Phase value) : phase(value), start(capacity_diagnostic_tick()) {}
    ~Measure() noexcept {
        const uint64_t end = capacity_diagnostic_tick();
        if (end < start || start < run_started) { ++clock_errors; return; }
        samples[phase].Add(end - start,end - run_started);
    }
};

template<size_t N> void Field(ScriptHook::Util::TextBuffer<N>& text, const char* key, uint64_t value) {
    text.Add(key).Add("=");
    char reversed[20], digits[21]{}; unsigned count = 0, used = 0;
    do { reversed[count++] = char('0' + value % 10); value /= 10; } while (value);
    while (count) digits[used++] = reversed[--count];
    text.Add(digits).Add("\n");
}
bool Report(const char* phase, const char* file = "status.txt") {
    const auto format_started = capacity_diagnostic_tick();
    // Static report storage avoids placing the diagnostic text on a 64 KiB script stack.
    using ReportText = ScriptHook::Util::TextBuffer<8192>;
    alignas(ReportText) static uint8_t storage[sizeof(ReportText)];
    auto& text = *new (static_cast<void*>(storage)) ReportText();
    text.Add("ScriptHookV-NX capacity acceptance probe\nphase=").Add(phase).Add("\n");
    Field(text,"schema",2); Field(text,"run",state.run); Field(text,"operation_error",state.error);
    Field(text,"session_start_tick",session_started); Field(text,"run_start_tick",run_started);
    const uint64_t now = capacity_diagnostic_tick();
    Field(text,"clock_hz",TicksPerSecond); Field(text,"clock_errors",clock_errors);
    Field(text,"elapsed_wall_us",now >= run_started ? Microseconds(now-run_started) : 0);
    Field(text,"startup_complete",state.startup_complete); Field(text,"workers_required",Workers);
    Field(text,"workers_admitted",state.admitted);
    unsigned progressing = 0; uint64_t minimum = UINT64_MAX, maximum = 0;
    for (auto value : state.worker_ticks) {
        if (value) ++progressing;
        if (value < minimum) minimum = value;
        if (value > maximum) maximum = value;
    }
    Field(text,"workers_progressing",progressing); Field(text,"minimum_worker_ticks",minimum);
    Field(text,"maximum_worker_ticks",maximum); Field(text,"heartbeats",state.heartbeats);
    Field(text,"fragmentation_cycles",state.cycles); Field(text,"render_frames",state.render_frames);
    Field(text,"image_pages_verified",state.image_pages); Field(text,"bss_pages_verified",state.bss_pages);
    Field(text,"children_created",state.children_created); Field(text,"children_destroyed",state.children_destroyed);
    Field(text,"children_before_parent",state.children_before_parent);
    Field(text,"sessions_destroyed",state.sessions_destroyed); Field(text,"stop_callbacks",state.stop_callbacks);
    Field(text,"cleanup_order_errors",state.cleanup_order_errors); Field(text,"report_failures",state.report_failures);
    Field(text,"resource_count",scriptResourceCount()); Field(text,"baseline_allocations",state.baseline.allocations);
    Field(text,"cleanup_reclaimed",state.cleanup_reclaimed); Field(text,"trim_succeeded",state.trim_succeeded);
    ShvNxMemoryStats memory{}; const bool stats = scriptGetMemoryStats(&memory);
    Field(text,"memory_stats_available",stats); Field(text,"live_allocations",memory.allocations);
    Field(text,"requested_bytes",memory.requested_bytes); Field(text,"charged_bytes",memory.charged_bytes);
    Field(text,"peak_requested_bytes",memory.peak_requested_bytes); Field(text,"memory_failures",memory.failures);
    Field(text,"release_failures",memory.release_failures);
    ScriptHook::Plugins::Timing timing{};
    const bool timed = self_slot != UINT32_MAX &&
        ScriptHook::Plugins::GetTiming(self_slot,ScriptHook::Plugins::TimingKind::Script,timing);
    Field(text,"script_timing_available",timed); Field(text,"completed_script_calls",timing.calls);
    Field(text,"script_total_us",timing.totalMicroseconds); Field(text,"script_worst_us",timing.worstMicroseconds);
    const bool delta = timed && timing_baseline_available && timing.calls >= timing_baseline.calls &&
        timing.totalMicroseconds >= timing_baseline.totalMicroseconds;
    Field(text,"run_timing_available",delta);
    Field(text,"run_completed_script_calls",delta ? timing.calls-timing_baseline.calls : 0);
    Field(text,"run_script_total_us",delta ? timing.totalMicroseconds-timing_baseline.totalMicroseconds : 0);
    Field(text,"script_baseline_worst_us",timing_baseline.worstMicroseconds);
    for (unsigned i = 0; i < PhaseCount; ++i) {
        text.Add("sample_phase=").Add(PhaseNames[i]).Add("\n");
        const auto& value = samples[i];
        Field(text,"sample_count",value.count); Field(text,"sample_total_us",Microseconds(value.total_ticks));
        Field(text,"sample_worst_us",Microseconds(value.worst_ticks));
        Field(text,"sample_worst_at_wall_us",Microseconds(value.worst_at_ticks));
        Field(text,"sample_over_2ms",value.over_2ms);
        constexpr const char* keys[] = {"le_100us","le_500us","le_1ms","le_2ms","le_10ms","le_50ms","gt_50ms"};
        for (unsigned n = 0; n < 7; ++n) Field(text,keys[n],value.buckets[n]);
    }
    Field(text,"callback_intervals",render_intervals);
    Field(text,"callback_interval_total_us",Microseconds(render_total_ticks));
    Field(text,"callback_interval_worst_us",Microseconds(render_worst_ticks));
    constexpr const char* gap_keys[] = {"callback_le_17ms","callback_le_34ms","callback_le_50ms",
        "callback_le_100ms","callback_le_250ms","callback_le_1s","callback_gt_1s"};
    for (unsigned n = 0; n < 7; ++n) Field(text,gap_keys[n],render_gaps[n]);
    // Snapshot once: the duration of this write appears in the next report.
    const bool complete = text.Complete(); const size_t bytes = text.Size();
    const auto format_finished = capacity_diagnostic_tick();
    if (format_finished >= format_started && format_started >= run_started)
        samples[ReportFormat].Add(format_finished-format_started,format_finished-run_started);
    else ++clock_errors;
    bool saved = false;
    { Measure measure(ReportWrite); saved = complete && scriptSaveFile(file,text.CStr(),bytes); }
    if (!saved) ++state.report_failures;
    return saved;
}
void Fail(uint32_t error) {
    if (!state.error) state.error = error;
    (void)Report("failed");
    scriptUnregisterAll();
}
void FinalAudit(void*) {
    ShvNxMemoryStats memory{};
    state.cleanup_reclaimed = scriptGetMemoryStats(&memory) && !memory.allocations &&
        !memory.requested_bytes && !scriptResourceCount();
    { Measure measure(HeapTrim); state.trim_succeeded = scriptTrimMemory(); }
    if ((!state.cleanup_reclaimed || !state.trim_succeeded || state.cleanup_order_errors) && !state.error)
        state.error = 90;
    const char* phase = state.cleanup_reclaimed && state.trim_succeeded ? "stopped" : "cleanup_failed";
    (void)Report(phase,"cleanup.txt");
    (void)Report(phase);
    char history[] = "run-1.txt";
    history[4] = char('1' + (state.run-1) % 8);
    (void)Report(phase,history);
}
struct Child {
    uint8_t payload[2048]{};
    ~Child() noexcept { ++state.children_destroyed; }
};
struct Session {
    ScriptHook::Memory::Managed<Child> children[2];
    void* blocks[Blocks]{};
    size_t sizes[Blocks]{};
    ~Session() noexcept {
        Measure measure(SessionCleanup);
        // Child registrations follow the parent, so cancellation drains them first.
        state.children_before_parent = state.children_destroyed;
        // Normal error returns can destroy the parent before the queued stop.
        // Release any still-active child explicitly; cancellation already drained it.
        for (auto& child : children) if (!child.Reset()) ++state.cleanup_order_errors;
        if (state.children_destroyed != state.children_created) ++state.cleanup_order_errors;
        for (auto& pointer : blocks) {
            if (pointer && !scriptFree(pointer) && !state.error) state.error = 15;
            pointer = nullptr;
        }
        ++state.sessions_destroyed;
    }
};
uint8_t Pattern(unsigned index) { return uint8_t(index * 29 + state.run * 11); }
void Fill(void* pointer, size_t size, unsigned index) {
    auto* bytes = static_cast<uint8_t*>(pointer);
    for (size_t n = 0; n < size; ++n) bytes[n] = Pattern(index);
}
bool Verify(const void* pointer, size_t size, unsigned index) {
    const auto* bytes = static_cast<const uint8_t*>(pointer);
    for (size_t n = 0; n < size; ++n) if (bytes[n] != Pattern(index)) return false;
    return true;
}
bool Cycle(Session& session) {
    for (unsigned i = 0; i < Blocks; ++i) {
        {
            Measure measure(VerifyBlock);
            if (session.blocks[i] && !Verify(session.blocks[i],session.sizes[i],i)) return false;
            if ((i & 1) && session.blocks[i]) {
                if (!scriptFree(session.blocks[i])) return false;
                session.blocks[i] = nullptr; session.sizes[i] = 0;
            }
        }
        if (i % 4 == 3) WAIT(0);
    }
    for (unsigned i = 0; i < Blocks; ++i) {
        {
            Measure measure(AllocateBlock);
            const size_t wanted = (1 + (i + state.cycles) % 6) * 2048 + i * 13;
            void* replacement = session.blocks[i] ? scriptReallocate(session.blocks[i],wanted) :
                scriptAllocate(wanted,64,0);
            if (!replacement) return false;
            const size_t preserved = session.sizes[i] < wanted ? session.sizes[i] : wanted;
            session.blocks[i] = replacement; session.sizes[i] = wanted;
            if (preserved && !Verify(replacement,preserved,i)) return false;
            Fill(replacement,wanted,i);
        }
        if (i % 4 == 3) WAIT(0);
    }
    ++state.cycles;
    return true;
}
bool Extents() {
    Measure measure(ExtentCheck);
    const volatile uint8_t* image = image_payload;
    for (size_t offset = 0; offset < Extent; offset += 4096) {
        if (image[offset] != (offset ? 0 : 0xa5)) return false;
        ++state.image_pages;
        bss_payload[offset] = uint8_t(offset / 4096 + state.run);
    }
    for (size_t offset = 0; offset < Extent; offset += 4096) {
        if (bss_payload[offset] != uint8_t(offset / 4096 + state.run)) return false;
        ++state.bss_pages;
    }
    return image[Extent-1] == 0;
}
template<unsigned Index> void Worker() {
    for (;;) { if (state.startup_complete) ++state.worker_ticks[Index]; WAIT(25); }
}
constexpr ScriptFunction workers[] = {
    Worker<0>,Worker<1>,Worker<2>,Worker<3>,Worker<4>,Worker<5>,Worker<6>,Worker<7>,Worker<8>,
    Worker<9>,Worker<10>,Worker<11>,Worker<12>,Worker<13>,Worker<14>,Worker<15>,Worker<16>,Worker<17>,
    Worker<18>,Worker<19>,Worker<20>,Worker<21>,Worker<22>,Worker<23>,Worker<24>,Worker<25>,Worker<26>,
    Worker<27>,Worker<28>,Worker<29>,Worker<30>,Worker<31>,Worker<32>,Worker<33>,Worker<34>,Worker<35>
};
static_assert(sizeof(workers)/sizeof(*workers) == Workers);
void Render(const ShvNxRenderFrame* frame) {
    if (!frame || frame->size < sizeof(*frame)) return;
    ++state.render_frames;
    if (last_render_tick) {
        if (frame->ticks < last_render_tick) ++clock_errors;
        else {
            const uint64_t gap = frame->ticks-last_render_tick;
            ++render_intervals; render_total_ticks += gap;
            if (gap > render_worst_ticks) render_worst_ticks = gap;
            constexpr uint64_t limits[] = {326400,652800,960000,1920000,4800000,19200000};
            unsigned bucket = 0;
            while (bucket < 6 && gap > limits[bucket]) ++bucket;
            ++render_gaps[bucket];
        }
    }
    last_render_tick = frame->ticks;
}
bool Initialize() {
    constexpr uint64_t required = SHV_NX_CAP_MULTIPLE_SCRIPTS | SHV_NX_CAP_RENDER_CALLBACKS;
    return scriptMemoryAvailable() && scriptResourcesAvailable() &&
        (getScriptHookCapabilities() & required) == required;
}
void Stop() { ++state.stop_callbacks; }
void Main() {
    state = {}; state.run = ++next_run; self_slot = UINT32_MAX;
    for (auto& sample : samples) sample = {};
    run_started = capacity_diagnostic_tick();
    if (state.run == 1) session_started = run_started;
    last_render_tick = render_total_ticks = render_worst_ticks = render_intervals = 0;
    for (auto& gap : render_gaps) gap = 0;
    clock_errors = 0; timing_baseline = {}; timing_baseline_available = false;
    ScriptHook::Memory::Managed<Session> session;
    {
        Measure measure(Setup);
        if (!state.run || !scriptGetMemoryStats(&state.baseline) || state.baseline.allocations) { Fail(1); return; }
        if (!scriptResourceRegister(FinalAudit,nullptr)) { Fail(2); return; }
        session = ScriptHook::Memory::Managed<Session>::TryMake();
        if (!session) { Fail(3); return; }
        for (auto& child : session.Get()->children) {
            child = ScriptHook::Memory::Managed<Child>::TryMake();
            if (!child) { Fail(4); return; }
            ++state.children_created;
        }
    }
    if (!Extents()) { Fail(10); return; }
    if (ScriptHook::Plugins::Available()) {
        const auto count = ScriptHook::Plugins::Count();
        for (uint32_t i = 0; i < count; ++i) {
            const char* id = ScriptHook::Plugins::Id(i);
            if (id && strcmp(id,"capacity-acceptance") == 0) { self_slot = i; break; }
        }
    }
    if (!renderCallbackRegister(Render)) { Fail(11); return; }
    timing_baseline_available = self_slot != UINT32_MAX &&
        ScriptHook::Plugins::GetTiming(self_slot,ScriptHook::Plugins::TimingKind::Script,timing_baseline);
    if (!Report("admitting")) { Fail(21); return; }
    for (auto worker : workers) {
        bool admitted = false;
        { Measure measure(Admission); admitted = scriptRegisterAdditionalThread(worker); }
        if (!admitted) { Fail(20); return; }
        ++state.admitted;
        if (state.admitted % 4 == 0) WAIT(0);
    }
    state.startup_complete = true;
    uint64_t last_status_tick = 0;
    for (;;) {
        if (!Cycle(*session.Get())) { Fail(12); return; }
        const uint64_t now = capacity_diagnostic_tick();
        // Preserve the first active snapshot, then cap periodic SD writes. Final
        // cleanup and failure reports remain immediate and separately measured.
        if (!last_status_tick || now-last_status_tick >= 30*TicksPerSecond) {
            if (!Report("running")) { Fail(22); return; }
            last_status_tick = capacity_diagnostic_tick();
        }
        for (unsigned i = 0; i < 10; ++i) { ++state.heartbeats; WAIT(100); }
    }
}
}
SHV_NX_PLUGIN_WITH_LIFECYCLE("capacity-acceptance",Main,Initialize,Stop)
