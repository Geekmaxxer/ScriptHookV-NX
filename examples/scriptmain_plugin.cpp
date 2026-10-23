#include "script.h"

void ScriptMain() {
    // Settings are isolated under switch/gtav-scripthook/settings/my-plugin/*.
    const char started[] = "ScriptMain started successfully.\n";
    (void)scriptSaveFile("started.txt", started, sizeof(started) - 1);

    while (true) {
        // Add your mod here. Native syntax includes PLAYER::PLAYER_ID(), etc.
        // Use WAIT(0) for drawing or input that needs every frame.
        WAIT(1000);
    }
}

SHV_NX_PLUGIN("my-plugin", ScriptMain)
