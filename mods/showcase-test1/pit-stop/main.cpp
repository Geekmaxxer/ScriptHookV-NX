#include "mod_support.hpp"

using namespace Showcase;

namespace {
constexpr uint32_t kSettingsMagic = 0x50535431; // PST1
constexpr uint32_t kCooldownMs = 2000;
constexpr uint32_t kPopupMs = 5000;

struct PitStop {
    SavedWords storage{kSettingsMagic};
    uint32_t repairs = 0;
    uint32_t popupUntil = 0;
    uint32_t nextRepair = 0;
    bool coolingDown = false;
    bool popupVisible = false;
    const char* message = "";

    void Notify(const char* text, uint32_t now) {
        message = text;
        popupUntil = now + kPopupMs;
        popupVisible = true;
        SaveStatus(text);
    }

    void Request(uint32_t now) {
        if (coolingDown && !Due(now, nextRepair)) {
            Notify("Please wait two seconds between repairs.", now);
            return;
        }
        // Complete every eligibility check before the first vehicle mutation.
        const bool paused = HUD::IS_PAUSE_MENU_ACTIVE();
        if (!NativeOk()) { Notify("Repair unavailable: pause state could not be read.", now); return; }
        if (paused) { Notify("Resume the game before repairing.", now); return; }

        const bool cutscene = CUTSCENE::IS_CUTSCENE_ACTIVE();
        if (!NativeOk()) { Notify("Repair unavailable: cutscene state could not be read.", now); return; }
        if (cutscene) { Notify("Repair unavailable during a cutscene.", now); return; }

        const Player player = PLAYER::PLAYER_ID();
        if (!NativeOk()) { Notify("Repair unavailable: player could not be read.", now); return; }
        const bool controlled = PLAYER::IS_PLAYER_CONTROL_ON(player);
        if (!NativeOk()) { Notify("Repair unavailable: player control could not be read.", now); return; }
        if (!controlled) { Notify("Wait until you have control of the player.", now); return; }

        const Ped ped = PLAYER::PLAYER_PED_ID();
        if (!NativeOk() || !ped) { Notify("Repair unavailable: player is not ready.", now); return; }
        const bool pedExists = ENTITY::DOES_ENTITY_EXIST(ped);
        if (!NativeOk() || !pedExists) { Notify("Repair unavailable: player is not ready.", now); return; }
        const bool playerDead = ENTITY::IS_ENTITY_DEAD(ped, false);
        if (!NativeOk()) { Notify("Repair unavailable: player state could not be read.", now); return; }
        if (playerDead) { Notify("Repair unavailable while the player is dead.", now); return; }

        const Vehicle vehicle = PlayerVehicle(true);
        if (!vehicle) { Notify("Sit in the driver's seat of a car or bike.", now); return; }

        const Hash model = ENTITY::GET_ENTITY_MODEL(vehicle);
        if (!NativeOk()) { Notify("Repair unavailable: vehicle model could not be read.", now); return; }
        const bool car = VEHICLE::IS_THIS_MODEL_A_CAR(model);
        if (!NativeOk()) { Notify("Repair unavailable: vehicle type could not be read.", now); return; }
        const bool bike = VEHICLE::IS_THIS_MODEL_A_BIKE(model);
        if (!NativeOk()) { Notify("Repair unavailable: vehicle type could not be read.", now); return; }
        if (!car && !bike) { Notify("Pit Stop supports cars and bikes only.", now); return; }

        const bool vehicleDead = ENTITY::IS_ENTITY_DEAD(vehicle, false);
        if (!NativeOk()) { Notify("Repair unavailable: vehicle state could not be read.", now); return; }
        if (vehicleDead) { Notify("Pit Stop cannot repair a destroyed vehicle.", now); return; }
        const bool inAir = ENTITY::IS_ENTITY_IN_AIR(vehicle);
        if (!NativeOk()) { Notify("Repair unavailable: vehicle position could not be read.", now); return; }
        if (inAir) { Notify("Park on the ground before repairing.", now); return; }

        const float speed = ENTITY::GET_ENTITY_SPEED(vehicle);
        if (!NativeOk()) { Notify("Repair unavailable: vehicle speed could not be read.", now); return; }
        // This form also rejects NaN and negative values.
        if (!(speed >= 0.0f && speed <= 0.5f)) {
            Notify("Stop the vehicle before repairing.", now);
            return;
        }

        nextRepair = now + kCooldownMs;
        coolingDown = true;
        VEHICLE::SET_VEHICLE_FIXED(vehicle);
        if (!NativeOk()) { Notify("Vehicle repair could not be applied.", now); return; }
        VEHICLE::SET_VEHICLE_DIRT_LEVEL(vehicle, 0.0f);
        if (!NativeOk()) { Notify("Vehicle repaired; washing could not be applied.", now); return; }

        if (repairs != 0xffffffffu) ++repairs;
        const uint32_t values[] = { repairs };
        const bool saved = storage.Save(values, 1);
        Notify(saved ? "Vehicle repaired and washed." :
                       "Vehicle repaired and washed; counter could not be saved.", now);
    }

    void Draw(uint32_t now) {
        if (!popupVisible) return;
        if (Due(now, popupUntil)) { popupVisible = false; return; }
        TextLine line;
        line.Add("Pit Stop: ").Add(message);
        DrawLine(0.04f, 0.72f, 0.82f, line.CStr(), { 110, 225, 170 });
    }
};
}

void ScriptMain() {
    if (!Start()) return;
    PitStop pitStop;
    uint32_t saved[] = { 0 };
    if (pitStop.storage.Load(saved, 1)) pitStop.repairs = saved[0];
    SaveStatus("ready; hold L + R and press D-Pad Left while parked in the driver's seat");
    for (;;) {
        const uint32_t now = Now();
        const bool visible = Visible();
        if (visible && Hotkey(Direction::Left)) pitStop.Request(now);
        if (visible) pitStop.Draw(now);
        WAIT(0);
    }
}

SHV_NX_PLUGIN("pit-stop", ScriptMain)
