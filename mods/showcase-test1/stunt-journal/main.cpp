#include "mod_support.hpp"

using namespace Showcase;

namespace {
constexpr uint32_t kMagic = 0x53544a31u; // STJ1
constexpr uint32_t kSampleMs = 50;
constexpr uint32_t kMinimumAirMs = 300;
constexpr uint32_t kMaximumAirMs = 60000;
constexpr uint32_t kPopupMs = 5000;

struct Jump {
    Vehicle vehicle = 0;
    bool groundSeen = false;
    bool inAir = false;
    uint32_t takeoffMs = 0;
    Vector3 takeoff;

    void Abort() {
        vehicle = 0;
        groundSeen = false;
        inAir = false;
    }
};

struct Popup {
    bool shown = false;
    bool bests = false;
    bool newBest = false;
    uint32_t until = 0;
    uint32_t distanceDm = 0;
    uint32_t airtimeMs = 0;
};

void DrawPopup(const Popup& popup) {
    const Color accent = {255, 210, 90};
    const char* heading = popup.bests ? "STUNT JOURNAL | PERSONAL BESTS" :
        (popup.newBest ? "STUNT JOURNAL | NEW PERSONAL BEST" : "STUNT JOURNAL | LAST JUMP");
    DrawLine(0.32f, 0.105f, 0.36f, heading, accent);
    TextLine result;
    result.Add("Distance ");
    result.Fixed1(static_cast<float>(popup.distanceDm) / 10.0f);
    result.Add(" m  |  Airtime ");
    result.Fixed1(static_cast<float>(popup.airtimeMs) / 1000.0f);
    result.Add(" s");
    DrawLine(0.32f, 0.150f, 0.36f, result.CStr(), accent);
}
}

void ScriptMain() {
    if (!Start()) return;

    SavedWords storage(kMagic);
    uint32_t best[2] = {}; // Horizontal distance in decimeters, airtime in milliseconds.
    storage.Load(best, 2);
    // Ignore unreasonable records rather than ever using corrupt settings as scores.
    if (best[0] > 20000 || best[1] > kMaximumAirMs) best[0] = best[1] = 0;

    Jump jump;
    Popup popup;
    uint32_t nextSample = Now();
    SaveStatus("running");

    for (;;) {
        const uint32_t now = Now();
        const bool visible = Visible();
        if (!visible) {
            // A pause, death or cutscene cannot turn into a landing or a fresh takeoff.
            jump.Abort();
            popup.shown = false;
            nextSample = now;
        } else {
            if (Hotkey(Direction::Up)) {
                popup.shown = true;
                popup.bests = true;
                popup.newBest = false;
                popup.until = now + kPopupMs;
                popup.distanceDm = best[0];
                popup.airtimeMs = best[1];
            }

            if (Due(now, nextSample)) {
                nextSample = now + kSampleMs;
                const Vehicle vehicle = PlayerVehicle(true);
                if (!vehicle) {
                    jump.Abort();
                } else {
                    if (vehicle != jump.vehicle) {
                        jump.Abort();
                        jump.vehicle = vehicle;
                    }
                    const bool inAir = ENTITY::IS_ENTITY_IN_AIR(vehicle);
                    if (!NativeOk()) {
                        SaveStatus("native-error");
                        return;
                    }
                    if (!inAir) {
                        if (jump.inAir) {
                            const uint32_t airtimeMs = now - jump.takeoffMs;
                            // Reject extreme flight or displacement values.
                            if (airtimeMs >= kMinimumAirMs && airtimeMs <= kMaximumAirMs) {
                                Vector3 landing = ENTITY::GET_ENTITY_COORDS(vehicle, true);
                                if (!NativeOk()) {
                                    SaveStatus("native-error");
                                    return;
                                }
                                landing.z = jump.takeoff.z;
                                const float distance = MISC::GET_DISTANCE_BETWEEN_COORDS(
                                    jump.takeoff, landing, false);
                                if (!NativeOk()) {
                                    SaveStatus("native-error");
                                    return;
                                }
                                // This comparison also rejects NaN without depending on libc math.
                                if (distance >= 0.0f && distance <= 2000.0f) {
                                    const uint32_t distanceDm = static_cast<uint32_t>(distance * 10.0f);
                                    const bool newBest = distanceDm > best[0] || airtimeMs > best[1];
                                    if (distanceDm > best[0]) best[0] = distanceDm;
                                    if (airtimeMs > best[1]) best[1] = airtimeMs;
                                    if (newBest && !storage.Save(best, 2)) SaveStatus("best-save-failed");
                                    popup.shown = true;
                                    popup.bests = false;
                                    popup.newBest = newBest;
                                    popup.until = now + kPopupMs;
                                    popup.distanceDm = distanceDm;
                                    popup.airtimeMs = airtimeMs;
                                }
                            }
                        }
                        jump.inAir = false;
                        jump.groundSeen = true;
                    } else if (!jump.inAir && jump.groundSeen) {
                        jump.takeoff = ENTITY::GET_ENTITY_COORDS(vehicle, true);
                        if (!NativeOk()) {
                            SaveStatus("native-error");
                            return;
                        }
                        jump.takeoffMs = now;
                        jump.inAir = true;
                        jump.groundSeen = false;
                    } else if (jump.inAir && now - jump.takeoffMs > kMaximumAirMs) {
                        // Re-arm only after seeing this vehicle grounded again.
                        jump.inAir = false;
                        jump.groundSeen = false;
                    }
                }
            }

            if (popup.shown) {
                if (Due(now, popup.until)) popup.shown = false;
                else DrawPopup(popup);
            }
        }
        // Input and popup rendering are frame-based; vehicle telemetry is sampled at 20 Hz.
        WAIT(0);
    }
}

SHV_NX_PLUGIN("stunt-journal", ScriptMain)
