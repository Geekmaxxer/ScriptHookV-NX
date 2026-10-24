// NX adaptation of rikkamus/yasm (MIT). See LICENSE.upstream.txt and PORT.md.
#include "port.hpp"
using namespace Port;
void ScriptMain() {
    App app(0, "Yet Another Speedometer NX");
    bool enabled = true, hideName = true;
    int32_t units = 0, anchor = 3, scale = 40, opacity = 100;
    ScriptHook::Util::SavedSettings<6> store(0x5941534d);
    uint32_t words[6]{1, 0, 3, 40, 100, 1};
    if (store.Load(words)) {
        enabled = words[0] != 0;
        units = int(words[1] % 3);
        anchor = int(words[2] % 4);
        scale = int(words[3] >= 20 && words[3] <= 80 ? words[3] : 40);
        opacity = int(words[4] <= 100 ? words[4] : 100);
        hideName = words[5] != 0;
    }
    const char *unitNames[] = {"km/h", "mph", "m/s"};
    const char *anchors[] = {"Top left", "Top right", "Bottom left", "Bottom right"};
    Report("ready");
    for (;;) {
        if (!app.Frame()) {
            WAIT(100);
            continue;
        }
        if (app.Open()) {
            app.menu.Toggle(1, "Enabled", enabled);
            app.menu.Choice(2, "Units", units, unitNames, 3);
            app.menu.Choice(3, "Anchor", anchor, anchors, 4);
            app.menu.Integer(4, "Text scale", scale, 20, 80, 5);
            app.menu.Integer(5, "Opacity", opacity, 10, 100, 10);
            app.menu.Toggle(6, "Hide vehicle name", hideName);
            app.menu.Action(7, "Save settings");
        }
        Vehicle car = Car();
        app.Status(!enabled      ? "OFF"
                   : Exists(car) ? "ON | Driving"
                                 : "ON | Waiting for vehicle",
                   "Change units, position and size in the menu");
        app.End();
        if (app.menu.Activated() == 7) {
            uint32_t out[6]{uint32_t(enabled), uint32_t(units),   uint32_t(anchor),
                            uint32_t(scale),   uint32_t(opacity), uint32_t(hideName)};
            Notice(store.Save(out) ? "Speedometer settings saved" : "Settings save failed");
        }
        if (enabled && Exists(car) && !app.Open()) {
            float speed = ENTITY::GET_ENTITY_SPEED(car);
            if (speed >= 0 && speed < 10000) {
                TextBuffer<64> t;
                t.UInt(uint32_t(speed * (units == 0   ? 3.6f
                                         : units == 1 ? 2.236936f
                                                      : 1.f)))
                    .Add(" ")
                    .Add(unitNames[units]);
                float x = (anchor & 1) ? .80f : .025f, y = anchor < 2 ? .12f : .85f;
                HUD::SET_TEXT_FONT(0);
                HUD::SET_TEXT_SCALE(scale * .01f, scale * .01f);
                HUD::SET_TEXT_COLOUR(240, 245, 250, opacity * 255 / 100);
                HUD::SET_TEXT_WRAP(.02f, .97f);
                HUD::SET_TEXT_CENTRE(false);
                HUD::SET_TEXT_RIGHT_JUSTIFY(false);
                HUD::BEGIN_TEXT_COMMAND_DISPLAY_TEXT("STRING");
                HUD::ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME(t.CStr());
                HUD::END_TEXT_COMMAND_DISPLAY_TEXT(x, y, 0);
            }
            if (hideName)
                HUD::HIDE_HUD_COMPONENT_THIS_FRAME(6);
        }
        WAIT(0);
    }
}
SHV_NX_PLUGIN("yasm-nx", ScriptMain)
