#include "mod_support.hpp"

namespace {
// Display modes are deliberately the only persisted state. No vehicle or player
// state is changed, so leaving the plugin enabled needs no gameplay cleanup.
enum DisplayMode : uint32_t { Kilometres = 0, Miles = 1, Hidden = 2 };

const char* Compass(float heading) {
    static constexpr const char* directions[8] = {
        "N", "NW", "W", "SW", "S", "SE", "E", "NE"
    };
    // GTA headings run anticlockwise from north. Reject non-finite/out-of-range
    // data before float-to-integer conversion; no math library is needed.
    if (!(heading >= 0.0f && heading <= 360.0f)) return "--";
    return directions[(static_cast<uint32_t>((heading + 22.5f) / 45.0f)) & 7u];
}

void ScriptMain() {
    using namespace Showcase;
    if (!Start()) return;

    SavedWords settings(0x44524831u); // DRH1: one display-mode word.
    uint32_t mode[1] = {Kilometres};
    if (!settings.Load(mode, 1) || mode[0] > Hidden) mode[0] = Kilometres;

    uint32_t nextTelemetry = Now();
    Vehicle sampledVehicle = 0;
    float speedMetres = 0.0f;
    const char* headingText = "--";
    bool telemetryAvailable = false;
    bool faultReported = false;

    for (;;) {
        if (!Visible()) {
            WAIT(100);
            continue;
        }
        const uint32_t now = Now();
        if (Hotkey(Direction::Right)) {
            mode[0] = (mode[0] + 1u) % 3u;
            const bool saved = settings.Save(mode, 1);
            SaveStatus(!saved ? "settings-save-failed" :
                       mode[0] == Hidden ? "hidden" :
                       mode[0] == Miles ? "mph" : "kph");
            nextTelemetry = now;
        }

        const bool refresh = Due(now, nextTelemetry);
        if (refresh) {
            nextTelemetry = now + 100u;
            sampledVehicle = PlayerVehicle();
            telemetryAvailable = false;
        }
        const bool display = mode[0] != Hidden && sampledVehicle != 0;
        if (display && refresh) {
            const float speed = ENTITY::GET_ENTITY_SPEED(sampledVehicle);
            const bool speedOk = NativeOk();
            const float heading = ENTITY::GET_ENTITY_HEADING(sampledVehicle);
            const bool headingOk = NativeOk();
            telemetryAvailable = speedOk && headingOk && speed >= 0.0f && speed <= 350.0f;
            if (telemetryAvailable) {
                speedMetres = speed;
                headingText = Compass(heading);
                if (faultReported) {
                    faultReported = false;
                    SaveStatus("ready");
                }
            } else if (!faultReported) {
                faultReported = true;
                SaveStatus("telemetry-unavailable");
            }
        }

        if (display && telemetryAvailable) {
            TextLine line;
            line.Fixed1(speedMetres * (mode[0] == Miles ? 2.2369363f : 3.6f));
            line.Add(mode[0] == Miles ? " mph  |  " : " km/h  |  ");
            line.Add(headingText);
            DrawLine(0.805f, 0.875f, 0.18f, line.CStr(), {90, 220, 255});
        }

        // Poll every frame even while hidden/on foot: just-pressed controller
        // events last one frame, and hidden mode must always be reversible.
        WAIT(0);
    }
}
}

SHV_NX_PLUGIN("drive-hud", ScriptMain)
