#include "state.hpp"
#include "shv/text.h"

namespace {
uint32_t next_run, destroyed;
uint64_t destroyed_worker_steps;
void FinalAudit(void*) {
    ShvNxMemoryStats memory{};
    const bool clear = !SharedSession::Get() && scriptGetMemoryStats(&memory) &&
        !memory.allocations && !scriptResourceCount() && scriptTrimMemory();
    ScriptHook::Util::TextBuffer<192> text;
    text.Add("run=").UInt(next_run).Add("\ndestructors=").UInt(destroyed)
        .Add("\nworker_progressed=").UInt(destroyed_worker_steps != 0)
        .Add("\nowner_cleanup_complete=").UInt(clear).Add("\n");
    (void)scriptSaveFile("cleanup.txt",text.CStr(),text.Size());
}
bool Initialize() {
    return scriptMemoryAvailable() && scriptResourcesAvailable() &&
        (getScriptHookCapabilities() & SHV_NX_CAP_MULTIPLE_SCRIPTS);
}
void Main() {
    ++next_run;
    if (!scriptResourceRegister(FinalAudit,nullptr)) return;
    auto* state = SharedSession::TryGet();
    if (!state || !state->label.TryAssign("shared session") || !state->entries.TryResize(128)) return;
    if (!scriptRegisterAdditionalThread(SessionWorker)) return;
    const char started[] = "shared_state_initialized=1\n";
    (void)scriptSaveFile("started.txt",started,sizeof(started)-1);
    for (;;) WAIT(1000);
}
}

SessionState::~SessionState() noexcept {
    ++destroyed;
    destroyed_worker_steps = worker_steps;
}
SHV_NX_PLUGIN_WITH_INIT("session-state-example",Main,Initialize)
