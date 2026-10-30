// NX adaptation of root-cause/v-spjammerhunt (MIT), original 50 locations.
#include "locations.hpp"
#include "port.hpp"
using namespace Port;
static Entity prop = 0;
static Blip pin = 0;
static Any generators[3]{};
static bool rewardSitesReady = false;
void Stop() {
    for (auto &generator : generators) {
        if (generator)
            VEHICLE::DELETE_SCRIPT_VEHICLE_GENERATOR(generator);
        generator = 0;
    }
    rewardSitesReady = false;
    ClearOwned();
    prop = 0;
    pin = 0;
}
Vector3 Pos(int i) {
    auto p = Data::jammers[i].pos;
    return V(p.x, p.y, p.z);
}
void ScriptMain() {
    Stop();
    App app(7, "Signal Jammer Hunt NX");
    ScriptHook::Util::SavedSettings<3> save(0x4a414d31);
    uint32_t progress[3]{};
    if (!save.Load(progress) || progress[1] > 0x3ffff || progress[2] > 1) {
        progress[0] = progress[1] = progress[2] = 0;
    }
    bool enabled = true, hints = false;
    int active = -1;
    uint32_t next = 0, retry = 0;
    Report("ready");
    for (;;) {
        if (!app.Frame()) {
            WAIT(100);
            continue;
        }
        Vector3 player = ENTITY::GET_ENTITY_COORDS(PedSelf(), true);
        int count = 0;
        for (int i = 0; i < 50; ++i)
            if (progress[i / 32] & (1u << (i % 32)))
                ++count;
        TextBuffer<64> status;
        status.Add(enabled ? "ON | " : "OFF | ").UInt(unsigned(count)).Add(" / 50 destroyed");
        app.Status(status.CStr(), count == 50 ? "Complete: Thruster reward sites unlocked"
                                              : "Locate a jammer in the menu, then shoot it");
        if (app.Open()) {
            app.menu.Toggle(1, "Hunt enabled", enabled,
                            "Streams only the closest remaining jammer within 250 metres. Requires the "
                            "installed Casino Heist jammer prop.");
            app.menu.Toggle(2, "Map hint", hints);
            TextBuffer<32> label;
            label.UInt(unsigned(count)).Add(" / 50");
            app.menu.ReadOnly(3, "Destroyed", label.CStr());
            app.menu.Action(4, "Locate nearest remaining jammer");
            app.menu.ReadOnly(5, "Rewards", "$2,000 each; $50,000 completion");
        }
        app.End();
        if (!enabled) {
            if (rewardSitesReady)
                Stop();
            if (prop)
                Delete(prop);
            Unpin(pin);
            active = -1;
            WAIT(0);
            continue;
        }
        if (count == 50 && !rewardSitesReady && Due(Now(), retry)) {
            retry = Now() + 60000;
            Hash thruster = H("thruster");
            if (STREAMING::IS_MODEL_VALID(thruster)) {
                const float sites[3][4] = {{1753.62f, 3323.f, 41.04f, 210.f},
                                           {2025.72f, 4726.97f, 41.4855f, 27.f},
                                           {-947.808f, -3530.81f, 13.9186f, 24.f}};
                for (int i = 0; i < 3; ++i) {
                    Vector3 p = V(sites[i][0], sites[i][1], sites[i][2]);
                    generators[i] = VEHICLE::CREATE_SCRIPT_VEHICLE_GENERATOR(
                        p, sites[i][3], 5.f, 3.f, thruster, -1, -1, -1, -1, true, 0, 0, false, true, -1);
                    Pin(p, 1, 3);
                }
                rewardSitesReady = true;
                Notice("All jammers cleared: Thrusters marked at the three original reward sites");
            } else
                Notice("Jammers complete; Thruster DLC model is not installed");
        }
        if (Due(Now(), next) || app.menu.Activated() == 4) {
            next = Now() + 1000;
            int closest = -1;
            float best = 1.e20f;
            for (int i = 0; i < 50; ++i)
                if (!(progress[i / 32] & (1u << (i % 32)))) {
                    float d = Dist2(player, Pos(i));
                    if (d < best) {
                        best = d;
                        closest = i;
                    }
                }
            if (app.menu.Activated() == 4 && closest >= 0) {
                Route(Pos(closest));
                Notice("Nearest remaining jammer waypoint set");
            }
            int near = best < 62500 ? closest : -1;
            if (near != active) {
                Delete(prop);
                Unpin(pin);
                active = near;
                retry = 0;
            }
            if (active >= 0 && !prop && Due(Now(), retry)) {
                retry = Now() + 5000;
                auto rot = Data::jammers[active].rot;
                prop = Prop("ch_prop_ch_mobile_jammer_01x", Pos(active), V(rot.x, rot.y, rot.z), false);
                if (prop) {
                    OBJECT::SET_OBJECT_TARGETTABLE(prop, true);
                    ENTITY::SET_ENTITY_ONLY_DAMAGED_BY_PLAYER(prop, true);
                    ENTITY::SET_ENTITY_PROOFS(prop, false, false, false, true, false, false, false,
                                              false);
                    retry = Now() + 250;
                }
            }
        }
        if (active >= 0) {
            if (hints && !pin)
                pin = Pin(Pos(active), 1, 3);
            if (!hints)
                Unpin(pin);
            if (prop && Due(Now(), retry) &&
                (!Exists(prop) || ENTITY::IS_ENTITY_DEAD(prop, false) ||
                 OBJECT::HAS_OBJECT_BEEN_BROKEN(prop, false) || ENTITY::IS_ENTITY_IN_AIR(prop))) {
                uint32_t before = progress[active / 32];
                progress[active / 32] |= 1u << (active % 32);
                if (count == 49)
                    progress[2] = 1;
                if (save.Save(progress)) {
                    Delete(prop);
                    Unpin(pin);
                    active = -1;
                    Pay(count == 49 ? 52000 : 2000);
                    Notice(count == 49 ? "All 50 signal jammers destroyed" : "Signal jammer destroyed");
                    Report("jammer_destroyed");
                    next = 0;
                } else {
                    progress[active / 32] = before;
                    progress[2] = 0;
                    retry = Now() + 5000;
                    Notice("Progress save failed: reward held");
                }
            }
        }
        WAIT(0);
    }
}
SHV_NX_PLUGIN_WITH_STOP("jammer-hunt-nx", ScriptMain, Stop)
