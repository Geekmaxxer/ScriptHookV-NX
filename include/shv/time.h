#pragma once
#include <stdint.h>

namespace ScriptHook { namespace Util {
// Unsigned subtraction survives GET_GAME_TIMER wrapping. Keep intervals below
// 2^31 ms; a timer must be serviced at least once within that range.
inline bool Due(uint32_t now, uint32_t deadline) { return uint32_t(now - deadline) < 0x80000000u; }
inline bool Elapsed(uint32_t now, uint32_t since, uint32_t interval) { return uint32_t(now - since) >= interval; }
class Every {
    uint32_t next = 0;
    bool started = false;
public:
    bool Ready(uint32_t now, uint32_t interval) {
        if (started && !Due(now, next)) return false;
        started = true; next = now + interval; return true;
    }
    void Reset() { started = false; }
};
} }
