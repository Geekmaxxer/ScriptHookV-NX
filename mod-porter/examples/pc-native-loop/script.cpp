// PC-shaped example: the port uses exact target declarations, not these PC hashes.
#include "script.h"
#include <Windows.h>

void ScriptMain() {
    while (true) {
        Ped player = PLAYER::PLAYER_PED_ID();
        (void)player;
        (void)GAMEPLAY::GET_GAME_TIMER();
        WAIT(1000);
    }
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        scriptRegister(module, ScriptMain);
    } else if (reason == DLL_PROCESS_DETACH) {
        scriptUnregister(module);
    }
    return TRUE;
}
