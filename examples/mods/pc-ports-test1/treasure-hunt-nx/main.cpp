// NX adaptation of root-cause/v-sptreasurehunt (MIT). Original location data.
#include "locations.hpp"
#include "port.hpp"
using namespace Port;
static Entity clue = 0;
static Blip pin = 0;
static Pickup rewards[5]{};
void Stop() {
    ClearOwned();
    clue = 0;
    pin = 0;
    for (auto &r : rewards) {
        if (r)
            OBJECT::REMOVE_PICKUP(r);
        r = 0;
    }
}
Vector3 Pos(Data::Location l) { return V(l.pos.x, l.pos.y, l.pos.z); }
void ScriptMain() {
    Stop();
    App app(8, "Single Player Treasure Hunt NX");
    ScriptHook::Util::SavedSettings<4> save(0x54524831);
    uint32_t state[4]{};
    if (!save.Load(state) || state[0] > 4 || state[1] >= 20 || state[2] > 7 || state[3] >= 20) {
        for (auto &s : state)
            s = 0;
    }
    int active = -1;
    uint32_t retry = 0, next = 0;
    bool guideNext = false;
    bool enabled = true;
    const char *clueNames[] = {"xm_prop_x17_corpse_03", "xm_prop_x17_shovel_01b",
                               "xm_prop_x17_chest_open"};
    Report("ready");
    for (;;) {
        if (!app.Frame()) {
            WAIT(100);
            continue;
        }
        const char *stages[] = {"Not started", "Find the note", "Find three clues",
                                "Find the treasure chest", "Revolver unlocked"};
        app.Status(!enabled ? "PAUSED" : stages[state[0]],
                   state[0] == 0   ? "Open the menu to begin the investigation"
                   : state[0] == 4 ? "Complete | Claim revolver again from the menu"
                                   : "Follow the waypoint; D-Pad Right near the clue");
        if (app.Open()) {
            app.menu.Toggle(1, "Hunt enabled", enabled);
            const char *stages[] = {"Not started", "Find the note", "Find three clues",
                                    "Find the treasure chest", "Double-action unlocked"};
            app.menu.ReadOnly(2, "Stage", stages[state[0]]);
            app.menu.Action(
                3, "Begin treasure hunt",
                "Selects one of the 20 original note sites. No PC email/photo UI is required.",
                state[0] == 0);
            app.menu.Action(4, "Locate current objective",
                            "Map guidance replaces the PC photo/email interface.",
                            state[0] > 0 && state[0] < 4);
            app.menu.Action(5, "Claim unlocked revolver", "Available after finishing the hunt.",
                            state[0] == 4);
            app.menu.Action(6, "Investigate nearby objective",
                            "Stand within three metres of the current note, clue or chest. D-Pad Right "
                            "also works with the menu closed.",
                            enabled && state[0] > 0 && state[0] < 4);
        }
        app.End();
        if (app.menu.Activated() == 3) {
            state[0] = 1;
            state[1] = uint32_t(MISC::GET_RANDOM_INT_IN_RANGE(0, 20));
            state[3] = uint32_t(MISC::GET_RANDOM_INT_IN_RANGE(0, 20));
            if (!save.Save(state)) {
                state[0] = 0;
                Notice("Progress save failed");
            } else {
                Report("hunt_started");
                guideNext = true;
                Notice("Treasure hunt started; note location marked");
            }
        }
        if (app.menu.Activated() == 5 && state[0] == 4) {
            WEAPON::GIVE_WEAPON_TO_PED(PedSelf(), H("WEAPON_DOUBLEACTION"), 60, false, true);
            Notice("Double-action Revolver equipped");
        }
        if (!enabled) {
            Delete(clue);
            Unpin(pin);
            active = -1;
            WAIT(0);
            continue;
        }
        int objective = -1;
        Data::Location location{};
        const char *model = nullptr;
        if (state[0] == 1) {
            objective = 0;
            location = Data::treasure_noteLocations[state[1]];
            model = "xm_prop_x17_note_paper_01a";
        }
        if (state[0] == 2) {
            for (int i = 0; i < 3; ++i)
                if (!(state[2] & (1u << i))) {
                    objective = i + 1;
                    location = Data::treasure_clueLocations[i];
                    model = clueNames[i];
                    break;
                }
        }
        if (state[0] == 3) {
            objective = 4;
            location = Data::treasure_finalChestLocations[state[3]];
            model = "xm_prop_x17_chest_closed";
        }
        if (objective != active) {
            Delete(clue);
            Unpin(pin);
            active = objective;
            retry = 0;
        }
        if (active >= 0) {
            Vector3 target = Pos(location), player = ENTITY::GET_ENTITY_COORDS(PedSelf(), true);
            float distance = Dist2(player, target);
            if (app.menu.Activated() == 4 || guideNext) {
                Route(target);
                guideNext = false;
                if (app.menu.Activated() == 4)
                    Notice("Current treasure objective waypoint set");
            }
            if (distance < 10000 && !clue && Due(Now(), retry)) {
                retry = Now() + 5000;
                clue = Prop(model, target, V(location.rot.x, location.rot.y, location.rot.z));
            }
            if (distance > 22500 && clue) {
                Delete(clue);
            }
            if (distance < 2500) {
                Marker(target);
            }
            const bool menuInspect = app.menu.Activated() == 6;
            if (menuInspect && (distance >= 9 || !clue))
                Notice("Approach the marked objective within three metres and wait for its prop");
            if (distance < 9 && clue) {
                if (!app.Open())
                    Text("Treasure clue | D-Pad Right: investigate", .025f, .86f);
                if (menuInspect || (!app.Open() && Press())) {
                    uint32_t before[4];
                    for (int i = 0; i < 4; ++i)
                        before[i] = state[i];
                    if (state[0] == 1)
                        state[0] = 2;
                    else if (state[0] == 2) {
                        state[2] |= 1u << (active - 1);
                        if (state[2] == 7)
                            state[0] = 3;
                    } else
                        state[0] = 4;
                    if (save.Save(state)) {
                        Delete(clue);
                        Unpin(pin);
                        active = -1;
                        guideNext = true;
                        Notice(state[0] == 4 ? "Treasure found: Double-action Revolver unlocked"
                                             : "Clue recorded; use the menu for the next objective");
                        if (state[0] == 4)
                            WEAPON::GIVE_WEAPON_TO_PED(PedSelf(), H("WEAPON_DOUBLEACTION"), 60, false,
                                                       true);
                        Report("progress_saved");
                    } else {
                        for (int i = 0; i < 4; ++i)
                            state[i] = before[i];
                        Notice("Progress save failed");
                    }
                }
            }
        } else if (state[0] == 4 && Due(Now(), next)) {
            next = Now() + 1000;
            Vector3 player = ENTITY::GET_ENTITY_COORDS(PedSelf(), true);
            for (int i = 0; i < 5; ++i) {
                auto l = Data::treasure_rewardLocations[i];
                if (Dist2(player, Pos(l)) < 2500 && !rewards[i])
                    rewards[i] =
                        OBJECT::CREATE_PICKUP_ROTATE(int(H("PICKUP_WEAPON_DOUBLEACTION")), Pos(l),
                                                     V(l.rot.x, l.rot.y, l.rot.z), 4, 60, 2, false, 0);
            }
        }
        WAIT(0);
    }
}
SHV_NX_PLUGIN_WITH_STOP("treasure-hunt-nx", ScriptMain, Stop)
