#include "script.h"

void ScriptMain() {
    const char message[] = "Hello from ScriptHookV-NX. This plugin can finish normally.\n";
    (void)scriptSaveFile("hello.txt", message, sizeof(message) - 1);
}
SHV_NX_PLUGIN("tutorial-hello", ScriptMain)
