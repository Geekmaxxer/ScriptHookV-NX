#include "../../include/main.h"
#include "../../include/natives.h"

namespace {

void MainScript() {
    // Opt-in built-in example; not registered or linked by the test build.
    for (;;) {
        const auto timer = ScriptHook::Natives::MISC::GET_GAME_TIMER();
        if (getNativeCallStatus() != NativeCall_Ok) return;
        (void)timer;
        scriptWait(1000);
    }
}

} // namespace

void RegisterExamplePlugin() {
    (void)scriptRegisterChecked(MainScript);
}
