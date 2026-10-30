#include "script.h"
#include "shv/managed.h"
#include "shv/containers.h"

namespace {
struct Session {
    ScriptHook::Containers::String label;
    ScriptHook::Containers::Vector<uint64_t> samples;
    ~Session() noexcept {
        constexpr char text[] = "cleanup_callback_invoked=1\n";
        (void)scriptSaveFile("cleanup.txt", text, sizeof(text) - 1);
    }
};
bool Initialize() { return scriptMemoryAvailable() && scriptResourcesAvailable(); }
void Main() {
    auto session = ScriptHook::Memory::Managed<Session>::TryMake();
    if (!session || !session.Get()->label.TryAssign("managed resource example")) return;
    uint64_t sample = 0;
    constexpr char started[] = "running=1\n";
    (void)scriptSaveFile("started.txt", started, sizeof(started) - 1);
    for (;;) {
        {
            Session* current = session.Get();
            if (!current) return;
            if (current->samples.Size() == 128) current->samples.Clear();
            if (!current->samples.TryPushBack(sample++)) return;
        }
        WAIT(100);
    }
}
}
SHV_NX_PLUGIN_WITH_INIT("managed-resources-example", Main, Initialize)
