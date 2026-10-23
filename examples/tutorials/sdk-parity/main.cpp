#include "script.h"
#include "shv/text.h"
#include "shv/input.h"

namespace {
using ScriptHook::Util::TextBuffer;
uint32_t starts,workerTicks,controllerPresses,renderFrames,samples;
int counts[4],texture;bool running,visible=true,workerStarted,renderReady,controlReady;
bool playerPointer,globalFound,paused;uint32_t globalBank;
constexpr uint64_t required=SHV_NX_CAP_MULTIPLE_SCRIPTS|SHV_NX_CAP_ENTITY_ENUMERATION|
    SHV_NX_CAP_SCRIPT_GLOBALS|SHV_NX_CAP_ENTITY_POINTERS|SHV_NX_CAP_CONTROL_CALLBACKS|
    SHV_NX_CAP_RENDER_CALLBACKS|SHV_NX_CAP_CUSTOM_TEXTURES;
void Save(const char* phase) {
    TextBuffer<1024> t;t.Add("ScriptHookV-NX SDK parity probe\nphase=").Add(phase)
        .Add("\nstarts=").UInt(starts).Add("\nworker_ticks=").UInt(workerTicks)
        .Add("\nworker_registered=").UInt(workerStarted).Add("\nrender_registered=").UInt(renderReady)
        .Add("\ncontrol_registered=").UInt(controlReady).Add("\nrender_frames=").UInt(renderFrames)
        .Add("\ncontroller_presses=").UInt(controllerPresses)
        .Add("\npeds=").Int(counts[0]).Add("\nvehicles=").Int(counts[1])
        .Add("\nobjects=").Int(counts[2]).Add("\npickups=").Int(counts[3])
        .Add("\nplayer_pointer_valid=").UInt(playerPointer).Add("\nglobal_bank_found=").UInt(globalFound)
        .Add("\nglobal_bank=").UInt(globalBank).Add("\ntexture_id=").Int(texture)
        .Add("\n");
    (void)scriptSaveFile("status.txt",t.CStr(),t.Size());
}
void Worker() {
    while(running){++workerTicks;WAIT(250);}
}
void Control(const ShvNxInputEvent* event) {
    if(event&&event->kind==ShvNxInput_Pressed){++controllerPresses;visible=!visible;}
}
void Text(float y,const char* text) {
    HUD::SET_TEXT_FONT(0);HUD::SET_TEXT_SCALE(.3f,.3f);HUD::SET_TEXT_COLOUR(255,255,255,255);
    HUD::SET_TEXT_CENTRE(false);HUD::SET_TEXT_RIGHT_JUSTIFY(false);HUD::SET_TEXT_WRAP(.025f,.80f);
    HUD::BEGIN_TEXT_COMMAND_DISPLAY_TEXT("STRING");(void)ScriptHook::Util::AppendHudText(text);
    HUD::END_TEXT_COMMAND_DISPLAY_TEXT(.025f,y,0);
}
void Render(const ShvNxRenderFrame* frame) {
    ++renderFrames;if(!visible||paused||!frame)return;
    Text(.23f,"SDK parity probe | Nintendo Y: show/hide");
    TextBuffer<192> line;line.Add("Pools: P ").Int(counts[0]).Add(" / V ").Int(counts[1])
        .Add(" / O ").Int(counts[2]).Add(" / pickups ").Int(counts[3]);Text(.265f,line.CStr());
    TextBuffer<192> detail;detail.Add("Worker ").UInt(workerTicks).Add(" | entity ").UInt(playerPointer)
        .Add(" | globals ").UInt(globalFound).Add(" | texture ").Int(texture)
        .Add(" | input ").UInt(controllerPresses).Add(" | ").UInt(frame->width).Add("x").UInt(frame->height);
    Text(.30f,detail.CStr());
    if(texture>0)drawTexture(texture,0,0,0,.10f,.17777778f,.5f,.5f,.88f,.32f,
        float(frame->frame%720)/720.f,1.f,1.f,1.f,1.f,1.f);
}
void Stop(){running=false;Save("stopped");}
void Main() {
    ++starts;running=true;visible=true;samples=0;
    if((getScriptHookCapabilities()&required)!=required){Save("runtime_extensions_missing");return;}
    texture=createTexture("palette.png");
    workerStarted=scriptRegisterAdditionalThread(Worker);
    controlReady=controlHandlerRegister(Control,2,uint32_t(ScriptHook::Input::Button::Y));
    renderReady=renderCallbackRegister(Render);
    for(;;){
        paused=HUD::IS_PAUSE_MENU_ACTIVE();
        int handles[128]{};
        counts[0]=worldGetAllPeds(handles,128);counts[1]=worldGetAllVehicles(handles,128);
        counts[2]=worldGetAllObjects(handles,128);counts[3]=worldGetAllPickups(handles,128);
        const auto player=PLAYER::PLAYER_PED_ID();
        playerPointer=player>0&&getScriptHandleBaseAddress(player)!=nullptr;
        globalFound=false;
        for(uint32_t bank=0;bank<64;++bank)if(getGlobalPtr(int(bank<<18))){globalFound=true;globalBank=bank;break;}
        if((samples++%5)==0)Save("running");
        WAIT(1000);
    }
}
}
SHV_NX_PLUGIN_WITH_STOP("sdk-parity-probe",Main,Stop)
