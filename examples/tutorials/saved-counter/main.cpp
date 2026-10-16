#include "script.h"
#include "shv/settings.h"
#include "shv/text.h"

void ScriptMain() {
    ScriptHook::Util::SavedSettings<1> settings(0x53435631u);
    uint32_t values[1]{};
    (void)settings.Load(values); // Missing/damaged records preserve these defaults.
    ++values[0];
    const bool saved = settings.Save(values); // Writes and verifies the other slot.
    ScriptHook::Util::TextBuffer<> report;
    report.Add("starts=").UInt(values[0]).Add(saved ? "\nsaved=yes\n" : "\nsaved=no\n");
    (void)scriptSaveFile("counter.txt", report.CStr(), report.Size());
    // No autosave loop: this example completes and can be started again with Load Now.
}
SHV_NX_PLUGIN("tutorial-counter", ScriptMain)
