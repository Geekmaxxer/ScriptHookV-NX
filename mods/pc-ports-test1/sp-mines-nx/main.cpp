// NX adaptation of root-cause/v-spmines, including its MIT default vehicle sets.
#include "mine_data.hpp"
#include "port.hpp"
using namespace Port;
static Hash pendingWeapon = 0;
void Stop() {
    if (pendingWeapon) {
        WEAPON::REMOVE_WEAPON_ASSET(pendingWeapon);
        pendingWeapon = 0;
    }
    ClearOwned();
}
void ScriptMain() {
    App app(3, "Single Player Vehicle Mines NX");
    bool enabled = true;
    uint32_t cooldown = 0;
    Report("ready");
    for (;;) {
        if (!app.Frame()) {
            WAIT(100);
            continue;
        }
        Vehicle car = Car();
        const MineData::Set *set = nullptr;
        Hash model = 0;
        if (Exists(car)) {
            model = ENTITY::GET_ENTITY_MODEL(car);
            for (const auto &v : MineData::vehicles)
                if (model == H(v.model)) {
                    set = &MineData::sets[v.set];
                    break;
                }
        }
        const int upgrade = set ? VEHICLE::GET_VEHICLE_MOD(car, 9) : -1;
        const bool driver = set && VEHICLE::GET_PED_IN_VEHICLE_SEAT(car, -1, false) == PedSelf();
        const bool upgraded = set && upgrade >= 0 && upgrade < set->count;
        app.Status(!enabled                ? "OFF"
                   : !set                  ? "Vehicle unsupported"
                   : !driver               ? "Driver seat required"
                   : !upgraded             ? "Mine upgrade required"
                   : !Due(Now(), cooldown) ? "Cooling down"
                                           : "READY",
                   "Eligible upgraded vehicle: press the horn to deploy");
        if (app.Open()) {
            app.menu.Toggle(
                1, "Vehicle mines", enabled,
                "Uses the original eligible vehicle sets and installed proximity-mine upgrade. "
                "Press the normal vehicle horn to deploy.");
            app.menu.ReadOnly(2, "Current vehicle", set ? "Supported" : "No mine set");
            app.menu.ReadOnly(3, "Mine upgrade", upgraded ? "Installed" : "Not installed / unsupported",
                              "Requires the proximity-mine upgrade in vehicle mod slot 9, from the "
                              "original supported vehicle sets.");
            app.menu.ReadOnly(4, "Cooldown", Due(Now(), cooldown) ? "Ready" : "Wait five seconds");
        }
        app.End();
        if (enabled && set && !app.Open() &&
            VEHICLE::GET_PED_IN_VEHICLE_SEAT(car, -1, false) == PedSelf() &&
            VEHICLE::IS_VEHICLE_ON_ALL_WHEELS(car) && !ENTITY::IS_ENTITY_UPSIDEDOWN(car)) {
            if (upgrade >= 0 && upgrade < set->count) {
                PAD::DISABLE_CONTROL_ACTION(0, 86, true);
                if (Press(86) && !Due(Now(), cooldown))
                    Notice("Mines are cooling down");
                if (Press(86) && Due(Now(), cooldown)) {
                    cooldown = Now() + 5000;
                    Hash mine = H(set->model), weapon = H(set->weapons[upgrade]);
                    if (Model(mine)) {
                        pendingWeapon = weapon;
                        WEAPON::REQUEST_WEAPON_ASSET(weapon, 31, 0);
                        uint32_t until = Now() + 3000;
                        for (unsigned n = 0;
                             n < 180 && !WEAPON::HAS_WEAPON_ASSET_LOADED(weapon) && !Due(Now(), until);
                             ++n)
                            WAIT(0);
                        if (WEAPON::HAS_WEAPON_ASSET_LOADED(weapon)) {
                            Vector3 min{}, max{};
                            MISC::GET_MODEL_DIMENSIONS(model, &min, &max);
                            Vector3 start = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(
                                        car, V(0, min.y + .5f, min.z + .2f)),
                                    end = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(
                                        car, V(0, min.y - .1f, min.z));
                            MISC::SHOOT_SINGLE_BULLET_BETWEEN_COORDS_IGNORE_ENTITY_NEW(
                                start, end, 0, true, weapon, PedSelf(), true, false, -1.f, car, true,
                                false, 0, true, true, false, false);
                            Notice("Mine deployment requested");
                            Report("mine_deployed");
                        } else {
                            Notice("Mine weapon asset unavailable");
                            Report("missing_weapon_asset");
                        }
                        WEAPON::REMOVE_WEAPON_ASSET(weapon);
                        pendingWeapon = 0;
                        Release(mine);
                    }
                }
            }
        }
        WAIT(0);
    }
}
SHV_NX_PLUGIN_WITH_STOP("sp-mines-nx", ScriptMain, Stop)
