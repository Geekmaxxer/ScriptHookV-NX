// Footprints by root-cause (MIT); Switch input/settings/lifecycle adaptation by MEGATARD.
#include "Header.hpp"
#include <shv/input.h>
#include <shv/ini.h>

namespace {
Ped effectPed=0;
bool effectOwned=false;
}

bool RequestPtfxAsset(const char* ptfxName) {
    STREAMING::REQUEST_NAMED_PTFX_ASSET(ptfxName);
    const unsigned start=unsigned(GAMEPLAY::GET_GAME_TIMER());
    while(!STREAMING::HAS_NAMED_PTFX_ASSET_LOADED(ptfxName)) {
        if(unsigned(GAMEPLAY::GET_GAME_TIMER())-start>=3000u)return false;
        WAIT(0);
    }
    return true;
}
void DisplaySubtitle(const char* text,int timeMs) {
    UI::_SET_TEXT_ENTRY_2("STRING");UI::_ADD_TEXT_COMPONENT_STRING(text);UI::_DRAW_SUBTITLE_TIMED(timeMs,1);
}
void StopFootprints() {
    if(!effectOwned)return;
    if(effectPed&&ENTITY::DOES_ENTITY_EXIST(effectPed))PED::SET_PED_CONFIG_FLAG(effectPed,421,false);
    GRAPHICS::_0xBA3D194057C79A7B("");effectOwned=false;effectPed=0;
}
void ToggleFootprints() {
    if(effectOwned){StopFootprints();DisplaySubtitle("Flaming footprints ~r~disabled.",4000);return;}
    const Ped ped=PLAYER::PLAYER_PED_ID();
    if(!ped||!ENTITY::DOES_ENTITY_EXIST(ped)||ENTITY::IS_ENTITY_DEAD(ped,false))return;
    if(PED::GET_PED_CONFIG_FLAG(ped,421,true)){DisplaySubtitle("Footprint override already active from another source.",4000);return;}
    if(!RequestPtfxAsset("scr_bike_adversary")){DisplaySubtitle("Footprints: particle asset unavailable on this build.",5000);return;}
    GRAPHICS::_0xBA3D194057C79A7B("scr_adversary_foot_flames");PED::SET_PED_CONFIG_FLAG(ped,421,true);
    effectPed=ped;effectOwned=true;DisplaySubtitle("Flaming footprints ~g~enabled.",4000);
}
void ScriptInit() {
    if(!nativeIsAvailable(0xc3f733a1a64bb1e8ULL))return;
    StopFootprints();
    ScriptHook::Util::Ini<> ini;const bool loaded=ini.Load();
    const char* button=ini.Get("Controls","ToggleButton","Y");
    const bool useX=(button[0]=='X'||button[0]=='x')&&button[1]==0;
    ScriptHook::Input::ShoulderChord chord(useX?ScriptHook::Input::Button::X:ScriptHook::Input::Button::Y);
    DisplaySubtitle(useX?"Footprints NX: hold L + R and press X to toggle.":"Footprints NX: hold L + R and press Y to toggle.",8000);
    if(loaded&&ini.Bool("Footprints","EnableOnStart",false))ToggleFootprints();
    while(true) {
        if(effectOwned&&effectPed!=PLAYER::PLAYER_PED_ID())StopFootprints();
        if(chord.Poll())ToggleFootprints();
        WAIT(0);
    }
}
SHV_NX_PLUGIN_WITH_STOP("footprints-nx",ScriptInit,StopFootprints)
