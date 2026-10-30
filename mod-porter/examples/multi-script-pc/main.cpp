#include <windows.h>
#include "script.h"

static uint32_t workerTicks;
static void Worker(){for(;;){++workerTicks;WAIT(250);}}
static void Present(void*){
    if(HUD::IS_PAUSE_MENU_ACTIVE())return;
    HUD::SET_TEXT_FONT(0);HUD::SET_TEXT_SCALE(.28f,.28f);HUD::SET_TEXT_COLOUR(255,255,255,255);
    HUD::SET_TEXT_CENTRE(false);HUD::SET_TEXT_RIGHT_JUSTIFY(false);HUD::SET_TEXT_WRAP(.025f,.75f);
    HUD::BEGIN_TEXT_COMMAND_DISPLAY_TEXT("STRING");
    HUD::ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME(workerTicks?"Multi-script source port: worker and render callback active":"Multi-script source port: starting");
    HUD::END_TEXT_COMMAND_DISPLAY_TEXT(.025f,.40f,0);
}
void ScriptMain(){for(;;)WAIT(0);}
BOOL APIENTRY DllMain(HMODULE module,DWORD reason,LPVOID reserved){
    switch(reason){
        case DLL_PROCESS_ATTACH:
            scriptRegister(module,ScriptMain);
            scriptRegisterAdditionalThread(module,Worker);
            presentCallbackRegister(Present);
            break;
        case DLL_PROCESS_DETACH:
            scriptUnregister(module);
            presentCallbackUnregister(Present);
            break;
    }
    return TRUE;
}
