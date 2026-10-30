#pragma once
#include <stdint.h>

namespace Port {
// Target CONTROL_ACTION values; L/R are 205/206. Leave Down to the plugin
// manager and Y to Footprints. Every port needs only one additional button.
constexpr int MenuButtons[10] = {189, 188, 190, 207, 208, 204, 201, 202, 209, 210};
constexpr const char *MenuButtonNames[10] = {"Left", "Up", "Right", "ZL",      "ZR",
                                             "X",    "A",  "B",     "L-stick", "R-stick"};
constexpr const char *ShortNames[10] = {
    "YASM",     "Lost Wallets", "Vehicle Control", "Vehicle Mines", "RC Bomb",
    "Pizzaboy", "Gun Locker",   "Jammer Hunt",     "Treasure Hunt", "Slasher"};
// No native calls: exercise exactly this state machine in host tests too.
class Shortcut {
    bool down = false, release = false;

  public:
    bool Poll(bool shoulders, bool trigger) {
        const bool pressed = shoulders && trigger;
        const bool edge = pressed && !down;
        down = pressed;
        if (edge)
            release = true;
        return edge;
    }
    bool NavigationReady(bool shoulders, bool trigger) {
        if (release && !shoulders && !trigger)
            release = false;
        return !release;
    }
    void Cancel(bool shoulders, bool trigger) {
        down = shoulders && trigger;
        release = true;
    }
};
} // namespace Port
