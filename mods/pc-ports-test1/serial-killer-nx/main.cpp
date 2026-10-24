// NX gameplay adaptation of root-cause/v-spkiller (MIT), original clue/van sites.
#include "locations.hpp"
#include "port.hpp"
using namespace Port;
static Entity clue = 0;
static Vehicle van = 0;
static Ped killer = 0;
static Blip pin = 0;
static Pickup rewards[5]{};
void Stop() {
    for (auto &reward : rewards) {
        if (reward)
            OBJECT::REMOVE_PICKUP(reward);
        reward = 0;
    }
    ClearOwned();
    clue = van = killer = 0;
    pin = 0;
}
Vector3 Pos(Data::Location p) { return V(p.pos.x, p.pos.y, p.pos.z); }
void ScriptMain() {
    Stop();
    App app(9, "Los Santos Slasher NX");
    ScriptHook::Util::SavedSettings<4> save(0x534b4c31);
    uint32_t state[4]{};
    if (!save.Load(state) || state[0] > 4 || state[1] > 15 || state[2] >= 5 || state[3] > 1) {
        for (auto &s : state)
            s = 0;
    }
    bool enabled = true;
    int active = -1;
    uint32_t retry = 0, attackAt = 0, rewardTick = 0;
    bool guideNext = false;
    const char *props[] = {"ch_prop_collectibles_limb_01a", "ch_prop_ch_serialkiller_01a",
                           "ch_prop_ch_boodyhand_01a", "ch_prop_ch_bloodymachete_01a"};
    Report("ready");
    for (;;) {
        if (!app.Frame()) {
            if (killer && ENTITY::IS_ENTITY_DEAD(PedSelf(), false)) {
                Delete(killer);
                attackAt = Now() + 120000;
            }
            WAIT(100);
            continue;
        }
        const char *stageNames[] = {"Not started", "Find four clues", "Find the killer's van",
                                    "Night ambush pending", "Navy Revolver unlocked"};
        const char *hint = state[0] == 0   ? "Open the menu to begin the investigation"
                           : state[0] == 4 ? "Complete | Claim revolver again from the menu"
                                           : "Follow the waypoint; investigate with D-Pad Right";
        TextBuffer<128> ambush;
        if (state[0] == 3) {
            const int hour = CLOCK::GET_CLOCK_HOURS();
            if (killer)
                hint = "AMBUSH: defend yourself against the killer";
            else if (hour < 19 && hour >= 5)
                hint = "Wait until 19:00-05:00";
            else if (Car())
                hint = "Exit your vehicle for the ambush";
            else if (PLAYER::GET_PLAYER_WANTED_LEVEL(PLAYER::PLAYER_ID()))
                hint = "Lose your wanted level";
            else if (uint32_t(ZONE::GET_HASH_OF_MAP_AREA_AT_COORDS(
                         ENTITY::GET_ENTITY_COORDS(PedSelf(), true))) != H("countryside"))
                hint = "Travel to the countryside";
            else {
                ambush.Add("Stay alert | ")
                    .UInt(!attackAt              ? 120
                          : Due(Now(), attackAt) ? 0
                                                 : (attackAt - Now()) / 1000)
                    .Add("s until ambush check");
                hint = ambush.CStr();
            }
        }
        app.Status(!enabled ? "PAUSED" : stageNames[state[0]], hint);
        if (app.Open()) {
            const char *stages[] = {"Not started", "Find four clues", "Find the killer's van",
                                    "Night ambush pending", "Navy Revolver unlocked"};
            app.menu.Toggle(1, "Investigation enabled", enabled);
            app.menu.ReadOnly(2, "Stage", stages[state[0]]);
            app.menu.Action(3, "Begin investigation",
                            "Four clues, a randomized van, then a countryside ambush after 19:00.",
                            state[0] == 0);
            app.menu.Action(4, "Locate next clue / van",
                            "Map guidance replaces PC investigation cameras and text messages.",
                            state[0] == 1 || state[0] == 2);
            app.menu.Action(5, "Claim unlocked revolver", "", state[0] == 4);
            app.menu.ReadOnly(6, "Ambush requirements", "On foot, countryside, 19:00-05:00");
            app.menu.Action(7, "Investigate nearby clue / van",
                            "Stand within four metres of the marked clue or van. D-Pad Right also works "
                            "with the menu closed.",
                            enabled && (state[0] == 1 || state[0] == 2));
        }
        app.End();
        int a = int(app.menu.Activated());
        if (a == 3) {
            state[0] = 1;
            state[2] = uint32_t(MISC::GET_RANDOM_INT_IN_RANGE(0, 5));
            if (!save.Save(state)) {
                state[0] = 0;
                Notice("Progress save failed");
            } else {
                guideNext = true;
                Notice("Investigation started; first clue marked");
            }
        }
        if (a == 5 && state[0] == 4) {
            WEAPON::GIVE_WEAPON_TO_PED(PedSelf(), H("WEAPON_NAVYREVOLVER"), 60, false, true);
            Notice("Navy Revolver equipped");
        }
        if (!enabled) {
            Stop();
            active = -1;
            WAIT(0);
            continue;
        }
        int objective = -1;
        Data::Location location{};
        if (state[0] == 1) {
            for (int i = 0; i < 4; ++i)
                if (!(state[1] & (1u << i))) {
                    objective = i;
                    location = Data::killer_clueLocations[i];
                    break;
                }
        } else if (state[0] == 2) {
            objective = 4;
            location = Data::killer_vanLocations[state[2]];
        }
        if (objective != active) {
            Delete(clue);
            Delete(van);
            Unpin(pin);
            active = objective;
            retry = 0;
        }
        if (active >= 0) {
            Vector3 pos = Pos(location), player = ENTITY::GET_ENTITY_COORDS(PedSelf(), true);
            float d = Dist2(pos, player);
            if (a == 4 || guideNext) {
                Route(pos);
                guideNext = false;
                if (a == 4)
                    Notice("Next investigation waypoint set");
            }
            if (d < 10000 && !clue && !van && Due(Now(), retry)) {
                retry = Now() + 5000;
                if (active == 4)
                    van = VehicleAt("speedo", pos, location.heading);
                else
                    clue = Prop(props[active], pos, V(location.rot.x, location.rot.y, location.rot.z));
            }
            if (d > 22500) {
                Delete(clue);
                Delete(van);
            }
            if (d < 2500)
                Marker(pos, 1);
            if (a == 7 && (d >= 16 || (!clue && !van)))
                Notice("Approach the marked clue or van within four metres");
            if (d < 16 && (clue || van)) {
                if (!app.Open())
                    Text("Slasher clue | D-Pad Right: investigate");
                if (a == 7 || (!app.Open() && Press())) {
                    uint32_t before[4];
                    for (int i = 0; i < 4; ++i)
                        before[i] = state[i];
                    if (state[0] == 1) {
                        state[1] |= 1u << active;
                        if (state[1] == 15)
                            state[0] = 2;
                    } else {
                        state[0] = 3;
                        attackAt = Now() + 120000;
                    }
                    if (save.Save(state)) {
                        Delete(clue);
                        Delete(van);
                        active = -1;
                        guideNext = true;
                        Notice(state[0] == 3
                                   ? "A threat received. Stay alert in the countryside at night"
                                   : "Clue recorded");
                        Report("clue_recorded");
                    } else {
                        for (int i = 0; i < 4; ++i)
                            state[i] = before[i];
                        Notice("Progress save failed");
                    }
                }
            }
        }
        if (state[0] == 3) {
            int hour = CLOCK::GET_CLOCK_HOURS();
            if (!attackAt)
                attackAt = Now() + 120000;
            Vector3 player = ENTITY::GET_ENTITY_COORDS(PedSelf(), true);
            if (!killer && Due(Now(), attackAt)) {
                attackAt = Now() + 5000;
                if ((hour >= 19 || hour < 5) && !Car() &&
                    PLAYER::GET_PLAYER_WANTED_LEVEL(PLAYER::PLAYER_ID()) == 0 &&
                    uint32_t(ZONE::GET_HASH_OF_MAP_AREA_AT_COORDS(player)) == H("countryside")) {
                    Vector3 behind =
                                ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(PedSelf(), V(0, -15, 0)),
                            safe{};
                    Hash h = H("a_m_y_salton_01");
                    if (PATH::GET_SAFE_COORD_FOR_PED(behind, false, &safe, 16) && Model(h)) {
                        killer = Own(PED::CREATE_PED(26, h, safe, 0.f, false, false));
                        Release(h);
                        if (killer) {
                            WEAPON::GIVE_WEAPON_TO_PED(killer, H("WEAPON_MACHETE"), 1, false, true);
                            PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(killer, true);
                            TASK::TASK_COMBAT_PED(killer, PedSelf(), 0, 16);
                            Report("ambush_started");
                        }
                    }
                }
            }
            if (killer) {
                if (ENTITY::IS_ENTITY_DEAD(killer, false)) {
                    state[0] = 4;
                    if (save.Save(state)) {
                        Delete(killer);
                        WEAPON::GIVE_WEAPON_TO_PED(PedSelf(), H("WEAPON_NAVYREVOLVER"), 60, false, true);
                        Pay(50000);
                        Notice("Slasher defeated: Navy Revolver unlocked");
                        Report("slasher_defeated");
                    } else {
                        state[0] = 3;
                        Notice("Completion save failed");
                    }
                } else if (Dist2(player, ENTITY::GET_ENTITY_COORDS(killer, true)) > 10000) {
                    Delete(killer);
                    attackAt = Now() + 120000;
                }
            }
        }
        if (state[0] == 4 && Due(Now(), rewardTick)) {
            rewardTick = Now() + 1000;
            Vector3 player = ENTITY::GET_ENTITY_COORDS(PedSelf(), true);
            for (int i = 0; i < 5; ++i) {
                auto l = Data::killer_rewardLocations[i];
                if (Dist2(player, Pos(l)) < 2500 && !rewards[i])
                    rewards[i] =
                        OBJECT::CREATE_PICKUP_ROTATE(int(H("PICKUP_WEAPON_NAVYREVOLVER")), Pos(l),
                                                     V(l.rot.x, l.rot.y, l.rot.z), 4, 60, 2, false, 0);
            }
        }
        WAIT(0);
    }
}
SHV_NX_PLUGIN_WITH_STOP("serial-killer-nx", ScriptMain, Stop)
