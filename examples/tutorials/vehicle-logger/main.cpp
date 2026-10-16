#include "script.h"
#include "shv/time.h"
#include "shv/text.h"

void ScriptMain() {
    const uint64_t required[] = {0xe2d3d51028f0428aULL, 0xe5965cdf24f93a9fULL,
        0xb3ff0049c1fd38ecULL, 0x320d1840b6daa1ccULL};
    for (uint64_t hash : required) if (!nativeIsAvailable(hash)) return;
    bool previous = false, havePrevious = false;
    ScriptHook::Util::Every sample;
    while (true) {
        const uint32_t now = uint32_t(MISC::GET_GAME_TIMER());
        if (sample.Ready(now, 250)) {
            const Ped ped = PLAYER::PLAYER_PED_ID();
            const bool inVehicle = ped && ENTITY::DOES_ENTITY_EXIST(ped) && PED::GET_VEHICLE_PED_IS_IN(ped, false);
            if (!havePrevious || inVehicle != previous) {
                const char* value = inVehicle ? "state=in_vehicle\n" : "state=on_foot\n";
                ScriptHook::Util::TextBuffer<> text; text.Add(value);
                // Retry failed writes at the next sample, never every frame.
                if (scriptSaveFile("vehicle.txt", text.CStr(), text.Size())) {
                    previous = inVehicle; havePrevious = true;
                }
            }
        }
        WAIT(100);
    }
}
SHV_NX_PLUGIN("tutorial-vehicle", ScriptMain)
