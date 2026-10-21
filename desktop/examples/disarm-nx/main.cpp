// Disarm NX: Switch behavior port of GTAVDisarm by ikt32.
// Original source: https://github.com/ikt32/GTAVDisarm
#include "disarm_support.hpp"

void ScriptMain() {
    DisarmNx::Controller disarm;
    if (!disarm.Start()) return;
    while (true) {
        disarm.Tick();
        WAIT(0);
    }
}
