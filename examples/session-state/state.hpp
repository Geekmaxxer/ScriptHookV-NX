#pragma once
#include "script.h"
#include "shv/containers.h"
#include "shv/session.h"

struct SessionState {
    ScriptHook::Containers::String label;
    ScriptHook::Containers::Vector<uint32_t> entries;
    uint64_t worker_steps = 0;
    ~SessionState() noexcept;
};
using SharedSession = ScriptHook::Memory::SessionLocal<SessionState>;
void SessionWorker();
