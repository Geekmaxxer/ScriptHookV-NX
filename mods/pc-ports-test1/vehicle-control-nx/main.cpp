// NX adaptation of ikt32/GTAVVehicleControl (MIT).
#include "port.hpp"
using namespace Port;
static Vehicle target = 0;
static Hash targetModel = 0;
static bool indicators = false;
bool Valid() { return Exists(target) && ENTITY::GET_ENTITY_MODEL(target) == targetModel; }
void Stop() {
    if (indicators && Valid()) {
        VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(target, 0, false);
        VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(target, 1, false);
    }
    indicators = false;
    target = 0;
    targetModel = 0;
    ClearOwned();
}
void ScriptMain() {
    Stop();
    App app(2, "Vehicle Control NX");
    bool autoRemember = true;
    int32_t door = 0, window = 0;
    const char *doors[] = {"Front left", "Front right", "Rear left", "Rear right", "Bonnet", "Boot"};
    const char *windows[] = {"Front left", "Front right", "Rear left", "Rear right"};
    Report("ready");
    for (;;) {
        if (!app.Frame()) {
            WAIT(100);
            continue;
        }
        Vehicle current = Car();
        if (autoRemember && Exists(current) && current != target &&
            VEHICLE::GET_PED_IN_VEHICLE_SEAT(current, -1, false) == PedSelf()) {
            Stop();
            target = current;
            targetModel = ENTITY::GET_ENTITY_MODEL(target);
            Notice("Driver's vehicle remembered for remote control");
        }
        bool valid = Valid();
        bool inRange = valid && Dist2(ENTITY::GET_ENTITY_COORDS(PedSelf(), true),
                                      ENTITY::GET_ENTITY_COORDS(target, true)) < 40000;
        bool engine = valid && VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(target);
        bool locked = valid && VEHICLE::GET_VEHICLE_DOOR_LOCK_STATUS(target) == 2;
        bool doorValid = inRange && VEHICLE::GET_IS_DOOR_VALID(target, door);
        app.Status(!valid     ? "No remembered vehicle"
                   : !inRange ? "Out of range"
                   : engine   ? "Engine ON"
                              : "Engine OFF",
                   !valid   ? "Enter the driver's seat to remember a car"
                   : locked ? "Doors locked | Remote range: 200m"
                            : "Doors unlocked | Remote range: 200m");
        if (app.Open()) {
            app.menu.Action(1, "Remember current vehicle",
                            "Exit it afterwards to control it remotely within 200 metres.");
            app.menu.ReadOnly(20, "Remembered vehicle",
                              valid ? (inRange ? "In range" : "Beyond 200m") : "None / despawned");
            app.menu.Action(2, engine ? "Stop engine" : "Start engine",
                            "Only your remembered vehicle is changed.", inRange);
            app.menu.Action(3, locked ? "Unlock doors" : "Lock doors", "No ownership change is made.",
                            inRange);
            app.menu.Choice(
                4, "Door", door, doors, 6,
                "Only doors fitted to this vehicle can be opened. Unavailable door actions are dimmed.");
            app.menu.Action(5, "Open selected door", "Some vehicles do not have every door.", doorValid);
            app.menu.Action(6, "Close selected door", "", doorValid);
            app.menu.Choice(7, "Window", window, windows, 4);
            app.menu.Action(8, "Lower window", "", inRange);
            app.menu.Action(9, "Raise window", "", inRange);
            app.menu.Action(10, indicators ? "Turn hazards off" : "Turn hazards on", "", inRange);
            app.menu.Action(11, "Lights on", "", inRange);
            app.menu.Action(12, "Lights normal", "", inRange);
            app.menu.Action(13, "Mark vehicle location", "", valid);
            app.menu.Action(14, "Forget vehicle");
            app.menu.Toggle(
                15, "Remember driven cars", autoRemember,
                "Remembers your latest driver's vehicle. Disable this to keep a specific vehicle.");
        }
        app.End();
        int action = int(app.menu.Activated());
        if (action == 1) {
            Stop();
            target = Car();
            if (Exists(target)) {
                targetModel = ENTITY::GET_ENTITY_MODEL(target);
                Notice("Vehicle remembered");
            } else
                Notice("Enter a vehicle first");
        }
        if (action == 14) {
            autoRemember = false;
            Stop();
            Notice("Vehicle forgotten; automatic remembering paused");
        }
        if (action == 13 && Valid()) {
            Route(ENTITY::GET_ENTITY_COORDS(target, true));
            Notice("Vehicle location marked");
        }
        if (Valid() && Dist2(ENTITY::GET_ENTITY_COORDS(PedSelf(), true),
                             ENTITY::GET_ENTITY_COORDS(target, true)) < 40000) {
            if (action == 2)
                VEHICLE::SET_VEHICLE_ENGINE_ON(target, !VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(target),
                                               true, false);
            if (action == 3)
                VEHICLE::SET_VEHICLE_DOORS_LOCKED(
                    target, VEHICLE::GET_VEHICLE_DOOR_LOCK_STATUS(target) == 2 ? 1 : 2);
            if (action == 5)
                VEHICLE::SET_VEHICLE_DOOR_OPEN(target, door, false, false);
            if (action == 6)
                VEHICLE::SET_VEHICLE_DOOR_SHUT(target, door, false);
            if (action == 8)
                VEHICLE::ROLL_DOWN_WINDOW(target, window);
            if (action == 9)
                VEHICLE::ROLL_UP_WINDOW(target, window);
            if (action == 10) {
                indicators = !indicators;
                VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(target, 0, indicators);
                VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(target, 1, indicators);
            }
            if (action == 11)
                VEHICLE::SET_VEHICLE_LIGHTS(target, 3);
            if (action == 12)
                VEHICLE::SET_VEHICLE_LIGHTS(target, 0);
            if (action == 2)
                Notice(engine ? "Engine stopped" : "Engine started");
            if (action == 3)
                Notice(locked ? "Doors unlocked" : "Doors locked");
            if (action == 5)
                Notice("Opening selected door");
            if (action == 6)
                Notice("Closing selected door");
            if (action == 8)
                Notice("Selected window lowered");
            if (action == 9)
                Notice("Selected window raised");
            if (action == 10)
                Notice(indicators ? "Hazards on" : "Hazards off");
            if (action == 11 || action == 12)
                Notice(action == 11 ? "Lights on" : "Lights returned to automatic");
        } else if (action >= 2 && action <= 12 && action != 4 && action != 7)
            Notice("Vehicle missing or more than 200 metres away");
        WAIT(0);
    }
}
SHV_NX_PLUGIN_WITH_STOP("vehicle-control-nx", ScriptMain, Stop)
