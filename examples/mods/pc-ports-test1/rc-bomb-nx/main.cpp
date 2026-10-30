// NX adaptation of root-cause/v-rcbomb (MIT). Native controller UI replaces NativeUI.
#include "port.hpp"
using namespace Port;
static Vehicle van = 0, rc = 0;
static bool active = false;
static float returnX = 0, returnY = 0, returnZ = 0;
static float returnHeading = 0;
static Blip vanPin = 0;
Vector3 VanPos() { return V(974.068f, -139.491f, 74.0638f); }
void Return() {
    if (active && Exists(PedSelf()) && !ENTITY::IS_ENTITY_DEAD(PedSelf(), false) && Car() == rc) {
        if (Exists(van) && VEHICLE::IS_VEHICLE_SEAT_FREE(van, -1, false))
            PED::SET_PED_INTO_VEHICLE(PedSelf(), van, -1);
        else {
            ENTITY::SET_ENTITY_COORDS(PedSelf(), V(returnX, returnY, returnZ), false, false, false,
                                      true);
            ENTITY::SET_ENTITY_HEADING(PedSelf(), returnHeading);
        }
    }
    active = false;
}
void Stop() {
    Return();
    ClearOwned();
    van = rc = 0;
    vanPin = 0;
}
void ScriptMain() {
    Stop();
    App app(4, "RC Bomb NX");
    Report("ready");
    for (;;) {
        if (!app.Frame()) {
            if (active && (!Exists(rc) || ENTITY::IS_ENTITY_DEAD(PedSelf(), false))) {
                Return();
                Delete(rc);
            }
            WAIT(100);
            continue;
        }
        app.Status(active        ? "RC ACTIVE"
                   : Exists(van) ? "Workshop van ready"
                                 : "Workshop not set up",
                   active ? "Open the menu to detonate or return safely"
                          : "Locate workshop, create van, approach it to deploy");
        if (app.Open()) {
            app.menu.Action(1, "Locate RC van", "Original workshop location near East Vinewood.");
            app.menu.Action(2, "Create van at workshop",
                            "Creates only the mod-owned van, never replaces a personal vehicle.");
            app.menu.Action(3, "Deploy RC Bandito",
                            "Within 12 metres of the van. Requires installed RC Bandito DLC.", !active);
            app.menu.Action(4, "Detonate",
                            "Returns the player first. Detonation is blocked near the return point.",
                            active);
            app.menu.Action(5, "Return / cancel RC",
                            "No invincibility, global timecycle or radar changes.", active);
            app.menu.Action(6, "Remove workshop vehicles");
        }
        app.End();
        int a = int(app.menu.Activated());
        if (a == 1) {
            Route(Exists(van) ? ENTITY::GET_ENTITY_COORDS(van, true) : VanPos());
            Notice("RC workshop waypoint set");
        }
        if (a == 2) {
            if (!Exists(van)) {
                van = VehicleAt("gburrito", VanPos(), 60.f);
                if (van)
                    vanPin = Pin(VanPos(), 67, 3);
                if (van)
                    Notice("Workshop van created and marked");
            } else {
                Route(ENTITY::GET_ENTITY_COORDS(van, true));
                Notice("Workshop van already exists; location marked");
            }
        }
        if (a == 3 && !active) {
            if (!Exists(van) || Dist2(ENTITY::GET_ENTITY_COORDS(PedSelf(), true),
                                      ENTITY::GET_ENTITY_COORDS(van, true)) > 144) {
                Notice("Create the van, then approach it");
            } else {
                Vector3 before = ENTITY::GET_ENTITY_COORDS(PedSelf(), true);
                returnX = before.x;
                returnY = before.y;
                returnZ = before.z;
                returnHeading = ENTITY::GET_ENTITY_HEADING(PedSelf());
                rc = VehicleAt("rcbandito",
                               ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(van, V(0, -4, 0)),
                               returnHeading);
                if (rc) {
                    PED::SET_PED_INTO_VEHICLE(PedSelf(), rc, -1);
                    active = Car() == rc;
                    if (!active) {
                        Delete(rc);
                        Notice("RC seat could not be entered");
                    } else {
                        app.menu.Close();
                        Notice("RC active: use the menu to return or detonate");
                        Report("rc_active");
                    }
                }
            }
        }
        if (a == 4 && active) {
            Vector3 blast = ENTITY::GET_ENTITY_COORDS(rc, true);
            Return();
            if (Dist2(blast, ENTITY::GET_ENTITY_COORDS(PedSelf(), true)) > 225) {
                VEHICLE::EXPLODE_VEHICLE(rc, true, false);
                Report("detonated");
            } else
                Notice("Too close to the return point: RC cancelled without explosion");
            Delete(rc);
        }
        if (a == 5) {
            Return();
            Delete(rc);
            Notice("Returned from RC control");
        }
        if (a == 6) {
            Stop();
            Notice("Workshop vehicles removed");
        }
        if (active) {
            const int controls[] = {23, 75, 37, 27};
            for (int control : controls)
                PAD::DISABLE_CONTROL_ACTION(0, control, true);
            if (!Exists(rc) || Car() != rc ||
                Dist2(V(returnX, returnY, returnZ), ENTITY::GET_ENTITY_COORDS(rc, true)) > 40000) {
                Return();
                Delete(rc);
                Notice("RC session ended: signal lost or vehicle exited");
            }
        }
        WAIT(0);
    }
}
SHV_NX_PLUGIN_WITH_STOP("rc-bomb-nx", ScriptMain, Stop)
