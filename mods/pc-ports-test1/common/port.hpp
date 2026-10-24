#pragma once
#include "script.h"
#include "shortcut.hpp"
#include "shv/ini.h"
#include "shv/settings.h"
#include "shv/ui.h"

// Shared Switch platform adaptation. Gameplay/data credits are per mod.
namespace Port {
using ScriptHook::Util::TextBuffer;
using Button = ScriptHook::Input::Button;
inline uint32_t Now() { return uint32_t(MISC::GET_GAME_TIMER()); }
inline bool Due(uint32_t now, uint32_t deadline) { return int32_t(now - deadline) >= 0; }
constexpr Hash H(const char *s) {
    uint32_t hash = 0;
    while (*s) {
        const char c = *s++;
        hash += uint8_t(c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c);
        hash += hash << 10;
        hash ^= hash >> 6;
    }
    hash += hash << 3;
    hash ^= hash >> 11;
    hash += hash << 15;
    return hash;
}
static_assert(H("player_zero") == 0x0d7114c9u && H("WEAPON_PISTOL") == 0x1b06d571u,
              "Match the target model/weapon name hashes");
inline Vector3 V(float x, float y, float z) {
    Vector3 v{};
    v.x = x;
    v.y = y;
    v.z = z;
    return v;
}
inline Vector3 Add(Vector3 a, Vector3 b) { return V(a.x + b.x, a.y + b.y, a.z + b.z); }
inline float Dist2(Vector3 a, Vector3 b) {
    float x = a.x - b.x, y = a.y - b.y, z = a.z - b.z;
    return x * x + y * y + z * z;
}
inline bool Exists(Entity e) { return e && ENTITY::DOES_ENTITY_EXIST(e); }
inline Ped PedSelf() { return PLAYER::PLAYER_PED_ID(); }
inline bool Live() {
    Ped p = PedSelf();
    return Exists(p) && !ENTITY::IS_ENTITY_DEAD(p, false) && !HUD::IS_PAUSE_MENU_ACTIVE() &&
           !CAMERA::IS_SCREEN_FADED_OUT() && PLAYER::IS_PLAYER_CONTROL_ON(PLAYER::PLAYER_ID());
}
inline bool UiSafe() {
    Ped p = PedSelf();
    return Exists(p) && !ENTITY::IS_ENTITY_DEAD(p, false) && !HUD::IS_PAUSE_MENU_ACTIVE() &&
           !CAMERA::IS_SCREEN_FADED_OUT();
}
inline Vehicle Car() {
    Ped p = PedSelf();
    return PED::IS_PED_IN_ANY_VEHICLE(p, false) ? PED::GET_VEHICLE_PED_IS_IN(p, false) : 0;
}
inline void Text(const char *s, float x = .025f, float y = .84f, float scale = .31f) {
    HUD::SET_TEXT_FONT(0);
    HUD::SET_TEXT_SCALE(scale, scale);
    HUD::SET_TEXT_COLOUR(230, 240, 240, 245);
    HUD::SET_TEXT_CENTRE(false);
    HUD::SET_TEXT_RIGHT_JUSTIFY(false);
    HUD::SET_TEXT_WRAP(.02f, .97f);
    HUD::BEGIN_TEXT_COMMAND_DISPLAY_TEXT("STRING");
    ScriptHook::Util::AppendHudText(s);
    HUD::END_TEXT_COMMAND_DISPLAY_TEXT(x, y, 0);
}
static char message[256]{};
static uint32_t messageUntil = 0;
static Hash pendingModel = 0;
inline void Notice(const char *s) {
    ScriptHook::UI::Copy(message, s);
    messageUntil = Now() + 5000;
}
static uint32_t menuOpens = 0, menuActions = 0;
static uint32_t inputSeen = 0;
static int shortcutControl = 0;
static char currentStatus[128] = "Starting";
inline void Report(const char *state) {
    TextBuffer<768> t;
    t.Add("ScriptHookV-NX PC gameplay port - Test 2\nstate=")
        .Add(state)
        .Add("\nstatus=")
        .Add(currentStatus)
        .Add("\nmenu_opens=")
        .UInt(menuOpens)
        .Add("\nmenu_actions=")
        .UInt(menuActions)
        .Add("\nmenu_control=")
        .UInt(uint32_t(shortcutControl))
        .Add("\ninput_seen_mask=")
        .UInt(inputSeen)
        .Add("\nswitch_gameplay_verified=false\n");
    scriptSaveFile("status.txt", t.CStr(), t.Size());
}
struct Owned {
    Entity entity = 0;
    Hash model = 0;
};
static Owned owned[16]{};
static Blip blips[16]{};
inline Entity Own(Entity e) {
    if (!Exists(e))
        return 0;
    for (auto &o : owned)
        if (!o.entity) {
            o = {e, ENTITY::GET_ENTITY_MODEL(e)};
            return e;
        }
    ENTITY::DELETE_ENTITY(&e);
    return 0;
}
inline void Delete(Entity &e) {
    for (auto &o : owned)
        if (o.entity == e) {
            if (Exists(e) && ENTITY::GET_ENTITY_MODEL(e) == o.model)
                ENTITY::DELETE_ENTITY(&e);
            o = {};
        }
    e = 0;
}
inline void ClearOwned() {
    for (auto &o : owned) {
        Entity e = o.entity;
        Delete(e);
    }
    for (auto &b : blips)
        if (b) {
            HUD::REMOVE_BLIP(&b);
            b = 0;
        }
    if (pendingModel) {
        STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(pendingModel);
        pendingModel = 0;
    }
}
inline Blip Pin(Vector3 p, int sprite = 1, int colour = 2) {
    for (auto &b : blips)
        if (!b) {
            b = HUD::ADD_BLIP_FOR_COORD(p);
            HUD::SET_BLIP_SPRITE(b, sprite);
            HUD::SET_BLIP_COLOUR(b, colour);
            HUD::SET_BLIP_SCALE(b, .7f);
            HUD::SET_BLIP_AS_SHORT_RANGE(b, false);
            return b;
        }
    return 0;
}
inline void Unpin(Blip &b) {
    for (auto &saved : blips)
        if (saved == b)
            saved = 0;
    if (b)
        HUD::REMOVE_BLIP(&b);
    b = 0;
}
inline bool Model(Hash h) {
    if (!STREAMING::IS_MODEL_VALID(h)) {
        Notice("Required model unavailable in this installation");
        Report("missing_model");
        return false;
    }
    pendingModel = h;
    STREAMING::REQUEST_MODEL(h);
    uint32_t end = Now() + 3000;
    for (unsigned attempts = 0; attempts < 180 && !STREAMING::HAS_MODEL_LOADED(h); ++attempts) {
        if (Due(Now(), end)) {
            STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(h);
            pendingModel = 0;
            Notice("Model request timed out");
            Report("model_timeout");
            return false;
        }
        WAIT(0);
    }
    if (!STREAMING::HAS_MODEL_LOADED(h)) {
        STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(h);
        pendingModel = 0;
        return false;
    }
    return true;
}
inline void Release(Hash h) {
    STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(h);
    if (pendingModel == h)
        pendingModel = 0;
}
inline Entity Prop(const char *name, Vector3 p, Vector3 rot = {}, bool freeze = true) {
    Hash h = H(name);
    if (!Model(h))
        return 0;
    Entity e = Own(OBJECT::CREATE_OBJECT_NO_OFFSET(h, p, false, false, false));
    Release(h);
    if (e) {
        ENTITY::SET_ENTITY_ROTATION(e, rot, 2, true);
        ENTITY::FREEZE_ENTITY_POSITION(e, freeze);
    }
    return e;
}
inline Vehicle VehicleAt(const char *name, Vector3 p, float heading) {
    Hash h = H(name);
    if (!Model(h))
        return 0;
    Vehicle e = Own(VEHICLE::CREATE_VEHICLE(h, p, heading, false, false, false));
    Release(h);
    if (e)
        VEHICLE::SET_VEHICLE_ON_GROUND_PROPERLY(e, 5.f);
    return e;
}
inline int CashKey() {
    Hash m = ENTITY::GET_ENTITY_MODEL(PedSelf());
    if (m == H("player_zero"))
        return int(H("SP0_TOTAL_CASH"));
    if (m == H("player_one"))
        return int(H("SP1_TOTAL_CASH"));
    if (m == H("player_two"))
        return int(H("SP2_TOTAL_CASH"));
    return 0;
}
inline bool Pay(int amount) {
    int key = CashKey(), cash = 0;
    if (!key || !STATS::STAT_GET_INT(key, &cash, -1) || cash < 0 || amount > 2000000000 - cash ||
        amount < -cash) {
        Notice("Cash transaction unavailable for this character");
        return false;
    }
    return STATS::STAT_SET_INT(key, cash + amount, true);
}
inline void Route(Vector3 p) { HUD::SET_NEW_WAYPOINT(p.x, p.y); }
inline uint32_t Metres(float squared) { return uint32_t(__builtin_sqrtf(squared > 0 ? squared : 0)); }
inline void Marker(Vector3 p, int colour = 0) {
    GRAPHICS::DRAW_MARKER(1, p, V(0, 0, 0), V(0, 0, 0), V(.4f, .4f, .3f), colour ? 235 : 70,
                          colour ? 80 : 210, 130, 180, false, false, 2, false, nullptr, nullptr, false);
}
inline bool Press(int action = 51) {
    return PAD::IS_CONTROL_JUST_PRESSED(0, action) || PAD::IS_DISABLED_CONTROL_JUST_PRESSED(0, action);
}
class App {
    const int index;
    const char *title;
    bool openBefore = false;
    bool controlsReady = false, showStatus = true;
    int button;
    const char *buttonName;
    Shortcut shortcut;
    uint32_t reportAt = 0;
    bool dirty = true;
    char objective[160]{};
    ScriptHook::Util::SavedSettings<1> uiSettings{0x55493232, 1, "ui-a.bin", "ui-b.bin"};

