// Native C++ adaptation of root-cause/v-lost-wallets (MIT).
#include "port.hpp"
using namespace Port;
struct Wallet {
    Pickup pickup = 0;
    Blip blip = 0;
    float x = 0, y = 0, z = 0;
};
static Wallet wallets[8]{};
void Stop() {
    for (auto &w : wallets) {
        if (w.pickup)
            OBJECT::REMOVE_PICKUP(w.pickup);
        Unpin(w.blip);
        w = {};
    }
    ClearOwned();
}
bool Spawn(bool guaranteed = false, int chance = 3) {
    Vector3 at = ENTITY::GET_ENTITY_COORDS(PedSelf(), true);
    Vector3 candidate = Add(at, V(MISC::GET_RANDOM_FLOAT_IN_RANGE(-25.f, 25.f),
                                  MISC::GET_RANDOM_FLOAT_IN_RANGE(-25.f, 25.f), 0));
    int zone = ZONE::GET_ZONE_SCUMMINESS(ZONE::GET_ZONE_AT_COORDS(candidate));
    if (zone < 0 || zone > 5)
        return false;
    if (!guaranteed && MISC::GET_RANDOM_FLOAT_IN_RANGE(0.f, 100.f) >= float(chance))
        return false;
    Vector3 safe{};
    bool found = false;
    for (int attempt = 0; attempt < 8 && !found; ++attempt) {
        if (attempt)
            candidate = Add(at, V(MISC::GET_RANDOM_FLOAT_IN_RANGE(-25.f, 25.f),
                                  MISC::GET_RANDOM_FLOAT_IN_RANGE(-25.f, 25.f), 0));
        found = PATH::GET_SAFE_COORD_FOR_PED(candidate, true, &safe, 13) && Dist2(at, safe) >= 4 &&
                Dist2(at, safe) <= 3600;
    }
    if (!found)
        return false;
    for (auto &w : wallets) {
        if (!w.pickup) {
            w.x = safe.x;
            w.y = safe.y;
            w.z = safe.z;
            w.pickup = OBJECT::CREATE_PICKUP(int(H("PICKUP_MONEY_WALLET")), safe, 24,
                                             MISC::GET_RANDOM_INT_IN_RANGE(80, 401), false, 0);
            if (w.pickup)
                w.blip = Pin(safe, 1, 2);
            return w.pickup != 0;
        }
    }
    return false;
}
void ScriptMain() {
    Stop();
    App app(1, "Lost Wallets NX");
    bool enabled = true;
    int32_t chance = 3, interval = 30;
    ScriptHook::Util::SavedSettings<3> settings(0x57414c32);
    uint32_t options[3]{1, 3, 30};
    if (settings.Load(options)) {
        enabled = options[0] != 0;
        chance = int32_t(options[1] <= 100 ? options[1] : 3);
        interval = int32_t(options[2] >= 5 && options[2] <= 120 ? options[2] : 30);
    }
    uint32_t next = Now() + 30000;
    Report("ready");
    for (;;) {
        if (!app.Frame()) {
            WAIT(100);
            continue;
        }
        if (app.Open()) {
            app.menu.Toggle(
                1, "Ambient wallet spawning", enabled,
                "Original default: 3% chance every 30 seconds, $80-$400. At most eight wallets.");
            app.menu.Action(
                2, "Try a nearby wallet now",
                "Checks a safe pavement coordinate. Useful for testing without the ambient wait.");
            app.menu.Action(3, "Remove spawned wallets");
            app.menu.Integer(
                4, "Chance per attempt (%)", chance, 0, 100, 1,
                "The original default is 3%. Manual wallet attempts always bypass this chance.");
            app.menu.Integer(5, "Attempt interval (seconds)", interval, 5, 120, 5);
        }
        unsigned count = 0;
        for (const auto &w : wallets)
            if (w.pickup)
                ++count;
        TextBuffer<80> status, hint;
        status.Add(enabled ? "ON" : "OFF").Add(" | ").UInt(count).Add(" wallets");
        hint.UInt(unsigned(chance))
            .Add("% every ")
            .UInt(unsigned(interval))
            .Add("s | Manual spawn in menu");
        app.Status(status.CStr(), hint.CStr());
        app.End();
        if (app.menu.Changed() >= 1 && app.menu.Changed() <= 5) {
            uint32_t out[3]{uint32_t(enabled), uint32_t(chance), uint32_t(interval)};
            Notice(settings.Save(out) ? "Wallet settings saved" : "Wallet settings save failed");
            next = Now() + uint32_t(interval) * 1000;
        }
        if (app.menu.Activated() == 2) {
            Notice(Spawn(true) ? "Wallet marked nearby" : "No safe pavement or wallet slots available");
            Report("manual_wallet_attempt");
        }
        if (app.menu.Activated() == 3) {
            Stop();
            Notice("Spawned wallets removed");
        }
        uint32_t now = Now();
        if (enabled && !app.Open() && Due(now, next)) {
            next = now + uint32_t(interval) * 1000;
            if (Spawn(false, chance))
                Notice("A lost wallet is marked nearby");
        }
        Vector3 pos = ENTITY::GET_ENTITY_COORDS(PedSelf(), true);
        for (auto &w : wallets)
            if (w.pickup &&
                (OBJECT::HAS_PICKUP_BEEN_COLLECTED(w.pickup) || Dist2(pos, V(w.x, w.y, w.z)) > 40000)) {
                if (OBJECT::HAS_PICKUP_BEEN_COLLECTED(w.pickup))
                    Notice("Wallet collected");
                OBJECT::REMOVE_PICKUP(w.pickup);
                Unpin(w.blip);
                w = {};
            }
        WAIT(0);
    }
}
SHV_NX_PLUGIN_WITH_STOP("lost-wallets-nx", ScriptMain, Stop)
