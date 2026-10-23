#include "script.h"
#include "shv/ui.h"
#include "shv/settings.h"

namespace {
void ScriptMain() {
    ScriptHook::UI::Menu<> menu;
    ScriptHook::UI::Controller controller;
    ScriptHook::UI::Renderer renderer;
    ScriptHook::Util::SavedSettings<3> settings(0x55494431,1);
    uint32_t saved[3]={1,50,0};(void)settings.Load(saved);
    bool overlay=saved[0]!=0;int32_t opacity=saved[1]<=100?int32_t(saved[1]):50,units=saved[2]<=1?int32_t(saved[2]):0;
    uint32_t noticeUntil=0;bool savedOK=true;
    const char* choices[]={"km/h","mph"};
    const char* state="ready";(void)scriptSaveFile("status.txt",state,5);
    for(;;) {
        if(HUD::IS_PAUSE_MENU_ACTIVE()||CAMERA::IS_SCREEN_FADED_OUT()){menu.Close();WAIT(100);continue;}
        if(controller.TogglePressed())menu.Toggle();
        if(menu.IsOpen()) {
            auto keys=controller.Poll(uint32_t(MISC::GET_GAME_TIMER()));
            menu.Begin(menu.Page()==0?"Switch mod UI":"Menu settings",keys);
            if(menu.Page()==0) {
                menu.Toggle(1,"Example overlay",overlay,"This demonstration changes its own display settings. It does not alter the player or world.");
                menu.Integer(2,"Overlay opacity",opacity,0,100,5,"Use D-Pad left or right. Values stay within the configured range.");
                menu.Choice(3,"Speed units",units,choices,2,"A reusable selector with Nintendo controller navigation.");
                menu.Submenu(4,"More settings",1,"Open the next page. B restores the selected row on this page.");
                for(uint32_t i=0;i<12;++i){ScriptHook::Util::TextBuffer<64> label;label.Add("Scrollable example ").UInt(i+1);menu.Action(100+i,label.CStr(),"This deliberately long description wraps inside a panel measured by the game text renderer, including when the selection moves through the scrolling list.");}
            } else {
                menu.ReadOnly(10,"Framework","ScriptHookV-NX");
                menu.Action(11,"Save settings","Save only on request. Each mod keeps its files in its own settings folder.");
                menu.Action(12,"Close menu","Close the menu and restore normal gameplay controls.");
            }
            menu.End();
            if(menu.Activated()==11){saved[0]=overlay?1:0;saved[1]=uint32_t(opacity);saved[2]=uint32_t(units);savedOK=settings.Save(saved);noticeUntil=uint32_t(MISC::GET_GAME_TIMER())+2500;}
            if(menu.Activated()==12)menu.Close();
            renderer.Draw(menu);
        }
        if(!menu.IsOpen()&&overlay) {
            HUD::SET_TEXT_FONT(0);HUD::SET_TEXT_SCALE(.29f,.29f);HUD::SET_TEXT_COLOUR(225,240,235,80+opacity*175/100);
            HUD::SET_TEXT_CENTRE(false);HUD::SET_TEXT_RIGHT_JUSTIFY(false);HUD::SET_TEXT_WRAP(.02f,.9f);
            HUD::BEGIN_TEXT_COMMAND_DISPLAY_TEXT("STRING");HUD::ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME("Controller UI demo | L + R + D-Pad Left");HUD::END_TEXT_COMMAND_DISPLAY_TEXT(.025f,.92f,0);
        }
        if(noticeUntil&&int32_t(uint32_t(MISC::GET_GAME_TIMER())-noticeUntil)<0) {
            HUD::SET_TEXT_FONT(0);HUD::SET_TEXT_SCALE(.32f,.32f);HUD::SET_TEXT_COLOUR(225,240,235,255);HUD::SET_TEXT_CENTRE(false);HUD::SET_TEXT_RIGHT_JUSTIFY(false);HUD::SET_TEXT_WRAP(.02f,.9f);
            HUD::BEGIN_TEXT_COMMAND_DISPLAY_TEXT("STRING");HUD::ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME(savedOK?"Settings saved":"Settings could not be saved");HUD::END_TEXT_COMMAND_DISPLAY_TEXT(.025f,.88f,0);
        }
        WAIT(0);
    }
}
}
SHV_NX_PLUGIN("controller-ui-demo",ScriptMain)