    static bool Held(int action) {
        // Sample before suppressing gameplay; disabled-control reads keep a
        // held shortcut working alongside the NSC menu or another NRO.
        return PAD::IS_CONTROL_PRESSED(2, action) || PAD::IS_DISABLED_CONTROL_PRESSED(2, action);
    }
    void Badge(bool shoulders) {
        if (!showStatus && !shoulders)
            return;
        const float y = .18f + float(index) * .046f;
        GRAPHICS::DRAW_RECT(.785f, y + .020f, .385f, .042f, 15, 24, 32, 190, false);
        TextBuffer<128> heading;
        heading.Add(ShortNames[index]).Add(" | ").Add(currentStatus);
        // Hard clipping avoids overflowing the status column at TV resolutions.
        char line[66]{};
        ScriptHook::UI::Copy(line, heading.CStr());
        TextBuffer<192> help;
        if (shoulders)
            help.Add("L + R + ").Add(buttonName).Add(" : menu");
        else
            help.Add(objective[0] ? objective : "Hold L + R for menu shortcuts");
        char hint[70]{};
        ScriptHook::UI::Copy(hint, help.CStr());
        TextBuffer<192> badge;
        badge.Add(line).Add("~n~").Add(hint);
        Text(badge.CStr(), .60f, y, .25f);
    }

  public:
    ScriptHook::UI::Menu<32, 8> menu;
    ScriptHook::UI::Controller controller;
    ScriptHook::UI::Renderer renderer;
    explicit App(int slot, const char *name)
        : index(slot), title(name), button(MenuButtons[slot]), buttonName(MenuButtonNames[slot]) {
        menuOpens = menuActions = 0;
        message[0] = 0;
        messageUntil = 0;
        ScriptHook::UI::Copy(currentStatus, "Starting");
        // Optional, never overwrite existing configuration/settings.
        ScriptHook::Util::Ini<1024, 12> config;
        if (config.Load("controls.ini")) {
            const int choice = config.Int("controls", "menu_button", slot, 0, 9);
            button = MenuButtons[choice];
            buttonName = MenuButtonNames[choice];
            showStatus = config.Bool("controls", "show_status", true);
        }
        uint32_t ui[1]{uint32_t(showStatus)};
        if (uiSettings.Load(ui))
            showStatus = ui[0] != 0;
        shortcutControl = button;
        inputSeen = 0;
    }
    void Status(const char *state, const char *hint = "") {
        if (!ScriptHook::UI::Same(currentStatus, state)) {
            ScriptHook::UI::Copy(currentStatus, state);
            dirty = true;
        }
        ScriptHook::UI::Copy(objective, hint);
    }
    bool Frame() {
        const bool left = Held(205), right = Held(206), trigger = Held(button);
        const bool shoulders = left && right;
        bool otherTrigger = false;
        if (shoulders && trigger)
            for (int other : MenuButtons) {
                if (other != button && Held(other)) {
                    otherTrigger = true;
                    break;
                }
            }
        const uint32_t seen = (left ? 1u : 0u) | (right ? 2u : 0u) | (trigger ? 4u : 0u) |
                              (shoulders && trigger ? 8u : 0u);
        if ((inputSeen | seen) != inputSeen) {
            inputSeen |= seen;
            dirty = true;
        }
        // Always clear transient actions, including frames where gameplay is
        // paused. A prior action must never replay after a transition.
        menu.Begin(title);
        if (!UiSafe() || (!menu.IsOpen() && !PLAYER::IS_PLAYER_CONTROL_ON(PLAYER::PLAYER_ID()))) {
            if (menu.IsOpen())
                menu.Close();
            shortcut.Cancel(shoulders, trigger);
            return false;
        }
        // Reserve gameplay actions as soon as the shoulders are held, before
        // the menu trigger arrives. Test 1 waited for all four buttons.
        if (shoulders) {
            const int actions[] = {27, 37, 44, 68, 76, 85, 22, 23, 71, 72, 233, 234};
            for (int a : actions)
                PAD::DISABLE_CONTROL_ACTION(0, a, true);
        }
        if (shortcut.Poll(shoulders, trigger && !otherTrigger)) {
            menu.Toggle();
            controller.Reset();
            controlsReady = false;
            if (menu.IsOpen()) {
                ++menuOpens;
                dirty = true;
            }
        }
        const bool ready = shortcut.NavigationReady(shoulders, trigger);
        ScriptHook::UI::Input input{};
        if (menu.IsOpen()) {
            if (ready && !controlsReady)
                controller.Reset();
            input = controller.Poll(Now());
            if (!ready)
                input = {};
        }
        controlsReady = ready;
        menu.Begin(title, input);
        return true;
    }
    void End() {
        if (menu.IsOpen()) {
            TextBuffer<96> control;
            control.Add("L + R + ")
                .Add(buttonName)
                .Add(" opens/closes. Release the shortcut before navigating.");
            menu.Toggle(9001, "Show status badge", showStatus,
                        "A compact status and objective at the right of the screen.");
            menu.ReadOnly(9002, "Status", currentStatus, objective);
            menu.ReadOnly(9003, "Menu shortcut", buttonName, control.CStr());
            menu.End();
            renderer.Draw(menu);
            if (menu.Changed() == 9001) {
                uint32_t ui[1]{uint32_t(showStatus)};
                Notice(uiSettings.Save(ui) ? "Status display preference saved"
                                           : "Status preference save failed");
            }
            if (menu.Activated() || menu.Changed()) {
                ++menuActions;
                dirty = true;
            }
        }
        if (!menu.IsOpen() && openBefore)
            dirty = true;
        openBefore = menu.IsOpen();
        const uint32_t now = Now();
        Badge(Held(205) && Held(206));
        if (dirty && Due(now, reportAt)) {
            Report(menu.IsOpen() ? "menu_open" : "running");
            reportAt = now + 3000;
            dirty = false;
        }
        if (!Due(now, messageUntil)) {
            const float y = .66f + float(index) * .027f;
            GRAPHICS::DRAW_RECT(.305f, y + .013f, .57f, .026f, 15, 24, 32, 220, false);
            Text(message, .025f, y, .25f);
        }
    }
    bool Open() const { return menu.IsOpen(); }
};
} // namespace Port
